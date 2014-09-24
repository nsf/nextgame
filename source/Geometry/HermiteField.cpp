#include "Geometry/HermiteField.h"
#include "Geometry/RLE.h"
#include "Math/Utils.h"
#include "Math/Mat.h"
#include "Math/Utils.h"
#include "Core/Defer.h"
#include "Core/HashMap.h"
#include "Geometry/DebugDraw.h"
#include <utility>
#include <cstdio>
#include <climits>

//------------------------------------------------------------------------------
// HermiteField
//------------------------------------------------------------------------------

HermiteField::HermiteField(const Vec3i &size): data(volume(size)), size(size)
{
}

void HermiteField::resize(const Vec3i &newsize)
{
	size = newsize;
	data.resize(volume(size));
}

void apply_union(HermiteField *f, const HermiteField &diff, const Vec3i &offset)
{
	for (int z = 0; z < diff.size.z; z++) {
	for (int y = 0; y < diff.size.y; y++) {
	for (int x = 0; x < diff.size.x; x++) {
		const Vec3i src = Vec3i(x, y, z);
		const Vec3i dst = offset + src;
		const HermiteData src_hd = diff.get(src);
		HermiteData &dst_hd = f->get(dst);
		if (src_hd.material != 0)
			dst_hd.material = src_hd.material;

		for (int i = 0; i < 3; i++) {
			if (src[i] == 0)
				continue; // these edge intersections cannot be valid
			Vec3i src_hd1p = src;
			Vec3i dst_hd1p = dst;
			src_hd1p[i]--;
			dst_hd1p[i]--;
			const HermiteData src_hd1 = diff.get(src_hd1p);
			HermiteData &dst_hd1 = f->get(dst_hd1p);
			if (!edge_has_intersection(src_hd, src_hd1))
				continue;
			if (!edge_has_intersection(dst_hd, dst_hd1)) {
				// write zero just in case there was a change, for
				// better RLE compression
				dst_hd.edges[i] = 0;
				continue;
			}

			dst_hd.edges[i] = src_hd.edges[i];
		}
	}}}
}

void apply_difference(HermiteField *f, const HermiteField &diff, const Vec3i &offset)
{
	for (int z = 0; z < diff.size.z; z++) {
	for (int y = 0; y < diff.size.y; y++) {
	for (int x = 0; x < diff.size.x; x++) {
		const Vec3i src = Vec3i(x, y, z);
		const Vec3i dst = offset + src;
		const HermiteData src_hd0 = diff.get(src);
		HermiteData &dst_hd0 = f->get(dst);
		const bool src_hd0_air = src_hd0.material == 0;
		const bool dst_hd0_air = dst_hd0.material == 0;
		for (int i = 0; i < 3; i++) {
			if (src[i] == 0)
				continue; // these edge intersections cannot be valid
			Vec3i src_hd1p = src;
			Vec3i dst_hd1p = dst;
			src_hd1p[i]--;
			dst_hd1p[i]--;
			const HermiteData src_hd1 = diff.get(src_hd1p);
			HermiteData &dst_hd1 = f->get(dst_hd1p);

			const bool src_hd1_air = src_hd1.material == 0;
			if (src_hd0_air == src_hd1_air)
				continue; // no valid edge here

			const bool dst_hd1_air = dst_hd1.material == 0;

			// Cases: A - air, G - ground

			// A A
			if (dst_hd0_air && dst_hd1_air)
				continue; // can't remove anything, there's air everywhere already

			if (src_hd0_air != dst_hd0_air && src_hd1_air != dst_hd1_air) {
				// A G or G A
				//
				// that's the case when there is a valid existing intersection
				// on the edge, but we will possibly modify it
				const uint8_t dst_edge = dst_hd0.edges[i];
				const uint8_t src_edge = src_hd0.edges[i];
				if (src_hd0_air) {
					// hd0 is air
					dst_hd0.edges[i] = src_edge < dst_edge ? src_edge : dst_edge;
				} else {
					dst_hd0.edges[i] = src_edge > dst_edge ? src_edge : dst_edge;
				}
			} else if (!dst_hd0_air && !dst_hd1_air) {
				// G G
				// we check for G G explicitly, because the case which is the
				// opposite of the case above results in air only, so there is
				// no need to modify the edge
				dst_hd0.edges[i] = src_hd0.edges[i];
			} else {
				dst_hd0.edges[i] = 0;
			}
		}
	}}}
	for (int z = 0; z < diff.size.z; z++) {
	for (int y = 0; y < diff.size.y; y++) {
	for (int x = 0; x < diff.size.x; x++) {
		const Vec3i src = Vec3i(x, y, z);
		const Vec3i dst = offset + src;
		const HermiteData src_hd0 = diff.get(src);
		HermiteData &dst_hd0 = f->get(dst);
		if (src_hd0.material != 0)
			dst_hd0.material = 0;
	}}}
}

