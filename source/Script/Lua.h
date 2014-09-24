#pragma once

#include "Script/InterLua.h"
#include "Core/Error.h"
#include "Core/Utils.h"
#include "Core/String.h"
#include "OS/ThreadLocal.h"

enum LuaVMType {
	LUA_VM_GLOBAL,
	LUA_VM_LOCAL,
};

struct LuaVM {
	LuaVMType type;
	lua_State *L = nullptr;
	String scripts_dir;
	InterLua::Ref on_event;

	NG_DELETE_COPY_AND_MOVE(LuaVM);
	explicit LuaVM(LuaVMType type = LUA_VM_LOCAL);
	~LuaVM();

	void init_common();
	void init_game();
	void init_worker();
	void do_file(const char *filename, Error *err = &DefaultError);
	void do_string(const char *str, Error *err = &DefaultError);
	void do_format(const char *format, ...);
	void do_format_error(Error *err, const char *format, ...);
};

extern LuaVM *NG_LuaVM;
extern ThreadLocal<LuaVM> NG_WorkerLuaWM;
