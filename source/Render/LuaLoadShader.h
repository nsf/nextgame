#pragma once

#include <lua.hpp>
#include "Render/OpenGL.h"
#include "Core/Error.h"
#include "Core/Vector.h"
#include "Core/Defer.h"

Vector<ShaderSection> load_shader_sections(lua_State *L, const char *filename,
	Error *err = &DefaultError);
