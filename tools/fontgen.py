#!/usr/bin/env python3

from argparse import ArgumentParser
import cairo
import math
import pickle
import struct
import common
from collections import namedtuple
from io import BytesIO

Glyph = namedtuple('Glyph', [
	'symbol',
	'x_bearing',
	'y_bearing',
	'width',
	'height',
	'x_advance',
	'y_advance',
])

DrawnGlyph = namedtuple('DrawnGlyph', [
	'symbol',
	'x_bearing',
	'y_bearing',
	'width',
	'height',
	'x_advance',
	'y_advance',
	'x',
	'y',
])

FontInfo = namedtuple('FontInfo', [
	'ascent',
	'descent',
	'height',
	'max_x_advance',
	'max_y_advance',
])

slant_map = {
	"normal" : cairo.FONT_SLANT_NORMAL,
	"italic" : cairo.FONT_SLANT_ITALIC,
	"oblique" : cairo.FONT_SLANT_OBLIQUE
}

weight_map = {
	"normal" : cairo.FONT_WEIGHT_NORMAL,
	"bold" : cairo.FONT_WEIGHT_BOLD
}

hint_style_map = {
	"default" : cairo.HINT_STYLE_DEFAULT,
	"none" : cairo.HINT_STYLE_NONE,
	"slight" : cairo.HINT_STYLE_SLIGHT,
	"medium" : cairo.HINT_STYLE_MEDIUM,
	"full" : cairo.HINT_STYLE_FULL,
}

def next_power_of_2(v):
	v -= 1
	v |= v >> 1
	v |= v >> 2
	v |= v >> 4
	v |= v >> 8
	v |= v >> 16
	return v + 1

def read_symbols_from_file(filename):
	with open(filename, "r", encoding="UTF-8") as f:
		text = f.read()
	return text.replace("\n", "").replace("\r", "")

def create_cairo_for_parameters(p, surface):
	cr = cairo.Context(surface)
	cr.select_font_face(p.font, p.slant, p.weight)
	cr.set_font_size(p.size)

	fnopts = cr.get_font_options()
	fnopts.set_hint_style(p.hint_style)
	cr.set_font_options(fnopts)

	return cr

def collect_glyphs(p):
	fake_surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, 32, 32)
	cr = create_cairo_for_parameters(p, fake_surface)
	glyphs = []
	for symbol in p.symbols:
		se = cr.text_extents(symbol)
		se = (int(round(se[0])), int(round(se[1])), int(round(se[2])), int(round(se[3])), int(round(se[4])), int(round(se[5])))
		glyphs.append(Glyph(
			*((symbol,) + se)
		))
	return glyphs

def check_glyphs_fit(glyphs, w, h):
	line_height = 0
	x = 0
	y = 0
	for g in glyphs:
		if x + g.width > w:
			x = 0
			y += line_height + 1
			line_height = 0
		line_height = max(line_height, g.height)
		if y + line_height > h:
			return False
		x += g.width + 1
	return True

def make_new_wh(w, h, i):
	if i % 2:
		return w / 2, h
	else:
		return w, h / 2

def calculate_font_texture_dimensions(glyphs):
	total_area = 0
	for g in glyphs:
		total_area += g.width * g.height

	size = next_power_of_2(int(math.sqrt(total_area)))
	w, h = size, size
	exact_fit = False

	# if font doesn't fit, try multiplying texture area up to 4 times
	if not check_glyphs_fit(glyphs, w, h):
		exact_fit = True
		w *= 2
		if not check_glyphs_fit(glyphs, w, h):
			h *= 2

	# if font fitted before growing, we need to try to shrink it to the exact fit
	if not exact_fit:
		fits = True
		i = 0
		while True:
			neww, newh = make_new_wh(w, h, i)
			fits = check_glyphs_fit(glyphs, neww, newh)
			if fits:
				w, h = neww, newh
			else:
				break
			i += 1

	return int(w), int(h)

def glyph_negative_height(g):
	return -g.height

