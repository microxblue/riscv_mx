//
// Copyright (c) 2021.  Micro Blue
//
// SPDX-License-Identifier: BSD-2-Clause-Patent
//

`include "soc_cfg.v"


`ifdef  CFG_USE_EXT_BOARD
  `define spi_di   gpio[15] // pin18
  `define spi_do   gpio[16] // pin19
  `define spi_cs   gpio[14] // pin17
  `define spi_ck   gpio[17] // pin20


  `define jstk_mo  gpio[19] // pin22
  `define jstk_mi  gpio[18] // pin21
  `define jstk_ck  gpio[20] // pin23
  `define jstk_cs  gpio[21] // pin24


  `define uart_tx  gpio[28] // pin33
  `define uart_rx  gpio[29] // pin44

  `define ps2_clk  gpio[33] // pin38
  `define ps2_dat  gpio[35] // pin40

  `define vga_r4   gpio[0]
  `define vga_r3   gpio[1]
  `define vga_r2   gpio[2]
  `define vga_r1   gpio[3]
  `define vga_r0   gpio[23] // pin26

  `define vga_b4   gpio[4]
  `define vga_b3   gpio[5]
  `define vga_b2   gpio[6]
  `define vga_b1   gpio[7]
  `define vga_b0   gpio[22] // pin25

  `define vga_g4   gpio[10]
  `define vga_g3   gpio[11]
  `define vga_g2   gpio[12]
  `define vga_g1   gpio[13]
  `define vga_g0   gpio[24] // pin27

  `define vga_vs   gpio[8]
  `define vga_hs   gpio[9]

`else

  `define spi_di   gpio[35]
  `define spi_do   gpio[33]
  `define spi_cs   gpio[31]
  `define spi_ck   gpio[29]

  `define uart_tx  arduino_io[13]
  `define uart_rx  arduino_io[12]

  `define vga_r    gpio[1]
  `define vga_g    gpio[3]
  `define vga_b    gpio[5]
  `define vga_hs   gpio[7]
  `define vga_vs   gpio[9]

`endif




