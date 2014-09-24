#include "stf.h"
#include "Core/Defer.h"
#include "Core/StaticVector.h"

STF_SUITE_NAME("Core.StaticVector")

/*
 _            _
| |          | |
| |_ ___   __| | ___
| __/ _ \ / _` |/ _ \
| || (_) | (_| | (_) |
 \__\___/ \__,_|\___/

#define CHECK_VECTOR(v, len, cap, data) \
do {                                    \
	STF_ASSERT(v.Length() == len);      \
	STF_ASSERT(v.Capacity() == cap);    \
	STF_ASSERT(v.Data() data);          \
} while (0)

#define CHECK_VECTOR_CONTENTS(v, ...) \
	STF_ASSERT(v.Sub() == Slice<const int>({__VA_ARGS__}))

STF_TEST("StaticVector::Init()") {
	StaticVector<int, 4> v;
	v.Init();
	CHECK_VECTOR(v, 0, 4, == v.m_static);
	v.Free();
}

STF_TEST("StaticVector::Append(const T&)") {
	StaticVector<int, 4> v = StaticVector<int, 4>().Init();
	DEFER { v.Free(); };
	CHECK_VECTOR(v, 0, 4, == v.m_static);
	v.Append(1);
	CHECK_VECTOR(v, 1, 4, == v.m_static);
	CHECK_VECTOR_CONTENTS(v, 1);
	v.Append(2);
	v.Append(3);
	v.Append(4);
	CHECK_VECTOR(v, 4, 4, == v.m_static);
	CHECK_VECTOR_CONTENTS(v, 1, 2, 3, 4);
	v.Append(5);
	STF_ASSERT(v.Length() == 5);
	STF_ASSERT(v.Capacity() >= 5);
	STF_ASSERT(v.Data() == v.m_dynamic.m_data);
	CHECK_VECTOR_CONTENTS(v, 1, 2, 3, 4, 5);
	v.Append(6);
	v.Append(7);
	v.Append(8);
	STF_ASSERT(v.Length() == 8);
	STF_ASSERT(v.Capacity() >= 8);
	STF_ASSERT(v.Data() == v.m_dynamic.m_data);
	CHECK_VECTOR_CONTENTS(v, 1, 2, 3, 4, 5, 6, 7, 8);
}

STF_TEST("StaticVector::Append(const T&)") {
	StaticVector<int, 4> v = StaticVector<int, 4>().Init();
	DEFER { v.Free(); };
	v.Append(1);
	v.Append(2);
	CHECK_VECTOR_CONTENTS(v, 1, 2);
	v.QuickRemove(1);
	CHECK_VECTOR_CONTENTS(v, 1);
	v.Append(2);
	v.Append(3);
	v.Append(4);
	v.Append(5);
	v.Append(6);
	CHECK_VECTOR_CONTENTS(v, 1, 2, 3, 4, 5, 6);
	v.QuickRemove(0);
	v.QuickRemove(0);
	STF_ASSERT(v.Data() == v.m_static);
	CHECK_VECTOR_CONTENTS(v, 5, 2, 3, 4);
}

*/
