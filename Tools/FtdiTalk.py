#
# Copyright (c) 2021.  Micro Blue
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#


import sys
import time
import binascii
import string
import msvcrt
from pyftdi.spi  import SpiController, SpiGpioPort
from pyftdi.ftdi import Ftdi
from array import array as Array
from functools import reduce
from ctypes import *

class SPI_PKT (Structure):
    _pack_ = 1
    _fields_ = [
        ('Addr',   c_uint16),
        ('Scmd',   c_uint16),
        ]

    def __init__ (self, spic, addr = 0):
      self.Scmd = spic & 0xFFFF
      self.Addr = addr & 0xFFFF


class CMD_PKT (Structure):
    _pack_ = 1
    _fields_ = [
        ('Arg',    c_uint32),
        ('Len',    c_uint32, 12),
        ('Cmd',    c_uint32, 4),
        ('Reserved',   c_uint32, 16),
        ]

    def __init__ (self, cmd, arg = 0, len = 0x200):
      self.Arg = arg
      self.Len = len & 0xfff
      self.Cmd = cmd & 0x0f


def Bytes2Val (Bytes):
    return reduce(lambda x,y: (x<<8)|y,  Bytes[::-1] )

def print_bytes (data, indent=0, offset=0, show_ascii = False):
    bytes_per_line = 16
    printable = ' ' + string.ascii_letters + string.digits + string.punctuation
    str_fmt = '{:s}{:04x}: {:%ds} {:s}' % (bytes_per_line * 3)
    bytes_per_line
    data_array = bytearray(data)
    for idx in range(0, len(data_array), bytes_per_line):
        hex_str = ' '.join('%02X' % val for val in data_array[idx:idx + bytes_per_line])
        asc_str = ''.join('%c' % (val if (chr(val) in printable) else '.')
                          for val in data_array[idx:idx + bytes_per_line])
        print (str_fmt.format(indent * ' ', offset + idx, hex_str, ' ' + asc_str if show_ascii else ''))


def Usage():
    print ("FtdiPrg Version 0.01")
    print ("Usage:")
    print ("  FtdiPrg.py RomFile Offset[:Length]  Type:UsbAddress  Flags")
    print ("    RomFile    - Binary file to program.")
    print ("    SpiOffset  - SPI offset to start with. Length is optional.")
    print ("    Type       - Adaptor type. Only 'DEDIPRIG' is supported.")
    print ("    UsbAddress - USB bus number (254 for list all)")
    print ("    Flags      - Flags to control the actions")
    print ("                 r - Read flash")
    print ("                 v - Verify flash")
    print ("                 b - Blank check")
    print ("                 o - Create output file")
    print ("                 c - Calculate CRC")
    print ("                 f - Force without CRC")
    print ("                 e - Erase flash")
    print ("                 p - Program flash")
    print ("                 l - Low voltage 1.8v")
    print ("                 m - Mid voltage 2.5v")
    print ("                 t - Test mode")


# D:\Embed\StmTalk\StmTalk SlimBoot\Build\BootloaderCorePkg\MiniLoader.bin 0x9F0000 DEDIPROG pe

