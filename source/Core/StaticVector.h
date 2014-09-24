#pragma once

#include "Core/Vector.h"

/*
 _            _
| |          | |
| |_ ___   __| | ___
| __/ _ \ / _` |/ _ \
| || (_) | (_| | (_) |
 \__\___/ \__,_|\___/

template <typename T, int N>
struct StaticVector {
	T m_static[N];
	Vector<T> m_dynamic;

	StaticVector &Init()
	{
		m_dynamic.m_cap = N;
		m_dynamic.m_len = 0;
		m_dynamic.m_data = nullptr;
		return *this;
	}

	void Free()
	{
		if (m_dynamic.m_len >= N)
			m_dynamic.~Vector<T>();
	}

	int Length() const { return m_dynamic.m_len; }
	int ByteLength() const { return m_dynamic.m_len * sizeof(T); }
	int Capacity() const { return m_dynamic.m_cap; }
	T *Data() { return m_dynamic.m_len > N ? m_dynamic.m_data : m_static; }
	const T *Data() const { return m_dynamic.m_len > N ? m_dynamic.m_data : m_static; }

	void Clear()
	{
		if (m_dynamic.m_len >= N)
			m_dynamic.~Vector<T>();
		m_dynamic.m_cap = N;
		m_dynamic.m_len = 0;
		m_dynamic.m_data = nullptr;
	}

	void Append(const T &item)
	{
		if (m_dynamic.m_len < N) {
			m_static[m_dynamic.m_len++] = item;
		} else {
			if (m_dynamic.m_len == N) {
				new (&m_dynamic) Vector<T>(N);
				CopyMemory(m_dynamic.m_data, m_static, N);
			}
			m_dynamic.Append(item);
		}
	}

	void QuickRemove(int idx)
	{
		NG_IDX_BOUNDS_CHECK(idx, m_dynamic.m_len);
		T *data = m_dynamic.m_len > N ? m_dynamic.m_data : m_static;
		if (idx != m_dynamic.m_len)
			data[idx] = data[m_dynamic.m_len-1];
		m_dynamic.m_len--;

		if (m_dynamic.m_len == N) {
			CopyMemory(m_static, m_dynamic.m_data, N);
			m_dynamic.~Vector<T>();
			m_dynamic.m_cap = N;
			m_dynamic.m_len = N;
			m_dynamic.m_data = nullptr;
		}
	}
	T &operator[](int idx)
	{
		NG_IDX_BOUNDS_CHECK(idx, Length());
		return Data()[idx];
	}
	const T &operator[](int idx) const
	{
		NG_IDX_BOUNDS_CHECK(idx, Length());
		return Data()[idx];
	}
	Slice<T> Sub()
	{
		return {Data(), Length()};
	}
	Slice<T> Sub(int begin)
	{
		NG_SLICE_BOUNDS_CHECK(begin, Length());
		return {Data() + begin, Length() - begin};
	}
	Slice<T> Sub(int begin, int end)
	{
		NG_ASSERT(begin <= end);
		NG_SLICE_BOUNDS_CHECK(begin, Length());
		NG_SLICE_BOUNDS_CHECK(end, Length());
		return {Data() + begin, end - begin};
	}
	Slice<const T> Sub() const
	{
		return {Data(), Length()};
	}
	Slice<const T> Sub(int begin) const
	{
		NG_SLICE_BOUNDS_CHECK(begin, Length());
		return {Data() + begin, Length() - begin};
	}
	Slice<const T> Sub(int begin, int end) const
	{
		NG_ASSERT(begin <= end);
		NG_SLICE_BOUNDS_CHECK(begin, Length());
		NG_SLICE_BOUNDS_CHECK(end, Length());
		return {Data() + begin, end - begin};
	}

	operator Slice<T>() { return {Data(), Length()}; }
	operator Slice<const T>() const { return {Data(), Length()}; }
};

template <typename T, int N>
const T *begin(const StaticVector<T, N> &v) { return v.Data(); }
template <typename T, int N>
const T *end(const StaticVector<T, N> &v) { return v.Data()+v.Length(); }
template <typename T, int N>
T *begin(StaticVector<T, N> &v) { return v.Data(); }
template <typename T, int N>
T *end(StaticVector<T, N> &v) { return v.Data()+v.Length(); }

*/
