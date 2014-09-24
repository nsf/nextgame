#pragma once

#include "Geometry/Global.h"
#include "Geometry/VertexFormats.h"
#include "Geometry/HermiteData.h"
#include "Geometry/RLE.h"
#include "Core/ByteIO.h"
#include "Core/Vector.h"
#include "Math/Vec.h"

//------------------------------------------------------------------------------
// HermiteField
//------------------------------------------------------------------------------

struct HermiteField {
	Vector<HermiteData> data;
	Vec3i size = Vec3i(0);

	HermiteField() = default;
	explicit HermiteField(const Vec3i &size);

	HermiteData &get(const Vec3i &p) { return data[offset_3d(p, size)]; }
	const HermiteData &get(const Vec3i &p) const { return data[offset_3d(p, size)]; }
	int byte_length() const { return data.byte_length(); }

	void resize(const Vec3i &newsize);
};

void apply_union(HermiteField *f, const HermiteField &diff, const Vec3i &offset);
void apply_difference(HermiteField *f, const HermiteField &diff, const Vec3i &offset);
void apply_paint(HermiteField *f, const HermiteField &diff, const Vec3i &offset);
void reduce_field(HermiteField *fnew, const HermiteField &fold);

void create_hermite_cube(HermiteField *f, Vec3i *location, const Vec3i &center,
	int size, uint8_t material);
void create_hermite_sphere(HermiteField *f, Vec3i *location, const Vec3 &center,
	float radius, uint8_t material);

//------------------------------------------------------------------------------
// HermiteRLEField and HermiteRLEIterator
//------------------------------------------------------------------------------

using HermiteRLEIterator = RLEIterator<HermiteData>;

struct HermiteRLEField {
	Vector<HermiteData> data;
	Vector<RLESeq> seqs;
	Vec3i size = Vec3i(0);

	HermiteRLEField() = default;
	explicit HermiteRLEField(const HermiteField &field);
	HermiteRLEField(const HermiteField &field,
		const Vec3i &offset, const Vec3i &size);

	void decompress(Slice<HermiteData> out) const;
	void decompress_to(Slice<HermiteData> out,
		const Vec3i &offset, const Vec3i &size) const;
	void partial_decompress(Slice<HermiteData> out,
		const Vec3i &from, const Vec3i &to) const;
	int byte_length() const;
	HermiteRLEIterator iterator() const;
	HermiteRLEIterator iterator(int start) const;
	HermiteRLEIterator iterator(const Vec3i &start) const;

	void append(HermiteData hd, int offset);

	void serialize(ByteWriter *out) const;
	void deserialize(ByteReader *in, const Vec3i &size,
		Error *err = &DefaultError);

	void clear();
	void finalize(const Vec3i &size);
};

//------------------------------------------------------------------------------
// HermiteFieldToMesh
//------------------------------------------------------------------------------

void hermite_rle_fields_to_mesh(
	Vector<V3N3M1_terrain> &vertices,
	Vector<uint32_t> &indices,
	Slice<const HermiteRLEField*> fields,
	Slice<const int> lods, int largest_lod,
	const Vec3 &base);

void debug_draw_mc(const Vec3 *verts, HermiteData *hd, const Vec3 &color = Vec3_X());
void generate_marching_cubes();
