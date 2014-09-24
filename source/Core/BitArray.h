#pragma once

#include "Core/Utils.h"
#include "Core/Memory.h"
#include <cstdint>
#include <cstring>

struct BitArray {
	uint32_t *m_data = nullptr;
	int m_len = 0;

	BitArray() = default;
	explicit BitArray(int n): m_len(n)
	{
		NG_ASSERT(n >= 0);
		if (m_len == 0)
			return;
		m_data = allocate_memory<uint32_t>(uint32_length());
		std::memset(m_data, 0, byte_length());
	}

	BitArray(BitArray &&r): m_data(r.m_data), m_len(r.m_len)
	{
		r._nullify();
	}

	BitArray(const BitArray &r) = delete;

	~BitArray()
	{
		free_memory(m_data);
	}

	BitArray &operator=(BitArray &&r)
	{
		free_memory(m_data);
		m_data = r.m_data;
		m_len = r.m_len;
		r._nullify();
		return *this;
	}

	BitArray &operator=(const BitArray &r) = delete;
	void copy_from(const BitArray &r);

	void _nullify()
	{
		m_data = nullptr;
		m_len = 0;
	}

	int uint32_length() const { return (m_len+31)/32; }
	int byte_length() const { return uint32_length()*4; }
	int length() const { return m_len; }

	void set();
	void clear();

	bool test_bit(int idx) const
	{
		NG_IDX_BOUNDS_CHECK(idx, m_len);
		const int offset = idx / 32;
		const uint32_t mask = 1 << (idx % 32);
		return m_data[offset] & mask;
	}

	void set_bit(int idx)
	{
		NG_IDX_BOUNDS_CHECK(idx, m_len);
		const int offset = idx / 32;
		const uint32_t mask = 1 << (idx % 32);
		m_data[offset] |= mask;
	}

	void clear_bit(int idx)
	{
		NG_IDX_BOUNDS_CHECK(idx, m_len);
		const int offset = idx / 32;
		const uint32_t mask = 1 << (idx % 32);
		m_data[offset] &= ~mask;
	}

	void set_bit_range(int beg, int end);

	// TODO: doesn't check shit, doesn't belong here
	void set_bit_range_2d(int x, int y, int w, int h, int img_w);

	BitArray &operator&=(const BitArray &r);
	BitArray &operator|=(const BitArray &r);
	BitArray &operator^=(const BitArray &r);
};
