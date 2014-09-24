#pragma once

#include "Render/OpenGL.h"

Texture load_texture_from_png_memory(Slice<const uint8_t> data,
	Error *err = &DefaultError);
Vector<uint8_t> load_data_from_png_memory(Slice<const uint8_t> data,
	int *width, int *height, Error *err = &DefaultError);
