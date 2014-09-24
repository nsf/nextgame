#pragma once

#include "Math/Vec.h"
#include "Core/Vector.h"
#include "Core/String.h"
#include "Core/Error.h"
#include "Render/OpenGL.h"
#include <cstdint>

struct AtlasImage {
	int32_t offset_x;
	int32_t offset_y;
	int32_t width;
	int32_t height;

	Vec2 t0;
	Vec2 t1;

	String name;

	Vec2 tex_coord_for(const Vec2i &p) const;
};

struct Atlas {
	Vector<AtlasImage> images;
	Texture texture;

	const AtlasImage *get(const char *image) const;
	void dump() const;
};

Atlas Atlas_FromFile(const char *filename, Error *err = &DefaultError);

