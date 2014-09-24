#include "stf.h"
#include "Core/Defer.h"
#include "Core/String.h"

STF_SUITE_NAME("Core.String")

#define CHECK_STRING(s, len, cap, datacmp) \
do {                                       \
	STF_ASSERT(s.length() len);            \
	STF_ASSERT(s.capacity() cap);          \
	STF_ASSERT(s.data() datacmp);          \
} while (0)

STF_TEST("String::String(int)") {
	String s1(0);
	CHECK_STRING(s1, == 0, == 0, != nullptr);

	String s2(5);
	CHECK_STRING(s2, == 5, == 5, != nullptr);
	s2.clear();
	CHECK_STRING(s2, == 0, != 0, != nullptr);

	String s3;
	CHECK_STRING(s3, == 0, == 0, != nullptr);
}

STF_TEST("String::String(const char*)") {
	String s1 = "hello, world";
	CHECK_STRING(s1, == 12, >= 12, != nullptr);
	STF_ASSERT(s1 == "hello, world");

	String s2 = u8"123";
	CHECK_STRING(s2, == 3, >= 3, != nullptr);
	STF_ASSERT(s2 == "123");

	String s3;
	s3 = "hello";
	CHECK_STRING(s3, == 5, >= 5, != nullptr);
	STF_ASSERT(s3 == "hello");
}

STF_TEST("String::operator=(Slice<const char>)") {
	String a = "1234";
	a = a.sub(0, 2);
	STF_ASSERT(a == "12");

	String b = "7890";
	b = b.sub(1, 3);
	STF_ASSERT(b == "89");

	String c = "123456";
	c = "123";
	STF_ASSERT(c == "123");
	c = "";
	STF_ASSERT(c == "");
	STF_ASSERT(c.capacity() >= 6);

	c = c;
	STF_ASSERT(c == "");
}

STF_TEST("String::clear()") {
	String a = "123";
	a.clear();
	STF_ASSERT(a == "");
	STF_ASSERT(a.capacity() == 3);
}

STF_TEST("String::reserve(int)") {
	String a;
	a.reserve(80);
	STF_ASSERT(a == "");
	STF_ASSERT(a.capacity() >= 80);

	String b = "hello";
	b.reserve(90);
	STF_ASSERT(b == "hello");
	STF_ASSERT(b.capacity() >= 90);

	String c;
	c.reserve(0);
	STF_ASSERT(c == "");
	STF_ASSERT(c.capacity() == 0);
	STF_ASSERT(c.data() != nullptr);
	c.reserve(1);
	STF_ASSERT(c == "");
	STF_ASSERT(c.capacity() >= 1);
	STF_ASSERT(c.data() != nullptr);
}

STF_TEST("String::shrink()") {
	String a = "000000000000000000000000";
	a = "123";
	a.shrink();
	STF_ASSERT(a == "123");
	STF_ASSERT(a.capacity() == 3);

	String b;
	b.shrink();
	STF_ASSERT(b == "");
	STF_ASSERT(b.capacity() == 0);
	STF_ASSERT(b.data() != nullptr);
}

STF_TEST("String::resize(int, T)") {
	String a = "123";
	a.resize(20);
	STF_ASSERT(a.sub(0, 3) == "123");
	STF_ASSERT(a.length() == 20);

	a = "456";
	a.resize(6, '-');
	STF_ASSERT(a == "456---");

	a.resize(0);
	STF_ASSERT(a == "");

	String b;
	b.resize(0);
	STF_ASSERT(b == "");
	STF_ASSERT(b.capacity() == 0);
	STF_ASSERT(b.data() != nullptr);
}

STF_TEST("String::insert(int, Slice<const char>)") {
	String a = "123";
	a.insert(0, 'a');
	STF_ASSERT(a == "a123");
	a.insert(3, 'b');
	STF_ASSERT(a == "a12b3");
	a.insert(1, 'c');
	STF_ASSERT(a == "ac12b3");
	a.insert(a.length(), 'x');
	STF_ASSERT(a == "ac12b3x");

	String b = "3";
	b.insert(0, "12");
	STF_ASSERT(b == "123");
	b.insert(2, a.sub());
	STF_ASSERT(b == "12ac12b3x3");
	b.insert(2, "123");
	STF_ASSERT(b == "12123ac12b3x3");

	String c = "aabbcc";
	c.insert(0, c.sub(0, 2));
	STF_ASSERT(c == "aaaabbcc");
	c.insert(c.length(), c.sub(0, 2));
	STF_ASSERT(c == "aaaabbccaa");
	c.insert(4, c.sub(2, 6));
	STF_ASSERT(c == "aaaaaabbbbccaa");
}

STF_TEST("String::append(Slice<const char>)") {
	// append
	String a;
	a.append('h');
	a.append('e');
	a.append('l');
	a.append('l');
	a.append('o');
	STF_ASSERT(a == "hello");

	String b;
	b.append(a.sub());
	b.append(" ");
	b.append(a.sub());
	STF_ASSERT(b == "hello hello");

	b.append(b);
	STF_ASSERT(b == "hello hellohello hello");
	b.append(b.sub(b.length()-1));
	STF_ASSERT(b == "hello hellohello helloo");
}

STF_TEST("String::remove(int, int)") {
	// remove
	String a = "12345";
	a.remove(0);
	STF_ASSERT(a == "2345");
	a.remove(3);
	STF_ASSERT(a == "234");
	a.remove(1);
	STF_ASSERT(a == "24");

	String b = "123456789";
	b.remove(0, 2);
	STF_ASSERT(b == "3456789");
	b.remove(2, 4);
	STF_ASSERT(b == "34789");
	b.remove(3, 5);
	STF_ASSERT(b == "347");
}

STF_TEST("String::operator[](int idx)") {
	String a = "abc";
	STF_ASSERT(a[0] == 'a');
	STF_ASSERT(a[2] == 'c');
}

STF_TEST("String::format") {
	String s = String::format("%d + %d == %d", 1, 2, 3);
	STF_ASSERT(s == "1 + 2 == 3");
}
