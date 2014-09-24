#include "Script/Environment.h"
#include "Script/InterLua.h"

#define DEFINE_ENV_VAR_GET_SET_REF(type)                                                \
NG_LUA_API void NG_EnvVar_##type##_Get(EnvironmentVariableRecord *v, type *out)         \
{                                                                                       \
	*out = *(type*)v->addr;                                                             \
}                                                                                       \
NG_LUA_API void NG_EnvVar_##type##_Set(EnvironmentVariableRecord *v, const type &value) \
{                                                                                       \
	*(type*)v->addr = value;                                                            \
	*(type*)v->last_sync_addr = value;                                                  \
}

#define DEFINE_ENV_VAR_GET_SET(type)                                             \
NG_LUA_API void NG_EnvVar_##type##_Get(EnvironmentVariableRecord *v, type *out)  \
{                                                                                \
	*out = *(type*)v->addr;                                                      \
}                                                                                \
NG_LUA_API void NG_EnvVar_##type##_Set(EnvironmentVariableRecord *v, type value) \
{                                                                                \
	*(type*)v->addr = value;                                                     \
	*(type*)v->last_sync_addr = value;                                           \
}

DEFINE_ENV_VAR_GET_SET(int)
DEFINE_ENV_VAR_GET_SET(bool)
DEFINE_ENV_VAR_GET_SET(float)
DEFINE_ENV_VAR_GET_SET(double)
DEFINE_ENV_VAR_GET_SET_REF(Vec3i)
DEFINE_ENV_VAR_GET_SET_REF(Vec3)

void EnvironmentBase::register_all(lua_State *L)
{
	auto tbl = InterLua::Global(L, "global");
	auto env_vars = tbl["GAME_ENV_VARS"];
	env_vars = InterLua::NewTable(L);
	int i = 1;

	for (EnvironmentVariableRecord &r : m_variables) {
		auto v = InterLua::NewTable(L);
		v["name"] = r.name;
		v["type"] = r.type;
		v["addr"] = (void*)&r;
		if (strlen(r.nice_name) > 0)
			v["nice_name"] = r.nice_name;
		v["desc"] = r.desc;
		v["flags"] = r.flags;
		v["gui_attrs"] = r.gui_attrs;
		r.index = i;
		env_vars[i++] = v;
	}
}

Slice<const int> EnvironmentBase::sync()
{
	m_changed.clear();
	for (EnvironmentVariableRecord &r : m_variables) {
		if (r.sync())
			m_changed.append(r.index);
	}
	return m_changed;
}
