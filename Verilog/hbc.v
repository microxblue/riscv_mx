//
// Copyright (c) 2021.  Micro Blue
//
// SPDX-License-Identifier: BSD-2-Clause-Patent
//

//
// HyperRAM 66WVH8M8ALL
// REGS:
//   Die0:
//     0xf1000000 ID 0
//     0xf1100000 ID 1
//     0xf1001000 Register 0
//     0xf1101000 Register 1
//   Die1:
//    +0x00800000
// 66WVH8M8ALL CFG TIMINGS:
//   CAS     x1 latency  x2 latency
//    3    3   9
//    4    5   13
//    5    7   17
//    6    9   21
// For 2 dies, only 2x latency should be used.
// EXAMPLES:
//   Use 1x latency 6 clk:
//     ww f1001000 8f17
//   Use 2x latency 6 clk:
//     ww f1001000 8f1f
//   Use 1x latency 3 clk:
//     ww f1001000 8fe7
//   Use 1x latency 6 clk:
//     ww f1001000 8f17
//   Write CAS 3:
//     wb f1000000 03
//   Init 2x laentcy 3 (allowed fastest timing for 66WVH8M8ALL)
//     ww f1001000 0x8fef
//     ww f1801000 0x8fef
//     wb f1000000 9
//

`ifdef  CFG_HYPER_RAM


module hbc
#(
  // default  80M (Hyperbus) / 80M (mem) /40M (ext)
	parameter HYPER_BUS_TO_MEM_BUS_FREQ_RATIO = 1,   // Valid value is 1 or 2
	parameter HYPER_BUS_TO_EXT_BUS_FREQ_RATIO = 2,   // Valid value is 2 or 4
	parameter HYPER_RAM_DEFAULT_LATENCY       = 6    // Valid value is 3, 4, 5, 6
)
  (
    input                 clk,
    input                 resetn,

    input                 mem_clk,
    input                 mem_enable,
    input                 mem_valid,
    output reg            mem_ready,
    input        [3:0]    mem_wstrb,
    input       [31:0]    mem_addr,
    input       [31:0]    mem_wdata,
    output reg  [31:0]    mem_rdata,

    output       [7:0]    dbg_pin,

    input                 ext_clk,
    input                 ext_rd_req,
    input        [7:0]    ext_burst_dw,
    output                ext_burst_ready,
    input       [31:0]    ext_addr,

    output                hpr_csn,
    output reg            hpr_clk,
    output                hpr_clkn,
    inout        [7:0]    hpr_dq,
    inout                 hpr_rwds,
    output                hpr_resetn
  );

  parameter IDLE        = 0;
  parameter CMDADR      = 1;
  parameter READ        = 2;
  parameter WRITE       = 3;

  reg  [47:0]   ca;
  reg   [2:0]   state;
  reg   [3:0]   mem_ready_cycle;
  reg  [31:0]   wdata_latch;
  reg   [3:0]   wstrb_latch;
  reg   [4:0]   count;
  reg   [7:0]   latency;
  reg  [31:0]   tdata_buf;
  reg           rd_req;
  reg           is_reg;
  reg           rd_next;
  reg           burst_ready;
  reg   [7:0]   burst_cnt;
  reg   [1:0]   ext_bus_state;
  reg   [1:0]   clk_cnt;

  //assign dbg_pin[0] = hpr_clk;
  //assign dbg_pin[1] = burst_ready;
  //assign dbg_pin[2] = mem_ck_rising;
  //assign dbg_pin[3] = ext_ck_rising;

  // detect edge for memory bus clock.
  // mem_clk freq needs to be half or same as clk
  // reg    [1:0]  mem_ckr;
  // always @(posedge clk)  mem_ckr <= {mem_ckr[0], mem_clk};
  // wire   mem_ck_rising = (mem_ckr == 2'b01);

  // detect edge for exernal bus clock.
  // ext_clk freq needs to be half or quad of clk
  // reg    [1:0]  ext_ckr;
  // always @(posedge clk)  ext_ckr <= {ext_ckr[0], ext_clk};

  wire   mem_ck_rising = (HYPER_BUS_TO_MEM_BUS_FREQ_RATIO == 2) ? ~clk_cnt[0] : 1;
  wire   ext_ck_rising = (HYPER_BUS_TO_EXT_BUS_FREQ_RATIO == 4) ? (clk_cnt[1:0] == 2'b01) : (clk_cnt[0] == 0);

  // populate data into 32bit dword
  wire  [31:0]  rdata_cur = {hpr_dq, tdata_buf[31:8]};

  always @(posedge clk or negedge resetn) begin

      if(!resetn) begin
        state           <= IDLE;
        mem_ready       <= 0;
        mem_ready_cycle <= 0;
        burst_ready     <= 0;
        ext_bus_state   <= 2'b00;
        clk_cnt         <= 0;
        latency         <= HYPER_RAM_DEFAULT_LATENCY * 4 - 3;
      end else begin
        // track clk alignment with mem and ext clk
        clk_cnt <= clk_cnt + 1;

        // hold the ready signal for required memory bus cycle length
        if (mem_ready_cycle)
          mem_ready_cycle <= mem_ready_cycle - 1;
        else
          mem_ready <= 0;

        // handle external bus read request
        if (ext_bus_state == 2'b00) begin

          if (ext_rd_req & ext_ck_rising) begin
            // log the request first, handle it later on when idle
            // burst read 16 * 4 bytes per request
            ext_bus_state <= 2'b01;
            burst_cnt     <= ext_burst_dw;
            burst_ready   <= 0;
          end
        end

        case (state)
        IDLE: begin
          burst_ready  <= 0;
          if (ext_bus_state == 2'b01) begin
              // align on external bus rising edge
              if (ext_ck_rising) begin
                ext_bus_state <= 2'b10;
                is_reg     <= 0;
                rd_req     <= 1;
                ca[47]     <= 1; // read
                ca[46]     <= 0; // mem
                ca[45]     <= 1; // linear burst
                ca[44:36]  <= 0;
                ca[35:16]  <= ext_addr[23:4];
                ca[15:3]   <= 0;
                ca[2:1]    <= ext_addr[3:2];
                ca[0]      <= 0;
                state      <= CMDADR;
                count      <= 5;
              end
            end else if (mem_enable & mem_valid & ~mem_ready) begin
              if (mem_ck_rising) begin
                is_reg      <= mem_addr[24];
                rd_req      <= ~(|mem_wstrb);        // 1: read  0:write
                ca[47]      <= ~(|mem_wstrb);        // 1: read  0:write
                ca[46]      <= mem_addr[24];         // 1: reg  0:mem
                ca[45]      <= 1;                    // 1: linear burst 0: wrapped burst
                ca[44:36]   <= 0;                    // reserved 0
                ca[35:22]   <= mem_addr[23:10];      // row address
                ca[21:16]   <= mem_addr[9:4];        // upper column address
                ca[15:3]    <= 0;                    // reserved 0
                ca[2:1]     <= mem_addr[3:2];        // lower column address
                ca[0]       <= mem_addr[24] ? mem_addr[20] : 1'b0;  // always 0 unless for reg 1 access
                wdata_latch <= mem_wdata;
                wstrb_latch <= mem_wstrb;
                state       <= CMDADR;
                count       <= 5;
                if (mem_addr[24] & ~mem_addr[12] & mem_wstrb[0]) begin
                  // redirect identification register write to update write latency register
                  latency <= mem_wdata[7:0];
                end
            end
          end
        end

        CMDADR: begin
          ca[47:0]  <= {ca[39:0], 8'h00};
          if (count) begin
            count   <= count - 1;
          end else begin
            // prepare latency for read and write
            rd_next <= 0;
            state   <= rd_req ? READ : WRITE;
            count   <= rd_req ? ((HYPER_BUS_TO_MEM_BUS_FREQ_RATIO == 2) ? 5 : 3) : (is_reg ? 1 : latency + 4);
          end
        end

        READ: begin
          if (hpr_rwds || rd_next) begin
            // each RWDS reads one WORD (2 bytes)
            rd_next   <= ~rd_next;
            tdata_buf <= rdata_cur;

            if (count) begin
              count   <= count - 1;
            end else begin
              // for register read, fill upper 16 bits with current latency value
              mem_rdata <= is_reg ? {8'h00, latency, rdata_cur[15:0]} : rdata_cur;

              if (ext_bus_state[1]) begin
                burst_ready     <= 1;
                if (burst_cnt) begin
                  burst_cnt     <= burst_cnt - 8'h01;
                  count         <= 3;
                end else begin
                  ext_bus_state <= 2'b00;
                  state         <= IDLE;
                end
              end else begin
                mem_ready       <= 1;
                mem_ready_cycle <= HYPER_BUS_TO_MEM_BUS_FREQ_RATIO - 1;
                state           <= IDLE;
              end
            end
          end
        end

        WRITE: begin
          if (count < 4) begin
            wstrb_latch         <= {1'h00,  wstrb_latch[3:1]};
            wdata_latch         <= {8'h00,  wdata_latch[31:8]};
          end
          if (count) begin
            count               <= count - 1;
          end else begin
            mem_ready           <= 1;
            mem_ready_cycle     <= HYPER_BUS_TO_MEM_BUS_FREQ_RATIO - 1;
            state               <= IDLE;
          end
        end

        endcase

      end
  end

  assign ext_burst_ready  = burst_ready;


  // generate hyper ram clock signal
  // Hyper RAM operates at DDR rate, the actual clock is half of the input CLK
  // note: use clock negedge to ensure enough setup time
  always @(negedge clk or negedge resetn) begin
    if (!resetn)    hpr_clk  <= 0;
    else            hpr_clk  <= ~hpr_clk  & ~hpr_csn;
  end

  // drive hyper ram signals
  assign hpr_clkn   = ~hpr_clk;
  assign hpr_resetn = resetn;
  assign hpr_csn    = (state == IDLE)   ? 1 : 0;
  assign hpr_rwds   = (state == WRITE)  ? ~wstrb_latch[0] : 1'bz;
  assign hpr_dq     = (state == CMDADR) ? ca[47:40] : ((state == WRITE) ? wdata_latch[7:0] : 8'bz);

endmodule

`endif
