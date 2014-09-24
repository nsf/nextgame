#include "Geometry/CubeField.h"
#include "Geometry/Global.h"
#include "Map/Config.h"
#include "Geometry/DebugDraw.h"

float ray_plane_intersection(const Plane &p, const Vec3 &orig, const Vec3 &dir)
{
	return -(dot(orig, p.n) + p.d) / dot(dir, p.n);
}

float ray_plane_intersection_axis(float p, int axis, const Vec3 &orig, const Vec3 &dir)
{
	return -(orig[axis] - p) / dir[axis];
}

Face ray_aabb_face(const Vec3 &min, const Vec3 &max, const Vec3 &orig,
	const Vec3 &dir, float *out)
{
	float planes[3];
	Face faces[3];
	if (dir.x < 0.0f) {
		planes[0] = max.x;
		faces[0] = X_POS;
	} else {
		planes[0] = min.x;
		faces[0] = X_NEG;
	}
	if (dir.y < 0.0f) {
		planes[1] = max.y;
		faces[1] = Y_POS;
	} else {
		planes[1] = min.y;
		faces[1] = Y_NEG;
	}
	if (dir.z < 0.0f) {
		planes[2] = max.z;
		faces[2] = Z_POS;
	} else {
		planes[2] = min.z;
		faces[2] = Z_NEG;
	}

	float results[3];
	for (int i = 0; i < 3; i++) {
		results[i] = ray_plane_intersection_axis(planes[i], i, orig, dir);
	}

	int amax = 2;
	if (results[1] > results[2]) {
		amax = 1;
	}
	if (results[0] > results[amax]) {
		amax = 0;
	}

	if (out)
		*out = results[amax];
	return faces[amax];
}

static const uint8_t TRIANGLE_CONFIGS[][7] = {
	{0},
	{0},
	{0},
	{1, 1, 3, 2},
	{0},
	{0},
	{1, 0, 2, 1},
	{1, 0, 2, 1},
	{0},
	{1, 0, 3, 2},
	{0},
	{1, 0, 3, 2},
	{1, 0, 3, 1},
	{1, 0, 3, 1},
	{1, 0, 2, 1},
	{2, 0, 2, 1, 0, 3, 2},
	{2, 0, 3, 1, 1, 3, 2}, // alternative winding
};

void cube_field_to_mesh(Vector<V3N3M1> &vertices, const CubeField &field)
{
	const Vec3i csize = field.size;
	vertices.clear();

	const int axes[] = {0, 0, 1, 1, 2, 2};
	const bool positive[] = {true, false, true, false, true, false};

	auto cube = [&](const Vec3i &p, const CubeData &cd) {
		// for each face
		for (int i = 0; i < 6; i++) {
			const int axis = axes[i];
			Vec3i neighbour = p;
			neighbour[axis] += positive[i] ? 1 : -1;
			if (Vec3i(0) <= neighbour && neighbour < csize) {
				const CubeData ncd = field.get(neighbour);
				if (ncd.material != 0)
					continue;
			}

			CubeVertex verts[4];
			const uint8_t cfg = cd.vertices_and_tri_config(i, verts);

			// get face vertices
			const uint8_t *tris = TRIANGLE_CONFIGS[cfg];
			if (tris[0] == 0)
				continue;

			if (cfg == 15) {
				// four faces, we also need to figure out the
				// proper winding
				NG_ASSERT(tris[0] == 2);
				CubeVertex e1 = (verts[0] + verts[2]) / 2;
				CubeVertex e2 = (verts[1] + verts[3]) / 2;
				if (!positive[i])
					std::swap(e1, e2);
				if (e2[axis] < e1[axis])
					tris = TRIANGLE_CONFIGS[16];
			}

			uint32_t m = cd.material - 1;
			for (int j = 0; j < tris[0]; j++) {
				int off = j*3 + 1;
				int ia = tris[off+0];
				int ib = tris[off+1];
				int ic = tris[off+2];
				const Vec3 a = (ToVec3(p) + (verts[ia] / 8.0f)) * CUBE_SIZE1;
				const Vec3 b = (ToVec3(p) + (verts[ib] / 8.0f)) * CUBE_SIZE1;
				const Vec3 c = (ToVec3(p) + (verts[ic] / 8.0f)) * CUBE_SIZE1;
				const Vec3 ab = a - b;
				const Vec3 cb = c - b;
				Vec3 n = normalize(cross(cb, ab));
				vertices.append({a, n, m});
				vertices.append({b, n, m});
				vertices.append({c, n, m});
			}
		}
	};

	for (int z = 0; z < csize.z; z++) {
	for (int y = 0; y < csize.y; y++) {
	for (int x = 0; x < csize.x; x++) {
		const Vec3i p(x, y, z);
		const CubeData &cd = field.get(p);
		if (cd.material == 0)
			continue;

		cube(p, cd);
	}}}
}

CubeField::CubeField(const Vec3i &size): data(volume(size)), size(size)
{
	CubeData air;
	for (int i = 0; i < 12; i++)
		air.edges[i] = CubeEdge(0, 8);
	for (int i = 0; i < data.length(); i++) {
		data[i] = air;
	}
}

void CubeField::resize(const Vec3i &newsize)
{
	size = newsize;
	data.resize(volume(size));
}

CubeRLEField::CubeRLEField(const CubeField &field)
{
	size = field.size;
	rle_compress(&seqs, &data, field.data.sub());
}

void CubeRLEField::partial_decompress(Slice<CubeData> out, const Vec3i &from, const Vec3i &to) const
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

CubeRLEIterator CubeRLEField::iterator() const
{
	return CubeRLEIterator(data.data(), seqs.data());
}

CubeRLEIterator CubeRLEField::iterator(int start) const
{
	return rle_find(seqs.sub(), data.sub(), start);
}

