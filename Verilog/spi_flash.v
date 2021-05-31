//
// Copyright (c) 2021.  Micro Blue
//
// SPDX-License-Identifier: BSD-2-Clause-Patent
//

`include "soc_cfg.v"

`ifdef  CFG_SPI_FLASH

`define  CLK_DLY      4'h3

//
// SPI flash for for picorv32
//
module spi_flash (
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

    output wire [3:0]  dbg_led,
    output wire [3:0]  dbg_pin,

     // spi flash interface
     output wire        spi_cs,
     output wire        spi_ck,
     output wire        spi_mosi,
     input  wire        spi_miso
);

    reg         rdy;
    reg         scs;
    reg         sck;
    reg         sdo;
    reg         busy;
    reg         scs_end;
    reg  [3:0]  state;
    reg  [7:0]  bcnt;
    reg [31:0]  data;
    reg [31:0]  sdata;
    reg [31:0]  rdata;
    reg [31:0]  spi_recv;
    reg  [3:0]  led_reg;

    always @(posedge clk) begin
        if (!resetn) begin
            rdy     <= 1'b0;
            scs     <= 1'b1;
            sck     <= 1'b0;
            sdo     <= 1'b0;
            bcnt    <= 8'h00;
            state   <= 4'h0;
            led_reg <= 4'h0;
            scs_end <= 1'b0;
        end else begin
            if (mem_valid & enable) begin
                if (|mem_wstrb) begin
                    if (&mem_wstrb) begin
                        if (mem_addr[3:2] == 2'b00) begin
                            // write SPI command
                            sdata    <= mem_wdata;
                        end else if (mem_addr[3:2] == 2'b01) begin
                            // write SPI control
                            scs      <= ~mem_wdata[8];
                            scs_end  <=  mem_wdata[9];
                            if (mem_wdata[31]) begin
                                spi_recv <= 32'b0;
                                data     <= sdata;
                                bcnt     <= {3'b00, mem_wdata[1:0] + 2'b11, 3'b111};
                                state    <= 4'h1;
                                busy     <= 1;
                            end
                        end
                    end
                    rdy   <= 1'b1;
                end else begin
                    if (mem_addr[3:2] == 2'b00) begin
                        rdata <= sdata;
                    end else if (mem_addr[3:2] == 2'b01) begin
                        rdata <= {busy, 7'h00, 8'h00, 6'h00, ~scs, scs_end, bcnt};
                    end else begin
                        rdata <= spi_recv;
                    end
                    rdy   <= 1'b1;
                end
            end else begin
                rdy   <= 1'b0;
            end

            case (state)
                4'h1: begin
                  scs    <= 1'b0;
                  state  <= 4'h3;
                end

                4'h2: begin
                  sck      <= 1'b1;
                  state    <= 4'h3;
                end

                4'h3: begin
                  spi_recv <= {spi_recv, spi_miso};
                  sck      <= 1'b0;
                  sdo      <= data[31];
                  data     <= {data[30:0], 1'b0};
                  if (bcnt == 8'hFF) begin
                    state  <= 4'h4;
                  end else begin
                    bcnt   <= bcnt - 8'h01;
                    state  <= 4'h2;
                  end
                end

                4'h4: begin
                  sck   <= 1'b0;
                  scs   <= ~scs_end;
                  state <= 4'h0;
                  busy  <= 0;
                end

            endcase
        end
    end

    // Tri-state the bus outputs.
    assign spi_ck    = sck;
    assign spi_cs    = scs;
    assign spi_mosi  = sdo;
    assign mem_rdata = enable ? rdata : 'b0;
    assign mem_ready = rdy;
    assign dbg_pin   = {scs, sck, sdo, spi_miso};
    assign dbg_led   = state; //led_reg;

endmodule

`endif