`default_nettype none

module  mx_soc (
        //Clock and Reset
        input  wire        c10_clk50m    ,
        input  wire        hbus_clk_50m  ,
        input  wire        c10_clk_adj   ,
        input  wire        enet_clk_125m ,
        input  wire        c10_resetn    ,

        //LED PB DIPSW
        output wire [3:0]  user_led ,
        input  wire [3:0]  user_pb  ,
        input  wire [2:0]  user_dip ,

        //Ethernet Port
        output wire        enet_mdc        ,
        inout  wire        enet_mdio       ,

        input  wire        enet_int        ,
        output wire        enet_resetn     ,
        input  wire        enet_rx_clk     ,
        output wire        enet_tx_clk     ,
        input  wire [3:0]  enet_rx_d       ,
        output wire [3:0]  enet_tx_d       ,
        output wire        enet_tx_en      ,
        input  wire        enet_rx_dv      ,
        //        input  wire        enet_led_link100,

        //PMOD PORT
        inout  wire [7:0]  pmod_d   ,

        //Side Bus
        input  wire         usb_reset_n        ,
        input  wire         c10_usb_clk        ,
        input  wire         usb_wr_n           ,
        input  wire         usb_rd_n           ,
        input  wire         usb_oe_n           ,
        output wire         usb_full           ,
        output wire         usb_empty          ,
        inout  wire   [7:0] usb_data           ,
        inout  wire   [1:0] usb_addr           ,
        inout  tri1         usb_scl            ,
        inout  tri1         usb_sda            ,


        //User IO & Clock
        inout  wire [35:0]  gpio            ,

        //ARDUINO IO
        inout  wire [13:0]  arduino_io      ,
        output wire         arduino_rstn    ,
        inout  wire         arduino_sda     ,
        inout  wire         arduino_scl     ,
        inout  wire         arduino_adc_sda ,
        inout  wire         arduino_adc_scl ,

        //Cyclone 10 to MAX 10 IO
        inout  wire [3:0]   c10_m10_io   ,

        //HyperRAM IO
        output wire         hbus_rstn   ,
        output wire         hbus_clk0p  ,
        output wire         hbus_clk0n  ,
        output wire         hbus_cs2n   , //HyperRAM chip select
        inout  wire         hbus_rwds   ,
        inout  wire [7:0]   hbus_dq     ,
        output wire         hbus_cs1n   , //For HyperFlash
        input  wire         hbus_rston  , //For HyperFlash
        input  wire         hbus_intn   , //For HyperFlash

        //QSPI
        output wire         qspi_dclk   ,
        output wire         qspi_sce    ,
        output wire         qspi_sdo    ,
        input  wire         qspi_data0
        );


  wire        trap;

  wire        mem_valid;
  wire        mem_instr;
  wire        mem_ready;

  wire [31:0] mem_addr;
  wire [31:0] mem_wdata;
  wire [3:0]  mem_wstrb;
  wire [31:0] mem_rdata;

  // Look-Ahead Interface
  wire        mem_la_read;
  wire        mem_la_write;
  wire [31:0] mem_la_addr;
  wire [31:0] mem_la_wdata;
  wire [3:0]  mem_la_wstrb;

  wire        pcpi_valid;
  wire [31:0] pcpi_insn;
  wire [31:0] pcpi_rs1;
  wire [31:0] pcpi_rs2;
  reg         pcpi_wr;
  reg  [31:0] pcpi_rd;
  reg         pcpi_wait;
  reg         pcpi_ready;

  // IRQ Interface
  wire [31:0] irq;
  wire [31:0] eoi;

  // Trace Interface
  wire        trace_valid;
  wire [35:0] trace_data;

  wire [15:0] enables;

  reg         resetn    = 0;
  reg [7:0]   reset_cnt = 0;

  wire        sys_clk;
  wire        sys_clk_dbl;
  wire        vga_clk;

  wire        clk_locked;

  wire        cpu_rst2;
  reg         cpu_rst;
  reg         sys_rst;

  wire [2:0]  hb_state;

  wire [15:0]   sys_ctl;
  wire [15:0]   sys_sts;
  wire [3:0]    sys_led;

  wire [7:0]    dbg_pin;

  assign    user_led  = 4'hf;


  // Reset logic
  always @(posedge sys_clk)
     begin
        if (~c10_resetn || sys_rst) begin
            resetn    <= 0;
            sys_rst   <= 0;
            cpu_rst   <= 0;
        end else begin
            sys_rst   <= sys_ctl[7];
            cpu_rst   <= sys_ctl[3];
            reset_cnt <= reset_cnt + 8'd1;
            if (reset_cnt == 100) resetn <= 1;
        end
     end

  // PLL
  wire enet_tx_25;
  wire pll_locked;
  pll pll_inst0 (
    .areset (!c10_resetn   ) ,
    .inclk0 (enet_clk_125m ) ,
    .c0     (sys_clk_dbl   ) ,
    .c1     (sys_clk       ) ,
    .c2     (vga_clk       ) ,
    .locked (pll_locked    ) );


   wire mem_ready_vga;
   wire [31:0] mem_rdata_vga;
   wire [7:0]  vga_burst_dw;
   wire        vga_rd_req;
`ifdef CFG_VGA
   // VGA
   wire vga_rd_ready;
   wire [31:0] vga_rd_addr;

`ifdef CFG_HYPER_RAM_DBL_CLK
   defparam  vga_inst.HYPER_BUS_TO_VGA_BUS_FREQ_RATIO	= 4;
`else
   defparam  vga_inst.HYPER_BUS_TO_VGA_BUS_FREQ_RATIO	= 2;
