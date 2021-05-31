//
// Copyright (c) 2021.  Micro Blue
//
// SPDX-License-Identifier: BSD-2-Clause-Patent
//

`default_nettype none

module keyboard (
    // Bus interface
    input  wire        clk,
    input  wire        resetn,
    input  wire        enable,
    input  wire        mem_valid,
    output wire        mem_ready,
    input  wire        mem_instr,
    input  wire [3:0]  mem_wstrb,
    input  wire [31:0] mem_wdata,
    input  wire [31:0] mem_addr,
    output wire [31:0] mem_rdata,

    output wire  [7:0] dbg_pin,

    input  wire        ps2_clk,
    input  wire        ps2_data

);

    assign  dbg_pin[0] = ps2_clk;
    assign  dbg_pin[3] = ps2_data;

    // bus read
    reg  [1:0] rd_state;
    reg        mem_ready_reg;
    always @ (posedge clk or negedge resetn) begin
        if (!resetn) begin
              kbd_rx_rd_req  <= 0;
              mem_ready_reg  <= 0;
              rd_state       <= 0;
        end else begin
            if (mem_valid & enable) begin
                // read
                mem_ready_reg <= 1;
                if (~(|mem_wstrb)) begin
                    case (mem_addr[3:2])
                    2'h0: begin
                        // generate fifo RD signal
                        case (rd_state)
                        0:  begin
                            mem_ready_reg    <= 0;
                            if (~kbd_rx_rd_empty)
                              kbd_rx_rd_req  <= 1;
                            rd_state         <= 1;
                        end
                        1:  begin
                            kbd_rx_rd_req    <= 0;
                            rd_state         <= 2;
                        end
                        2:  begin
                            rd_state         <= 0;
                        end
                        endcase
                    end
                    endcase
                end
            end else begin
                mem_ready_reg <= 0;
            end
        end
    end


    // FIFO
    wire       kbd_rx_rd_empty;
    wire       kbd_rx_rd_full;
    reg        kbd_rx_wr_req;
    wire [7:0] kbd_rx_rd_data;
    reg        kbd_rx_rd_req;
    reg  [7:0] kbd_rx_wr_data;
    byte_fifo  kbd_rx_fifo (
        .q       (kbd_rx_rd_data),
        .rdclk   (clk),
        .rdreq   (kbd_rx_rd_req),
        .wrclk   (clk),
        .wrreq   (kbd_rx_wr_req),
        .data    (kbd_rx_wr_data),
        .wrempty (kbd_rx_rd_empty),
        .wrfull  (kbd_rx_rd_full)
        );

    assign mem_rdata = {24'b0, (mem_addr[3:2] == 0) ? kbd_rx_rd_data : {7'h00, ~kbd_rx_rd_empty}};
    assign mem_ready = mem_ready_reg;


    // PS2 keyboard logic
    reg [1:0]  state;
    reg [1:0]  datasr;
    reg [1:0]  clksr;
    reg [10:0] rx_data;
    reg [17:0] rx_timeout;

    always @(posedge clk or negedge resetn)   begin
      if (!resetn) begin
        datasr     <= 2'b11;
        clksr      <= 2'b11;
        state      <= 0;

      end else begin

        kbd_rx_wr_req  <= 0;
        datasr         <= {datasr[0], ps2_data};
        clksr          <= {clksr[0],  ps2_clk};

        if (clksr == 2'b10)
          rx_data <= {datasr[1], rx_data[10:1]};

        case (state)
        0: begin
          rx_data    <= 11'h7ff;
          rx_timeout <= 17'h00001;
          if (datasr[1] == 0  && clksr[1] == 1) begin
            state    <= 1;
          end
        end
        1: begin
          if (rx_timeout == 17'h18000) begin
            state <= 0;
          end else if (rx_data[0] == 0) begin
            kbd_rx_wr_data <= rx_data[8:1];
            state          <= 2;
          end
        end
        2: begin
          state <= 0;
          kbd_rx_wr_req  <= 1;
        end
        endcase

      end
    end

endmodule