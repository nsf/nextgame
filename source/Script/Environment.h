#pragma once

#include <lua.hpp>
#include "Core/Vector.h"
#include "Math/Vec.h"

enum EnvironmentVariableFlags {
	// automatically dump to ~/.config/nextgame/global.lua
	EVF_PERSISTENT = 1 << 0,

	// show in the gui
	EVF_GUI = 1 << 1,
};

template <typename T>
bool sync_compare(void *addr, void *last_sync_addr)
{
	bool desynced = *(const T*)addr != *(const T*)last_sync_addr;
	*(T*)addr = *(const T*)last_sync_addr;
	return desynced;
}

struct EnvironmentVariableRecord {
	void *addr = nullptr;
	const char *type = "";
	const char *name = "";
	unsigned flags = 0;
	const char *nice_name = "";
	const char *gui_attrs = "";
	const char *desc = "";

	int index = 0;
	void *last_sync_addr = nullptr;
	bool (*sync_compare)(void*, void*) = nullptr;

	bool sync() const
	{
		return (*sync_compare)(addr, last_sync_addr);
	}
};

struct EnvironmentBase {
	Vector<EnvironmentVariableRecord> m_variables;
	Vector<int> m_changed;

	void register_all(lua_State *L);
	Slice<const int> sync();

	EnvironmentBase() = default;
	NG_DELETE_COPY_AND_MOVE(EnvironmentBase);
};

struct EnvironmentVariableRecordHelper {
	EnvironmentVariableRecordHelper(EnvironmentBase *owner, void *last_sync_addr,
		bool (*sync_compare)(void*, void*),
		void *addr, const char *type, const char *name, unsigned flags = 0,
		const char *nice_name = "", const char *gui_attrs = "",
		const char *desc = "")
	{
		EnvironmentVariableRecord *r = owner->m_variables.append();
		r->addr = addr;
		r->type = type;
		r->name = name;
		r->flags = flags;
		r->nice_name = nice_name;
		r->gui_attrs = gui_attrs;
		r->desc = desc;
		r->last_sync_addr = last_sync_addr;
		r->sync_compare = sync_compare;
	}
};

#define ENV_VAR(type, name, init, ...)                             \
	type name = init;                                              \
	type _last_sync_##name = init;                                 \
	EnvironmentVariableRecordHelper _record_##name {               \
		this, &_last_sync_##name, sync_compare<type>,              \
		&name, #type, #name, __VA_ARGS__                           \
	}
