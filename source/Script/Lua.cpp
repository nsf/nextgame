#include "Script/Lua.h"
#include "Script/InterLua.h"
#include "Core/Memory.h"
#include "Core/String.h"
#include "Core/Defer.h"
#include "OS/IO.h"
#include <cstdarg>

LuaVM::LuaVM(LuaVMType type): type(type)
{
	if (type == LUA_VM_GLOBAL) {
		if (NG_LuaVM)
			die("There can only be one global LuaVM");
		NG_LuaVM = this;
	}

	L = luaL_newstate();
	luaL_openlibs(L);
}

LuaVM::~LuaVM()
{
	on_event.Unref();
	lua_close(L);
	if (type == LUA_VM_GLOBAL)
		NG_LuaVM = nullptr;
}

void LuaVM::init_common()
{
	do_string("global = {}");

	String base_dir = IO::get_environment("NEXTGAME_BASE_DIR");
	if (base_dir == "") {
		base_dir = IO::get_current_directory();
	}
	String mods_dir = base_dir + "/mods";
	scripts_dir = base_dir + "/scripts";

	auto global = InterLua::Global(L, "global");
	global["CONFIG_DIR"] = IO::get_application_directory().c_str();
	global["BASE_DIR"] = base_dir.c_str();
	global["SCRIPTS_DIR"] = scripts_dir.c_str();
	global["MODS_DIR"] = mods_dir.c_str();
	global["DEBUG"] = true;
	global["LUAVM"] = (void*)this;
}

void LuaVM::init_game()
{
	init_common();

	auto global = InterLua::Global(L, "global");
	global["MODE"] = "game";
}

void LuaVM::init_worker()
{
	init_common();

	auto global = InterLua::Global(L, "global");
	global["MODE"] = "worker";
}

static int traceback(lua_State *L)
{
	const char *msg = lua_tostring(L, -1);
	luaL_traceback(L, L, msg, 1);
	return 1;
}

static void do_with_traceback(lua_State *L, Error *err)
{
	lua_pushcfunction(L, traceback);
	lua_pushvalue(L, -2);
	const int result = lua_pcall(L, 0, LUA_MULTRET, -2);
	if (result != 0) {
		err->set("%s", lua_tostring(L, -1));
		lua_pop(L, 1);
	}
}

void LuaVM::do_file(const char *filename, Error *err)
{
	String fullpath = scripts_dir + "/" + filename;
	const int result = luaL_loadfile(L, fullpath.c_str());
	if (result != 0) {
		err->set("%s", lua_tostring(L, -1));
		lua_pop(L, 1);
		return;
	}

	do_with_traceback(L, err);
}

void LuaVM::do_string(const char *str, Error *err)
{
	const int result = luaL_loadstring(L, str);
	if (result != 0) {
		err->set("%s", lua_tostring(L, -1));
		lua_pop(L, 1);
		return;
	}

	do_with_traceback(L, err);
}

void LuaVM::do_format(const char *format, ...)
{
	va_list va;
	va_start(va, format);
	auto s = String::vformat(format, va);
	va_end(va);

	do_string(s.c_str());
}

void LuaVM::do_format_error(Error *err, const char *format, ...)
{
	va_list va;
	va_start(va, format);
	auto s = String::vformat(format, va);
	va_end(va);

	do_string(s.c_str(), err);
}

LuaVM *NG_LuaVM = nullptr;
ThreadLocal<LuaVM> NG_WorkerLuaWM;
