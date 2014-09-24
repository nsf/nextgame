#pragma once

#include "Core/Error.h"
#include <type_traits>
#include <cstdio>

// Essentially all serialization is built around the following template (both
// C++ template and textually a template):
//
// template <TranslationType TT, typename T> struct Type { static void
// execute((const) void *addr, Context *context, Error *err); };
//
// The 'addr' argument is const when TT == TT_SERIALIZE and is not otherwise.
//
// When TT == TT_SERIALIZE, the 'execute' function shall serialize the value at
// 'addr' into the 'context'.  When TT == TT_DESERIALIZE, the 'execute' function
// shall deserialize the value at 'addr' from the 'context'.
//
// That way you can using a little bit of template magic, specify the function
// once and use it for both serialization and deserialization.

enum TranslationType {
	TT_SERIALIZE,
	TT_DESERIALIZE,
};

// Used a lot to provide uniform definitions for const and non-const versions
// of 'execute'.
template <TranslationType TT, typename T>
using sd_pointer_type =
	typename std::conditional<TT == TT_SERIALIZE, const T*, T*>::type;

template <
	template <TranslationType, typename> class SD,
	typename T,
	typename Context
>
void serialize(const T &v, Context *ctx, Error *err = &DefaultError)
{
	SD<TT_SERIALIZE, T>::execute(&v, ctx, err);
}

template <
	template <TranslationType, typename> class SD,
	typename T,
	typename Context
>
void deserialize(T *v, Context *ctx, Error *err = &DefaultError)
{
	SD<TT_DESERIALIZE, T>::execute(v, ctx, err);
}

#define SD_FIELD(field) \
	MethodType<TT, decltype(v->field)>::execute(&v->field, ctx, err)

#define SD_DEFINE_METHOD_FOR_TYPE(method, type, contents)                         \
template <TranslationType TT>                                                     \
struct method<TT, type> {                                                         \
	using VoidPtr = sd_pointer_type<TT, void>;                                    \
	using ValuePtr = sd_pointer_type<TT, type>;                                   \
	template <TranslationType _TT, typename _T>                                   \
	using MethodType = method<_TT, _T>;                                           \
	static void execute(VoidPtr addr, cmp_ctx_t *ctx, Error *err = &DefaultError) \
	{                                                                             \
		ValuePtr v = static_cast<ValuePtr>(addr);                                 \
		contents;                                                                 \
	}                                                                             \
}
