#pragma once

#include "Render/OpenGL.h"

Texture load_texture_array_from_dds_files(
	Slice<String> filenames, Error *err = &DefaultError);
