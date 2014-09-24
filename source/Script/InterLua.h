#pragma once

#include <lua.hpp>
#include <type_traits>
#include <utility>
#include <new>
#include <cstdint>
//#include <cstddef>
#include <cstring>

//----------------------------------------------------------------------------
// Workarounds for Lua versions prior to 5.2
//----------------------------------------------------------------------------
#if LUA_VERSION_NUM < 502
//----------------------------------------------------------------------------

static inline int _interlua_absindex(lua_State *L, int idx) {
	if (idx > LUA_REGISTRYINDEX && idx < 0)
		return lua_gettop(L) + idx + 1;
	else
		return idx;
}

static inline void _interlua_rawgetp(lua_State *L, int idx, const void *p) {
	idx = _interlua_absindex(L, idx);
	lua_pushlightuserdata(L, const_cast<void*>(p));
	lua_rawget(L, idx);
}

static inline void _interlua_rawsetp(lua_State *L, int idx, const void *p) {
	idx = _interlua_absindex(L, idx);
	lua_pushlightuserdata(L, const_cast<void*>(p));
	lua_insert(L, -2); // put key behind value
	lua_rawset(L, idx);
}

#define _INTERLUA_OPEQ 1
#define _INTERLUA_OPLT 2
#define _INTERLUA_OPLE 3

inline int _interlua_compare(lua_State *L, int idx1, int idx2, int op) {
	switch (op) {
	case _INTERLUA_OPEQ:
		return lua_equal(L, idx1, idx2);
	case _INTERLUA_OPLT:
		return lua_lessthan(L, idx1, idx2);
	case _INTERLUA_OPLE:
		return lua_equal(L, idx1, idx2) || lua_lessthan(L, idx1, idx2);
	default:
		return 0;
	}
}

inline size_t _interlua_rawlen(lua_State *L, int idx) {
	return lua_objlen(L, idx);
}

#define _INTERLUA_OK 0
#define _interlua_pushglobaltable(L) lua_pushvalue(L, LUA_GLOBALSINDEX)

//----------------------------------------------------------------------------
#else
//----------------------------------------------------------------------------

#define _interlua_absindex lua_absindex
#define _interlua_rawgetp lua_rawgetp
#define _interlua_rawsetp lua_rawsetp
#define _INTERLUA_OPEQ LUA_OPEQ
#define _INTERLUA_OPLT LUA_OPLT
#define _INTERLUA_OPLE LUA_OPLE
#define _interlua_compare lua_compare
#define _interlua_rawlen lua_rawlen
#define _INTERLUA_OK LUA_OK
#define _interlua_pushglobaltable lua_pushglobaltable

//----------------------------------------------------------------------------
#endif
//----------------------------------------------------------------------------

namespace InterLua {

//============================================================================
// Misc helpers
//============================================================================

void die(const char *str, ...);
void stack_dump(lua_State *L);

// pushes t[key] onto the stack, where t is the table at the given index
static inline void rawgetfield(lua_State *L, int index, const char *key) {
	index = _interlua_absindex(L, index);
	lua_pushstring(L, key);
	lua_rawget(L, index);
}

// does t[key] = v, where t is the table at the given index and v is the value
// at the top of the stack, pops the value from the stack
static inline void rawsetfield(lua_State *L, int index, const char *key) {
	index = _interlua_absindex(L, index);
	lua_pushstring(L, key);
	lua_insert(L, -2);
	lua_rawset(L, index);
}

// RAII helper for popping things from the lua stack
class stack_pop {
	lua_State *L;
	int count;

public:
	stack_pop(lua_State *L, int count): L(L), count(count) {}
	~stack_pop() { lua_pop(L, count); }
};

//============================================================================
// Errors
//============================================================================

enum ErrorVerbosity {
	Quiet,
	Verbose,
};

class Error {
protected:
	ErrorVerbosity verbosity = Verbose;
	int code = _INTERLUA_OK;
	char *message = nullptr;

public:
	Error() = default;
	explicit Error(ErrorVerbosity v): verbosity(v) {}
	virtual ~Error();

	void Reset();
	int Code() const { return code; }
	ErrorVerbosity Verbosity() const { return verbosity; }
	const char *What() const { return message ? message : ""; }
	explicit operator bool() const { return this->code != _INTERLUA_OK; }

