//
// Copyright (c) 2021.  Micro Blue
//
// SPDX-License-Identifier: BSD-2-Clause-Patent
//

module ps2_joystick
(
    //Clock and Reset
    input  wire         clk,
    input  wire         resetn,

    output wire [7:0]   dbg_pin,

    //NES joypad
    input  wire         jp_latch,
    input  wire         jp_clk,
    output wire         jp_dat1,
    output wire         jp_dat2,

    //PS2 joystick
    output wire         jstk_cs,
    output wire         jstk_ck,
    output wire         jstk_mo,
    input  wire         jstk_mi,

    //Key state
    output wire [15:0]  jstk_state
);

    // ps2 joy stick
    reg [19:0] clk_cnt;
    reg        js_cs_pre;
    reg        js_cs;
    reg        js_ck;
    reg        js_mo;
    reg [31:0] temp;
    reg [15:0] data;
    always @ (posedge clk or negedge resetn) begin
        if (!resetn) begin
          clk_cnt    <= 0;
          js_cs      <= 1;
          js_ck      <= 1;
          js_cs_pre  <= 0;
          data       <= 16'hffff;
        end else begin

          js_cs_pre <= (clk_cnt > 20'hf4000) ? 0 : 1;
          js_cs     <= (clk_cnt < 20'h11f00) ? 0 : 1;

          if (clk_cnt == 20'hf4240) begin
              clk_cnt <= 0;
              data    <= {temp[23:16], temp[7:0]};
          end else
              clk_cnt <= clk_cnt + 1;

          if (~js_cs) begin
              js_ck <= (clk_cnt[12:9] < 4'h8) ? clk_cnt[8] : 1;
              if (((clk_cnt[16:13] == 4'h0) && (clk_cnt[12:9] == 4'h0)) ||
                  ((clk_cnt[16:13] == 4'h1) && (clk_cnt[12:9] == 4'h1)) ||
                  ((clk_cnt[16:13] == 4'h1) && (clk_cnt[12:9] == 4'h6)))
                js_mo <= 1;
              else
                js_mo <= 0;

              if ((clk_cnt[16:13] == 4'h3) || (clk_cnt[16:13] == 4'h4)) begin
                // sample 3 and 4 bytes
                if (clk_cnt[8:0] == 9'hff)
                  temp <= {jstk_mi, temp[31:1]};
              end

          end else begin
              js_ck <= 1;
              js_mo <= 1;
          end

        end
    end

    assign jstk_cs = js_cs & js_cs_pre;
    assign jstk_ck = js_ck;
    assign jstk_mo = js_mo;

    // detect rising edge
    reg    [1:0] jp_ck_csr;
    always @(posedge clk) jp_ck_csr <= {jp_ck_csr[0], jp_clk};
    wire   jp_ck_rising  = (jp_ck_csr[1:0]==2'b01);

    reg    [1:0] jp_lat_csr;
    always @(posedge clk) jp_lat_csr <= {jp_lat_csr[0], jp_latch};
    wire   jp_lat_rising = (jp_lat_csr[1:0]==2'b01);

    reg  [7:0] jp_data;
    always @ (posedge clk or negedge resetn) begin
      if (!resetn) begin
        jp_data <= 8'hff;
      end else begin
        if (jp_lat_rising) begin
          jp_data <= {data[12],data[13],data[15],data[14],data[4],data[6],data[7],data[5]};
        end else if (jp_ck_rising) begin
          jp_data <= {jp_data[6:0], 1'b1};
        end
      end
    end

    assign  jp_dat1    = jp_data[7];
    assign  jp_dat2    = 1'b1;
    assign  jstk_state = data;

endmodule