void apply_paint(HermiteField *f, const HermiteField &diff, const Vec3i &offset)
{
	for (int z = 0; z < diff.size.z; z++) {
	for (int y = 0; y < diff.size.y; y++) {
	for (int x = 0; x < diff.size.x; x++) {
		const Vec3i src = Vec3i(x, y, z);
		const Vec3i dst = offset + src;
		const HermiteData src_hd0 = diff.get(src);
		HermiteData &dst_hd0 = f->get(dst);
		if (src_hd0.material != 0 && dst_hd0.material != 0)
			dst_hd0.material = src_hd0.material;
	}}}
}

void reduce_field(HermiteField *fnew, const HermiteField &fold)
{
	const int offsets2[] = {1, fold.size.x, fold.size.x*fold.size.y};

	for (int z = 0; z < fnew->size.z; z++) {
	for (int y = 0; y < fnew->size.y; y++) {
	for (int x = 0; x < fnew->size.x; x++) {
		const Vec3i pos(x, y, z);
		const int offset = offset_3d(pos, fnew->size);
		const int offset2 = offset_3d(pos*Vec3i(2), fold.size);
		HermiteData &hd = fnew->data[offset];
		const HermiteData &hd0 = fold.data[offset2];
		hd.material = hd0.material;
		hd.x_edge = 0;
		hd.y_edge = 0;
		hd.z_edge = 0;
		for (int i = 0; i < 3; i++) {
			if (pos[i] == 0)
				continue;

			const HermiteData &hd1 = fold.data[offset2 - offsets2[i]];
			const HermiteData &hd2 = fold.data[offset2 - offsets2[i]*2];
			if (!edge_has_intersection(hd0, hd2)) {
				continue;
			}

			if (edge_has_intersection(hd0, hd1)) {
				hd.edges[i] = hd0.edges[i] / 2;
			} else {
				hd.edges[i] = hd1.edges[i] / 2 + HermiteData_HalfEdge();
			}
		}
	}}}
}

void create_hermite_cube(HermiteField *f, Vec3i *location, const Vec3i &center,
	int size, uint8_t material)
{
	*location = center - Vec3i(size/2);
	f->resize(Vec3i(size+2));
	for (auto &hd : f->data)
		hd = HermiteData_Air();

	const Vec3i last = f->size - Vec3i(1);
	for (int z = 0; z < f->size.z; z++) {
	for (int y = 0; y < f->size.y; y++) {
	for (int x = 0; x < f->size.x; x++) {
		const Vec3i pos(x, y, z);
		HermiteData &hd = f->get(pos);
		if (Vec3i(0) < pos && pos < last)
			hd.material = material;

		for (int i = 0; i < 3; i++) {
			if (pos[i] == 1)
				hd.edges[i] = HermiteData_HalfEdge();
			else if (pos[i] == last[i])
				hd.edges[i] = HermiteData_HalfEdge();
		}
	}}}
}

void create_hermite_sphere(HermiteField *f, Vec3i *location, const Vec3 &center, float radius, uint8_t material)
{
	// convert center to voxels
	const Vec3 vcenter = center / CUBE_SIZE;
	const int d = std::ceil(radius);
	*location = floor(vcenter) - Vec3i(d);
	f->resize(Vec3i(d*2+2));
	for (auto &hd : f->data)
		hd = HermiteData_Air();

	Vector<float> tmp(f->size.x * f->size.y * 2);
	for (int z = 0; z < f->size.z; z++) {
	for (int y = 0; y < f->size.y; y++) {
	for (int x = 0; x < f->size.x; x++) {
		const Vec3i pos(x, y, z);
		const Vec3 fpos = ToVec3(*location) + ToVec3(pos);
		const float a = length(fpos - vcenter) - radius;
		HermiteData &hd = f->get(pos);
		if (a < 0.0f)
			hd.material = material;
		tmp[offset_3d_slab(pos, f->size)] = a;
		for (int i = 0; i < 3; i++) {
			if (pos[i] == 0)
				continue;

			Vec3i lpos = pos;
			lpos[i]--;
			const float b = tmp[offset_3d_slab(lpos, f->size)];
			if ((a < 0.0f) != (b < 0.0f))
				hd.edges[i] = (a / (a-b)) * HermiteData_FullEdge();
		}
	}}}
}

