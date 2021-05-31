//
// Copyright (c) 2021.  Micro Blue
//
// SPDX-License-Identifier: BSD-2-Clause-Patent
//

module sys_bus (

		     input wire [31:0] 	mem_addr,
         output reg 	      mem_ready,
		     output reg [31:0]  mem_rdata,

         input wire       	mem_ready_memory,
		     input wire [31:0] 	mem_rdata_memory,

         input wire 	      mem_ready_uart,
		     input wire [31:0] 	mem_rdata_uart,

         input wire       	mem_ready_timer,
         input wire [31:0] 	mem_rdata_timer,

         input wire 	      mem_ready_sys_ctrl,
		     input wire [31:0] 	mem_rdata_sys_ctrl,

         input wire 	      mem_ready_spi_flash,
		     input wire [31:0] 	mem_rdata_spi_flash,

         input wire 	      mem_ready_dpram,
		     input wire [31:0] 	mem_rdata_dpram,

         input wire 	      mem_ready_vga,
		     input wire [31:0] 	mem_rdata_vga,


         input wire 	      mem_ready_kbd,
		     input wire [31:0] 	mem_rdata_kbd,

         input wire 	      mem_ready_usb_dpram,
		     input wire [31:0] 	mem_rdata_usb_dpram,

		     input wire 	      mem_ready_gpio,
		     input wire [31:0] 	mem_rdata_gpio,

         input wire 	      mem_ready_hyper_ram,
		     input wire [31:0] 	mem_rdata_hyper_ram,

		     output reg [15:0] 	enables

		     );

   always @(*) begin
      enables = 0;
      case (mem_addr[31:8])
        24'hffff01: enables[1]  = 1'd1;
        24'hffff02: enables[2]  = 1'd1;
        24'hffff03: enables[3]  = 1'd1;
        24'hffff04: enables[4]  = 1'd1;
        24'hffff05: enables[5]  = 1'd1;
        24'hffff06: enables[10] = 1'd1;
        default: begin
           if (mem_addr[31:26] == 6'b111100)
              enables[0] = 1'd1;
           else if (mem_addr[31:12] == 20'hffff1)
              enables[6] = 1'd1;
           else if (mem_addr[31:12] == 20'hffff2)
              enables[8] = 1'd1;
           else if ((mem_addr[31:12] == 20'hffff4) || (mem_addr[31:12] == 20'hffff5)) // hffff4 and hffff5
              enables[9] = 1'd1;
           else
              enables[7] = 1'd1;
        end
      endcase
      case (mem_addr[31:8])
        24'hffff01: mem_ready = mem_ready_spi_flash;
        24'hffff02: mem_ready = mem_ready_gpio;
        24'hffff03: mem_ready = mem_ready_timer;
        24'hffff04: mem_ready = mem_ready_uart;
        24'hffff05: mem_ready = mem_ready_sys_ctrl;
        24'hffff06: mem_ready = mem_ready_kbd;
        default: begin
           if (mem_addr[31:26] == 6'b111100)
              mem_ready = mem_ready_hyper_ram;
           else if (mem_addr[31:12] == 20'hffff1)
              mem_ready = mem_ready_dpram;
           else if (mem_addr[31:12] == 20'hffff2)
              mem_ready = mem_ready_usb_dpram;
           else if ((mem_addr[31:12] == 20'hffff4) || (mem_addr[31:12] == 20'hffff5)) // hffff4 and hffff5
              mem_ready = mem_ready_vga;
           else
              mem_ready = mem_ready_memory;
        end
      endcase
      case (mem_addr[31:8])
        24'hffff01: mem_rdata = mem_rdata_spi_flash;
        24'hffff02: mem_rdata = mem_rdata_gpio;
        24'hffff03: mem_rdata = mem_rdata_timer;
        24'hffff04: mem_rdata = mem_rdata_uart;
        24'hffff05: mem_rdata = mem_rdata_sys_ctrl;
        24'hffff06: mem_rdata = mem_rdata_kbd;
        default: begin
           if (mem_addr[31:26] == 6'b111100)
              mem_rdata = mem_rdata_hyper_ram;
           else if (mem_addr[31:12] == 20'hffff1)
              mem_rdata = mem_rdata_dpram;
           else if (mem_addr[31:12] == 20'hffff2)
              mem_rdata = mem_rdata_usb_dpram;
           else if ((mem_addr[31:12] == 20'hffff4) || (mem_addr[31:12] == 20'hffff5)) // hffff4 and hffff5
              mem_rdata = mem_rdata_vga;
           else
              mem_rdata = mem_rdata_memory;
        end

      endcase
   end

endmodule


