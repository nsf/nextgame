#pragma once

#include "Core/Utils.h"
#include "Core/Slice.h"
#include <cstdarg>

struct String {
	char *m_data;
	int m_len = 0;
	int m_cap = 0;

	int _new_size(int requested) const;
	void _ensure_capacity(int n);
	void _self_insert(int idx, Slice<const char> s);
	void _nullify();

	String();
	explicit String(int n);
	String(Slice<const char> s);
	String(const char *str);
	String(const String &r);
	String(String &&r);

	String &operator=(Slice<const char> r);
	String &operator=(const char *str);
	String &operator=(const String &r);
	String &operator=(String &&r);

	~String();

	int length() const { return m_len; }
	int byte_length() const { return m_len * sizeof(char); }
	int capacity() const { return m_cap; }
	char *data() { return m_data; }
	const char *data() const { return m_data; }
	const char *c_str() const { return m_data; }

	void clear();
	void reserve(int n);
	void shrink();
	void resize(int n);
	void resize(int n, char elem);
	void insert(int idx, char elem);
	void insert(int idx, Slice<const char> s);
	void append(char elem);
	void append(Slice<const char> s);
	void remove(int idx);
	void remove(int begin, int end);

	char *release();

	char &operator[](int idx)
	{
		NG_IDX_BOUNDS_CHECK(idx, m_len);
		return m_data[idx];
	}
	const char &operator[](int idx) const
	{
		NG_IDX_BOUNDS_CHECK(idx, m_len);
		return m_data[idx];
	}
	Slice<char> sub()
	{
		return {m_data, m_len};
	}
	Slice<char> sub(int begin)
	{
		NG_SLICE_BOUNDS_CHECK(begin, m_len);
		return {m_data + begin, m_len - begin};
	}
	Slice<char> sub(int begin, int end)
	{
		NG_ASSERT(begin <= end);
		NG_SLICE_BOUNDS_CHECK(begin, m_len);
		NG_SLICE_BOUNDS_CHECK(end, m_len);
		return {m_data + begin, end - begin};
	}
	Slice<const char> sub() const
	{
		return {m_data, m_len};
	}
	Slice<const char> sub(int begin) const
	{
		NG_SLICE_BOUNDS_CHECK(begin, m_len);
		return {m_data + begin, m_len - begin};
	}
	Slice<const char> sub(int begin, int end) const
	{
		NG_ASSERT(begin <= end);
		NG_SLICE_BOUNDS_CHECK(begin, m_len);
		NG_SLICE_BOUNDS_CHECK(end, m_len);
		return {m_data + begin, end - begin};
	}

	operator Slice<char>() { return {m_data, m_len}; }
	operator Slice<const char>() const { return {m_data, m_len}; }

	static String format(const char *fmt, ...);
	static String vformat(const char *fmt, va_list va);
};

// String vs. const char*
String operator+(const String &lhs, const char *rhs);
String operator+(const char *lhs, const String &rhs);
String operator+(String &&lhs, const char *rhs);
String operator+(const char *lhs, String &&rhs);

// String vs. String
String operator+(const String &lhs, const String &rhs);
String operator+(String &&lhs, String &&rhs);
String operator+(String &&lhs, const String &rhs);
String operator+(const String &lhs, String &&rhs);

// String vs. slice<const char>
String operator+(const String &lhs, Slice<const char> rhs);
String operator+(Slice<const char> lhs, const String &rhs);
String operator+(String &&lhs, Slice<const char> rhs);
String operator+(Slice<const char> lhs, String &&rhs);

int compute_hash(const String &s);
