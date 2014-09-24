#pragma once

#include "Core/Slice.h"
#include "Core/Vector.h"
#include "Math/Utils.h"
#include <type_traits>

//------------------------------------------------------------------------------
// Generic RLECompress function
//------------------------------------------------------------------------------

enum RLERunType {
	RLE_UNCOMPRESSED,
	RLE_COMPRESSED,
};

template <typename T, typename F>
static void rle_compress(Slice<T> field, F f)
{
	if (field.length == 0)
		return;

	typename std::remove_const<T>::type last = field[0];
	int num = 0;   // number of entries
	RLERunType rt = RLE_COMPRESSED;
	for (int i = 0, n = field.length; i < n; i++) {
		switch (rt) {
		case RLE_UNCOMPRESSED:
			if (field[i] == last) {
				// commit previous mode
				f(rt, num);
				num = 0;
				rt = RLE_COMPRESSED;
			} else {
				last = field[i];
			}
			break;
		case RLE_COMPRESSED:
			if (field[i] != last) {
				f(rt, num);
				num = 0;
				last = field[i];
				rt = RLE_UNCOMPRESSED;
			}
			break;
		}
		num++;
	}

	f(rt, num);
}

//------------------------------------------------------------------------------
// RLESeq - RLE sequence header
//------------------------------------------------------------------------------

const uint16_t RLE_SEQ_COMPRESSED_FLAG = 1 << 15;
const uint16_t RLE_SEQ_DATA_OFFSET_MASK = (1 << 15) ^ 0xFFFF;

struct RLESeq {
	// represents offset in uncomressed data, use that to do binary search
	uint16_t m_offset = 0;
	// points to data in HermiteRLEField
	uint16_t m_data_offset = 0;

	RLESeq() = default;
	RLESeq(uint16_t offset, uint16_t data_offset, bool compressed):
		m_offset(offset), m_data_offset(data_offset)
	{
		if (compressed)
			this->m_data_offset |= RLE_SEQ_COMPRESSED_FLAG;
	}
	uint16_t offset() const { return m_offset; }
	uint16_t data_offset() const { return m_data_offset & RLE_SEQ_DATA_OFFSET_MASK; }
	bool is_compressed() const { return m_data_offset & RLE_SEQ_COMPRESSED_FLAG; }

	bool operator==(const RLESeq &rhs) const { return m_offset == rhs.m_offset && m_data_offset == rhs.m_data_offset; }
	bool operator!=(const RLESeq &rhs) const { return m_offset != rhs.m_offset || m_data_offset != rhs.m_data_offset; }
};

//------------------------------------------------------------------------------
// RLEIterator
//------------------------------------------------------------------------------

template <typename T>
struct RLEIterator {
	const T *data = nullptr;
	const RLESeq *seqs = nullptr;
	int n = 0;

	RLEIterator() = default;
	RLEIterator(const T *data, const RLESeq *seqs): data(data), seqs(seqs) {}

	T operator*() const { return seqs->is_compressed() ? *data : data[n]; }
	RLEIterator &operator++()
	{
		skip_one();
		return *this;
	}
	RLEIterator operator++(int)
	{
		RLEIterator copy = *this;
		skip_one();
		return copy;
	}

	void skip_one()
	{
		const int cur = seqs[0].offset();
		const int next = seqs[1].offset();
		if (cur + n < next)
			n++;

		if (cur+n == next) {
			data += seqs[0].is_compressed() ? 1 : next-cur;
			seqs++;
			n = 0;
		}
	}

	void skip(int nskip)
	{
		while (nskip) {
			const int cur = seqs[0].offset();
			const int next = seqs[1].offset();
			const int canskip = next - (cur+n);
			const int willskip = min(canskip, nskip);
			nskip -= willskip;
			n += willskip;
			if (cur+n == next) {
				data += seqs[0].is_compressed() ? 1 : next-cur;
				seqs++;
				n = 0;
			}
		}
	}

	int can_skip() const
	{
		if (seqs->is_compressed()) {
			const int cur = seqs[0].offset();
			const int next = seqs[1].offset();
			return next - (cur+n);
		}
		return 0;
	}

