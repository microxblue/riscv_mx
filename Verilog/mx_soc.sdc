create_clock -name enet_clk_125m -period 8 [get_ports {enet_clk_125m}]
create_clock -name spi_clk -period 25      [get_ports {gpio[17]}]
create_clock -name vga_ctl -period 25      [get_ports {vga:vga_inst|vga_ctl[10]}]

derive_pll_clocks
derive_clock_uncertainty