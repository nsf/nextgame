#include "Core/String.h"
#include "Core/Memory.h"

static char EmptyString = 0;

int String::_new_size(int requested) const
{
	int newcap = m_cap * 2;
	return newcap < requested ? requested : newcap;
}

void String::_ensure_capacity(int n)
{
	if (m_len + n > m_cap)
		reserve(_new_size(m_len + n));
}

void String::_self_insert(int idx, Slice<const char> s)
{
	int sidx = s.data - m_data;
	_ensure_capacity(s.length);

	// restore the slice after possible realloc
	s = Slice<const char>(m_data + sidx, s.length);

	if (idx == m_len) {
		std::memcpy(m_data + idx, s.data, s.length);
		m_len += s.length;
		m_data[m_len] = 0;
		return;
	}

	// move tail further towards the end so that there is a free space for
	// data insertion
	std::memmove(m_data + idx + s.length, m_data + idx, m_len - idx);
	m_len += s.length;

	// adjust slice so that it points to the right data and if we're
	// splitting the slice, insert first half of it right away
	if (idx <= sidx) {
		s = Slice<const char>(s.data + s.length, s.length);
	} else {
		const int lhslen = idx - sidx;
		std::memmove(m_data + idx, m_data + sidx, lhslen);
		idx += lhslen;
		s = Slice<const char>(s.data + s.length + lhslen, s.length - lhslen);
	}
	std::memmove(m_data + idx, s.data, s.length);
	m_data[m_len] = 0;
}

// Puts a string into an "empty state".
void String::_nullify()
{
	m_data = &EmptyString;
	m_len = 0;
	m_cap = 0;
}

String::String(): m_data(&EmptyString)
{
}

String::String(int n): m_data(&EmptyString), m_len(n), m_cap(n)
{
	if (m_len > 0) {
		m_data = new (OrDie) char[m_len + 1];
		m_data[0] = 0;
	}
}

String::String(Slice<const char> s):
	m_data(&EmptyString), m_len(s.length), m_cap(s.length)
{
	if (m_len == 0)
		return;
	m_data = new (OrDie) char[m_cap + 1];
	std::memcpy(m_data, s.data, s.length);
	m_data[m_len] = 0;
}

String::String(const char *str): String(Slice<const char>(str))
{
}

String::String(const String &r): String(r.sub())
{
}

String::String(String &&r): m_data(r.m_data), m_len(r.m_len), m_cap(r.m_cap)
{
	r._nullify();
}

String &String::operator=(Slice<const char> r)
{
	if (m_data == r.data && m_len == r.length)
		return *this;

	if (m_cap < r.length) {
		// 'r' cannot point to ourselves
		if (m_data != &EmptyString)
			delete [] m_data;
		m_cap = m_len = r.length;
		m_data = new (OrDie) char[m_cap + 1];
		std::memcpy(m_data, r.data, m_len);
		m_data[m_len] = 0;
	} else {
		// 'r' may point to ourselves, let's use memmove
		m_len = r.length;
		std::memmove(m_data, r.data, m_len);
		if (m_data != &EmptyString)
			m_data[m_len] = 0;
	}
	return *this;
}

String &String::operator=(const char *str)
{
	return operator=(Slice<const char>(str));
}

String::~String()
{
	if (m_data != &EmptyString)
		delete [] m_data;
}

String &String::operator=(const String &r)
{
	return operator=(r.sub());
}

String &String::operator=(String &&r)
{
	clear();
	m_data = r.m_data;
	m_len = r.m_len;
	m_cap = r.m_cap;
	r._nullify();
	return *this;
}

void String::clear()
{
	m_len = 0;
	if (m_data != &EmptyString)
		m_data[0] = 0;
}

void String::reserve(int n)
{
	if (m_cap >= n)
		return;

	char *old_data = m_data;
	m_cap = n;
	m_data = new (OrDie) char[m_cap + 1];
	if (m_len > 0) {
		std::memcpy(m_data, old_data, m_len + 1);
	} else {
		m_data[0] = 0;
	}
	if (old_data != &EmptyString)
		delete [] old_data;
}

void String::shrink()
{
	if (m_cap == m_len)
		return;

	char *old_data = m_data;
	if (m_len > 0) {
		m_data = new (OrDie) char[m_len + 1];
		std::memcpy(m_data, old_data, m_len + 1);
	} else {
		m_data = &EmptyString;
	}
	m_cap = m_len;
	if (old_data != &EmptyString)
		delete [] old_data;
}

