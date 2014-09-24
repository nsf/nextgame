#include "Script/InterLua.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdarg>
#include <unordered_map>
#include <memory>

namespace InterLua {

struct or_die_t {};
const or_die_t or_die = {};

} // namespace InterLua

void* operator new(size_t size, const InterLua::or_die_t&) noexcept
{
	void *out = operator new(size, std::nothrow);
	if (!out)
		InterLua::die("InterLua: out of memory");
	return out;
}

void *operator new[](size_t size, const InterLua::or_die_t&) noexcept
{
	void *out = operator new[](size, std::nothrow);
	if (!out)
		InterLua::die("InterLua: out of memory");
	return out;
}

namespace InterLua {

static std::unique_ptr<char[]> vasprintf(const char *format, va_list va)
{
	va_list va2;
	va_copy(va2, va);

	int n = std::vsnprintf(nullptr, 0, format, va2);
	va_end(va2);
	if (n < 0)
		die("null vsnprintf error");
	if (n == 0) {
		char *buf = new (or_die) char[1];
		buf[0] = '\0';
		return std::unique_ptr<char[]>{buf};
	}

	char *buf = new (or_die) char[n+1];
	int nw = std::vsnprintf(buf, n+1, format, va);
	if (n != nw)
		die("vsnprintf failed to write n bytes");

	return std::unique_ptr<char[]>{buf};
}

static std::unique_ptr<char[]> asprintf(const char *format, ...)
{
	va_list va;
	va_start(va, format);
	auto s = vasprintf(format, va);
	va_end(va);
	return s;
}

//============================================================================
// misc
//============================================================================

void die(const char *str, ...)
{
	va_list va;
	va_start(va, str);
	std::vfprintf(stderr, str, va);
	va_end(va);
	std::fprintf(stderr, "\n");
	std::abort();
}

void stack_dump(lua_State *L)
{
      int i;
      int top = lua_gettop(L);
      printf("lua stack dump -------------\n");
      for (i = 1; i <= top; i++) {  /* repeat for each level */
        int t = lua_type(L, i);
        switch (t) {
          case LUA_TSTRING:  /* strings */
            printf("string: `%s'", lua_tostring(L, i));
            break;
          case LUA_TBOOLEAN:  /* booleans */
            printf(lua_toboolean(L, i) ? "boolean: true" : "boolean: false");
            break;
          case LUA_TNUMBER:  /* numbers */
            printf("number: %g", lua_tonumber(L, i));
            break;
          default:  /* other values */
            printf("other: %s", lua_typename(L, t));
            break;
        }
        printf(" | ");  /* put a separator */
      }
      printf("\n");  /* end the listing */
      printf("----------------------------\n");
}

Error::~Error()
{
	delete[] message;
}

void Error::Reset()
{
	delete[] message;
	code = _INTERLUA_OK;
	message = nullptr;
}

void Error::Set(int code, const char *format, ...)
{
	Reset();

	this->code = code;
	if (verbosity == Quiet)
		return;

	va_list va;
	va_start(va, format);
	message = vasprintf(format, va).release();
	va_end(va);
}

void AbortError::Set(int code, const char *format, ...)
{
	std::fprintf(stderr, "INTERLUA ABORT (%d): ", code);
	va_list va;
	va_start(va, format);
	std::vfprintf(stderr, format, va);
	va_end(va);
	std::fprintf(stderr, "\n");
	std::abort();
}

AbortError DefaultError;

// similar to luaL_where call with level == 1
static std::unique_ptr<char[]> where(lua_State *L)
{
	lua_Debug ar;
	if (lua_getstack(L, 1, &ar)) {
		lua_getinfo(L, "Sl", &ar);
		if (ar.currentline > 0) {
			return asprintf("%s:%d: ",
				ar.short_src, ar.currentline);
		}
	}
	return asprintf("");
}

static void tag_error(lua_State *L, int narg, int tag, Error *err)
{
	const char *expected = lua_typename(L, tag);
	auto msg = asprintf("%s expected, got %s",
		expected, luaL_typename(L, narg));
	argerror(L, narg, msg.get(), err);
}

void argerror(lua_State *L, int narg, const char *extra, Error *err)
{
	if (err->Verbosity() == Quiet) {
		err->Set(LUA_ERRRUN, "");
		return;
	}

	lua_Debug ar;
	auto loc = where(L);
	if (!lua_getstack(L, 0, &ar)) {
		// no stack frame?
		err->Set(LUA_ERRRUN, "%s" "bad argument: #%d (%s)",
			loc.get(), narg, extra);
		return;
	}
	lua_getinfo(L, "n", &ar);
	if (strcmp(ar.namewhat, "method") == 0) {
		narg--; // do not count 'self'
		if (narg == 0) {
			err->Set(LUA_ERRRUN, "%s" "calling " LUA_QS " on bad self (%s)",
				loc.get(), ar.name, extra);
			return;
		}
	}
	if (ar.name == nullptr)
		ar.name = "?";
	err->Set(LUA_ERRRUN, "%s" "bad argument #%d to " LUA_QS " (%s)",
		loc.get(), narg, ar.name, extra);
}

void checkany(lua_State *L, int narg, Error *err)
{
	if (lua_type(L, narg) == LUA_TNONE)
		argerror(L, narg, "value expected", err);
}

void checklightuserdata(lua_State *L, int narg, Error *err)
{
	if (lua_type(L, narg) != LUA_TLIGHTUSERDATA)
		argerror(L, narg, "lightuserdata expected", err);
}

void checkinteger(lua_State *L, int narg, Error *err)
{
#if LUA_VERSION_NUM < 502
	lua_Integer d = lua_tointeger(L, narg);
	if (d == 0 && !lua_isnumber(L, narg))
		tag_error(L, narg, LUA_TNUMBER, err);
#else
	int isnum;
	lua_tointegerx(L, narg, &isnum);
	if (!isnum)
		tag_error(L, narg, LUA_TNUMBER, err);
#endif
}

void checknumber(lua_State *L, int narg, Error *err)
{
#if LUA_VERSION_NUM < 502
	lua_Number d = lua_tonumber(L, narg);
	if (d == 0 && !lua_isnumber(L, narg))
		tag_error(L, narg, LUA_TNUMBER, err);
#else
	int isnum;
	lua_tonumberx(L, narg, &isnum);
	if (!isnum)
		tag_error(L, narg, LUA_TNUMBER, err);
#endif
}

void checkstring(lua_State *L, int narg, Error *err)
{
	const char *s = lua_tolstring(L, narg, nullptr);
	if (!s)
		tag_error(L, narg, LUA_TSTRING, err);
}

void ManualError::LJCheckAndDestroy(lua_State *L)
{
	Error *err = Get();
	if (*err) {
		lua_pushstring(L, err->What());
		Destroy();
		lua_error(L);
	} else {
		Destroy();
	}
}

NSWrapper NSWrapper::Namespace(const char *name)
{
	rawgetfield(L, -1, name);
	if (!lua_isnil(L, -1))
		return {L};

	// pop nil left from the rawgetfield call above
	lua_pop(L, 1);

	lua_newtable(L);
	lua_pushvalue(L, -1);
	rawsetfield(L, -3, name);
	return {L};
}

} // namespace InterLua
