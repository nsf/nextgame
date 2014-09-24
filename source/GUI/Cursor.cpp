#include "GUI/Cursor.h"
#include "OS/IO.h"
#include "Render/PNG.h"
#include "Core/ByteIO.h"
#include "Core/Defer.h"
#include <cstdint>

SDL_Cursor *load_cursor_from_file(const char *filename, Error *err)
{
	auto data = IO::read_file(filename, err);
	if (*err)
		return nullptr;

	if (data.length() < 4 || slice_cast<const char>(data.sub(0, 4)) != "NGCR") {
		err->set("Bad magic, NGCR expected");
		return nullptr;
	}
	ByteReader r(data.sub(4));
	int hotspot_x = r.read_int32(err);
	int hotspot_y = r.read_int32(err);
	if (*err)
		return nullptr;

	int w, h;
	auto pngdata = load_data_from_png_memory(r.data, &w, &h, err);
	if (*err)
		return nullptr;

	SDL_Surface *s = SDL_CreateRGBSurfaceFrom(pngdata.data(), w, h, 32, 32*4,
		0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000);
	DEFER { SDL_FreeSurface(s); };
	return SDL_CreateColorCursor(s, hotspot_x, hotspot_y);
}