CubeRLEIterator CubeRLEField::iterator(const Vec3i &start) const
{
	return iterator(offset_3d(start, size));
}

int CubeRLEField::byte_length() const
{
	return data.byte_length() + seqs.byte_length() +
		sizeof(data) + sizeof(seqs) + sizeof(size);
}

void cube_quad_field_to_mesh(Vector<V3M1_layer1> *pvertices, Slice<const CubeRLEField*> fields)
{
	const Vec3i voffset = (CHUNK_SIZE - Vec3i(2, 0, 1)) * Vec3i(1, 0, 1);
	const int line_width = CHUNK_SIZE.x + 2;

	//      (y)
	//       1
	// (z) 2 3 0
	//       4
	const Vec3i line_offsets[5] = {
		Vec3i(0, 0, -1),
		Vec3i(0, 1, 0),
		Vec3i(0, 0, 1),
		Vec3i(0, 0, 0),
		Vec3i(0, -1, 0),
	};

	Vector<CubeData> tmp(line_width * 5);
	Slice<CubeData> lines[5];
	for (int i = 0; i < 5; i++) {
		const int offset = i * line_width;
		lines[i] = tmp.sub(offset, offset + line_width);
	}
	auto unpack_line = [&](Slice<CubeData> out, const Vec3i &from) {
		if (from.y < 0 || from.y >= CHUNK_SIZE.y) {
			fill(out, CubeData(1));
			return;
		}

		const Vec3i chunk0 = from / CHUNK_SIZE;
		const Vec3i chunk1 = chunk0 + Vec3i_X();
		const Vec3i ioffset0 = from % CHUNK_SIZE;
		const Vec3i ioffset1 = ioffset0 * Vec3i(0, 1, 1);
		const int i0 = chunk0.z * 2 + chunk0.x;
		const int beg0 = 0;
		const int end0 = CHUNK_SIZE.x - ioffset0.x;
		const int i1 = chunk1.z * 2 + chunk1.x;
		const int beg1 = end0;
		const int end1 = end0 + CHUNK_SIZE.x;
		if (fields[i0])
			fields[i0]->iterator(ioffset0).read(out.sub(beg0, end0));
		else
			fill(out.sub(beg0, end0), CubeData(0));
		if (fields[i1])
			fields[i1]->iterator(ioffset1).read(out.sub(beg1, end1));
		else
			fill(out.sub(beg1, end1), CubeData(0));
		NG_ASSERT(end1 == line_width);
	};

	const Vec3 chunk_size_f = ToVec3(CHUNK_SIZE) * CUBE_SIZE;
	for (int z = 0; z < CHUNK_SIZE.z; z++) {
	for (int y = 0; y < CHUNK_SIZE.y; y++) {
		const Vec3i offset = voffset + Vec3i(0, y, z);
		if (y == 0) {
			for (int i = 0; i < 5; i++)
				unpack_line(lines[i], offset + line_offsets[i]);
		} else {
			std::swap(lines[4], lines[3]);
			std::swap(lines[3], lines[1]);
			for (int i = 0; i < 3; i++)
				unpack_line(lines[i], offset + line_offsets[i]);
		}

		// do line
		for (int i = 0; i < CHUNK_SIZE.x; i++) {
			const CubeData &cd = lines[3][i+1];
			if (cd.material == 0)
				continue;

			const CubeData neighbours[6] = {
				lines[3][i+2],
				lines[3][i],
				lines[1][i+1],
				lines[4][i+1],
				lines[2][i+1],
				lines[0][i+1],
			};

			const Vec3 p = ToVec3(offset + Vec3i(i+1, 0, 0)) * CUBE_SIZE - chunk_size_f;
			const int axes[] = {0, 0, 1, 1, 2, 2};
			for (int j = 0; j < 6; j++) {
				if (neighbours[j].is_solid()) {
					continue;
				}

				const int axis = axes[j];
				CubeVertex verts[4];
				const uint8_t cfg = cd.vertices_and_tri_config(j, verts);

				// get face vertices
				const uint8_t *tris = TRIANGLE_CONFIGS[cfg];
				if (tris[0] == 0)
					continue;

				if (cfg == 15) {
					// quad face, we also need to figure out the
					// proper winding
					NG_ASSERT(tris[0] == 2);
					CubeVertex e1 = (verts[0] + verts[2]) / 2;
					CubeVertex e2 = (verts[1] + verts[3]) / 2;
					if ((j & 1) == 0)
						std::swap(e1, e2);
					if (e2[axis] < e1[axis])
						tris = TRIANGLE_CONFIGS[16];
				}

				uint32_t m = cd.material - 1;
				for (int k = 0; k < tris[0]; k++) {
					int off = k*3 + 1;
					int ia = tris[off+0];
					int ib = tris[off+1];
					int ic = tris[off+2];
					const Vec3 a = p + (verts[ia] / 8.0f) * CUBE_SIZE;
					const Vec3 b = p + (verts[ib] / 8.0f) * CUBE_SIZE;
					const Vec3 c = p + (verts[ic] / 8.0f) * CUBE_SIZE;
					pvertices->append({a, m});
					pvertices->append({b, m});
					pvertices->append({c, m});
				}
			}
		}
	}}
}

CubeFieldIndices::CubeFieldIndices()
{
	if (NG_CubeFieldIndices)
		die("There can only be one CubeFieldIndices");
	NG_CubeFieldIndices = this;

	const int size = volume(CHUNK_SIZE)*6*6;
	indices.resize(size);
	for (int i = 0; i < size; i++)
		indices[i] = i;
}

CubeFieldIndices::~CubeFieldIndices()
{
	NG_CubeFieldIndices = nullptr;
}

CubeFieldIndices *NG_CubeFieldIndices = nullptr;
