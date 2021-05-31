//
// Copyright (c) 2021.  Micro Blue
//
// SPDX-License-Identifier: BSD-2-Clause-Patent
//

//
// SPI debug for for picorv32
//
`timescale 1 ps / 1 ps

`define  SPI_IDLE   4'h0
`define  SPI_CMD    4'h1
`define  SPI_ADR    4'h2
`define  SPI_RD     4'h3
`define  SPI_WR     4'h4
`define  SPI_NOP    4'hf

module spi_dbg (
    // Bus interface
    input  wire        clk,
    input  wire        clk2,
    input  wire        resetn,

    input  wire        spi_cs,
    input  wire        spi_ck,
    input  wire        spi_mosi,
    output wire        spi_miso,

    input  wire [15:0]   sys_sts,
    output reg  [15:0]   sys_ctl,

    output wire  [7:0]   dbg_pin,

    output wire        sram_wr,
    output wire [9:0]  sram_addr,
    output wire [15:0] sram_wdata,
    input  wire [15:0] sram_rdata
);

reg    rd_s;
reg    samp;

// always sample spi data at spi clk rising edge
reg    spi_in;
always @(posedge spi_ck)  spi_in  <= spi_mosi;

// sample spi_ck at 2 x clk
// using a 3-bits shift register so as to keep the flag
// asserting for 1 full 1 x clk cycle
reg    [2:0] spi_ckr;
always @(posedge clk2)  spi_ckr <= {spi_ckr[1:0], spi_ck};
wire   spi_ck_rising  = ({spi_ckr[2],spi_ckr[0]} == 2'b01);
wire   spi_ck_falling = ({spi_ckr[2],spi_ckr[0]} == 2'b10);

// same thing for spi_cs, but no need to 2 x clk
reg    [1:0] spi_csr;
always @(posedge clk) spi_csr <= {spi_csr[0], spi_cs};
wire   spi_cs_rising  = (spi_csr[1:0]==2'b01);
wire   spi_cs_falling = (spi_csr[1:0]==2'b10);
wire   spi_cs_active  = ~spi_csr[1];

// TX
reg  [15:0] spi_rx_data;
reg  [15:0] spi_cmd;
reg  [15:0] spi_addr;
wire [7:0]  spi_rx_wire;
wire [15:0] spi_rx_word;
reg  [7:0]  spi_rx_shift;
reg  [3:0]  spi_ck_cnt;
reg  [3:0]  spi_rx_rdy;
reg  [9:0]  spi_dp_waddr;
reg  [9:0]  spi_dp_raddr;
reg  [3:0]  spi_state;
reg  [1:0]  spi_flag;

reg  [15:0]  spi_tx_shift;
reg          spi_tx_out;

assign spi_rx_wire = {spi_rx_shift[6:0], spi_in};
assign spi_rx_word = {spi_rx_wire, spi_rx_data[7:0]};

always @(posedge clk) begin
  if (~resetn) begin
    sys_ctl      <= 16'h0000;
    spi_ck_cnt   <= 4'h00;
    spi_rx_rdy   <= 4'h00;
    spi_flag     <= 2'b00;
    spi_state    <= `SPI_IDLE;

  end else begin
    if (spi_cs_falling || spi_cs_rising) begin
      spi_ck_cnt   <= 4'h00;
      spi_rx_rdy   <= 4'h00;
      spi_flag     <= 2'b00;
      spi_state    <= `SPI_IDLE;
      spi_rx_data  <= 16'h00;
      spi_tx_shift <= 8'h00;
      spi_rx_shift <= 8'h00;
    end else begin

      // handle RX
      spi_rx_rdy   <= {1'b0, spi_rx_rdy[3:1]};
      if (spi_ck_rising) begin

        if (spi_ck_cnt == 4'b0000) begin
          if (spi_state == `SPI_IDLE) begin
            spi_state <= `SPI_ADR;
          end
        end

        if (spi_ck_cnt[2:0] == 3'b111) begin
          if (spi_ck_cnt[3]) begin

            if (spi_state == `SPI_ADR) begin
              spi_state     <= `SPI_CMD;
              spi_dp_raddr   <= spi_rx_word[10:1];
              spi_addr[15:0] <= spi_rx_word;
            end

            if (spi_state == `SPI_CMD) begin
              spi_cmd[15:0] <= spi_rx_word;
              if (spi_rx_data[0]) begin
                // Write need to set address at N-1 so that SPI_WR state has proper address
                spi_state    <= `SPI_WR;
                if (spi_rx_data[1]) begin
                  spi_dp_waddr <= spi_dp_raddr;
                  if (spi_rx_data[2] == 1'b0) begin
                    sys_ctl <= spi_addr;
                  end
                end else begin
                  spi_dp_waddr <= spi_dp_raddr + 10'h3FF;
                end
              end else begin
                // Read need to set address at N
                spi_state    <= `SPI_RD;
                spi_dp_raddr <= spi_dp_raddr + 10'h001;
              end
            end

            if (spi_state == `SPI_RD) begin
              spi_dp_raddr <= spi_dp_raddr + 10'h001;
            end

            if (spi_state == `SPI_WR) begin
              // DP RAM is running at half of clk,
              // so keep sram_wr for 2 cycles
              spi_dp_waddr <= spi_dp_waddr + 10'h001;
              spi_rx_rdy   <= spi_cmd[1] ? 4'b0000 : 4'b0001;
            end
            spi_rx_data[15:8] <= spi_rx_wire;
          end else begin
            spi_rx_data[7:0]  <= spi_rx_wire;
          end
        end else begin
          spi_rx_shift <= spi_rx_wire;
        end
        spi_ck_cnt   <= spi_ck_cnt + 4'b0001;
      end

      // handle TX
      if (spi_ck_falling) begin
        if (spi_ck_cnt[3:0] == 4'b1111) begin
            if ((spi_state == `SPI_RD) && spi_cmd[1]) begin
              case (spi_dp_raddr)
              10'h000: spi_tx_shift <= {sys_sts[7:0], sys_sts[15:8]};
              endcase
            end
        end else begin
          spi_tx_shift <= {spi_tx_shift[14:0], 1'b0};
        end
        spi_tx_out <= spi_tx_shift[15];
      end

    end
  end
end

assign spi_miso   = spi_tx_out;
assign sram_wr    = spi_rx_rdy[0] & spi_cs_active;
assign sram_addr  = sram_wr ? spi_dp_waddr : spi_dp_raddr;
assign sram_wdata = spi_rx_data;

endmodule

`include "spi_dbg_dpram.v"
