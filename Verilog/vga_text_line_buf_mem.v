//
// Copyright (c) 2021.  Micro Blue
//
// SPDX-License-Identifier: BSD-2-Clause-Patent
//

// synopsys translate_off
`timescale 1 ps / 1 ps

// synopsys translate_on
module vga_text_line_buf_mem (
	input  wire         clk,
	input  wire [5:0]   rdaddress,
	input  wire [4:0]   wraddress,
	input  wire [31:0]  wdata,
	input               wren,
	output reg  [15:0]  rdata
	);

reg [31:0] text_line_buf[32];

reg [31:0] data;
always @(posedge clk)
begin
  if (wren) begin
			text_line_buf[wraddress[4:0]] <= wdata;
	end else begin
			rdata <= (rdaddress[0]) ? text_line_buf[rdaddress[5:1]][31:16] : text_line_buf[rdaddress[5:1]][15:0];
  end
end

endmodule
