#include "Core/ByteIO.h"
#include <type_traits>
#include <lz4.h>
#include <lz4hc.h>

//----------------------------------------------------------------------
// ByteReader
//----------------------------------------------------------------------

template <typename T>
T read_int(ByteReader *br, Error *err)
{
	static_assert(std::is_integral<T>::value, "T should be an integer type");
	if (*err)
		return 0;

	if (br->data.length < (int)sizeof(T)) {
		err->set("Not enough data to read the integer of size: %d", sizeof(T));
		return 0;
	}

	auto b = br->data.sub(0, sizeof(T));
	br->data = br->data.sub(sizeof(T));
	switch (sizeof(T)) {
	case 1:
		return b[0];
	case 2:
		return
			((uint16_t)b[0] << 0) |
			((uint16_t)b[1] << 8) ;
	case 4:
		return
			((uint32_t)b[0] << 0) |
			((uint32_t)b[1] << 8) |
			((uint32_t)b[2] << 16) |
			((uint32_t)b[3] << 24) ;
	case 8:
		return
			((uint64_t)b[0] << 0) |
			((uint64_t)b[1] << 8) |
			((uint64_t)b[2] << 16) |
			((uint64_t)b[3] << 24) |
			((uint64_t)b[4] << 32) |
			((uint64_t)b[5] << 40) |
			((uint64_t)b[6] << 48) |
			((uint64_t)b[7] << 56);
	default:
		err->set("Bad integer size: %d", sizeof(T));
		return 0;
	}
}

template <typename T>
T read_fp(ByteReader *br, Error *err = &DefaultError)
{
	static_assert(std::is_floating_point<T>::value, "T should be a float type");
	if (*err)
		return 0;

	if (br->data.length < (int)sizeof(T)) {
		err->set("Not enough data to read the float of size: %d", sizeof(T));
		return 0;
	}

	T out;
	switch (sizeof(T)) {
	case 4: {
		auto v = read_int<uint32_t>(br, err);
		if (*err)
			return 0;
		memcpy(&out, &v, sizeof(T));
		return out;
	}
	case 8: {
		auto v = read_int<uint64_t>(br, err);
		if (*err)
			return 0;
		memcpy(&out, &v, sizeof(T));
		return out;
	}
	default:
		err->set("Bad float size: %d", sizeof(T));
		return 0;
	}
}

uint8_t  ByteReader::read_uint8(Error *err)  { return read_int<uint8_t>(this, err); }
uint16_t ByteReader::read_uint16(Error *err) { return read_int<uint16_t>(this, err); }
uint32_t ByteReader::read_uint32(Error *err) { return read_int<uint32_t>(this, err); }
uint64_t ByteReader::read_uint64(Error *err) { return read_int<uint64_t>(this, err); }
int8_t   ByteReader::read_int8(Error *err)   { return read_int<int8_t>(this, err); }
int16_t  ByteReader::read_int16(Error *err)  { return read_int<int16_t>(this, err); }
int32_t  ByteReader::read_int32(Error *err)  { return read_int<int32_t>(this, err); }
int64_t  ByteReader::read_int64(Error *err)  { return read_int<int64_t>(this, err); }
float    ByteReader::read_float(Error *err)  { return read_fp<float>(this, err); }
double   ByteReader::read_double(Error *err) { return read_fp<double>(this, err); }

void ByteReader::read(Slice<uint8_t> out, Error *err)
{
	if (*err)
		return;
	if (out.length > data.length) {
		err->set("Not enough data to read a slice of size: %d", out.length);
		return;
	}

	copy(out, data);
	data = data.sub(out.length);
}

Vector<uint8_t> ByteReader::read_compressed(Error *err)
{
	if (*err)
		return {};

	const int decompressed_len = read_int32(err);
	const int compressed_len = read_int32(err);
	if (*err)
		return {};

	if (compressed_len > data.length)
		err->set("Malformed compressed sequence (byte stream size: %d, compressed stream size: %d",
			data.length, compressed_len);

	Vector<uint8_t> out(decompressed_len);
	int bytes_read = LZ4_decompress_safe((const char*)data.data, (char*)out.data(),
		compressed_len, decompressed_len);
	NG_ASSERT(bytes_read == out.length());
	return out;
}

//----------------------------------------------------------------------
// ByteWriter
//----------------------------------------------------------------------

void ByteWriter::write_uint8(uint8_t v)   { data.append(v); }
void ByteWriter::write_uint16(uint16_t v) { data.append(slice_cast<uint8_t>(Slice<uint16_t>(&v, 1))); }
void ByteWriter::write_uint32(uint32_t v) { data.append(slice_cast<uint8_t>(Slice<uint32_t>(&v, 1))); }
void ByteWriter::write_uint64(uint64_t v) { data.append(slice_cast<uint8_t>(Slice<uint64_t>(&v, 1))); }
void ByteWriter::write_int8(int8_t v)     { data.append(slice_cast<uint8_t>(Slice<int8_t>(&v, 1))); }
void ByteWriter::write_int16(int16_t v)   { data.append(slice_cast<uint8_t>(Slice<int16_t>(&v, 1))); }
void ByteWriter::write_int32(int32_t v)   { data.append(slice_cast<uint8_t>(Slice<int32_t>(&v, 1))); }
void ByteWriter::write_int64(int64_t v)   { data.append(slice_cast<uint8_t>(Slice<int64_t>(&v, 1))); }
void ByteWriter::write_float(float v)     { data.append(slice_cast<uint8_t>(Slice<float>(&v, 1))); }
void ByteWriter::write_double(double v)   { data.append(slice_cast<uint8_t>(Slice<double>(&v, 1))); }

void ByteWriter::write_string(Slice<const char> s) { data.append(slice_cast<uint8_t>(s)); }
void ByteWriter::write(Slice<const uint8_t> s) { data.append(s); }

void ByteWriter::write_compressed(Slice<const uint8_t> s)
{
	// decompressed length
	// compressed length
	// data
	write_int32(s.length);
	const int compressed_len_pos = data.length();
	write_int32(0);
	if (s.length == 0)
		return;
	const int len = data.length();
	const int compressed_size = LZ4_compressBound(s.length);
	data.resize(len + compressed_size);
	const int bytes_written = LZ4_compressHC(
		(const char*)s.data, (char*)data.data() + len, s.length);
	data.resize(len + bytes_written);
	copy(data.sub(compressed_len_pos, compressed_len_pos + 4),
		slice_cast<const uint8_t>(Slice<const int>(&bytes_written, 1)));
}

Slice<uint8_t> ByteWriter::sub() { return data.sub(); }
Slice<const uint8_t> ByteWriter::sub() const { return data.sub(); }
