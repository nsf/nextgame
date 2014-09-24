#pragma once

#include "Math/Vec.h"
#include "Math/Plane.h"
#include "Core/Vector.h"
#include "Geometry/VertexFormats.h"
#include "Geometry/Global.h"
#include "Geometry/CubeData.h"
#include "Geometry/RLE.h"
#include <cstdint>

enum Face {
	X_POS,
	X_NEG,
	Y_POS,
	Y_NEG,
	Z_POS,
	Z_NEG,
};

float ray_plane_intersection(const Plane &p, const Vec3 &orig, const Vec3 &dir);
float ray_plane_intersection_axis(float p, int axis, const Vec3 &orig, const Vec3 &dir);
Face ray_aabb_face(const Vec3 &min, const Vec3 &max,
	const Vec3 &orig, const Vec3 &dir, float *out = nullptr);

//------------------------------------------------------------------------------
// CubeField
//------------------------------------------------------------------------------

struct CubeField {
	Vector<CubeData> data;
	Vec3i size = Vec3i(0);

	CubeField() = default;
	CubeField(const Vec3i &size);

	void resize(const Vec3i &newsize);

	CubeData &get(const Vec3i &p) { return data[offset_3d(p, size)]; }
	const CubeData &get(const Vec3i &p) const { return data[offset_3d(p, size)]; }
};

//------------------------------------------------------------------------------
// CubeRLEField and CubeRLEIterator
//------------------------------------------------------------------------------

using CubeRLEIterator = RLEIterator<CubeData>;

struct CubeRLEField {
	Vector<CubeData> data;
	Vector<RLESeq> seqs;
	Vec3i size = Vec3i(0);

	CubeRLEField() = default;
	explicit CubeRLEField(const CubeField &field);

	void partial_decompress(Slice<CubeData> out,
		const Vec3i &from, const Vec3i &to) const;

	CubeRLEIterator iterator() const;
	CubeRLEIterator iterator(int start) const;
	CubeRLEIterator iterator(const Vec3i &start) const;
	int byte_length() const;
};

void cube_quad_field_to_mesh(Vector<V3M1_layer1> *pvertices, Slice<const CubeRLEField*> fields);
void cube_field_to_mesh(Vector<V3N3M1> &vertices, const CubeField &field);

// Global array of indices (0 1 2...), contains enough indices for all cube
// meshes. Used for bullet, because it doesn't support non-indexed meshes.
struct CubeFieldIndices {
	Vector<int> indices;

	NG_DELETE_COPY_AND_MOVE(CubeFieldIndices);
	CubeFieldIndices();
	~CubeFieldIndices();
};

extern CubeFieldIndices *NG_CubeFieldIndices;
