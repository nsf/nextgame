#!/usr/bin/env python3

from PIL import Image
from PIL import ImageDraw
from collections import namedtuple
from argparse import ArgumentParser
from types import SimpleNamespace
from functools import cmp_to_key
import math, struct, os, common

Rect = namedtuple("Rect", "x y w h")
ImageInfo = SimpleNamespace

class ImagePack:
	def __init__(self, rect=Rect(0,0,512,512)):
		self.filled = False
		self.childr = None
		self.childl = None
		self.rect = rect

	def insert(self, w, h):
		r = self.rect # for less typing
		if self.childr:
			ret = self.childl.insert(w, h)
			if ret:
				return ret
			return self.childr.insert(w, h)
		else:
			if w > r.w or h > r.h or self.filled:
				return None

			if w == r.w and h == r.h:
				self.filled = True
				return r

			dw = r.w - w
			dh = r.h - h
			if dw > dh:
				# split vertically
				self.childl = ImagePack(Rect(r.x, r.y, w, r.h))
				self.childr = ImagePack(Rect(r.x+w, r.y, r.w-w-1, r.h))
			else:
				# split horizontally
				self.childl = ImagePack(Rect(r.x, r.y, r.w, h))
				self.childr = ImagePack(Rect(r.x, r.y+h, r.w, r.h-h-1))

			return self.childl.insert(w, h)

def load_images_info(image_files):
	out = []
	for f in image_files:
		img = Image.open(f)
		img.load()
		rect = Rect(*((0,0) + img.size))
		out.append(ImageInfo(img = img, rect = rect, name = f))
	return out

def sort_images(images):
	def rect_area(r):
		return r.w * r.h
	def cmp(a, b):
		return (a > b) - (a < b)
	def image_info_cmp(x, y):
		a = x.rect
		b = y.rect
		if a.w > a.h and b.w > b.h:
			return -cmp(a.h, b.h)
		elif a.h > a.w and b.h > b.w:
			return -cmp(a.w, b.w)
		else:
			return -cmp(rect_area(a), rect_area(b))

	images.sort(key=cmp_to_key(image_info_cmp))

def check_images_fit(images, w, h):
	pack = ImagePack(Rect(0,0,w,h))
	for image in images:
		r = pack.insert(image.rect.w, image.rect.h)
		if not r:
			return False
	return True

def make_new_wh(w, h, i):
	if i % 2:
		return w / 2, h
	else:
		return w, h / 2

def next_power_of_2(v):
	v -= 1
	v |= v >> 1
	v |= v >> 2
	v |= v >> 4
	v |= v >> 8
	v |= v >> 16
	return v + 1

def calculate_atlas_texture_dimensions(images):
	total_area = 0
	for image in images:
		total_area += image.rect.w * image.rect.h

	size = next_power_of_2(int(math.sqrt(total_area)))
	w, h = size, size
	exact_fit = False

	# if images doesn't fit, try multiplying texture area up to 4 times
	if not check_images_fit(images, w, h):
		exact_fit = True
		w *= 2
		if not check_images_fit(images, w, h):
			h *= 2

	# if atlas was ok before growing, we need to try to shrink it to the exact fit
	if not exact_fit:
		fits = True
		i = 0
		while True:
			neww, newh = make_new_wh(w, h, i)
			fits = check_images_fit(images, neww, newh)
			if fits:
				w, h = neww, newh
			else:
				break
			i += 1

	return w, h

def common_prefix(images):
	prefix = images[0].name
	for img in images[1:]:
		while not img.name.startswith(prefix):
			prefix = prefix[:-1]

	i = prefix.rfind(os.sep)
	if i != -1:
		prefix = prefix[:i+1]

	return len(prefix)

def draw_atlas(p):
	images = load_images_info(p.images)
	sort_images(images)
	w, h = calculate_atlas_texture_dimensions(images)

	pack = ImagePack(Rect(0, 0, w, h))
	for image in images:
		r = pack.insert(image.rect.w, image.rect.h)
		# no errors here
		image.rect = Rect(*r)

	outline = p.outline
	atlas = Image.new("RGBA", (w, h))
	draw = ImageDraw.Draw(atlas)
	for image in images:
		if not image.rect:
			continue
		r = image.rect
		box = (r.x, r.y, r.x + r.w, r.y + r.h)
		atlas.paste(image.img, box)
		if outline:
			draw.rectangle(box)

	prefix = common_prefix(images)
	imgw, imgh = atlas.size
	with open(p.output, "wb") as f:
		# magic, because it's cool
		f.write("NGAT".encode("utf-8"))

		# amount of entries
		f.write(struct.pack("<i", len(images)))

		for image in images:
			name = image.name[prefix:].encode("UTF-8")
			r = image.rect
			tx, ty, tx2, ty2 = common.convert_xywh_to_texcoords(
				r.x, r.y, r.w, r.h, imgw, imgh)

			# name length, name
			f.write(struct.pack("<i", len(name)))
			f.write(name)

			# offset_x, offset_y, width, height, tx, ty, tx2, ty2
			f.write(struct.pack("<iiiiffff",
				r.x, r.y, r.w, r.h, tx, ty, tx2, ty2))

		atlas.save(f, "PNG")

	if p.outpng:
		atlas.save(p.outpng)


parser = ArgumentParser(description='Generate an atlas out of multiple images')
parser.add_argument("images", nargs="+",
	help="image file to generate atlas out of",
	metavar="image")
parser.add_argument("-o", "--output",
	default="outatlas.atlas",
	help="write resulting atlas to FILE",
	metavar="FILE")
parser.add_argument("--outline",
	action="store_true",
	default=False,
	help="draw white outline rectangles around packed images")
#parser.add_argument("-x", "--width",
#	default=-1,
#	type=int,
#	help="specify exact width for atlas(es)")
#parser.add_argument("-y", "--height",
#	default=-1,
#	type=int,
#	help="specify exact height for atlas(es)")
parser.add_argument("--outpng",
	default=None,
	help="write output to png FILE as well",
	metavar="FILE")

p = parser.parse_args()
draw_atlas(p)
