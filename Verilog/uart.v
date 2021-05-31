//
// Copyright (c) 2021.  Micro Blue
//
// SPDX-License-Identifier: BSD-2-Clause-Patent
//

//
// Uart for for picorv32
//
`timescale 1 ps / 1 ps

module uart  #(
   // f / 115200 = N
   // f= 60M  N=521
   // f= 80M  N=694
   // f=100M  N=868
   // f=200M  N=1736
    parameter [31:0] BAUD_DIVIDER = 694
) (
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

    output wire [7:0]  dbg_pin,

    // Serial interface
    output reg         uart_txd,
    input  wire        uart_rxd
);

    wire [7:0]    uart_lsr_sts;
    reg  [1:0]  curr_rxd;
    reg  [9:0]  sample_cnt;
    reg  [9:0]  rx_data;
    reg  [3:0]  bit_cnt;
    reg  [7:0]  rd_buf;
    reg  rx_sts_dav;
    reg  rx_start;
    reg  uart_rx_err_ovr;
    reg  uart_rx_err_frm;
    reg  rx_rdy;
    reg  [7:0] uart_fcr;
    reg  [7:0] uart_thr;
    reg  [7:0] uart_rx_wr_data;
    reg        mem_ready_reg;

    assign  dbg_pin[0] = mem_valid & enable & (mem_addr[4:2] == 0) & (~(|mem_wstrb));
    assign  dbg_pin[1] = uart_rx_rd_req;
    assign  dbg_pin[2] = mem_ready_reg;
    assign  dbg_pin[3] = clk;

    // UART RX Logic
    always @ (posedge clk or negedge resetn) begin
        if (!resetn) begin
            curr_rxd     <= 2'b11;
            sample_cnt  <= 0;
            rx_data     <= 0;
            bit_cnt     <= 0;
            rx_start    <= 0;
            rd_buf      <= 0;
            rx_sts_dav  <= 0;
            uart_rx_err_ovr  <= 0;
            uart_rx_err_frm  <= 0;
            rx_rdy      <= 0;
        end else begin
            uart_rx_wr_req <= 0;
            if (mem_valid & enable) begin
                if (mem_wstrb[0] == 1'b1) begin
                    // write
                    case (mem_addr[2:0])
                    3'h5: begin
                        if (mem_wdata[1])  uart_rx_err_ovr <= 1'b0;
                        if (mem_wdata[3])  uart_rx_err_frm <= 1'b0;
                    end
                    endcase
                end
            end

            if (rx_start == 0) begin
                if (curr_rxd == 2'b10) begin
                    sample_cnt <= BAUD_DIVIDER >>> 1;
                    rx_start   <= 1;
                    bit_cnt    <= 0;
                end
            end else begin
                if (sample_cnt == 0) begin
                    sample_cnt  <= BAUD_DIVIDER;
                    rx_data     <= {curr_rxd[0], rx_data[9:1]};
                    if ((bit_cnt == 0) && curr_rxd[0]) begin
                        // start bit is 1
                        uart_rx_err_frm <= 1;
                    end else begin
                        if (bit_cnt == 9) begin
                            bit_cnt  <= 0;
                            rx_start <= 0;
                            if (curr_rxd[0] & ~rx_data[1]) begin
                                // received sucessfully
                                rx_sts_dav     <= 1;
                                if (~uart_rx_rd_full) begin
                                    uart_rx_wr_req  <= 1;
                                    uart_rx_wr_data <= rx_data[9:2];
                                end else begin
                                    uart_rx_err_ovr <= 1;
                                end
                            end else begin
                                // data error
                                uart_rx_err_frm <= 1;
                            end
                        end else begin
                            bit_cnt <= bit_cnt + 1;
                    end
                    end
                end else begin
                    sample_cnt <= sample_cnt - 1;
              end
            end
            curr_rxd <= {curr_rxd[0], uart_rxd};
        end
    end


    // Internal Variables
    reg [7:0]  shifter;
    reg [7:0]  wr_buf;
    reg [7:0]  state;
    reg [3:0]  tx_bit_cnt;
    reg [19:0] bit_timer;
    reg        tx_sts_rdy;          // TRUE when ready to accept next character.
    reg  [1:0] rd_state;
    reg        uart_rx_rd_req;

    // UART TX Logic
    always @ (posedge clk or negedge resetn) begin
        if (!resetn) begin
            state          <= 0;
            wr_buf         <= 0;
            tx_sts_rdy     <= 1;
            shifter        <= 0;
            uart_txd       <= 1;
            tx_bit_cnt     <= 0;
            bit_timer      <= 0;
            uart_fcr       <= 8'h00;
            uart_rx_rd_req <= 0;
            rd_state       <= 0;

        end else begin
            if (mem_valid & enable) begin
                // read
                mem_ready_reg <= 1;
                if (~(|mem_wstrb)) begin
                    case (mem_addr[4:2])
                    3'h0: begin
                        // generate fifo RD signal
                        case (rd_state)
                        0:  begin
                            mem_ready_reg    <= 0;
                            if (~uart_rx_rd_empty)
                              uart_rx_rd_req <= 1;
                            rd_state         <= 1;
                        end
                        1:  begin
                            uart_rx_rd_req   <= 0;
                            rd_state         <= 2;
                        end
                        2:  begin
                            rd_state         <= 0;
                        end
                        endcase
                    end
                    endcase
                end else if (mem_wstrb[0] == 1) begin
                    // write
                    case (mem_addr[4:2])
                    3'h0: begin
                        if (tx_sts_rdy == 1) begin
                            wr_buf     <= mem_wdata[7:0];
                            tx_sts_rdy <= 0;
                        end
                    end
                    3'h2: uart_fcr <= mem_wdata[7:0];  // FCR
                    endcase
                end
            end else begin
                mem_ready_reg <= 0;
            end

            // Generate bit clock timer for 115200 baud from 50MHz clock
            if (bit_timer == BAUD_DIVIDER)
                bit_timer <= 0;
            else
                bit_timer <= bit_timer + 1;

            if (bit_timer == 0) begin
                case (state)
                    // Idle
                    0 : begin
                        if (tx_sts_rdy == 0) begin
                            shifter    <= wr_buf;
                            tx_sts_rdy <= 1;
                            tx_bit_cnt   <= 8;
                            // Start bit
                            uart_txd <= 0;
                            state <= 1;
                        end
                    end
                    // Transmitting
                    1 : begin
                        if (tx_bit_cnt > 0) begin
                            // Data bits
                            uart_txd <= shifter[0];
                            tx_bit_cnt <= tx_bit_cnt - 1;
                            shifter <= shifter >> 1;
                        end else begin
                            // Stop bit
                            uart_txd <= 1;
                            state <= 0;
                        end
                    end
                endcase
            end
        end
    end

    wire       uart_rx_rd_empty;
    wire       uart_rx_rd_full;
    reg        uart_rx_wr_req;
    wire [7:0] uart_rx_rd_data;

    byte_fifo uart_rx_fifo (
        .aclr    (uart_fcr[2]),
        .q       (uart_rx_rd_data),
        .rdclk   (clk),
        .rdreq   (uart_rx_rd_req),
        .wrclk   (clk),
        .wrreq   (uart_rx_wr_req),
        .data    (uart_rx_wr_data),
        .wrempty (uart_rx_rd_empty),
        .wrfull  (uart_rx_rd_full)
        );

    reg [7:0] rdata;
    always @(*) begin
       case (mem_addr[4:2])
       3'h0:    rdata = uart_rx_rd_data;
       3'h5:    rdata = uart_lsr_sts;
     default: rdata = 0;
       endcase
    end

    assign uart_lsr_sts = {2'b0, tx_sts_rdy, 1'b0, uart_rx_err_frm, 1'b0, uart_rx_err_ovr, ~uart_rx_rd_empty};
    assign mem_rdata = {24'b0, rdata};
    assign mem_ready = mem_ready_reg;

endmodule

`include  "byte_fifo.v"
