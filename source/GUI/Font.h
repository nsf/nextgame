#pragma once

#include "Math/Vec.h"
#include "Core/Vector.h"
#include "Core/Error.h"
#include "Render/OpenGL.h"
#include <cstdint>

struct FontGlyph {
	int32_t offset_x;
	int32_t offset_y;
	int32_t width;
	int32_t height;
	int32_t x_advance;
	Vec2 t0;
	Vec2 t1;
};

struct FontEncoding {
	int32_t unicode;
	int32_t index;
};

static inline bool operator<(const FontEncoding &l, const FontEncoding &r)  { return l.unicode < r.unicode; }
static inline bool operator==(const FontEncoding &l, const FontEncoding &r) { return l.unicode == r.unicode; }
static inline bool operator!=(const FontEncoding &l, const FontEncoding &r) { return l.unicode != r.unicode; }

struct Font {
	Vector<FontEncoding> encoding;
	Vector<FontGlyph> glyphs;
	Texture texture;
	int32_t y_advance;

	void dump() const;
	const FontGlyph *find_glyph(int32_t r) const;

	static Font new_from_file(const char *filename, Error *err = &DefaultError);
};

Font Font_FromFile(const char *filename, Error *err = &DefaultError);

