#include "GUI/Font.h"
#include "OS/IO.h"
#include "Core/ByteIO.h"
#include "Core/Defer.h"
#include "Render/PNG.h"
#include <algorithm>
#include <cstdio>

void Font::dump() const
{
	printf("Font height: %d\n", y_advance);
	printf("N Glyphs: %d\n", glyphs.length());
	for (const auto &e : encoding) {
		printf("Encoding entry: %d = %d\n", e.unicode, e.index);
	}
	for (const auto &g : glyphs) {
		printf("Glyphs entry: %d %d %d %d (%.3f %.3f %.3f %.3f) %d\n",
			g.offset_x, g.offset_y, g.width, g.height,
			g.t0.x, g.t0.y, g.t1.x, g.t1.y, g.x_advance);
	}
}

const FontGlyph *Font::find_glyph(int32_t r) const
{
	const FontEncoding *beg = encoding.data();
	const FontEncoding *end = beg + encoding.length();
	auto it = std::lower_bound(beg, end, FontEncoding{r, 0});
	if (it == end)
		return nullptr;

	if (*it != FontEncoding{r, 0})
		return nullptr;

	return &glyphs[it->index];
}

Font Font_FromFile(const char *filename, Error *err)
{
	Font f;
	auto data = IO::read_file(filename, err);
	if (*err)
		return Font();

	if (data.length() < 4 || slice_cast<const char>(data.sub(0, 4)) != "NGFN") {
		err->set("Bad magic, NGFN expected");
		return Font();
	}

	ByteReader r(data.sub(4));

	f.y_advance = r.read_int32(err);
	int n = r.read_int32(err);
	if (*err)
		return Font();

	f.encoding.resize(n);
	f.glyphs.resize(n);
	for (auto &e : f.encoding) {
		e.unicode = r.read_int32(err);
		e.index = r.read_int32(err);
		if (*err)
			return Font();
	}
	for (auto &g : f.glyphs) {
		g.offset_x = r.read_int32(err);
		g.offset_y = r.read_int32(err);
		g.width = r.read_int32(err);
		g.height = r.read_int32(err);
		g.x_advance = r.read_int32(err);
		g.t0.x = r.read_float(err);
		g.t0.y = r.read_float(err);
		g.t1.x = r.read_float(err);
		g.t1.y = r.read_float(err);
		if (*err)
			return Font();
	}

	f.texture = load_texture_from_png_memory(r.data, err);
	if (*err)
		return Font();
	return f;
}
