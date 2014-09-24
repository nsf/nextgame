#pragma once

#include <SDL2/SDL_mouse.h>
#include "Core/Error.h"

SDL_Cursor *load_cursor_from_file(const char *filename,
	Error *err = &DefaultError);