	virtual void Set(int code, const char *format, ...);
};

class AbortError : public Error {
public:
	void Set(int code, const char *format, ...) override;
};

extern AbortError DefaultError;

// helper for using Error as a C object (we'll be using it in a potential
// longjmp context, hence we need to construct and deconstruct it manually)
struct ManualError {
	uint8_t storage[sizeof(Error)];
	Error *Init() {	return new (&storage) Error; }
	Error *Get() { return reinterpret_cast<Error*>(storage); }
	void LJCheckAndDestroy(lua_State *L);
	void Destroy() { Get()->~Error(); }
};

//============================================================================
// Error helpers
//============================================================================

// function similar to many luaL_ functions, with two differences:
// 1. They only check for an error without returning anything.
// 2. And instead of raising a lua error, they call err->Set(...).
void argerror(lua_State *L, int narg, const char *extra, Error *err);
void checkany(lua_State *L, int narg, Error *err);
void checkinteger(lua_State *L, int narg, Error *err);
void checknumber(lua_State *L, int narg, Error *err);
void checkstring(lua_State *L, int narg, Error *err);
void checklightuserdata(lua_State *L, int narg, Error *err);

//============================================================================
// Stack operations
//============================================================================

template <typename T>
struct _decay {
	using PURE_T = typename std::decay<T>::type;
	using type = typename std::conditional<
		std::is_class<PURE_T>::value,
		T,
		typename std::decay<T>::type
	>::type;
};

// Special Decay, does std::decay<T> on non-class types, leaves class types as
// is. InterLua applies it on every StackOps call site.
template <typename T>
using Decay = typename _decay<T>::type;

template <typename T>
struct StackOps;

// pointer cases are covered separately however
template <typename T>
struct StackOps<T*> {
	static inline int Push(lua_State *L, void *value) {
		lua_pushlightuserdata(L, value);
		return 1;
	}
	static inline void Check(lua_State *L, int index, Error *err = &DefaultError) {
		checklightuserdata(L, index, err);
	}
	static inline void *Get(lua_State *L, int index) {
		return lua_touserdata(L, index);
	}
	static inline void *LJGet(lua_State *L, int index) {
		// TODO: error check here
		return lua_touserdata(L, index);
	}
};

template <>
struct StackOps<std::nullptr_t> {
	static inline int Push(lua_State *L, std::nullptr_t) {
		lua_pushnil(L);
		return 1;
	}
};

template <>
struct StackOps<lua_State*> {
	static inline void Check(lua_State*, int, Error* = &DefaultError) {}
	static inline lua_State *Get(lua_State *L, int) { return L; }
	static inline lua_State *LJGet(lua_State *L, int) { return L; }
};

#define _stack_ops_integer(T)                                                       \
template <>                                                                         \
struct StackOps<T> {                                                                \
	static inline int Push(lua_State *L, T value) {                                 \
		lua_pushinteger(L, value);                                                  \
		return 1;                                                                   \
	}                                                                               \
	static inline void Check(lua_State *L, int index, Error *err = &DefaultError) {	\
		checkinteger(L, index, err);                                                \
	}                                                                               \
	static inline T Get(lua_State *L, int index) {                                  \
		return lua_tointeger(L, index);                                             \
	}                                                                               \
	static inline T LJGet(lua_State *L, int index) {                                \
		return luaL_checkinteger(L, index);                                         \
	}                                                                               \
};

_stack_ops_integer(signed char)
_stack_ops_integer(unsigned char)
_stack_ops_integer(short)
_stack_ops_integer(unsigned short)
_stack_ops_integer(int)
_stack_ops_integer(unsigned int)
_stack_ops_integer(long)
_stack_ops_integer(unsigned long)
_stack_ops_integer(long long)
_stack_ops_integer(unsigned long long)

#undef _stack_ops_integer


#define _stack_ops_float(T)                                                         \
template <>                                                                         \
struct StackOps<T> {                                                                \
	static inline int Push(lua_State *L, T value) {                                 \
		lua_pushnumber(L, value);                                                   \
		return 1;                                                                   \
	}                                                                               \
	static inline void Check(lua_State *L, int index, Error *err = &DefaultError) {	\
		checknumber(L, index, err);                                                 \
	}                                                                               \
	static inline T Get(lua_State *L, int index) {                                  \
		return lua_tonumber(L, index);                                              \
	}                                                                               \
	static inline T LJGet(lua_State *L, int index) {                                \
		return luaL_checknumber(L, index);                                          \
	}                                                                               \
};

_stack_ops_float(float)
_stack_ops_float(double)

#undef _stack_ops_float

template <>
struct StackOps<const char*> {
	static inline int Push(lua_State *L, const char *str) {
		if (str)
			lua_pushstring(L, str);
		else
			lua_pushnil(L);
		return 1;
	}
	static inline void Check(lua_State *L, int index, Error *err = &DefaultError) {
		if (!lua_isnil(L, index))
			checkstring(L, index, err);
	}
	static inline const char *Get(lua_State *L, int index) {
		return lua_isnil(L, index) ? nullptr : lua_tostring(L, index);
	}
	static inline const char *LJGet(lua_State *L, int index) {
		return lua_isnil(L, index) ? nullptr : luaL_checkstring(L, index);
	}
};

template <>
struct StackOps<char> {
	static inline int Push(lua_State *L, char value) {
		char str[2] = {value, 0};
		lua_pushstring(L, str);
		return 1;
	}
	static inline void Check(lua_State *L, int index, Error *err = &DefaultError) {
		checkstring(L, index, err);
	}
	static inline char Get(lua_State *L, int index) {
		return lua_tostring(L, index)[0];
	}
	static inline char LJGet(lua_State *L, int index) {
		return luaL_checkstring(L, index)[0];
	}
};

template <>
struct StackOps<bool> {
	static inline int Push(lua_State *L, bool value) {
		lua_pushboolean(L, value ? 1 : 0);
		return 1;
	}
	static inline void Check(lua_State *L, int narg, Error *err = &DefaultError) {
		checkany(L, narg, err);
	}
	static inline bool Get(lua_State *L, int index) {
		return lua_toboolean(L, index) ? true : false;
	}
	static inline bool LJGet(lua_State *L, int index) {
		luaL_checkany(L, index);
		return lua_toboolean(L, index) ? true : false;
	}
};

// simply ignore errors when they are passed as an argument
#define _stack_ops_ignore_push(T)			\
template <>                                 \
struct StackOps<T> {                        \
	static inline int Push(lua_State*, T) {	\
		return 0;                           \
	}                                       \
};

_stack_ops_ignore_push(Error*)
_stack_ops_ignore_push(AbortError*)

#undef _stack_ops_ignore_push

//============================================================================
// Recursive check helper
//============================================================================

template <int I>
static inline void recursive_check(lua_State*, Error*) {}

template <int I, typename T, typename ...Args>
static inline void recursive_check(lua_State *L, Error *err) {
	StackOps<Decay<T>>::Check(L, I, err);
	if (*err)
		return;
	recursive_check<I+1, Args...>(L, err);
}

template <int I, typename ...Args>
static inline void lj_recursive_check(lua_State *L) {
	ManualError merr;
	Error *err = merr.Init();
	recursive_check<I, Args...>(L, err);
	merr.LJCheckAndDestroy(L);
}

//============================================================================
// Function binding helpers
//============================================================================

template<int ...I> struct index_tuple_type {
        template<int N> using append = index_tuple_type<I..., N>;
};
template<int N> struct make_index_impl {
        using type = typename make_index_impl<N-1>::type::template append<N-1>;
};
template<> struct make_index_impl<0> { using type = index_tuple_type<>; };
template<int N> using index_tuple = typename make_index_impl<N>::type;

template <typename ...Args>
struct is_all_pod : std::true_type {};

template <typename T>
struct is_all_pod<T> :
	std::integral_constant<
		bool,
		std::is_pod<T>::value
	> {};

template <typename T, typename ...Args>
struct is_all_pod<T, Args...> :
	std::integral_constant<
		bool,
		std::is_pod<T>::value && is_all_pod<Args...>::value
	> {};

template <typename F, typename IT, bool POD>
struct func_traits;

// is_pod == false
template <typename R, typename ...Args, int ...I>
struct func_traits<R (Args...), index_tuple_type<I...>, false> {
	static R call(lua_State *L, R (*fp)(Args...)) {
		(void)L; // silence notused warning for cases with no arguments
		lj_recursive_check<1, Args...>(L);
		return (*fp)(StackOps<Decay<Args>>::Get(L, I+1)...);
	}
};

// is_pod == true
template <typename R, typename ...Args, int ...I>
struct func_traits<R (Args...), index_tuple_type<I...>, true> {
	static R call(lua_State *L, R (*fp)(Args...)) {
		(void)L; // silence notused warning for cases with no arguments
		return (*fp)(StackOps<Decay<Args>>::LJGet(L, I+1)...);
	}

};

template <typename F>
struct call {};

// ordinary function call wrapper
template <typename R, typename ...Args>
struct call<R (*)(Args...)> {
	static int cfunction(lua_State *L) {
		typedef R (*FP)(Args...);
		auto fp = *(FP*)lua_touserdata(L, lua_upvalueindex(1));
		return StackOps<Decay<R>>::Push(L,
			func_traits<
				R (Args...),
				index_tuple<sizeof...(Args)>,
				is_all_pod<Args...>::value
			>::call(L, fp)
		);
	}
};

// ordinary function call wrapper (no return value)
template <typename ...Args>
struct call<void (*)(Args...)> {
	static int cfunction(lua_State *L) {
		typedef void (*FP)(Args...);
		auto fp = *(FP*)lua_touserdata(L, lua_upvalueindex(1));
		func_traits<
			void (Args...),
			index_tuple<sizeof...(Args)>,
			is_all_pod<Args...>::value
		>::call(L, fp);
		return 0;
	}
};

//============================================================================
// Namespace
//============================================================================

class NSWrapper {
	lua_State *L = nullptr;

public:
	NSWrapper() = delete;
	NSWrapper(lua_State *L): L(L) {}

