#include "stf.h"
#include "Core/Defer.h"
#include "Core/Vector.h"

STF_SUITE_NAME("Core.Vector")

#define CHECK_VECTOR(v, len, cap, datacmp) \
do {                                       \
	STF_ASSERT(v.length() == len);         \
	STF_ASSERT(v.capacity() == cap);       \
	STF_ASSERT(v.data() datacmp);          \
} while (0)

#define CHECK_VECTOR_CONTENTS(v, ...) \
	STF_ASSERT(v.sub() == Slice<const int>({__VA_ARGS__}))

STF_TEST("Vector::Vector(int)") {
	Vector<int> v1(0);
	CHECK_VECTOR(v1, 0, 0, == nullptr);

	Vector<int> v2(5);
	CHECK_VECTOR(v2, 5, 5, != nullptr);

	Vector<int> v3;
	CHECK_VECTOR(v3, 0, 0, == nullptr);
}

STF_TEST("Vector::Vector(int, const T&)") {
	Vector<int> v1(0, 1);
	CHECK_VECTOR(v1, 0, 0, == nullptr);

	Vector<int> v2(3, 333);
	CHECK_VECTOR(v2, 3, 3, != nullptr);
	CHECK_VECTOR_CONTENTS(v2, 333, 333, 333);
}

STF_TEST("Vector::Vector(Slice<const T>)") {
	Vector<int> v = {1, 2, 3, 4, 5};
	CHECK_VECTOR_CONTENTS(v, 1, 2, 3, 4, 5);

	Vector<int> v2;
	v2 = {-1, -2, -3, -4, -5};
	CHECK_VECTOR_CONTENTS(v2, -1, -2, -3, -4, -5);
}

STF_TEST("Vector::operator=(Slice<const T>)") {
	Vector<int> v = {1, 2, 3};
	v = {4, 5, 6, 7, 8};
	CHECK_VECTOR_CONTENTS(v, 4, 5, 6, 7, 8);
	v = {1, 2};
	CHECK_VECTOR_CONTENTS(v, 1, 2);
}

STF_TEST("Vector::operator=(Vector<T>&&)") {
	Vector<int> v = {1, 2, 3};
	v = Vector<int>();
	CHECK_VECTOR(v, 0, 0, == nullptr);
}

STF_TEST("Vector::byte_length()") {
	Vector<int> vint(10);
	Vector<char> vchar(5);

	STF_ASSERT(vint.byte_length() == 10 * sizeof(int));
	STF_ASSERT(vchar.byte_length() == 5 * sizeof(char));
}

STF_TEST("Vector::reserve(int)") {
	Vector<int> v(5, 777);

	CHECK_VECTOR(v, 5, 5, != nullptr);
	v.reserve(3);
	v.reserve(-100);
	CHECK_VECTOR(v, 5, 5, != nullptr);

	v.reserve(10);
	CHECK_VECTOR(v, 5, 10, != nullptr);
	CHECK_VECTOR_CONTENTS(v, 777, 777, 777, 777, 777);
}

STF_TEST("Vector::resize(int)") {
	Vector<int> v = {333, 334, 335};
	CHECK_VECTOR(v, 3, 3, != nullptr);
	v.resize(5);
	CHECK_VECTOR(v, 5, 5, != nullptr);
	STF_ASSERT(v[0] == 333 && v[2] == 335);

	v.resize(1);
	CHECK_VECTOR(v, 1, 5, != nullptr);
	STF_ASSERT(v[0] == 333);

	v.resize(0);
	CHECK_VECTOR(v, 0, 5, != nullptr);
}

STF_TEST("Vector::resize(int, const T&)") {
	Vector<int> v = {-3, -2, -1};
	CHECK_VECTOR(v, 3, 3, != nullptr);
	v.resize(7, -7);
	CHECK_VECTOR(v, 7, 7, != nullptr);
	CHECK_VECTOR_CONTENTS(v, -3, -2, -1, -7, -7, -7, -7);

	v.resize(4, -7);
	CHECK_VECTOR(v, 4, 7, != nullptr);
	CHECK_VECTOR_CONTENTS(v, -3, -2, -1, -7);

	v.resize(0, -7);
	CHECK_VECTOR(v, 0, 7, != nullptr);
}

