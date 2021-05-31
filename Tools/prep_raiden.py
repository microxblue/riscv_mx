import os
import sys
import matplotlib.pyplot as plt
from PIL import Image

class SDL_Rect:
    def __init__(self, x, y, w, h):
        self.x = x
        self.y = y
        self.w = w
        self.h = h

    def get_box (self):
        return (self.x, self.y, self.x+self.w, self.y+self.h)

def show (image):
    plt.imshow(image)
    plt.show()


def convert_img (image_rgb, out_file, trans=None):
    (x, y)     = image_rgb.size
    bins = bytearray()
    for j in range(y):
      for i in range(x):
        color     = image_rgb.getpixel((i,j))
        new_color = convert_color (color, bins, trans)
        image_rgb.putpixel((i,j), new_color)

    if out_file:
      fd = open (out_file, 'wb')
      fd.write(bins)
      fd.close()
      return image_rgb
    else:
      return bins


def create_background (in_file, out_file):
    # foreground
    foreground = SDL_Rect(352, 134, 351, 3265);

    # Background / sky
    background = SDL_Rect (0, 134, 351, 3265);

    im_full = Image.open(in_file).convert("RGBA")
    im_fg   = im_full.crop(foreground.get_box())
    im_bg   = im_full.crop(background.get_box())

    img = Image.alpha_composite(im_bg, im_fg)
    size = img.size
    new_im = img.resize((320, int(320.0 / size[0] * size[1])))
    convert_img (new_im, out_file)


def create_shooting (in_file, out_file):
    shoot    = SDL_Rect(182, 34, 8, 8);
    im_full  = Image.open(in_file)
    im_shoot = im_full.crop(shoot.get_box())
    convert_img (im_shoot, out_file, trans=0x7fff)


def create_bomb (in_file, out_file):
    bomb =[ ( 0, 247, 73, 64),	( 73, 247, 73, 64),	(146, 247, 73, 64),	(219, 247, 73, 64),
            (292, 247, 73, 64),	(365, 247, 73, 64),	(438, 247, 73, 64),	(511, 247, 73, 64),
            (584, 247, 73, 64) ]
    im_full  = Image.open(in_file)
    bins = bytearray()
    for each in bomb:
        box = (each[0],each[1],each[0]+each[2],each[1]+each[3])
        im = im_full.crop(box)
        im = im.resize((64,64))
        bins.extend(convert_img (im, '', trans=0x7fff))

    fd = open (out_file, 'wb')
    fd.write(bins)
    fd.close()


def convert_color (color, bins, trans):
    mask  = 0xf8
    value = ((color[0]>>3)<<10) + ((color[1]>>3)<<5) + ((color[2]>>3)<<0)
    if trans is not None:
        if value == trans:
            value = 0x8000
    bins.append(value & 0xff)
    bins.append((value>>8) & 0xff)
    return (color[0] & mask, color[1] & mask, color[2] & mask)


if __name__ == '__main__':

    img_dir = sys.argv[1]
    out_dir = sys.argv[2]

    # Create background
    create_background('%s/lvl1_tilemap.png' % img_dir, '%s/bg.bin' % out_dir)

    # Create flighter
    create_shooting ('%s/Raiden_Spaceship.png' % img_dir, '%s/shoot_8_1.bin' % out_dir)

    # Create flighter
    create_bomb ('%s/Particles_Spritesheet.png' % img_dir, '%s/bomb_64_9.bin' % out_dir)

