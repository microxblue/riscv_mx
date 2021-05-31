//
// Copyright (c) 2021.  Micro Blue
//
// SPDX-License-Identifier: BSD-2-Clause-Patent
//

`include "soc_cfg.v"

`ifdef  CFG_VGA

`timescale 1ns / 1ns
`default_nettype none

`define  XS  11'd320
`define  YS  11'd240
`define  XM  11'd800
`define  YM  11'd600
`define  Y1  ((`YM - `YS) / 2)
`define  Y2  (`Y1 + `YS)
`define  X1  ((`XM - `XS) / 2)
`define  X2  (`X1 + `XS)

`define  SPRITE_NUM           128
`define  SPRITE_HIGH_BIT      ($clog2(`SPRITE_NUM)-1)
`define  VGA_CHAR_HEIGHT      (`VGA_TILE_EN ? 8 : 12)
`define  BYTE_PER_PIXEL       2
`define  PREFETCH_SIZE        64*2
`define  LINE_BUFFER_SIZE     2048
`define  PREFETCH_PIXEL_CNT   (`PREFETCH_SIZE / `BYTE_PER_PIXEL)
`define  HC_HIGH_BIT          ($clog2(`PREFETCH_PIXEL_CNT) - 1)
`define  PIXEL_SHIFT          1
`define  TEXT_DELAY           8
`define  DW_ADDR_HIGH_BIT     ($clog2(`LINE_BUFFER_SIZE) - 3)

// Disable HyperRAM access
`define  VGA_ALLOW_HBC  (vga_ctl[3])
`define  VGA_TILE_EN    (vga_ctl[6])
`define  VGA_TEXT_START (vga_ctl[15:8])

module vga
#(
  // default  80M (Hyperbus) / 80M (mem) /40M (ext)
	parameter HYPER_BUS_TO_VGA_BUS_FREQ_RATIO = 2   // Valid value is 2 or 4
) (
  // interface video RAM
  input  wire         cpu_clk,
  input  wire         resetn,
  input  wire         enable,
  input  wire         mem_valid,
  output wire         mem_ready,
  input  wire [3:0]   mem_wstrb,
  input  wire [31:0]  mem_wdata,
  input  wire [31:0]  mem_addr,
  output wire [31:0]  mem_rdata,

  output wire [7:0]   dbg_pin,

  input  wire [31:0]  ext_rdata,
  output reg          ext_rd,
  input  wire         ext_burst_ready,
  output reg [31:0]   ext_addr,
  output reg [7:0]    ext_burst_dw,

  // VGA output (1-bit)
  input  wire         vclk,   // 40M
  output wire [4:0]   red,
  output wire [4:0]   green,
  output wire [4:0]   blue,
  output wire         hsync,
  output wire         vsync
  );

  wire         ext_4x_clk_en = (HYPER_BUS_TO_VGA_BUS_FREQ_RATIO == 4) ? 1'b1 : 1'b0;

  assign     dbg_pin[0] = disp_on;
  assign     dbg_pin[1] = wr_req;
  assign     dbg_pin[2] = hsync;
  assign     dbg_pin[3] = vsync;
  assign     dbg_pin[4] = ext_burst_ready;
  assign     dbg_pin[5] = sprite_cur_id[0];
  assign     dbg_pin[6] = wr_sprite;
  assign     dbg_pin[7] = tile_wip;

  `define  SPRITE_REG_POS_Y   sprite_cfg_cpy[15:8]
  `define  SPRITE_REG_POS_X   sprite_cfg_cpy[24:16]
  `define  SPRITE_REG_BUF_PTR sprite_cfg_cpy[31:25]
  `define  SPRITE_REG_FRONT   sprite_cfg_cpy[6]
  `define  SPRITE_REG_EN      sprite_cfg_cpy[7]
  `define  SPRITE_REG_SIZE    sprite_cfg_cpy[5:4]
  `define  SPRITE_REG_SIZE_H  sprite_cfg_cpy[5]
  `define  SPRITE_REG_SIZE_L  sprite_cfg_cpy[4]
  `define  SPRITE_REG_SUB_IDX sprite_cfg_cpy[3:2]
  `define  SPRITE_REG_MIR     sprite_cfg_cpy[0]

  // Sprites
  reg [31:0] sprite_cfg [`SPRITE_NUM];
  reg [`SPRITE_HIGH_BIT+1:0] sprite_cur_id;

  reg  [23:0] vga_base [4];
  reg   [1:0] vga_rdy;
  reg  [31:0] vga_ctl;
  reg  [31:0] sprite_cur_cfg;
  reg  [31:0] sprite_cfg_cpy;
  reg         sprite_vis;
  reg  [10:0] sprite_last_pos;
  reg   [3:0] sprite_len;
  always @(posedge cpu_clk or negedge resetn)
  begin
      if (~resetn) begin
          vga_ctl        <= 32'h0;
          vga_rdy        <= 0;
          vga_base[0]    <= 24'h7a0000; // GFX base
          vga_base[1]    <= 24'h740000; // SPR base (3x64KB)
          vga_base[2]    <= 24'h7ed000; // TXT base
          vga_base[3]    <= 24'h770000; // Tile base
          sprite_cfg_cpy <= 32'h0;
      end else begin
          vga_rdy  <= {1'b0, vga_rdy[1]};
          if (enable & mem_valid) begin
              vga_rdy <= 2'b01;
              if (|mem_wstrb) begin
                  // Handle write
                  if (mem_addr[12:5] == 8'h00) begin
                      case (mem_addr[4:2])
                        3'h0: begin
                           if (mem_wstrb[0])  vga_ctl[ 7: 0] <= mem_wdata[ 7: 0];
                           if (mem_wstrb[1])  vga_ctl[15: 8] <= mem_wdata[15: 8];
                           if (mem_wstrb[2])  vga_ctl[23:16] <= mem_wdata[23:16];
                           if (mem_wstrb[3])  vga_ctl[31:24] <= mem_wdata[31:24];
                        end
                        default: begin
                           if (mem_addr[4])
                              vga_base[mem_addr[3:2]] <= mem_wdata;
                        end
                      endcase
                  end else if (~mem_addr[12] & mem_addr[10]) begin
                      sprite_cfg[mem_addr[`SPRITE_HIGH_BIT+2:2]] <= mem_wdata;
                  end
              end else begin
                  if (mem_addr[10]) begin
                      // Read sprite_cur_cfg, data will be available one cycle later
                      // So need to delay ready signal for one cycle
                      if (~vga_rdy[1]) begin
                        vga_rdy <= 2'b10;
                      end
                  end
              end
          end else begin
              // Access only one element at a time to allow sprite_cfg to be mapped into FPGA block memory resource
              sprite_cfg_cpy <= sprite_cur_cfg;
          end
          sprite_cur_cfg <= sprite_cfg[(enable & mem_valid) ? mem_addr[`SPRITE_HIGH_BIT+2:2] : sprite_cur_id[`SPRITE_HIGH_BIT:0]];
      end
  end


  // Handle read
  reg [31:0]  data_out;
  always @(*) begin
      if (mem_addr[12]) begin
        data_out = char_dword;
      end else if (mem_addr[10]) begin
        data_out = sprite_cur_cfg;
      end else begin
        case (mem_addr[4:2])
          3'h0:    data_out = vga_ctl;
          3'h3:    data_out = `SPRITE_NUM | {32'hAA010000};
          default: begin
            if (mem_addr[4])
              data_out = vga_base[mem_addr[3:2]];
            else
              data_out = 0;
          end
        endcase
      end
  end

  assign mem_rdata = data_out;
  assign mem_ready = vga_rdy[0];


  // Translate 16 color index into RGB
  function [15:0]  get_color (input [3:0] i);
    begin
    case (i)
    4'h0:  get_color = 16'h0421;
    4'h1:  get_color = 16'h4421;
    4'h2:  get_color = 16'h0621;
    4'h3:  get_color = 16'h4621;
    4'h4:  get_color = 16'h0431;
    4'h5:  get_color = 16'h4431;
    4'h6:  get_color = 16'h0631;
    4'h7:  get_color = 16'h6739;
    4'h8:  get_color = 16'h4631;
    4'h9:  get_color = 16'h7c21;
    4'hA:  get_color = 16'h07e1;
    4'hB:  get_color = 16'h7fe1;
    4'hC:  get_color = 16'h043f;
    4'hD:  get_color = 16'h7c3f;
    4'hE:  get_color = 16'h07ff;
    4'hF:  get_color = 16'h7fff;
    endcase
    end
  endfunction


  // Control
  // Fill line buffer in advance
  reg  [3:0]  state;
  reg  [3:0]  ret_state;
  reg  [15:0] burst_cnt;
  reg   [1:0] wr_req;
  reg         wr_odd;
  reg  [15:0] wr_addr;
  reg  [31:0] wr_data;
  reg   [1:0] wr_sel;
  reg         wr_done;
  reg         wr_sprite;
  reg         tile_wip;
  reg  [15:0] remain;
  reg  [8:0]  pixel_cnt;
  reg  [7:0]  text_color;
  reg  [7:0]  text_color_delayed;

  wire [10:0] vc_off     = vc - `Y1;
  wire [10:0] hc_off     = hc - `X1;
  wire [10:0] vc_off_a   = vc_off + 1;
  wire  [5:0] text_row   = vc_off_a / `VGA_CHAR_HEIGHT;
  wire  [3:0] text_row_y = vc_off_a % `VGA_CHAR_HEIGHT;
  wire  [7:0] burst_dw_num = 1 << (`SPRITE_REG_SIZE + 2);
  wire        addr_odd     = `SPRITE_REG_POS_X & 1'b1;
  reg         wr_reverse;
  wire [31:0] ext_odata    = wr_reverse ? {ext_rdata[15:0], ext_rdata[31:16]} : ext_rdata;

  always @ (posedge vclk or negedge resetn) begin
      if (!resetn) begin
          state          <= 4'h0;
          wr_req         <= 0;
          ext_rd         <= 0;
          sprite_cur_id  <= 0;
          wr_sprite    <= 0;
      end else begin
          case (state)

          4'h0: begin
              // Fetch one segment - 64 pixels
              if ((hc[`HC_HIGH_BIT:0] == 0)  && (vc >= (`Y1-1)) && (vc < (`Y2-1)) && `VGA_ALLOW_HBC) begin
                if (vc_off_a >= `VGA_TEXT_START) begin
                  if (hc[10:`HC_HIGH_BIT+1] == 0) begin
                      // Fetch text buffer line
                      burst_cnt          <= 0;
                      ext_burst_dw       <= `XS / 16;
                      ext_addr           <= vga_base[2] + text_row * (`XS / 8 * 2);
                      ext_rd             <= 1;
                      wr_addr            <= 0;
                      wr_odd             <= 0;
                      wr_sprite          <= 0;
                      wr_reverse         <= 0;
                      wr_sel             <= 2'b10;
                      ret_state          <= 4'h0;
                      state              <= 4'h1;
                  end else if (hc[10:`HC_HIGH_BIT+1] == 1) begin
                      // Fetch char glygh buffer
                      pixel_cnt          <= 9'h0;
                      wr_str             <= 4'h0;
                      wr_addr            <= {vc_off[0], 8'h0};
                      text_line_buf_addr <= 0;
                      tile_wip           <= 0;
                      state              <= 4'h3;
                  end
                end else begin
                  // Start fill graphics line buffer from HyperRAM
                  ext_burst_dw           <= `PREFETCH_SIZE / 4;
                  burst_cnt              <= 0;
                  if (hc[10:`HC_HIGH_BIT+1] < (`XS + `PREFETCH_PIXEL_CNT - 1) / `PREFETCH_PIXEL_CNT) begin
                      // Fetch framebuffer line
                      ext_rd             <= 1;
                      ext_addr           <= vga_base[0] + vc_off_a * `XS * `BYTE_PER_PIXEL + hc[10:`HC_HIGH_BIT+1] * `PREFETCH_SIZE;
                      wr_addr            <= {vc_off[0], 8'h0} + hc[10:`HC_HIGH_BIT+1] * (`PREFETCH_SIZE / 4);
                      wr_odd             <= 0;
                      wr_reverse         <= 0;
                      wr_sprite          <= 0;
                      wr_sel             <= 2'b01;
                      ret_state          <= 4'h0;
                      state              <= 4'h1;
                  end else if (hc[10:`HC_HIGH_BIT+1] == (`XS + `PREFETCH_PIXEL_CNT - 1) / `PREFETCH_PIXEL_CNT) begin
                      // Prepare reading sprites line
                      sprite_cur_id      <= 0;
                      state              <= 4'h2;
                  end
                end
              end
          end


          4'h1: begin
              // Copy 64 graphics pixel from framebuffer into linebuffer
              ext_rd <= 0;
              if (ext_burst_ready) begin
                wr_done   <= ~wr_odd;
                burst_cnt <= burst_cnt + 1;
                if ((burst_cnt[0] == 0) || ext_4x_clk_en) begin
                    wr_req  <= wr_sel;
                    if (wr_odd) begin
                       // Handle write to odd WORD address on 32bit bus
                       remain  <= ext_rdata[31:16];
                       if (wr_reverse) begin
                         wr_data <= {remain, ext_rdata[15:0]};
                         if (burst_cnt == 0)
                            wr_str <= (wr_sprite) ? {2'b0, ~ext_rdata[15], ~ext_rdata[15]} : 4'h0011;
                         else
                            wr_str <= (wr_sprite) ? {~remain[15], ~remain[15], ~ext_rdata[15], ~ext_rdata[15]} : 4'h1111;
                       end else begin
                         wr_data <= {ext_rdata[15:0],  remain};
                         if (burst_cnt == 0)
                            wr_str <= (wr_sprite) ? {~ext_rdata[15], ~ext_rdata[15], 2'b0} : 4'h1100;
                         else
                            wr_str <= (wr_sprite) ? {~ext_rdata[15], ~ext_rdata[15], ~remain[15], ~remain[15]} : 4'h1111;
                       end
                    end else begin
                       wr_data <= ext_odata;
                       wr_str  <= (wr_sprite) ? {~ext_odata[31], ~ext_odata[31], ~ext_odata[15], ~ext_odata[15]} : 4'hf;
                    end
                    if (burst_cnt != 0)
                        wr_addr <= wr_addr + ((wr_reverse) ? -1 : 1);
                end else begin
                    wr_req  <= 0;
                end
              end else begin
                if (wr_req) begin
                    if (~wr_done) begin
                       // generate one more write for odd address
                       wr_addr <= wr_addr + ((wr_reverse) ? -1 : 1);
                       wr_data <= {remain,  remain};
                       if (wr_reverse) begin
                          wr_str  <= (wr_sprite) ? {~remain[15], ~remain[15], 2'b0} : 4'h1100;
                       end else begin
                          wr_str  <= (wr_sprite) ? {2'b0, ~remain[15], ~remain[15]} : 4'h0011;
                       end
                       wr_done <= 1;
                    end else begin
                       // write done
                       wr_req <= 0;
                       if (wr_sprite) begin
                         sprite_cur_id <= sprite_cur_id + 1;
                       end
                       state <= ret_state;
                    end
                end else
                    wr_req <= 0;
              end

          end

          4'h2: begin
              // Copy sprite pixel into linebuffer
              if (sprite_cur_id == `SPRITE_NUM) begin
                  state <= 0;
              end else begin
                  if ((((vc_off_a - `SPRITE_REG_POS_Y) & {5'b11111, ~{`SPRITE_REG_SIZE_H & `SPRITE_REG_SIZE_L, `SPRITE_REG_SIZE_H, `SPRITE_REG_SIZE_H | `SPRITE_REG_SIZE_L}, 3'h0}) == 0)  && `SPRITE_REG_EN) begin
                      // Sprite is visible in current line
                      case (`SPRITE_REG_SIZE)
                      2'b11: begin   // 64x64
                        ext_addr <= vga_base[1] + 20'h20000 + {`SPRITE_REG_BUF_PTR & 3'h7, 13'h0} + {(vc_off_a - `SPRITE_REG_POS_Y) & 6'h3F, 7'h00};
                      end
                      2'b10: begin   // 32x32
                        ext_addr <= vga_base[1] + 20'h10000 + {`SPRITE_REG_BUF_PTR & 5'h1F, 11'h0} + {(vc_off_a - `SPRITE_REG_POS_Y) & 5'h1F, 6'h00};
                      end
                      2'b01: begin  // 16x16
                        ext_addr <= vga_base[1] + {`SPRITE_REG_BUF_PTR, 9'h0} + {(vc_off_a - `SPRITE_REG_POS_Y) & 4'hF, 5'h00};
                      end
                      default: begin // 8x8
                        ext_addr <= vga_base[1] + {`SPRITE_REG_BUF_PTR, 9'h0} + {`SPRITE_REG_SUB_IDX, 7'h0} + {(vc_off_a - `SPRITE_REG_POS_Y) & 3'hF, 4'h00};
                      end
                      endcase
                      burst_cnt      <= 0;
                      ext_burst_dw   <= burst_dw_num;
                      ext_rd         <= 1;
                      wr_addr        <= {vc_off[0], 8'h0} + (`SPRITE_REG_POS_X >> 1) + (`SPRITE_REG_MIR ? burst_dw_num - (~addr_odd) - 2: 0);
                      wr_sprite      <= 1;
                      wr_odd         <= addr_odd;
                      wr_reverse     <= `SPRITE_REG_MIR;
                      wr_sel         <= 2'b01;
                      ret_state      <= 4'h2;
                      state          <= 4'h1;
                  end else begin
                      sprite_cur_id  <= sprite_cur_id + 1;
                  end
              end
          end

          4'h3: begin
              // Copy text char pixel into linebuffer
              if (`VGA_TILE_EN) begin
                  // Tile mode
                  if (pixel_cnt > `XS + 8) begin
                      // Continue to draw sprites
                      sprite_cur_id      <= 0;
                      state              <= 4'h2;
                      tile_wip           <= 0;
                  end else begin
                      if (tile_wip) begin
                          text_line_buf_addr <= text_line_buf_addr + 1;
                          pixel_cnt          <= pixel_cnt + 8;
                          burst_cnt          <= 0;
                          ext_burst_dw       <= 4;
                          ext_addr           <= vga_base[3] + {text_line_buf_rdata[10:0], 7'b0} + {vc_off_a[2:0], 4'h0};
                          ext_rd             <= 1;
                          wr_addr            <= {vc_off[0], 8'h0} + pixel_cnt[8:1];
                          wr_odd             <= 0;
                          wr_sprite          <= 0;
                          wr_reverse         <= 0;
                          wr_sel             <= 2'b01;
                          ret_state          <= 4'h3;
                          state              <= 4'h1;
                      end
                      tile_wip           <= 1;
                  end
              end else begin
                  if (pixel_cnt > `XS + `TEXT_DELAY + 4) begin
                      // Continue to draw sprites
                      wr_req             <= 2'b00;
                      sprite_cur_id      <= 0;
                      state              <= 4'h2;
                      tile_wip           <= 0;
                  end else begin
                      text_line_buf_addr <= pixel_cnt[8:3];
                      char_glygh_addr    <= (text_line_buf_rdata[7:0] * `VGA_CHAR_HEIGHT / 2) + text_row_y[3:1];
                      text_color         <= text_line_buf_rdata[15:8];
                      text_color_delayed <= text_color;
                      pixel_cnt          <= pixel_cnt + 2;
                      if (pixel_cnt >= `TEXT_DELAY) begin
                        // several cycles behind due to RAM read latency for text buffer and char glyph ram
                        wr_data[15:0]    <= (char_word[{vc_off_a[0], 3'h7 - pixel_cnt[2:0]}]) ? get_color(text_color_delayed[3:0]) : get_color(text_color_delayed[7:4]);
                        wr_data[31:16]   <= (char_word[{vc_off_a[0], 3'h6 - pixel_cnt[2:0]}]) ? get_color(text_color_delayed[3:0]) : get_color(text_color_delayed[7:4]);
                        wr_str           <= 4'hf;
                        wr_req           <= 2'b01;
                        if (wr_str)
                          wr_addr        <= wr_addr + 1;
                      end
                  end
              end
          end

          default:;
          endcase
      end
  end


  // Calculate current line buffer read address
  // The display will be 1 cycle behind, so prepare 1 cycle in advance.
  always @(posedge vclk) begin
      if (hc == 0) begin
          line_buf_addr <= {~vc_off[0], 9'h0};
      end else if (vc >= `Y1 && vc < `Y2 && hc >= (`X1 - `PIXEL_SHIFT) && hc < `X2) begin
          line_buf_addr <= line_buf_addr + 1;
      end
  end


  // GFX display
  reg  [15:0] rgb;
  reg         disp_on;
  always @* begin
    disp_on = 0;
    if (vc >= `Y1 && vc < `Y2 && hc >= `X1 && hc < `X2) begin
        disp_on = 1;
        rgb = line_buf_data;
    end else begin
        rgb = 16'h0000;
    end
  end
  assign {red, green, blue} = {rgb[14:10], rgb[9:5], rgb[4:0]};


  // VGA char RAM to provide font glyph
  wire [15:0] char_word;
  reg  [10:0] char_glygh_addr;
  wire [31:0] char_dword;
  char_dpram char_dpram_inst (
    .clock_a(cpu_clk),
    .address_a(mem_addr[11:2]),
    .q_a(char_dword),
    .wren_a(enable & mem_valid & mem_addr[12] & (|mem_wstrb)),
    .data_a(mem_wdata),
    .byteena_a(mem_wstrb),
    .clock_b(vclk),
    .address_b(char_glygh_addr),
    .q_b(char_word),
    .wren_b(1'b0)
  );

  // Text line buffer to hold char ascii and attribute for current display line
  reg  [5:0]   text_line_buf_addr;
  wire [15:0]  text_line_buf_rdata;
  vga_text_line_buf_mem  vga_text_line_buf_mem_inst (
	  .clk (vclk),
    .rdaddress (text_line_buf_addr),
    .rdata (text_line_buf_rdata),
    .wraddress (wr_addr),
	  .wdata (wr_data),
	  .wren  (wr_req[1]),
  );

  // VGA display line buffer to hold a full scanline pixels including graphics, text and sprites
  wire [15:0]  line_buf_data;
  reg  [15:0]  line_buf_addr;
  reg   [3:0]  wr_str;
  vga_line_buf_mem  vga_line_buf_mem_inst (
    .rdaddress(line_buf_addr),
    .rdclock(vclk),
    .q(line_buf_data),
    .wraddress(wr_addr),
    .byteena_a (wr_str),
    .data(wr_data),
    .wrclock(vclk),
    .wren(wr_req[0])
    );


  // VGA Sync
  wire vga_disp;
  wire [10:0] hc,vc;
  videosyncs sincronismos (  // defaults to VGA 800x600@60Hz, 40 MHz
    .clk(vclk),
    .freeze(1'b0),
    .hs(hsync),
    .vs(vsync),
    .hc(hc),
    .vc(vc),
    .display_enable(vga_disp)
  );

endmodule



module videosyncs (
  input wire clk,        // Clock must be as close as possible to nominal pixel clock according to ModeLine used
  input wire freeze,     // Freeze clock
  output reg hs,         // Horizontal sync output, right to the monitor
  output reg vs,         // Vertical sync output, right to the monitor
  output wire [10:0] hc, // Pixel (horizontal) counter. Note that this counter is shifted 8 pixels, so active display begins at HC=8
  output wire [10:0] vc, // Scan (vertical) counter.
  output reg display_enable // Display enable signal.
  );

  // https://www.mythtv.org/wiki/Modeline_Database#VESA_ModePool
  // ModeLine "1024x768" 65.00 1024 1048 1184 1344  768 771 777 806 -HSync -VSync
  // ModeLine  "800x600" 40.00  800  840  968 1056  600 601 605 628 +HSync +VSync
  // ModeLine  "640x480" 25.18  640  656  752  800  480 490 492 525 -HSync -VSync
  //                      ^
  //                      +---- Pixel clock frequency in MHz
  parameter HACTIVE     = 800;
  parameter HFRONTPORCH = 840;
  parameter HSYNCPULSE  = 968;
  parameter HTOTAL      = 1056;
  parameter VACTIVE     = 600;
  parameter VFRONTPORCH = 601;
  parameter VSYNCPULSE  = 605;
  parameter VTOTAL      = 628;
  parameter HSYNCPOL    = 1'b0;  // 0 = Negative polarity, 1 = positive polarity
  parameter VSYNCPOL    = 1'b0;  // 0 = Negative polarity, 1 = positive polarity

  reg [10:0] hcont = 0;
  reg [10:0] vcont = 0;

  assign hc = hcont;
  assign vc = vcont;

  // Horizontal and vertical counters management.
  always @(posedge clk) begin
      if (~freeze) begin
          if (hcont == HTOTAL-1) begin
             hcont <= 11'd0;
             if (vcont == VTOTAL-1) begin
                vcont <= 11'd0;
             end
             else begin
                vcont <= vcont + 11'd1;
             end
          end
          else begin
             hcont <= hcont + 11'd1;
          end
      end
  end

  // Sync and active display enable generation, according to values in ModeLine.
  always @* begin
    if (hcont>=8 && hcont<HACTIVE+8 && vcont>=0 && vcont<VACTIVE)
      display_enable = 1'b1;
    else
      display_enable = 1'b0;

    if (hcont>=HFRONTPORCH+8 && hcont<HSYNCPULSE+8)
      hs = HSYNCPOL;
    else
      hs = ~HSYNCPOL;

    if (vcont>=VFRONTPORCH && vcont<VSYNCPULSE)
      vs = VSYNCPOL;
    else
      vs = ~VSYNCPOL;
  end

endmodule


`include "vga_char_dpram.v"
`include "vga_line_buf_mem.v"
`include "vga_text_line_buf_mem.v"

`default_nettype wire

`endif