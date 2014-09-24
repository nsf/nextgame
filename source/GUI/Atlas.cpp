#include "GUI/Atlas.h"
#include "OS/IO.h"
#include "Core/Defer.h"
#include "Core/ByteIO.h"
#include "Render/PNG.h"
#include <cstdio>

Vec2 AtlasImage::tex_coord_for(const Vec2i &p) const
{
	NG_ASSERT(p.x <= width && p.x >= 0);
	NG_ASSERT(p.y <= height && p.y >= 0);

	const Vec2 tsize(t1.x - t0.x, t1.y - t0.y);
	const Vec2 pos = ToVec2(p) / Vec2(width, height);
	return t0 + pos * tsize;
}

const AtlasImage *Atlas::get(const char *name) const
{
	for (const auto &image : images) {
		if (image.name == name)
			return &image;
	}
	warn("Image '%s' was not found in atlas", name);
	return nullptr;
}

void Atlas::dump() const
{
	printf("Atlas\n");
	printf("N Images: %d\n", images.length());
	for (const auto &g : images) {
		printf("Image entry: (%s) %d %d %d %d (%.3f %.3f %.3f %.3f)\n",
			g.name.c_str(),
			g.offset_x, g.offset_y, g.width, g.height,
			g.t0.x, g.t0.y, g.t1.x, g.t1.y);
	}
}

Atlas Atlas_FromFile(const char *filename, Error *err)
{
	Atlas a;
	auto data = IO::read_file(filename, err);
	if (*err)
		return Atlas();

	if (data.length() < 4 || slice_cast<const char>(data.sub(0, 4)) != "NGAT") {
		err->set("Bad magic, NGAT expected");
		return Atlas();
	}

	ByteReader r(data.sub(4));
	int n = r.read_int32(err);
	if (*err)
		return Atlas();

	a.images.reserve(n);
	for (int i = 0; i < n; i++) {
		int len = r.read_int32(err);
		if (*err)
			return Atlas();

		AtlasImage *img = a.images.append();
		img->name = String(len);

		r.read(slice_cast<uint8_t>(img->name.sub()), err);
		if (*err)
			return Atlas();

		img->offset_x = r.read_int32(err);
		img->offset_y = r.read_int32(err);
		img->width = r.read_int32(err);
		img->height = r.read_int32(err);
		img->t0.x = r.read_float(err);
		img->t0.y = r.read_float(err);
		img->t1.x = r.read_float(err);
		img->t1.y = r.read_float(err);
		if (*err)
			return Atlas();
	}

	a.texture = load_texture_from_png_memory(r.data, err);
	if (*err)
		return Atlas();

	return a;
}
