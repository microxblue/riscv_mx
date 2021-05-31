//
// Copyright (c) 2021.  Micro Blue
//
// SPDX-License-Identifier: BSD-2-Clause-Patent
//

// Component : Memory
module memory (
      input   io_enable,
      input   io_mem_valid,
      input   io_mem_instr,
      input  [3:0] io_mem_wstrb,
      input  [31:0] io_mem_wdata,
      input  [15:0] io_mem_addr,
      output [31:0] io_mem_rdata,
      output  io_mem_ready,
      input   clk,
      input   reset);

    reg rdy;
    wire [31:0] q;

    altsyncram the_altsyncram
      (
        .address_a (io_mem_addr[15:2]),
        .byteena_a (io_mem_wstrb),
        .clock0 (clk),
        .clocken0 (1'b1),
        .data_a (io_mem_wdata),
        .q_a (q),
        .wren_a (io_mem_valid & io_enable & (|io_mem_wstrb))
      );

    defparam the_altsyncram.intended_device_family = "Cyclone 10 LP",
             the_altsyncram.lpm_hint = "ENABLE_RUNTIME_MOD=NO",
             the_altsyncram.lpm_type = "altsyncram",
             the_altsyncram.init_file = "../RiscvApp/Bootloader/out/firmware.mif",
             the_altsyncram.lpm_type = "altsyncram",
             the_altsyncram.numwords_a = 10240,
             the_altsyncram.operation_mode = "SINGLE_PORT",
             the_altsyncram.outdata_reg_a = "UNREGISTERED",
             the_altsyncram.ram_block_type = "AUTO",
             the_altsyncram.read_during_write_mode_mixed_ports = "DONT_CARE",
             the_altsyncram.width_a = 32,
             the_altsyncram.width_byteena_a = 4,
             the_altsyncram.widthad_a = 14,
	           the_altsyncram.clock_enable_input_a = "BYPASS",
	           the_altsyncram.clock_enable_output_a = "BYPASS";

	  reg rdy1 = 0;
    always @(negedge clk)
    begin
          if (io_mem_valid & io_enable) begin
             rdy1 <= 1;
             rdy  <= rdy1;
          end  else begin
             rdy1 <= 0;
             rdy  <= 0;
          end
    end

    // Tri-state the outputs.
    assign io_mem_rdata = io_enable ? q : 'b0;
    assign io_mem_ready = rdy;

endmodule
