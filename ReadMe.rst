MX_RISCV
========

Overview
########
This is a project built on top of PicoRv32 core to show case how to build many
different interfaces using FPGA to extend the capabilty of a system. It uses Intel
Cyclone10LP evaluation kit as FPGA development platform.

- SPI debuger interface

  Provide high speed data communication between target and host PC.
  It uses FTDI FT2232HQ Breakout Board to convert SPI to USB interface.

- UART interface

  Provide intput/output serial console.

- HyperRAM controller

  Provide system bus to HyperRAM bridge to allow CPU to direct access HyperRAM.

- PS2 keyboard interface

  Provide keyboard input console.

- PlayStation2 joystick interface

  Provide flexible game control capability.

- VGA controller

  Provide VGA console (320x240) mode for the system.

- SPI flash controller

  Read/write flash chip on Intel Cyclone10LP evaluation kit.

- Misc

  Timer, GPIO, SRAM, etc.


Compiling
#########

- Prerequisite

  This project is developed under Windows environment. Before compiling, please make sure the following required tools are installed and under the PATH environment variable.

  - `make <http://gnuwin32.sourceforge.net/packages/make.htm>`_ tool

  - `riscv-none-embed-gcc <https://github.com/xpack-dev-tools/riscv-none-embed-gcc-xpack/releases>`_ toolchain

- Build bootloader

  Run following commands::

    cd RiscvApp\Bootloader
    make clean && make

- Build Shell

  Run following commands::

    cd RiscvApp\Shell
    make clean && make

- Build MX_RISCV verilog code

  Open mx_soc.qpf with "Quartus 18", and then use "Start Compilation" menu to start complilation.
  Once done, the final SOF binary will be generated at output_files\\mx_soc.sof.

Setup
#####

- Connect FPGA SPI debug to FTDI FT2232HQ chip and then connect FTDI FT2232HQ to PC host through USB. Please
  replace the default UART driver with libusb-win32 driver for its interface 0 using `zadig <https://zadig.akeo.ie/>`_ tool.
- Connect FPAA UART interface to PC host through a USB to serial adaptor. Open the UART with a terminal tool
  such as "Putty", and set baud rate to 115200 without flow control.
- Connect FPGA VGA interface to a VGA compatible monitor.
- Connect FPGA PS2 keyboard interface to a PS2 keyboard.
- Connect FPGA PlayStation 2 interface to a PlayStation 2 joystick.
- Connect Intel Cyclone10LP evaluation kit USB connector to PC host.
- Connect Intel Cyclone10LP evaluation kit power supply.
- Download SOF to Intel Cyclone10LP evaluation kit
  Intel Cyclone10LP FPGA evaluation kit provides built-in USB blaster and can be used to
  program the FPGA device. Use "Quartus Programmer" tool to download mx_soc.sof into FPGA
  through USB blaster interface. If everything works well, you should see a print from UART
  console "CYCLONE10LP RISC-V Loader v1.0: OK".

Testing
#######
- Prerequisite

  This project is tested under Windows environment. Please install Python 3.8 or above.

- Boot to Shell

  Run following commands::

    cd RiscvApp\Shell
    make run

  The Shell banner should be printed on serial console. And more shell command can be used from serial console.

- Run Shell

  Run following commands::

    cd RiscvApp\Shell
    make run

  The Shell banner should be printed on serial console. And more shell command can be used from serial console.

- Run LodeRunner

  Run following commands::

    cd RiscvApp\LodeRunner
    make run

  LodeRunner should show up on VGA console.

  .. image:: Images/LodeRunner.png
     :align: center

  To play, use PS2 arrow keys to move the runner and 'z' and 'x' to dig holes.

- Run Sprite demo

  Run following commands::

    cd RiscvApp\Shell
    make images
    make sprite

  Demo should show up on VGA console.

  .. image:: Images/sprite_demo.png
     :align: center

- Run Raiden demo

  Run following commands::

    cd RiscvApp\Shell
    make images
    make raiden

  Demo should show up on VGA console.

  .. image:: Images/Raiden.png
     :align: center

  PlayStation2 joystick can be used to move the fighters and shoot.


Acknowledgements
################

- The RISC-V core is borrowed from `picorv32 <https://github.com/cliffordwolf/picorv32>`_.
  Thanks to cliffordwolf for sharing such a neat RV32 implemention. Very helpful for beginners like me.

- The LodeRunner game is ported from `x16-LodeRunner <https://github.com/CJLove/x16-LodeRunner>`_.
  Thanks to CJLove for providing the LodeRunner game source. It brought back lots of my childhood memory. :)

- The Raiden/Sprite demo used many images from `Raiden1990 <https://github.com/Margeli/Raiden1990>`_.
  Thanks to Margeli for those wonderful images.