	NSWrapper Namespace(const char *name);
	inline NSWrapper End() { lua_pop(L, 1); return {L}; }

	NSWrapper &CFunction(const char *name, int (*fp)(lua_State*)) {
		lua_pushcfunction(L, fp);
		rawsetfield(L, -2, name);
		return *this;
	}

	template <typename FP>
	NSWrapper &Function(const char *name, FP fp) {
		*(FP*)lua_newuserdata(L, sizeof(fp)) = fp;
		lua_pushcclosure(L, call<FP>::cfunction, 1);
		rawsetfield(L, -2, name);
		return *this;
	}
};

static inline NSWrapper GlobalNamespace(lua_State *L) {
	_interlua_pushglobaltable(L);
	return {L};
}

static inline NSWrapper NewNamespace(lua_State *L) {
	lua_newtable(L);
	return {L};
}

//============================================================================
// Get last argument if it's an *Error
//============================================================================

// is_error_ptr<T>::value is true, if T is a pointer to Error or a pointer to a
// class derived from Error (or a reference to any of those).
template <typename T, typename NOREF_T = typename std::remove_reference<T>::type>
struct is_error_ptr :
	public std::integral_constant<bool,
		std::is_pointer<NOREF_T>::value &&
		std::is_base_of<
			Error,
			typename std::remove_pointer<NOREF_T>::type
		>::value
	> {};

template <typename T, bool OK = is_error_ptr<T>::value>
struct get_error_or_null;

template <typename T>
struct get_error_or_null<T, true> {
	static inline Error *get(T &&v) { return v; }
};

template <typename T>
struct get_error_or_null<T, false> {
	static inline Error *get(T&&) { return nullptr; }
};

static inline Error *get_last_if_error() {
	return nullptr;
}

template <typename T>
static inline Error *get_last_if_error(T &&last) {
	return get_error_or_null<T>::get(std::forward<T>(last));
}

template <typename T, typename ...Args>
static inline Error *get_last_if_error(T&&, Args &&...args) {
	return get_last_if_error(std::forward<Args>(args)...);
}

//============================================================================
// Recursive push helper
//============================================================================

static inline int recursive_push(lua_State*) {
	// no arguments
	return 0;
}

template <typename T>
static inline int recursive_push(lua_State *L, T &&arg) {
	return StackOps<Decay<T>>::Push(L, std::forward<T>(arg));
}

template <typename T, typename ...Args>
static inline int recursive_push(lua_State *L, T &&arg, Args &&...args) {
	const int n = StackOps<Decay<T>>::Push(L, std::forward<T>(arg));
	return n + recursive_push(L, std::forward<Args>(args)...);
}

//============================================================================
// Ref
//============================================================================

class Ref {
	lua_State *L = nullptr;
	int ref = LUA_NOREF;
	int tableref = LUA_NOREF;

public:
	Ref() = default;
	Ref(lua_State *L): L(L) {}
	Ref(lua_State *L, int ref): L(L), ref(ref) {}
	Ref(lua_State *L, int ref, int tableref):
		L(L), ref(ref), tableref(tableref) {}