	void read(Slice<T> out)
	{
		int dsti = 0;
		while (true) {
			const bool is_compressed = seqs[0].is_compressed();
			const int cur = seqs[0].offset();
			const int length = seqs[1].offset() - cur;

			if (is_compressed) {
				// compressed run
				while (dsti < out.length && n < length) {
					out[dsti++] = data[0];
					n++;
				}
			} else {
				// uncompressed run
				while (dsti < out.length && n < length) {
					out[dsti++] = data[n];
					n++;
				}
			}
			if (n == length) {
				data += is_compressed ? 1 : length;
				seqs++;
				n = 0;
			}
			if (dsti == out.length)
				return;
		}
	}
};

//------------------------------------------------------------------------------
// Generic RLESeq-based functions
//------------------------------------------------------------------------------

template <typename T>
void rle_append(Vector<RLESeq> *seqs, Vector<T> *data, T elem, int offset)
{
	if (seqs->length() == 0) {
		NG_ASSERT(offset == 0);
		seqs->pappend(0, 0, true);
		data->append(elem);
		return;
	}

	T last_data = data->last();
	RLESeq last_seq = seqs->last();
	if (last_seq.is_compressed()) {
		if (last_data != elem) {
			seqs->pappend(offset, data->length(), false);
			data->append(elem);
		}
	} else {
		if (last_data == elem) {
			seqs->pappend(offset, data->length(), true);
			data->append(elem);
		} else {
			data->append(elem);
		}
	}
}

template <typename T>
RLEIterator<T> rle_find(Slice<const RLESeq> seqs, Slice<const T> data, int offset)
{
	int first = 0;
	int last = seqs.length-1;
	while (first < last) {
		int mid = first + (last-first) / 2;
		if (seqs[mid].offset() < offset) {
			first = mid+1;
		} else {
			last = mid;
		}
	}
	if (seqs[first].offset() > offset)
		first--;
	RLEIterator<T> it;
	it.data = data.data + seqs[first].data_offset();
	it.seqs = seqs.data + first;
	it.n = offset - seqs[first].offset();
	return it;
}

template <typename T>
void rle_compress(Vector<RLESeq> *seqs, Vector<T> *data, Slice<const T> s)
{
	// first let's calculate length of the RLE data
	int data_len = 0;
	int seqs_len = 0;
	rle_compress(s, [&](RLERunType rt, int num) {
		switch (rt) {
		case RLE_UNCOMPRESSED:
			seqs_len++;
			data_len += num;
			break;
		case RLE_COMPRESSED:
			seqs_len++;
			data_len++;
			break;
		}
	});

	data->resize(data_len);
	seqs->resize(seqs_len+1);
	int srci = 0;
	int datai = 0;
	int seqsi = 0;
	int offset = 0;
	rle_compress(s, [&](RLERunType rt, int num) {
		switch (rt) {
		case RLE_UNCOMPRESSED:
			(*seqs)[seqsi++] = RLESeq(offset, datai, false);
			for (int i = 0; i < num; i++)
				(*data)[datai+i] = s[srci+i];
			datai += num;
			offset += num;
			srci += num;
			break;
		case RLE_COMPRESSED:
			(*seqs)[seqsi++] = RLESeq(offset, datai, true);
			(*data)[datai++] = s[srci];
			srci += num;
			offset += num;
			break;
		}
	});
	(*seqs)[seqsi] = RLESeq(s.length, 0, false);
}

template <typename T>
void rle_decompress(Slice<T> out, Slice<const RLESeq> seqs, Slice<const T> data)
{
	const int uncompressed_length = out.length;
	int dsti = 0;
	int seqsi = 0;
	int datai = 0;
	while (dsti < uncompressed_length) {
		const int stop = seqs[seqsi+1].offset();
		if (seqs[seqsi].is_compressed()) {
			// compressed run
			while (dsti < stop)
				out[dsti++] = data[datai];
			seqsi++;
			datai++;
		} else {
			// uncompressed run
			while (dsti < stop)
				out[dsti++] = data[datai++];
			seqsi++;
		}
	}
}
