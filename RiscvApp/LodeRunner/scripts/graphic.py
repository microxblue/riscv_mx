#!/usr/bin/env python3
#
# Parse a graphic (sprite/tile).txt file and create .bin file suitable for loading
# by x16 application
#

import argparse
import sys

parser = argparse.ArgumentParser(formatter_class=argparse.RawDescriptionHelpFormatter,
    description='Converts .txt file to .bin file')
parser.add_argument('--input', help='.json filename')
parser.add_argument('--output', help='.bin filename')
parser.add_argument('--x', type=int, default=8, help='x dimension')
parser.add_argument('--y', type=int, default=8, help='y dimension')
parser.add_argument('--bpp',  type=int, default=4, help='bits per pixel for input file')
parser.add_argument('--obpp', type=int, default=0, help='bits per pixel for output file')
parser.add_argument('--addr', help='hex load address')
args = parser.parse_args()

addr=0
try:
    addr = int(args.addr,16)
except:
    pass

if args.obpp == 0:
    args.obpp = args.bpp

values=[]
pixels=0
if args.bpp == 1:
    values=[0,1]
    pixels=8
elif args.bpp == 2:
    values=[0,1,2,3]
    pixels=4
elif args.bpp == 4:
    values=[0,1,2,3,4,5,6,7]
    pixels=2
elif args.bpp == 8:
    pixels=[x for x in range(0,256)]
    pixels=1

with open(args.output,"wb") as out:
    # Handle 2-byte header
    header=[addr & 0xff, (addr >> 8) & 0xff]
    out.write(bytearray(header))

    with open(args.input,"r") as file:
        for line in file:
            if not line:
                continue
            if line[0] == '\n':
                continue
            if line[0] != '#':
                row=[]
                if args.obpp == args.bpp:
                    for idx in range(0,args.x,pixels):
                        if pixels==8:
                            # 8 pixels per byte
                            value = (values[int(line[idx])] << 7 |
                                     values[int(line[idx+1])] << 6 |
                                     values[int(line[idx+2])] << 5 |
                                     values[int(line[idx+3])] << 4 |
                                     values[int(line[idx+4])] << 3 |
                                     values[int(line[idx+5])] << 2 |
                                     values[int(line[idx+6])] << 1 |
                                     values[int(line[idx+7])]
                            )
                        elif pixels==4:
                            # 4 pixels per byte
                            value = (values[int(line[idx])]   << 6 |
                                     values[int(line[idx+1])] << 4 |
                                     values[int(line[idx+2])] << 2 |
                                     values[int(line[idx+3])]
                            )
                        elif pixels == 2:
                            # 2 pixels per byte
                            value = (values[int(line[idx])] << 4 |
                                     values[int(line[idx+1])])
                            #print("Values %d %d -> %d" %(values[int(line[idx])], values[int(line[idx+1])],value))
                        elif pixels == 1:
                            # 1 pixel per byte
                            value = values[int(line[idx])]
                        row.append(value)
                else:
                    if pixels != 4 and args.obpp != 16:
                        raise Exception ('Unsupported yet !')
                    clrdict = {0:0x0000, 1:0xFC00, 2:0xFFE0, 3:0xFFFF}
                    for idx in range(0,args.x,pixels):
                        for i in range (pixels):
                            c = clrdict[values[int(line[idx+i]) & 3]]
                            row.append (c & 0xff)
                            row.append ((c >> 8) & 0xff)
                out.write(bytearray(row))
                #print("Row values %s" %row)



