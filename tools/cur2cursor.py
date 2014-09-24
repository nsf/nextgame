#!/usr/bin/env python3

from argparse import ArgumentParser
import struct, os

def extract_hotspot(filename):
	with open(filename, "rb") as f:
		contents = f.read()

	_, _, nimages = struct.unpack("<hhh", contents[:6])
	contents = contents[6:]

	largest_area, lx, ly = 0, 0, 0
	for i in range(nimages):
		w, h, _, _, x, y = struct.unpack("<BBBBhh", contents[:8])
		if w == 0:
			w = 256
		if h == 0:
			h = 256
		area = w * h
		if area > largest_area:
			largest_area, lx, ly = area, x, y
		contents = contents[16:]
	return lx, ly

parser = ArgumentParser(description="Convert windows .cur to NG's .cursor")
parser.add_argument("cursor", nargs=1,
	help="windows .cur cursor file",
	metavar="FILE")
parser.add_argument("-o", "--output",
	default="outcursor.cursor",
	help="write resulting cursor to FILE",
	metavar="FILE")
parser.add_argument("--outpng",
	default=None,
	help="write output to png FILE as well",
	metavar="FILE")

p = parser.parse_args()

hotx, hoty = extract_hotspot(p.cursor[0])
with open(p.output, "wb") as f:
	f.write("NGCR".encode("utf-8"));
	f.write(struct.pack("<ii", hotx, hoty))

# NOTE: sadly PIL doesn't support BMPs with alpha properly, therefore I use
# os.system with ImageMagick inside
os.system("convert %s png:- >> %s" % (p.cursor[0], p.output))
if p.outpng:
	os.system("convert %s %s" % (p.cursor[0], p.outpng))
