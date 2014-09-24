#include "Core/Slice.h"
#include "Core/Error.h"
#include "Core/Vector.h"

//----------------------------------------------------------------------
// ByteReader
//----------------------------------------------------------------------

// little-endian only at the moment
struct ByteReader {
	Slice<const uint8_t> data;

	uint8_t         read_uint8(Error *err = &DefaultError);
	uint16_t        read_uint16(Error *err = &DefaultError);
	uint32_t        read_uint32(Error *err = &DefaultError);
	uint64_t        read_uint64(Error *err = &DefaultError);
	int8_t          read_int8(Error *err = &DefaultError);
	int16_t         read_int16(Error *err = &DefaultError);
	int32_t         read_int32(Error *err = &DefaultError);
	int64_t         read_int64(Error *err = &DefaultError);
	float           read_float(Error *err = &DefaultError);
	double          read_double(Error *err = &DefaultError);
	void            read(Slice<uint8_t> out, Error *err = &DefaultError);
	Vector<uint8_t> read_compressed(Error *err = &DefaultError);

	ByteReader(Slice<const uint8_t> data): data(data) {}
};

//----------------------------------------------------------------------
// ByteWriter
//----------------------------------------------------------------------

struct ByteWriter {
	Vector<uint8_t> data;

	void write_uint8(uint8_t v);
	void write_uint16(uint16_t v);
	void write_uint32(uint32_t v);
	void write_uint64(uint64_t v);
	void write_int8(int8_t v);
	void write_int16(int16_t v);
	void write_int32(int32_t v);
	void write_int64(int64_t v);
	void write_float(float v);
	void write_double(double v);
	void write_string(Slice<const char> s);
	void write(Slice<const uint8_t> s);
	void write_compressed(Slice<const uint8_t> s);

	Slice<uint8_t> sub();
	Slice<const uint8_t> sub() const;
};
