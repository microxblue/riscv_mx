//
// Copyright (c) 2021.  Micro Blue
//
// SPDX-License-Identifier: BSD-2-Clause-Patent
//

//
// Timer for for picorv32
//
module sys_ctrl (
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

    output wire        cpu_rst,
    input  wire [15:0] jstk_state
);

    reg   fpga_rst = 1'b0;
    reg   rdy;

    assign cpu_rst = fpga_rst;
    always @(posedge clk) begin
        if (!resetn) begin
            fpga_rst <= 1'b0;
            rdy <= 0;
        end else begin
            fpga_rst <= 0;
            if (mem_valid & enable) begin
                if (|mem_wstrb) begin
                    fpga_rst <= mem_wdata[31];
                end
                rdy <= 1;
            end else begin
                rdy <= 0;
            end
        end
    end

    reg [31:0]  data_out;
    always @(*) begin
        data_out  = 32'h00;
        if (mem_valid & enable & ~(|mem_wstrb)) begin
          case (mem_addr[4:2])
              3'h0: data_out = {16'h8000, jstk_state};
              default:;
          endcase
        end
    end

    // Tri-state the bus outputs.
    assign mem_rdata = data_out;
    assign mem_ready = rdy;

endmodule
