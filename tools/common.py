def convert_xywh_to_texcoords(x, y, w, h, imagew, imageh):
	xs = 1.0 / imagew
	ys = 1.0 / imageh
	tx = x * xs# + (xs/2)
	tx2 = (x + w) * xs# + (xs/2)
	ty = y * ys# + (ys/2)
	ty2 = (y + h) * ys# + (ys/2)
	return (tx, ty, tx2, ty2)
