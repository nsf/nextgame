#pragma once

#include "Serialize/Serialize.h"

typedef struct cmp_ctx_s cmp_ctx_t;

template <TranslationType TT, typename T>
struct MsgPack;

#define _MSG_PACK_FOR_TYPE(T)                                                          \
template <>                                                                            \
struct MsgPack<TT_SERIALIZE, T> {                                                      \
	static void execute(const void *addr, cmp_ctx_t *ctx, Error *err = &DefaultError); \
};                                                                                     \
template <>                                                                            \
struct MsgPack<TT_DESERIALIZE, T> {                                                    \
	static void execute(void *addr, cmp_ctx_t *ctx, Error *err = &DefaultError);       \
};

_MSG_PACK_FOR_TYPE(int)
_MSG_PACK_FOR_TYPE(float)
_MSG_PACK_FOR_TYPE(double)

#undef _MSG_PACK_FOR_TYPE