def draw_glyphs(p):
	glyphs = collect_glyphs(p)
	glyphs.sort(key=glyph_negative_height)
	w, h = calculate_font_texture_dimensions(glyphs)
	surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, w, h)
	cr = create_cairo_for_parameters(p, surface)
	fontinfo = FontInfo(*cr.font_extents())
	fontinfo = FontInfo(int(round(fontinfo[0])), int(round(fontinfo[1])), int(round(fontinfo[2])), int(round(fontinfo[3])), int(round(fontinfo[4])))
	drawnglyphs = []

	def draw_glyph(cr, glyph, x, y):
		cr.move_to(x - glyph.x_bearing, y - glyph.y_bearing)
		cr.show_text(glyph.symbol)

	cr.set_operator(cairo.OPERATOR_SOURCE)
	cr.set_source_rgba(1, 1, 1, 1)
	line_height = 0
	x = 0
	y = 0
	for g in glyphs:
		if x + g.width > w:
			x = 0
			y += line_height + 1
			line_height = 0
		line_height = max(line_height, g.height)
		draw_glyph(cr, g, x, y)
		drawnglyphs.append(DrawnGlyph(*(g + (int(x), int(y)))))
		x += g.width + 1

	out_glyphs = []
	for g in drawnglyphs:
		tx, ty, tx2, ty2 = common.convert_xywh_to_texcoords(
			g.x, g.y, g.width, g.height, w, h)
		out_glyphs.append({
			"symbol": g.symbol,
			"offset_x": g.x_bearing,
			"offset_y": fontinfo.ascent + g.y_bearing,
			"width": g.width,
			"height": g.height,
			"x_advance": g.x_advance,
			"tx": tx,
			"ty": ty,
			"tx2": tx2,
			"ty2": ty2,
		})
	out_glyphs.sort(key=lambda g: ord(g["symbol"]))

	with open(p.output, "wb") as f:
		# magic, because it's cool
		f.write("NGFN".encode("utf-8"))

		# font height, num glyphs
		f.write(struct.pack("<ii", fontinfo.height, len(out_glyphs)))

		# encoding table: unicode code point -> index
		for i, g in enumerate(out_glyphs):
			f.write(struct.pack("<ii", ord(g["symbol"]), i))

		# the glyphs:
		# offset_x, offset_y, width, height, tx, ty, tx2, ty2, x_advance
		for g in out_glyphs:
			f.write(struct.pack("<iiiiiffff",
				g["offset_x"],
				g["offset_y"],
				g["width"],
				g["height"],
				g["x_advance"],
				g["tx"],
				g["ty"],
				g["tx2"],
				g["ty2"],
			))

		# and the image itself
		surface.write_to_png(f)

	if p.outpng:
		surface.write_to_png(p.outpng)

parser = ArgumentParser(description='Generate bitmap font')
parser.add_argument("-o", "--output",
	default="outfont.font",
	help="write resulting bitmap font to FILE",
	metavar="FILE")

parser.add_argument("--font",
	default="DejaVu Sans",
	help="font face name",
	metavar="FACE")

parser.add_argument("--slant",
	default="normal",
	help="font slant",
	choices=("normal", "italic", "oblique"))

parser.add_argument("--weight",
	default="normal",
	help="font weight",
	choices=("normal", "bold"))

parser.add_argument("--size",
	default=8,
	help="size of the font in pixels",
	type=int)

parser.add_argument("--hint-style",
	default="default",
	help="hint style",
	choices=("default", "none", "slight", "medium", "full"))

parser.add_argument("--symbols",
	default=None,
	help="file containing symbols (utf-8 encoded)",
	metavar="FILE")

parser.add_argument("--outpng",
	default=None,
	help="write png file separately to FILE as well",
	metavar="FILE")

p = parser.parse_args()

if p.symbols:
	p.symbols = read_symbols_from_file(p.symbols)
else:
	p.symbols = """ `1234567890-=\~!@#$%^&*()_+|qwertyuiop[]QWERTYUIOP{}asdfghjkl;'ASDFGHJKL:"zxcvbnm,./ZXCVBNM<>?"""
p.slant = slant_map[p.slant]
p.weight = weight_map[p.weight]
p.hint_style = hint_style_map[p.hint_style]

draw_glyphs(p)