class CSpiPrg:
    CMD_OFF     = 0x0600
    DAT_OFF     = 0x0000
    SHL_CMD_OFF = 0x07C0
    SHL_STR_OFF = 0x07C4
    KBD_OFF     = 0x07F0

    def __init__(self, url, lv = 'h'):
        OUP  = 0 << 7  # Unsued output
        IO2  = 0 << 6  # IO2/CS2 shared
        INP  = 0 << 5  # Unsued input
        IO3  = 1 << 4  # IO3/VPP shared

        VC33 = (0x00 << 8)
        VC25 = (0x01 << 8)
        VC18 = (0x02 << 8)
        VC17 = (0x03 << 8)

        BUF  = 0 << 10
        PWR  = 1 << 11

        self.addr4 = False
        self.ctrl = SpiController(cs_count=1, turbo=False)
        self.ctrl.configure(url)

        self.gpio = SpiGpioPort(self.ctrl)
        self.gpio.set_direction (0x0ff0, 0x0fd0)
        val = OUP | IO2 | IO3 | (PWR | BUF)
        if lv == 'l':
            val = val | VC18
        elif lv == 'm':
            val = val | VC25
        else:
            val = val | VC33
        self.gpio.write (val)
        self.spi  = self.ctrl.get_port(0)
        self.spi.set_frequency(24000000)
        #time.sleep (0.2)

    def SendAndRecv (self, data, len = 0):
        if len == 0:
            self.spi.exchange (data)
        else:
            return self.spi.exchange (data, len)

    def MemWrite (self, addr, data):
        # Write data to DPRAM
        spid = SPI_PKT(1, addr)
        data = bytearray(spid) + data
        self.SendAndRecv (data)

    def DpWrite (self, addr, data):
        # Write data to DPRAM
        spid = SPI_PKT(1, addr)
        data = bytearray(spid) + data
        self.SendAndRecv (data)

    def DpRead (self, addr, len):
        # Write data to DPRAM
        spid = SPI_PKT(0, addr)
        data = bytearray(spid)
        return self.SendAndRecv (data, len)

    def IsReadyForCmd (self):
        spis = SPI_PKT(0, CSpiPrg.CMD_OFF)
        data = bytearray(spis)
        res = self.SendAndRecv (data, 0x08)
        if res == b'\xff' * 8:
            raise SystemExit ('Target system is not alive !')
        cmd_sts = CMD_PKT.from_buffer(res)
        if cmd_sts.Cmd == 0:
            return True
        else:
            return False

    def WaitForCmdExe (self):
        wait = True
        while (wait):
          wait = not self.IsReadyForCmd()

    def WaitForShellReady (self):
        spis = SPI_PKT(0, CSpiPrg.SHL_CMD_OFF)
        cmd_sts = 1
        start   = time.time()
        while cmd_sts != 0:
          csts = self.SendAndRecv (bytearray(spis), 4)
          cmd_sts = csts[0]
          if cmd_sts == 0xff:
            raise SystemExit ('Target system is not alive !')


    def ShellCmd (self, cmd, wait=True):
        if (len(cmd) & 1) == 0:
          cmd += b' '
        cmd += b'\x00'
        max = 0x3c
        if len(cmd) >= max:
          cmd[max-1] = b'\x00'
          cmd = cmd[:max]

        self.WaitForShellReady()

        spis = SPI_PKT(1, CSpiPrg.SHL_STR_OFF)
        data = bytearray(spis) + bytearray(cmd) + b'\x00'
        self.SendAndRecv (data)

        spis = SPI_PKT(1, CSpiPrg.SHL_CMD_OFF)
        data = bytearray(spis) + b'CMD:'
        self.SendAndRecv (data)

        # Wait to done
        if wait:
          self.WaitForShellReady()


    def UploadData (self, bins, addr, hyper):
        # Write data to DPRAM
        lpkt = 0x200
        spic = SPI_PKT(1, CSpiPrg.CMD_OFF)
        cmdb = CMD_PKT(7 if hyper else 1, addr, lpkt)

        doff = 0
        dlen = len(bins)
        while dlen > 0:
          if dlen < lpkt:
            lpkt = dlen

          self.WaitForCmdExe()
          self.DpWrite (CSpiPrg.DAT_OFF, bins[doff:doff+lpkt])
          dlen -= lpkt
          doff += lpkt

          cmdb.Len = lpkt
          data = bytearray(spic) + bytearray(cmdb)
          self.SendAndRecv (data)
          if cmdb.Cmd != 3:  # WR
            cmdb.Arg = addr
            cmdb.Cmd = 3     # WR CONT

        return 0


    def DownloadData (self, addr, dlen):
        # Read data from target
        lpkt = 0x200
        spid = SPI_PKT(1, 0)
        spic = SPI_PKT(1, CSpiPrg.CMD_OFF)
        cmdb = CMD_PKT(2, addr, lpkt)

        rdata = bytearray()

        doff = 0
        while dlen > 0:
          if dlen < lpkt:
            lpkt = dlen

          cmdb.Len = lpkt
          data = bytearray(spic) + bytearray(cmdb)
          self.SendAndRecv (data)
          if cmdb.Cmd == 2:  # RD
            cmdb.Arg = addr
            cmdb.Cmd = 4     # RD CONT
          self.WaitForCmdExe()

          rdata += self.DpRead (CSpiPrg.DAT_OFF, lpkt)

          dlen -= lpkt
          doff += lpkt

        return rdata

    def Download (self, file, addr, dlen, quit = True):
        print ('Downloading 0x%x bytes from target @ 0x%04X ... ' % (dlen, addr))
        bins = self.DownloadData (addr, dlen)
        print ('Done')

        if file:
          fd = open (file, 'wb')
          fd.write(bins)
          fd.close()
        else:
          print_bytes (bins)

        if quit:
          self.WaitForCmdExe()
          spic = SPI_PKT(1, CSpiPrg.CMD_OFF)
          cmdb = CMD_PKT(0x0d, addr, 0)
          data = bytearray(spic) + bytearray(cmdb)
          self.SendAndRecv (data)

        return 0

    def Upload (self, file, addr = 0, quit = True, hyper = False):
        offset = 0
        if ':0x' in file:
            parts = file.split(':')
            file   = parts[0]
            offset = int (parts[1], 16)

        fd = open (file, 'rb')
        bins = bytearray (fd.read()[offset:])
        fd.close()
        dlen = len(bins)
        print ('Uploading 0x%x bytes to target @ 0x%08X ... ' % (dlen, addr))
        self.UploadData (bins, addr, hyper)

        if quit:
          self.WaitForCmdExe()
          spic = SPI_PKT(1, CSpiPrg.CMD_OFF)
          cmdb = CMD_PKT(0x0d, addr, 0)
          data = bytearray(spic) + bytearray(cmdb)
          self.SendAndRecv (data)

        return 0

    def Execute (self, file, addr = 0):
        self.IsReadyForCmd ()

        fd = open (file, 'rb')
        bins = bytearray (fd.read())
        fd.close()

        dlen = len(bins)
        print ('Uploading 0x%x bytes to target @ 0x%08X ... ' % (dlen, addr))
        self.UploadData (bins, addr, False)
        print ('Done')

        self.WaitForCmdExe()
        spic = SPI_PKT(1, CSpiPrg.CMD_OFF)
        cmdb = CMD_PKT(0x0e, addr, 0)
        data = bytearray(spic) + bytearray(cmdb)
        self.SendAndRecv (data)

        return 0

    def RawWrite (self, offset, data):
        # Write data to target DPRAM
        spic   = SPI_PKT(1, offset)
        data   = bytearray(spic) + data
        self.spi.exchange (data, 0)

    def RawRead (self, offset, rlen):
        # Read data from target DPRAM
        spic   = SPI_PKT(0, offset)
        data   = bytearray(spic)
        rdata  = bytearray()
        rdata += self.spi.exchange (data, rlen)
        return rdata

    def SysCtrl (self, ctrl):
        # Write control
        spid = SPI_PKT(3, ctrl)
        data = bytearray(spid)
        self.spi.exchange (data, 0)

    def SysStat (self):
        # Read state
        spid   = SPI_PKT(2, 0)
        data   = bytearray(spid)
        rdata  = bytearray()
        rdata += self.spi.exchange (data, 2)
        return rdata

    def MemWrite (self, addr, data):
        # Write data to DPRAM
        spid = SPI_PKT(1, addr)
        data = bytearray(spid) + data
        self.SendAndRecv (data)

    def DpWrite (self, addr, data):
        # Write data to DPRAM
        spid = SPI_PKT(1, addr)
        data = bytearray(spid) + data
        self.SendAndRecv (data)

    def DpRead (self, addr, len):
        # Write data to DPRAM
        spid = SPI_PKT(0, addr)
        data = bytearray(spid)
        return self.SendAndRecv (data, len)

    def Keyboard (self):
        data = bytearray ()
        while True:

            if len(data) > 0:
                sts = self.DpRead  (CSpiPrg.KBD_OFF, 2)
                if sts[1] == 0:
                    self.DpWrite (CSpiPrg.KBD_OFF, bytearray([data[0]]) + b'\xff')
                    data.pop(0)

            if not msvcrt.kbhit():
                time.sleep (.1)
                continue

            ch = msvcrt.getch()
            if ch == b'\xe0':
               ch = msvcrt.getch()

            if ch == b'\x1b':
                break

            if ch in b'HPKMxz+- \r':
                # print ('%02X' % ord(ch))
                if len(data) > 8:
                    data.pop(0)
                data.append (ord(ch))


    def __del__(self):
        #self.ctrl._ftdi.write_data(Array('B', [Ftdi.SET_BITS_LOW,   0x00, 0x00]))
        #self.ctrl._ftdi.write_data(Array('B', [Ftdi.SET_BITS_HIGH,  0x00, 0x00]))
        self.ctrl.terminate()


