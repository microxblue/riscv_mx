@echo off

set PATH=C:\WINDOWS\system32;C:\WINDOWS;C:\Python3;D:\Embed\Utility;C:\"Program Files"\Git\usr\bin
set QUARTUS=E:\Cyclone\13.1\quartus\bin
set MEM=sram

make clean %MKENV%
make all   %MKENV%
