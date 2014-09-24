#include "Serialize/MsgPack.h"
#include "Serialize/Cmp/cmp.h"

void MsgPack<TT_SERIALIZE, int>::execute(const void *addr, cmp_ctx_t *ctx, Error *err)
{
	if (!cmp_write_sint(ctx, *(int*)addr))
		err->set("MsgPack: failed to serialize an int: %s", cmp_strerror(ctx));
}

void MsgPack<TT_DESERIALIZE, int>::execute(void *addr, cmp_ctx_t *ctx, Error *err)
{
	int64_t i;
	if (!cmp_read_sinteger(ctx, &i))
		err->set("MsgPack: failed to deserialize an int: %s", cmp_strerror(ctx));
	*(int*)addr = i;
}

void MsgPack<TT_SERIALIZE, float>::execute(const void *addr, cmp_ctx_t *ctx, Error *err)
{
	if (!cmp_write_float(ctx, *(float*)addr))
		err->set("MsgPack: failed to serialize a float: %s", cmp_strerror(ctx));
}

void MsgPack<TT_DESERIALIZE, float>::execute(void *addr, cmp_ctx_t *ctx, Error *err)
{
	if (!cmp_read_float(ctx, (float*)addr))
		err->set("MsgPack: failed to deserialize a float: %s", cmp_strerror(ctx));
}

void MsgPack<TT_SERIALIZE, double>::execute(const void *addr, cmp_ctx_t *ctx, Error *err)
{
	if (!cmp_write_double(ctx, *(double*)addr))
		err->set("MsgPack: failed to serialize a double: %s", cmp_strerror(ctx));
}

void MsgPack<TT_DESERIALIZE, double>::execute(void *addr, cmp_ctx_t *ctx, Error *err)
{
	if (!cmp_read_double(ctx, (double*)addr))
		err->set("MsgPack: failed to deserialize a double: %s", cmp_strerror(ctx));
}