	Ref(const Ref &r): L(r.L) {
		if (r.ref == LUA_NOREF) {
			return;
		}
		r.Push(L);
		ref = luaL_ref(L, LUA_REGISTRYINDEX);
	}

	Ref(Ref &&r): L(r.L), ref(r.ref), tableref(r.tableref) {
		r.L = nullptr;
		r.ref = LUA_NOREF;
		r.tableref = LUA_NOREF;
	}

	~Ref() {
		if (L) {
			luaL_unref(L, LUA_REGISTRYINDEX, ref);
			luaL_unref(L, LUA_REGISTRYINDEX, tableref);
		}
	}

#define _generic_op(op, n1, n2, luaop)                   \
	template <typename T>                                \
	bool op(T &&r) const {                               \
		stack_pop p(L, 2);                               \
		Push(L);                                         \
		StackOps<Decay<T>>::Push(L, std::forward<T>(r)); \
		return _interlua_compare(L, n1, n2, luaop) == 1; \
	}

	_generic_op(operator==, -2, -1, _INTERLUA_OPEQ)
	_generic_op(operator<, -2, -1, _INTERLUA_OPLT)
	_generic_op(operator<=, -2, -1, _INTERLUA_OPLE)
	_generic_op(operator>, -1, -2, _INTERLUA_OPLT)
	_generic_op(operator>=, -1, -2, _INTERLUA_OPLE)

#undef _generic_op

