#include "Core/BitArray.h"


void BitArray::set()
{
	for (int i = 0, n = uint32_length(); i < n; i++)
		m_data[i] = 0xFFFFFFFF;
}

void BitArray::clear()
{
	for (int i = 0, n = uint32_length(); i < n; i++)
		m_data[i] = 0;
}

void BitArray::set_bit_range(int beg, int end)
{
	NG_ASSERT(beg <= end);
	NG_SLICE_BOUNDS_CHECK(beg, m_len);
	NG_SLICE_BOUNDS_CHECK(end, m_len);
	int len = end-beg;
	if (len == 0)
		return;

	int offset = beg / 32;

	// first part
	const int bit_offset = beg % 32;
	const int avail = 32 - bit_offset;
	const int to_set = len < avail ? len : avail;
	const uint32_t mask = (to_set == 32) ?
	0xFFFFFFFF :
	((1 << to_set) - 1) << bit_offset;
	m_data[offset++] |= mask;
	len -= to_set;

	// second part, always bit_offset == 0
	while (len > 0) {
		const int to_set = len < 32 ? len : 32;
		const uint32_t mask = (to_set == 32) ?
		0xFFFFFFFF :
		((1 << to_set) - 1);
		m_data[offset++] |= mask;
		len -= to_set;
	}
}

void BitArray::set_bit_range_2d(int x, int y, int w, int h, int img_w)
{
	int offset = img_w * y + x;
	for (int i = 0; i < h; i++) {
		set_bit_range(offset, offset+w);
		offset += img_w;
	}
}

void BitArray::copy_from(const BitArray &r)
{
	if (m_len != r.m_len) {
		free_memory(m_data);
		m_data = allocate_memory<uint32_t>(r.uint32_length());
		m_len = r.m_len;
	}
	std::memcpy(m_data, r.m_data, byte_length());
}

BitArray &BitArray::operator&=(const BitArray &r)
{
	const int ulen_lhs = uint32_length();
	const int ulen_rhs = r.uint32_length();
	const int minlen = ulen_lhs < ulen_rhs ? ulen_lhs : ulen_rhs;
	for (int i = 0; i < minlen; i++) {
		m_data[i] &= r.m_data[i];
	}
	return *this;
}

BitArray &BitArray::operator|=(const BitArray &r)
{
	const int ulen_lhs = uint32_length();
	const int ulen_rhs = r.uint32_length();
	const int minlen = ulen_lhs < ulen_rhs ? ulen_lhs : ulen_rhs;
	for (int i = 0; i < minlen; i++) {
		m_data[i] |= r.m_data[i];
	}
	return *this;
}

BitArray &BitArray::operator^=(const BitArray &r)
{
	const int ulen_lhs = uint32_length();
	const int ulen_rhs = r.uint32_length();
	const int minlen = ulen_lhs < ulen_rhs ? ulen_lhs : ulen_rhs;
	for (int i = 0; i < minlen; i++) {
		m_data[i] ^= r.m_data[i];
	}
	return *this;
}