def Main():
    #
    # Parse the options and args
    #
    if len(sys.argv) < 4:
        Usage()
        return 1

    url   = 'ftdi:///1'
    parts = sys.argv[3].split(':')
    dtype = parts[0]
    if len(parts) > 1:
      if parts[1] == '?':
        ftdi = Ftdi()
        ftdi.open_from_url('ftdi:///?')
        return 0
      else:
        url = 'ftdi://%s' % (':'.join(parts[1:]))

    print (url)
    if dtype.upper() != 'DEDIPROG':
        print ("Type '%s' is not supported!" % sys.argv[3])
        return 1

    try:
        items  = sys.argv[2].split(':')
        offset = int(items[0], 16)
        if len(items) > 1:
            length = int(items[1], 16)
        else:
            length = 0
    except:
        print ("Invalid argument '%s'!" % sys.argv[2])
        return 1

    flags = sys.argv[4]
    if 'l' in flags:
        lv = 'l'
    elif 'm' in flags:
        lv = 'm'
    else:
        lv ='h'
    SpiPrg = CSpiPrg(url, lv)

    file = sys.argv[1]
    if 'c' in flags:
        cmd  = sys.argv[1]
        wait = True if '+' in flags else False
        SpiPrg.ShellCmd (cmd.encode(), wait)
        return 0

    if 'k' in flags:
        SpiPrg.Keyboard ()
        return 0

    if 'u' in flags:
        quit  = False if '+' in flags else True
        hyper = True  if 'h' in flags else False
        SpiPrg.Upload (file, offset, quit, hyper)
        return 0

    if 'e' in flags:
        SpiPrg.Execute (file, offset)
        return 0

    if 'd' in flags:
        if file == 'none':
          file = ''
        quit = False if '+' in flags else True
        SpiPrg.Download (file, offset, length, quit)
        return 0

    if 's' in flags:
        #SpiPrg.SysCtrl (0x0004)
        data = SpiPrg.SysStat ()
        #SpiPrg.SysCtrl (0x0000)
        print_bytes (data)

    if 'b' in flags:
        fd = open (file, 'rb')
        data = bytearray (fd.read())
        fd.close()
        SpiPrg.RawWrite (0, data)
        SpiPrg.SysCtrl  (0x0000)
        data = SpiPrg.RawRead (0, 0x40)
        return 0

    if 'r' in flags:
        # set sys ctrl
        print ("Set SYSCTRL: 0x%04X" % offset)
        SpiPrg.SysCtrl (offset)
        return 0

    if 't' in flags:
        data = SpiPrg.RawRead (CSpiPrg.CMD_OFF, 4)
        print_bytes (data)


    return 0

if __name__ == '__main__':
    sys.exit(Main())