//------------------------------------------------------------------------------
// HermiteRLEField and HermiteRLEIterator
//------------------------------------------------------------------------------

HermiteRLEField::HermiteRLEField(const HermiteField &field)
{
	size = field.size;
	rle_compress(&seqs, &data, field.data.sub());
}

HermiteRLEField::HermiteRLEField(const HermiteField &f,
	const Vec3i &offset, const Vec3i &size)
{
	int offseti = 0;
	for (int z = 0; z < size.z; z++) {
	for (int y = 0; y < size.y; y++) {
	for (int x = 0; x < size.x; x++) {
		append(f.get(offset + Vec3i(x, y, z)), offseti++);
	}}}
	finalize(size);
}

void HermiteRLEField::finalize(const Vec3i &size)
{
	this->size = size;
	seqs.pappend(volume(size), 0, false);
}

void HermiteRLEField::decompress(Slice<HermiteData> out) const
{
	rle_decompress(out.sub(0, volume(size)), seqs.sub(), data.sub());
}

void HermiteRLEField::decompress_to(Slice<HermiteData> out,
	const Vec3i &offset, const Vec3i &size) const
{
	auto it = iterator();
	for (int z = 0; z < this->size.z; z++) {
		const int zoffset = size.x * size.y * (offset.z + z);
	for (int y = 0; y < this->size.y; y++) {
		const int dst_offset = zoffset + size.x * (offset.y + y) + offset.x;
		//const int dst_offset = Offset3D(offset + Vec3i(0, y, z), size);
		auto s = out.sub(dst_offset, dst_offset + this->size.x);
		it.read(s);
	}}
}

void HermiteRLEField::partial_decompress(Slice<HermiteData> out,
	const Vec3i &from, const Vec3i &to) const
{
	NG_ASSERT(from < to);
	if (!(Vec3i(0) <= from && to <= size)) {
		die("out of bounds when partially decompressing RLE data");
	}

	const Vec3i dstsize = to - from;
	const int line_start = from.y * size.x + from.x;
	const int line_skip = size.x - dstsize.x;

	int offset = 0;
	for (int slicei = from.z; slicei < to.z; slicei++) {
		auto it = iterator(size.x * size.y * slicei + line_start);
		for (int i = 0; i < dstsize.y; i++) {
			it.read(out.sub(offset, offset+dstsize.x));
			offset += dstsize.x;
			if (i != dstsize.y-1)
				it.skip(line_skip);
		}
	}
}

int HermiteRLEField::byte_length() const
{
	return data.byte_length() + seqs.byte_length() / 2 +
	sizeof(data) + sizeof(seqs) + sizeof(size);
}

HermiteRLEIterator HermiteRLEField::iterator() const
{
	return HermiteRLEIterator(data.data(), seqs.data());
}

HermiteRLEIterator HermiteRLEField::iterator(int start) const
{
	return rle_find(seqs.sub(), data.sub(), start);
}

HermiteRLEIterator HermiteRLEField::iterator(const Vec3i &start) const
{
	return iterator(offset_3d(start, size));
}

void HermiteRLEField::append(HermiteData hd, int offset)
{
	rle_append(&seqs, &data, hd, offset);
}

void HermiteRLEField::serialize(ByteWriter *out) const
{
	out->write_int32(data.length());
	out->write_int32(seqs.length());
	out->write(slice_cast<const uint8_t>(data.sub()));
	out->write(slice_cast<const uint8_t>(seqs.sub()));
}

void HermiteRLEField::deserialize(ByteReader *in, const Vec3i &size, Error *err)
{
	const int data_n = in->read_int32(err);
	if (*err)
		return;

	const int seqs_n = in->read_int32(err);
	if (*err)
		return;

	this->size = size;
	data.resize(data_n);
	seqs.resize(seqs_n);
	in->read(slice_cast<uint8_t>(data.sub()), err);
	in->read(slice_cast<uint8_t>(seqs.sub()), err);
}

void HermiteRLEField::clear()
{
	data.clear();
	seqs.clear();
	size = Vec3i(0);
}

