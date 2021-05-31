#
# Copyright (c) 2021.  Micro Blue
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

import sys

pmap1 = {'0':'0',        '1':'1',        '2':'1',        '3':'1'}
pmap2 = {'0':'00',       '1':'01',       '2':'10',       '3':'11'}
pmap4 = {'0':'0000',     '1':'0101',     '2':'0110',     '3':'0011'}
pmap8 = {'0':'00000000', '1':'00000111', '2':'00000010', '3':'00000001'}

def value_to_bytes (value, length):
    return value.to_bytes(length, 'big')


def handle_image (line, ftype, bpp, wide = 2):
    # 10000000 10000001
    if bpp == 1:
        pmap = pmap1
    elif bpp == 2:
        pmap = pmap2
    elif bpp == 4:
        pmap = pmap4
    else:
        pmap = pmap8

    if ftype == 'tile':
        n   = len(pmap['0'])
        new = []
        for each in line:
            new.append(pmap[each] * wide)
        line = ''.join(new)
        new = bytearray()
        for idx in range (0, 8*n*wide, 8*wide):
          new.extend(value_to_bytes (int(line[idx:idx+8*wide],2), wide))
        return new
    elif ftype == 'spr_c16':
        new = bytearray()
        clrdict = {0:0xFFFF, 1:0x7C00, 2:0x7FE0, 3:0x7FFF}
        for each in line:
            ch = clrdict[int(each)]
            new.extend (ch.to_bytes(2, 'little'))
        return new
    else:
        new = []
        for each in line:
            ch = int(each)
            if ch == 1:
                ch = 0x5
            elif ch == 2:
                ch = 0xa # runner
            elif ch == 3:
                ch = 0xf # guard
            new.append(ch)
        return bytearray(bytearray(new))


def parse_image (file, ftype, bpp, wide):
    fd = open(file, 'r')
    lines = fd.readlines()
    fd.close()

    bin = []
    cnt = 0
    for line in lines:
        if not line.strip():
           continue
        if line.startswith('#'):
           continue
        cnt += 1
        line = line.rstrip()
        bin.extend(handle_image(line, ftype, bpp, wide))

    return bytearray(bin)


def main ():
    itype  = sys.argv[1]
    wide   = int(sys.argv[2]) // 8
    bpp    = int(sys.argv[3])
    input  = sys.argv[4]
    output = sys.argv[5]

    print ("Build spirit ...")
    bin  = parse_image (input, itype, bpp, wide)
    if len(bin) > 0x1000:
      bin = bin[:0x1000]
    print ("Write file %s" % output)
    fd   = open(output, 'wb')
    fd.write(bin)
    fd.close()
    print ("Done")

main()