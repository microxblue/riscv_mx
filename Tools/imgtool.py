#
# Copyright (c) 2021.  Micro Blue
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#


import os
import sys
import matplotlib.pyplot as plt
from PIL import Image

def convert_color (color, bins, trans):
    mask  = 0xf8
    value = ((color[0]>>3)<<10) + ((color[1]>>3)<<5) + ((color[2]>>3)<<0)
    if trans is not None:
        if value == trans:
            value = 0x8000
    bins.append(value & 0xff)
    bins.append((value>>8) & 0xff)
    return (color[0] & mask, color[1] & mask, color[2] & mask)

def convert_img (in_file, out_file, size = (320,240), trans = None):
    data = bytearray()
    im1        = Image.open(in_file)
    image_rgb  = im1.resize(size).convert("RGB")
    (x, y)     = image_rgb.size
    bins = bytearray()
    for j in range(y):
      for i in range(x):
        color     = image_rgb.getpixel((i,j))
        new_color = convert_color (color, bins, trans)
        image_rgb.putpixel((i,j), new_color)
    fd = open (out_file, 'wb')
    fd.write(bins)
    fd.close()
    return image_rgb

def convert_tile (in_file, out_file):
    size = (320,240)
    data = bytearray()
    im1        = Image.open(in_file)
    image_rgb  = im1.resize(size).convert("RGB")
    (x, y)     = image_rgb.size

    bins = bytearray()
    for s in range(0, y, 8):
      for t in range(0, x, 8):
        for j in range(s, s + 8):
          for i in range(t, t+8):
            color = image_rgb.getpixel((i,j))
            new_color = convert_color (color, bins, -1)

    fd = open (out_file, 'wb')
    fd.write(bins)
    fd.close()
    return image_rgb

def show (image):
    plt.imshow(image)
    plt.show()

def convert_sprite (in_file, out_file):
    convert_img (in_file, out_file, (16,16), 0)


def convert_sprite32 (in_file, out_file, trans = 0x7fff):
    size       = (32,32)
    im1        = Image.open(in_file)
    image_rgb  = im1.resize(size).convert("RGB")
    (x, y)     = image_rgb.size
    bins = bytearray()
    for j in range(y):
      for i in range(x):
        color     = image_rgb.getpixel((i,j))
        new_color = convert_color (color, bins, trans)
        image_rgb.putpixel((i,j), new_color)

    sprite = bytearray()
    for n in range (4):
        y = (n >> 1) * 16
        x = (n  % 2) * 16
        box = (x,   y, x+16, y+16)
        new_img = image_rgb.crop(box)

        y = 16
        x = 16
        bins = bytearray()
        for j in range(y):
          for i in range(x):
            color = new_img.getpixel((i,j))
            new_color = convert_color (color, bins, trans)
            new_img.putpixel((i,j), new_color)
        sprite.extend(bins)

        show(new_img)

    fd = open (out_file, 'wb')
    fd.write(sprite)
    fd.close()

def convert_sprite (in_file, out_file, trans = 0x7fff, size=(16,16)):
    im1        = Image.open(in_file)
    image_full = im1.resize(size).convert("RGB")
    (x, y)     = image_full.size

    sprite  = bytearray()
    bins    = bytearray()
    new_img = image_full
    for j in range(y):
      for i in range(x):
        color = new_img.getpixel((i,j))
        new_color = convert_color (color, bins, trans)
        new_img.putpixel((i,j), new_color)
    sprite.extend(bins)
    fd = open (out_file, 'wb')
    fd.write(sprite)
    fd.close()

    return image_full

def convert_sprite96 (in_file, out_file, trans = 0x7fff, flags = ''):
    size       = (96,32)
    im1        = Image.open(in_file)
    image_full = im1.resize(size).convert("RGB")
    (x, y)     = image_full.size

    sprite = bytearray()
    for m in range (3):
        x = m * 32
        y = 0
        box = (x, y, x+32, y+32)
        image_rgb = image_full.crop(box)
        #show(image_rgb)
        if 'b' in flags:
          x = 32
          y = 32
          bins = bytearray()
          new_img = image_rgb
          for j in range(y):
            for i in range(x):
              color = new_img.getpixel((i,j))
              new_color = convert_color (color, bins, trans)
              new_img.putpixel((i,j), new_color)
          sprite.extend(bins)
        else:
          for n in range (4):
            y = (n >> 1) * 16
            x = (n  % 2) * 16
            box = (x,   y, x+16, y+16)
            new_img = image_rgb.crop(box)

            y = 16
            x = 16
            bins = bytearray()
            for j in range(y):
              for i in range(x):
                color = new_img.getpixel((i,j))
                new_color = convert_color (color, bins, trans)
                new_img.putpixel((i,j), new_color)
            sprite.extend(bins)

    fd = open (out_file, 'wb')
    fd.write(sprite)
    fd.close()

    return image_full


def main ():

    if len(sys.argv) < 4:
        print ('%s <img1|img2> input_image output_image [-s]' % os.path.basename(__file__))
        print ('    img1: create 320x240 image')
        print ('    img2: create 320x480 image')
        print ('    img3: convert 320x240 image to char tiles')
        print ('    sprs: create 16x16 sprites')
        print ('    sprb: create 32x32 sprites')
        print ('    sprh: create 64x64 sprites')
        print ('   spr3s: create 16x16 sprites from a set of image group')
        print ('   spr3b: create 32x32 sprites from a set of image group')
        print ('       s: show ne image')
        print ('')
        return 0

    if len(sys.argv) == 4:
        flags = ''
    else:
        flags = sys.argv[4]

    mode     = sys.argv[1]
    in_file  = sys.argv[2]
    out_file = sys.argv[3]

    #convert_img (in_file, out_file)
    #convert_sprite96 (in_file, out_file)
    im = None
    if mode == 'img1':
      # 320x240
      im = convert_img (in_file, out_file)
    elif mode == 'img2':
      # 320x480
      im = convert_img (in_file, out_file, (320,480))
    elif mode == 'img3':
      # 320x240 to 8x8 tiles
      im = convert_tile (in_file, out_file)
    elif mode == 'spr3s':
      im = convert_sprite96 (in_file, out_file, flags = 's')
    elif mode == 'spr3b':
      im = convert_sprite96 (in_file, out_file, flags = 'b')
    elif mode == 'sprs':
      im = convert_sprite (in_file, out_file, size=(16,16))
    elif mode == 'sprb':
      im = convert_sprite (in_file, out_file, size=(32,32))
    elif mode == 'sprh':
      im = convert_sprite (in_file, out_file, size=(64,64))
    else:
      print ('Unsupported mode %s !' % mode)

    if im and 's' in flags:
      show (im)

main()