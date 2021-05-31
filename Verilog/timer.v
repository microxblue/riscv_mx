//
// Copyright (c) 2021.  Micro Blue
//
// SPDX-License-Identifier: BSD-2-Clause-Patent
//

//
// Timer for for picorv32
//
`timescale 1 ps / 1 ps

module timer (
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
    output wire [31:0] mem_rdata
);
    reg [31:0] timers [6:0];
    reg [31:0] timer_hi_shadow;
    reg rdy;

    always @(posedge clk) begin
        if (!resetn) begin
            rdy <= 0;
            timers[0] <= 32'h0;
            timers[1] <= 32'h0;
            timers[2] <= 32'h0;
            timers[3] <= 32'h0;
            timers[4] <= 32'h0;
            timers[5] <= 32'h0;
            timer_hi_shadow <= 32'h0;
        end else begin
            if (timers[0]) timers[0] <= timers[0] - 32'h1;
            if (timers[1]) timers[1] <= timers[1] - 32'h1;
            if (timers[2]) timers[2] <= timers[2] - 32'h1;
            if (timers[3]) timers[3] <= timers[3] - 32'h1;
            {timers[5],timers[4]} <= {timers[5],timers[4]} + 64'h1;
            if (mem_valid & enable)
                begin
                    if (|mem_wstrb)
                        case (mem_addr[4:2])
                        3'h0:    timers[0] <= mem_wdata;
                        3'h1:    timers[1] <= mem_wdata;
                        3'h2:    timers[2] <= mem_wdata;
                        3'h3:    timers[3] <= mem_wdata;
                        default:;
                        endcase
                    else begin
                        if (mem_addr[4:2] == 3'h4)
                            timer_hi_shadow <= timers[5];
                    end
                    rdy <= 1;
                end
            else
                begin
                    rdy <= 0;
                end
        end
    end

    reg [31:0]  data_out;
    always @(*) begin
        data_out  = 32'h00;
        case (mem_addr[4:2])
            3'h0: data_out = timers[0];
            3'h1: data_out = timers[1];
            3'h2: data_out = timers[2];
            3'h3: data_out = timers[3];
            3'h4: data_out = timers[4];
            3'h5: data_out = timer_hi_shadow;
            default:;
        endcase
    end

    // Tri-state the bus outputs.
    assign mem_rdata = data_out;
    assign mem_ready = rdy;

endmodule