void String::resize(int n, char elem)
{
	NG_ASSERT(n >= 0);
	if (m_len == n)
		return;

	if (m_len > n) {
		m_len = n;
		m_data[m_len] = 0;
		return;
	}

	reserve(n);
	for (int i = m_len; i < n; i++)
		m_data[i] = elem;
	m_len = n;
	m_data[m_len] = 0;
}

void String::insert(int idx, Slice<const char> s)
{
	NG_SLICE_BOUNDS_CHECK(idx, m_len);
	if (s.length == 0) {
		return;
	}
	if (s.data >= m_data && s.data < m_data + m_len) {
		_self_insert(idx, s);
		return;
	}
	_ensure_capacity(s.length);
	char *dst = m_data + idx;
	std::memmove(dst + s.length, dst, m_len - idx);
	std::memcpy(dst, s.data, s.length);
	m_len += s.length;
	m_data[m_len] = 0;
}

void String::append(Slice<const char> s)
{
	insert(m_len, s);
}

void String::remove(int begin, int end)
{
	NG_ASSERT(begin <= end);
	NG_SLICE_BOUNDS_CHECK(begin, m_len);
	NG_SLICE_BOUNDS_CHECK(end, m_len);
	const int len = end - begin;
	if (end < m_len) {
		std::memmove(m_data + begin, m_data + end, m_len - end);
	}
	m_len -= len;
	m_data[m_len] = 0;
}

char *String::release()
{
	char *out = m_data;
	_nullify();
	return out;
}

void String::remove(int idx)
{
	NG_IDX_BOUNDS_CHECK(idx, m_len);
	if (idx == m_len - 1) {
		m_data[--m_len] = 0;
		return;
	}
	char *dst = m_data + idx;
	std::memmove(dst, dst+1, m_len-idx-1);
	m_data[--m_len] = 0;
}

void String::append(char elem)
{
	_ensure_capacity(1);
	m_data[m_len++] = elem;
	m_data[m_len] = 0;
}

void String::insert(int idx, char elem)
{
	NG_SLICE_BOUNDS_CHECK(idx, m_len);
	_ensure_capacity(1);
	if (idx < m_len) {
		char *dst = m_data + idx;
		std::memmove(dst + 1, dst, m_len - idx);
	}
	m_data[idx] = elem;
	m_data[++m_len] = 0;
}

void String::resize(int n)
{
	NG_ASSERT(n >= 0);
	if (m_len == n)
		return;

	if (m_len > n) {
		m_len = n;
		m_data[m_len] = 0;
		return;
	}

	reserve(n);
	m_len = n;
}

// String vs. const char*
String operator+(const String &lhs, const char *rhs) {
	return operator+(lhs, Slice<const char>(rhs));
}

String operator+(const char *lhs, const String &rhs) {
	return operator+(Slice<const char>(lhs), rhs);
}

String operator+(String &&lhs, const char *rhs) {
	return operator+(lhs, Slice<const char>(rhs));
}

String operator+(const char *lhs, String &&rhs) {
	return operator+(Slice<const char>(lhs), rhs);
}

// String vs. String
String operator+(const String &lhs, const String &rhs) {
	String out;
	out.reserve(lhs.length() + rhs.length());
	out.append(lhs);
	out.append(rhs);
	return out;
}

String operator+(String &&lhs, String &&rhs) {
	lhs.append(rhs);
	return lhs;
}

String operator+(String &&lhs, const String &rhs) {
	lhs.append(rhs);
	return lhs;
}

String operator+(const String &lhs, String &&rhs) {
	rhs.insert(0, lhs);
	return rhs;
}

// String vs. Slice<const char>
String operator+(const String &lhs, Slice<const char> rhs) {
	String out;
	out.reserve(lhs.length() + rhs.length);
	out.append(lhs);
	out.append(rhs);
	return out;
}

String operator+(Slice<const char> lhs, const String &rhs) {
	String out;
	out.reserve(lhs.length + rhs.length());
	out.append(lhs);
	out.append(rhs);
	return out;
}

String operator+(String &&lhs, Slice<const char> rhs) {
	lhs.append(rhs);
	return lhs;
}

String operator+(Slice<const char> lhs, String &&rhs) {
	rhs.insert(0, lhs);
	return rhs;
}

int compute_hash(const String &s)
{
	return compute_hash(s.sub());
}