	template <typename T>
	bool operator!=(T &&r) const {
		return !operator==(std::forward<T>(r));
	}

	template <typename T>
	Ref &operator=(T &&r) {
		if (tableref != LUA_NOREF) {
			stack_pop p(L, 1);
			lua_rawgeti(L, LUA_REGISTRYINDEX, tableref);
			lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
			// TODO: Make sure Push returns 1
			StackOps<Decay<T>>::Push(L, std::forward<T>(r));
			lua_settable(L, -3);
		} else {
			luaL_unref(L, LUA_REGISTRYINDEX, ref);
			// TODO: Make sure Push returns 1
			StackOps<Decay<T>>::Push(L, std::forward<T>(r));
			ref = luaL_ref(L, LUA_REGISTRYINDEX);
		}
		return *this;
	}

	Ref &operator=(Ref &&r) {
		if (tableref != LUA_NOREF) {
			// if 'this' represents a table entry, we will actually copy 'r'
			stack_pop p(L, 1);
			lua_rawgeti(L, LUA_REGISTRYINDEX, tableref);
			lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
			r.Push(L);
			lua_rawset(L, -3);

			luaL_unref(L, LUA_REGISTRYINDEX, r.ref);
			luaL_unref(L, LUA_REGISTRYINDEX, r.tableref);
		} else {
			// the usual move operation
			luaL_unref(L, LUA_REGISTRYINDEX, ref);
			luaL_unref(L, LUA_REGISTRYINDEX, tableref);
			L = r.L;
			ref = r.ref;
			tableref = r.tableref;
		}
		r.L = nullptr;
		r.ref = LUA_NOREF;
		r.tableref = LUA_NOREF;
		return *this;
	}

	Ref &operator=(const Ref &r) {
		if (tableref != LUA_NOREF) {
			stack_pop p(L, 1);
			lua_rawgeti(L, LUA_REGISTRYINDEX, tableref);
			lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
			r.Push(L);
			lua_rawset(L, -3);
		} else {
			luaL_unref(L, LUA_REGISTRYINDEX, ref);
			r.Push(L);
			ref = luaL_ref(L, LUA_REGISTRYINDEX);
		}
		return *this;
	}

	template <typename ...Args>
	Ref operator()(Args &&...args) const {
		Error *err = get_last_if_error(std::forward<Args>(args)...);
		if (!err)
			err = &DefaultError;

		if (IsNil()) {
			err->Set(LUA_ERRRUN, "InterLua::Ref object is nil, cannot call a nil");
			return {L};
		}

		Push(L);
		const int nargs = recursive_push(L, std::forward<Args>(args)...);
		const int code = lua_pcall(L, nargs, 1, 0);
		if (code != _INTERLUA_OK) {
			err->Set(code, lua_tostring(L, -1));
			lua_pop(L, 1); // pop the error message from the stack
			return {L};
		}
		return {L, luaL_ref(L, LUA_REGISTRYINDEX)};
	}

