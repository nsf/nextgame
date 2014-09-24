#include "stf.h"
#include "Core/Vector.h"
#include "Serialize/Cmp/cmp.h"
#include "Serialize/MsgPack.h"
#include "Serialize/MsgPack/Vec.h"

STF_SUITE_NAME("Serialize.MsgPack")

static size_t write_to_vector(cmp_ctx_t *ctx, const void *data, size_t n)
{
	Vector<uint8_t> *v = (Vector<uint8_t>*)ctx->buf;
	v->append(Slice<const uint8_t>((const uint8_t*)data, n));
	return n;
}

bool read_from_vector(cmp_ctx_t *ctx, void *data, size_t n)
{
	Vector<uint8_t> *v = (Vector<uint8_t>*)ctx->buf;
	if (v->length() < (int)n)
		return false;
	memcpy(data, v->data(), n);
	v->remove(0, n);
	return true;
}

STF_TEST("random") {
	Vector<uint8_t> buf;
	cmp_ctx_t ctx;
	cmp_init(&ctx, &buf, read_from_vector, write_to_vector);

	int a = 1000;
	serialize<MsgPack>(a, &ctx);
	int b;
	deserialize<MsgPack>(&b, &ctx);
	STF_ASSERT(a == b && b == 1000);

	float pi_a = 3.1415f;
	float pi_b;
	serialize<MsgPack>(pi_a, &ctx);
	deserialize<MsgPack>(&pi_b, &ctx);
	STF_ASSERT(pi_a == pi_b && pi_b == 3.1415f);

	Vec3 va, vb;
	va = Vec3(1.0f, -2.0f, 3.0f);
	serialize<MsgPack>(va, &ctx);
	STF_ASSERT(buf.length() != 0);
	deserialize<MsgPack>(&vb, &ctx);
	STF_ASSERT(va == vb && vb == Vec3(1.0f, -2.0f, 3.0f));

	Vec3i via, vib;
	via = Vec3i(10, -20, 30);
	serialize<MsgPack>(via, &ctx);
	deserialize<MsgPack>(&vib, &ctx);
	STF_ASSERT(via == vib && vib == Vec3i(10, -20, 30));


}