`endif

   vga  vga_inst (
      .cpu_clk(sys_clk),
      .resetn(resetn),
      .enable        (enables[9]),
      .mem_valid     (mem_valid),
      .mem_ready     (mem_ready_vga),
      .mem_wstrb     (mem_wstrb),
      .mem_rdata     (mem_rdata_vga),
      .mem_wdata     (mem_wdata),
      .mem_addr      (mem_addr),

      .dbg_pin         (dbg_pin[7:0]),

      .ext_rdata       (mem_rdata_hyper_ram),
      .ext_rd          (vga_rd_req),
      .ext_burst_ready (vga_rd_ready),
      .ext_addr        (vga_rd_addr) ,
      .ext_burst_dw    (vga_burst_dw),

      .vclk   (vga_clk),      // 40M for 800x600

`ifdef  CFG_USE_EXT_BOARD
      .red    ({`vga_r4, `vga_r3, `vga_r2, `vga_r1, `vga_r0}),
      .green  ({`vga_g4, `vga_g3, `vga_g2, `vga_g1, `vga_g0}),
      .blue   ({`vga_b4, `vga_b3, `vga_b2, `vga_b1, `vga_b0}),
`else
      .red    ({`vga_r, `vga_r, `vga_r, `vga_r, `vga_r}),
      .green  ({`vga_g, `vga_g, `vga_g, `vga_g, `vga_g}),
      .blue   ({`vga_b, `vga_b, `vga_b, `vga_b, `vga_b}),
`endif

      .hsync  (`vga_hs),      // Horizontal sync, negative polarity
      .vsync  (`vga_vs)       // Vertical sync, negative polarity
   );

`else
   assign vga_rd_req    = 0;
   assign mem_ready_vga = 1;
`endif


  // Uart receive
  wire mem_ready_uart;
  wire [31:0] mem_rdata_uart;

  defparam  uart_inst.BAUD_DIVIDER = `CPU_FREQ / 115200;
  uart  uart_inst (
      .clk(sys_clk),
      .resetn(resetn),

      .enable(enables[4]),
      .mem_valid(mem_valid),
      .mem_ready(mem_ready_uart),
      .mem_instr(mem_instr),
      .mem_addr(mem_addr),
      .mem_wstrb(mem_wstrb),
      .mem_wdata(mem_wdata),
      .mem_rdata(mem_rdata_uart),

      //.dbg_pin (dbg_pin[3:0]),

      .uart_txd(`uart_tx),
      .uart_rxd(`uart_rx)
      );


  // Timer
  wire mem_ready_timer;
  wire [31:0] mem_rdata_timer;
  timer timer_inst (
        .clk(sys_clk),
        .resetn(resetn),
        .enable(enables[3]),
        .mem_valid(mem_valid),
        .mem_ready(mem_ready_timer),
        .mem_instr(mem_instr),
        .mem_wstrb(mem_wstrb),
        .mem_wdata(mem_wdata),
        .mem_addr(mem_addr),
        .mem_rdata(mem_rdata_timer)
    );


  // GPIO
  wire mem_ready_gpio;
  wire [31:0] mem_rdata_gpio;
  gpio gpio_inst (
        .clk(sys_clk),
        .resetn(resetn),
        .enable(enables[2]),
        .mem_valid(mem_valid),
        .mem_ready(mem_ready_gpio),
        .mem_instr(mem_instr),
        .mem_wstrb(mem_wstrb),
        .mem_wdata(mem_wdata),
        .mem_addr(mem_addr),
        .mem_rdata(mem_rdata_gpio),

        //.gpio (gpio[15:0]),
        //.btn (user_pb[2:0]),
        .dip (user_dip)
    );

  // Keyboard
  wire mem_ready_kbd;
  wire [31:0] mem_rdata_kbd;
  keyboard  keyboard_inst (
       .clk(sys_clk),
       .resetn(resetn),
       .enable(enables[10]),
       .mem_valid(mem_valid),
       .mem_ready(mem_ready_kbd),
       .mem_instr(mem_instr),
       .mem_wstrb(mem_wstrb),
       .mem_wdata(mem_wdata),
       .mem_addr(mem_addr),
       .mem_rdata(mem_rdata_kbd),

       //.dbg_pin (dbg_pin[7:0]),

       .ps2_data (`ps2_dat),
       .ps2_clk  (`ps2_clk)
  );

  // PS2 Joystick
  wire [15:0]  jstk_state;
  ps2_joystick  ps2_joystick_inst (
      .clk (sys_clk),
      .resetn (resetn),

      //.dbg_pin (dbg_pin),
      .jstk_cs (`jstk_cs),
      .jstk_ck (`jstk_ck),
      .jstk_mo (`jstk_mo),
      .jstk_mi (`jstk_mi),

      .jstk_state (jstk_state)
    );

  // System
  wire mem_ready_sys_ctrl;
  wire [31:0] mem_rdata_sys_ctrl;
  wire [15:0] jstk_state_input = jstk_state;
  sys_ctrl sys_ctrl_inst (
       .clk(sys_clk),
       .resetn(resetn),
       .enable(enables[5]),
       .mem_valid(mem_valid),
       .mem_ready(mem_ready_sys_ctrl),
       .mem_instr(mem_instr),
       .mem_wstrb(mem_wstrb),
       .mem_wdata(mem_wdata),
       .mem_addr(mem_addr),
       .mem_rdata(mem_rdata_sys_ctrl),

       .cpu_rst    (cpu_rst2),
       .jstk_state (jstk_state_input)
  );

  // SPI Flash
  // ========================================
  wire mem_ready_spi_flash;
  wire [31:0] mem_rdata_spi_flash;
`ifdef  CFG_SPI_FLASH
  spi_flash spi_flash_inst (
       .clk(sys_clk),
       .resetn(resetn),
       .enable(enables[1]),
       .mem_valid(mem_valid),
       .mem_ready(mem_ready_spi_flash),
       .mem_instr(mem_instr),
       .mem_wstrb(mem_wstrb),
       .mem_wdata(mem_wdata),
       .mem_addr(mem_addr),
       .mem_rdata(mem_rdata_spi_flash),

       .spi_cs   (qspi_sce),
       .spi_ck   (qspi_dclk),
       .spi_mosi (qspi_sdo),
       .spi_miso (qspi_data0),
   );
`else
   assign mem_ready_spi_flash = 1;
`endif

  // Memory
  wire mem_ready_memory;
  wire [31:0] mem_rdata_memory;
  memory mem_inst (
              .clk(sys_clk),
              .io_enable(enables[7]),
              .io_mem_valid(mem_valid),
              .io_mem_ready(mem_ready_memory),
              .io_mem_instr(mem_instr),
              .io_mem_wstrb(mem_wstrb),
              .io_mem_wdata(mem_wdata),
              .io_mem_addr(mem_addr),
              .io_mem_rdata(mem_rdata_memory)
        );


  // HyperRAM
  // ==================
  wire         mem_ready_hyper_ram;
  wire [31:0]  mem_rdata_hyper_ram;
`ifdef CFG_HYPER_RAM
`ifdef CFG_HYPER_RAM_DBL_CLK
  defparam  hbc_inst.HYPER_BUS_TO_MEM_BUS_FREQ_RATIO	= 2;
  defparam  hbc_inst.HYPER_BUS_TO_EXT_BUS_FREQ_RATIO	= 4;
`else
  defparam  hbc_inst.HYPER_BUS_TO_MEM_BUS_FREQ_RATIO	= 1;
  defparam  hbc_inst.HYPER_BUS_TO_EXT_BUS_FREQ_RATIO	= 2;
`endif

  hbc  hbc_inst  (
`ifdef CFG_HYPER_RAM_DBL_CLK
     .clk          (sys_clk_dbl),
`else
     .clk          (sys_clk),
`endif
     .mem_clk      (sys_clk),
     .resetn       (resetn),
     .mem_enable   (enables[0]),
     .mem_valid    (mem_valid),
     .mem_ready    (mem_ready_hyper_ram),
     .mem_wstrb    (mem_wstrb),
     .mem_addr     (mem_addr),
     .mem_wdata    (mem_wdata),
     .mem_rdata    (mem_rdata_hyper_ram),

     //.dbg_pin      (dbg_pin[3:0]),

     .ext_clk      (vga_clk),
     .ext_rd_req   (vga_rd_req),
     .ext_burst_ready (vga_rd_ready),
     .ext_addr     (vga_rd_addr),
     .ext_burst_dw (vga_burst_dw),

     .hpr_csn      (hbus_cs2n),
     .hpr_clk      (hbus_clk0p),
     .hpr_clkn     (hbus_clk0n),
     .hpr_dq       (hbus_dq),
     .hpr_rwds     (hbus_rwds),
     .hpr_resetn   (hbus_rstn)
  );
`else
  assign mem_ready_hyper_ram = 1;
`endif


  // SPI Debug
  spi_dbg spi_dbg_inst (
      .clk (sys_clk),
      .clk2 (sys_clk_dbl),

      .resetn(resetn),

      .spi_cs   (`spi_cs),
      .spi_ck   (`spi_ck),
      .spi_mosi (`spi_do),
      .spi_miso (`spi_di),

      .sys_sts  (sys_sts),
      .sys_ctl  (sys_ctl),

      .sram_wr (sram_wr),
      .sram_addr (sram_addr),
      .sram_wdata (sram_wdata),
      .sram_rdata (sram_rdata)
  );


   // SPI DBG DP RAM
   wire [9:0]  sram_addr;
   wire        sram_wr;
   wire [15:0] sram_wdata;
   wire [15:0] sram_rdata;
   wire [31:0] mem_rdata_dpram;
   wire        mem_ready_dpram = spi_dbg_rdy;
   reg         spi_dbg_rdy;
   always @(posedge sys_clk) begin
      if (!resetn)    spi_dbg_rdy <= 0;
      else            spi_dbg_rdy <= (mem_valid & enables[6]) ? 1 : 0;
   end
   spi_dbg_dpram  spi_dbg_dpram_inst (
	    .clock_a (sys_clk),
	    .address_a (mem_addr[10:2]),
	    .q_a (mem_rdata_dpram),
	    .byteena_a (mem_wstrb),
	    .wren_a (enables[6] & mem_valid & (|mem_wstrb)),
	    .data_a (mem_wdata),

	    .clock_b (sys_clk),
	    .address_b (sram_addr),
	    .q_b (sram_rdata),
	    .data_b (sram_wdata),
	    .wren_b (sram_wr)
 	 );



   // RISC-V CPU
   defparam cpu_inst.ENABLE_COUNTERS = 0;
   defparam cpu_inst.ENABLE_COUNTERS64 = 0;
   defparam cpu_inst.BARREL_SHIFTER = 1;
   defparam cpu_inst.TWO_CYCLE_COMPARE = 0;
   defparam cpu_inst.TWO_CYCLE_ALU = 0;
   defparam cpu_inst.ENABLE_PCPI = 0;
   defparam cpu_inst.ENABLE_FAST_MUL = 1;
   defparam cpu_inst.ENABLE_DIV = 1;
   defparam cpu_inst.ENABLE_IRQ = 1;
   defparam cpu_inst.ENABLE_IRQ_TIMER = 1;


   picorv32 cpu_inst (
     .clk(sys_clk),
     .resetn(resetn & ~cpu_rst & ~cpu_rst2),
     .trap(trap),

     .mem_valid(mem_valid),
     .mem_instr(mem_instr),
     .mem_ready(mem_ready),

     .mem_addr(mem_addr),
     .mem_wdata(mem_wdata),
     .mem_wstrb(mem_wstrb),
     .mem_rdata(mem_rdata),

     // Look-Ahead Interface
     .mem_la_read(mem_la_read),
     .mem_la_write(mem_la_write),
     .mem_la_addr(mem_la_addr),
     .mem_la_wdata(mem_la_wdata),
     .mem_la_wstrb(mem_la_wstrb),

     // Pico Co-Processor Interface (PCPI)
     .pcpi_valid(pcpi_valid),
     .pcpi_insn(pcpi_insn),
     .pcpi_rs1(pcpi_rs1),
     .pcpi_rs2(pcpi_rs2),
     .pcpi_wr(pcpi_wr),
     .pcpi_rd(pcpi_rd),
     .pcpi_wait(pcpi_wait),
     .pcpi_ready(pcpi_ready),

     // IRQ Interface
     .irq(irq),
     .eoi(eoi),

     // Trace Interface
     .trace_valid(trace_valid),
     .trace_data(trace_data)
     );


   // Peripheral's mem_ready and mem_data.
   sys_bus sys_bus_inst (

      .mem_addr(mem_addr),
      .mem_ready(mem_ready),
      .mem_rdata(mem_rdata),

      .mem_rdata_memory(mem_rdata_memory),
      .mem_ready_memory(mem_ready_memory),

      .mem_rdata_uart(mem_rdata_uart),
      .mem_ready_uart(mem_ready_uart),

      .mem_rdata_timer(mem_rdata_timer),
      .mem_ready_timer(mem_ready_timer),

      .mem_rdata_dpram(mem_rdata_dpram),
      .mem_ready_dpram(mem_ready_dpram),

      .mem_rdata_vga (mem_rdata_vga),
      .mem_ready_vga (mem_ready_vga),

      .mem_rdata_gpio(mem_rdata_gpio),
      .mem_ready_gpio(mem_ready_gpio),

      .mem_rdata_kbd(mem_rdata_kbd),
      .mem_ready_kbd(mem_ready_kbd),

      .mem_rdata_sys_ctrl(mem_rdata_sys_ctrl),
      .mem_ready_sys_ctrl(mem_ready_sys_ctrl),

      .mem_rdata_spi_flash(mem_rdata_spi_flash),
      .mem_ready_spi_flash(mem_ready_spi_flash),

      .mem_rdata_hyper_ram(mem_rdata_hyper_ram),
      .mem_ready_hyper_ram(mem_ready_hyper_ram),

      .enables(enables)
      );

  assign  irq = {20'h0, ~user_pb[3:0], 8'h00};
  wire    hbc_busy;

  assign  arduino_io[7:0] = dbg_pin[7:0];


endmodule
