//
// GPIO controller
//

module gpio (
	     // Bus interface
	     input wire 	clk,
	     input wire 	resetn,
	     input wire 	enable,
	     input wire 	mem_valid,
	     output wire 	mem_ready,
	     input wire 	mem_instr,
	     input wire [3:0] 	mem_wstrb,
	     input wire [31:0] 	mem_wdata,
	     input wire [31:0] 	mem_addr,
	     output wire [31:0] mem_rdata,

	     // gpio interface
	     output wire [31:0] 	gpio,
	     input wire [2:0] 	  dip,
	     input wire [3:0] 	  btn
	     );

   reg [7:0] 	q;
   reg 				rdy;
   reg [31:0] gpio_reg;

   assign gpio = gpio_reg;

   always @(posedge clk) begin
      if (!resetn) begin
         rdy <= 0;
         q <= 0;
         gpio_reg <= 0;
      end else if (mem_valid & enable) begin
         if (mem_wstrb[0])
           gpio_reg[7:0]  <= mem_wdata[7:0];
         if (mem_wstrb[1])
           gpio_reg[15:8] <= mem_wdata[15:8];
         if (mem_wstrb[2])
           gpio_reg[23:16] <= mem_wdata[23:16];
         if (mem_wstrb[3])
           gpio_reg[31:24] <= mem_wdata[31:24];

         rdy <= 1;
      end else begin
         rdy <= 0;
      end
      if (mem_addr[4] == 1'b1)
         q <= {9'b0, dip, btn};
      else
         q <= {16'h00, gpio};
   end

   // Wire-OR'ed bus outputs.
   assign mem_rdata = enable ? q : 1'b0;
   assign mem_ready = enable ? rdy : 1'b0;

endmodule