	template <typename T>
	Ref operator[](T &&key) const {
		// TODO: make sure Push returns 1
		StackOps<Decay<T>>::Push(L, std::forward<T>(key));
		int keyref = luaL_ref(L, LUA_REGISTRYINDEX);
		Push(L);
		int tableref = luaL_ref(L, LUA_REGISTRYINDEX);
		return {L, keyref, tableref};
	}

	void Push(lua_State *L) const {
		if (tableref != LUA_NOREF) {
			lua_rawgeti(L, LUA_REGISTRYINDEX, tableref);
			lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
			lua_gettable(L, -2);
			lua_remove(L, -2);
		} else {
			lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
		}
	}

	int Type() const {
		if (ref != LUA_NOREF) {
			stack_pop p(L, 1);
			Push(L);
			return lua_type(L, -1);
		} else {
			return LUA_TNIL;
		}
	}

	inline bool IsNil() const { return Type() == LUA_TNIL; }
	inline bool IsNumber() const { return Type() == LUA_TNUMBER; }
	inline bool IsString() const { return Type() == LUA_TSTRING; }
	inline bool IsTable() const { return Type() == LUA_TTABLE; }
	inline bool IsFunction() const { return Type() == LUA_TFUNCTION; }
	inline bool IsUserdata() const { return Type() == LUA_TUSERDATA; }
	inline bool IsThread() const { return Type() == LUA_TTHREAD; }
	inline bool IsLightUserdata() const { return Type() == LUA_TLIGHTUSERDATA; }

	template <typename T>
	void Append(T &&v) const {
		Push(L);
		// TODO: Make sure Push returns 1
		StackOps<Decay<T>>::Push(L, std::forward<T>(v));
		luaL_ref(L, -2);
		lua_pop(L, 1);
	}

	int Length() const {
		stack_pop p(L, 1);
		Push(L);
		return _interlua_rawlen(L, -1);
	}

	template <typename T>
	inline T As() const {
		stack_pop p(L, 1);
		Push(L);
		StackOps<Decay<T>>::Check(L, -1, &DefaultError);
		return StackOps<Decay<T>>::Get(L, -1);
	}

	template <typename T>
	inline operator T() const {
		return As<T>();
	}

	void Unref()
	{
		if (L) {
			luaL_unref(L, LUA_REGISTRYINDEX, ref);
			luaL_unref(L, LUA_REGISTRYINDEX, tableref);
			L = nullptr;
			ref = LUA_NOREF;
			tableref = LUA_NOREF;
		}
	}
};

static inline Ref FromStack(lua_State *L, int index) {
	lua_pushvalue(L, index);
	return {L, luaL_ref(L, LUA_REGISTRYINDEX)};
}

template <typename T>
static inline Ref New(lua_State *L, T &&v) {
	// TODO: Make sure Push returns 1
	StackOps<Decay<T>>::Push(L, std::forward<T>(v));
	return {L, luaL_ref(L, LUA_REGISTRYINDEX)};
}

static inline Ref NewTable(lua_State *L) {
	lua_newtable(L);
	return {L, luaL_ref(L, LUA_REGISTRYINDEX)};
}

static inline Ref Global(lua_State *L, const char *name) {
	lua_getglobal(L, name);
	return {L, luaL_ref(L, LUA_REGISTRYINDEX)};
}

#define _stack_ops_ref(T)                                                          \
template <>                                                                        \
struct StackOps<T> {                                                               \
	static inline int Push(lua_State *L, const Ref &v) {                           \
		v.Push(L);                                                                 \
		return 1;                                                                  \
	}                                                                              \
	static inline void Check(lua_State *L, int narg, Error *err = &DefaultError) { \
		checkany(L, narg, err);                                                    \
	}                                                                              \
	static inline Ref Get(lua_State *L, int index) {                               \
		return FromStack(L, index);                                                \
	}                                                                              \
};

_stack_ops_ref(Ref)
_stack_ops_ref(Ref&)
_stack_ops_ref(Ref&&)
_stack_ops_ref(const Ref&)

#undef _stack_ops_ref

} // namespace InterLua