STF_TEST("Vector::remove(int)") {
	Vector<int> v = {1, 2, 3, 4, 5, 6, 7};
	v.remove(0);
	CHECK_VECTOR_CONTENTS(v, 2, 3, 4, 5, 6, 7);
	v.remove(5);
	CHECK_VECTOR_CONTENTS(v, 2, 3, 4, 5, 6);
	v.remove(2);
	CHECK_VECTOR_CONTENTS(v, 2, 3, 5, 6);
	for (int i = 0; i < 4; i++)
		v.remove(0);
	CHECK_VECTOR(v, 0, 7, != nullptr);
}

STF_TEST("Vector::remove(int, int)") {
	Vector<int> v = {1, 2, 3, 4, 5, 6};
	v.remove(4, 6);
	CHECK_VECTOR_CONTENTS(v, 1, 2, 3, 4);
	v.remove(1, 3);
	CHECK_VECTOR_CONTENTS(v, 1, 4);
	v.remove(0, 2);
	CHECK_VECTOR_CONTENTS(v);
}

STF_TEST("Vector::insert(int, const T&)") {
	Vector<int> v;
	v.insert(0, 4);
	CHECK_VECTOR_CONTENTS(v, 4);
	v.insert(0, 2);
	CHECK_VECTOR_CONTENTS(v, 2, 4);
	v.insert(1, 3);
	CHECK_VECTOR_CONTENTS(v, 2, 3, 4);
	v.insert(v.length(), 5);
	CHECK_VECTOR_CONTENTS(v, 2, 3, 4, 5);
}

STF_TEST("Vector::insert(int, Slice<const T>)") {
	Vector<int> a = {10, -10};
	Vector<int> b = {1, 2, 3};
	b.insert(0, a);
	CHECK_VECTOR_CONTENTS(b, 10, -10, 1, 2, 3);
	b.insert(4, a);
	CHECK_VECTOR_CONTENTS(b, 10, -10, 1, 2, 10, -10, 3);
	b.insert(2, a);
	CHECK_VECTOR_CONTENTS(b, 10, -10, 10, -10, 1, 2, 10, -10, 3);
	b.insert(b.length(), a);
	CHECK_VECTOR_CONTENTS(b, 10, -10, 10, -10, 1, 2, 10, -10, 3, 10, -10);

	// inserting self
	Vector<int> c = {1, 2, 3, 4, 5};
	c.insert(0, c.sub(3));
	CHECK_VECTOR_CONTENTS(c, 4, 5, 1, 2, 3, 4, 5);
	c.insert(c.length(), c.sub(2, 4));
	CHECK_VECTOR_CONTENTS(c, 4, 5, 1, 2, 3, 4, 5, 1, 2);

	// cutting insertion
	c.insert(2, c.sub(0, 4));
	CHECK_VECTOR_CONTENTS(c, 4, 5, 4, 5, 1, 2, 1, 2, 3, 4, 5, 1, 2);

	// empty insertion
	Vector<int> d = {7, 8, 9, 10};
	d.insert(0, Slice<int>(nullptr, 0));
}

STF_TEST("Vector::append()") {
	Vector<int> v;
	int *n;
	n = v.append();
	*n = 1;
	n = v.append();
	*n = 2;
	n = v.append();
	*n = 3;
	CHECK_VECTOR_CONTENTS(v, 1, 2, 3);
}

STF_TEST("Vector::append(const T&)") {
	Vector<int> v;
	v.append(1);
	v.append(2);
	v.append(3);
	CHECK_VECTOR_CONTENTS(v, 1, 2, 3);
}

STF_TEST("Vector::append(Slice<const T>)") {
	Vector<int> v;
	v.append({1, 2, 3});
	v.append({3, 4, 5});
	CHECK_VECTOR_CONTENTS(v, 1, 2, 3, 3, 4, 5);
}
