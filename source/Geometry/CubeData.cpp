#include "Geometry/CubeData.h"
#include "Core/Utils.h"

uint8_t CubeData::vertices_and_tri_config(int i, CubeVertex out_verts[]) const
{
	switch (i) {
	case 0:
		out_verts[0] = {edges[0].pos(), edges[5].neg(), edges[9].neg()};
		out_verts[1] = {edges[2].pos(), edges[7].neg(), edges[9].pos()};
		out_verts[2] = {edges[3].pos(), edges[7].pos(), edges[11].pos()};
		out_verts[3] = {edges[1].pos(), edges[5].pos(), edges[11].neg()};
		return
			((edges[5].length  != 0) << 3) |
			((edges[9].length  != 0) << 2) |
			((edges[7].length  != 0) << 1) |
			((edges[11].length != 0) << 0);
	case 1:
		out_verts[0] = {edges[1].neg(), edges[4].pos(), edges[10].neg()};
		out_verts[1] = {edges[3].neg(), edges[6].pos(), edges[10].pos()};
		out_verts[2] = {edges[2].neg(), edges[6].neg(), edges[8].pos()};
		out_verts[3] = {edges[0].neg(), edges[4].neg(), edges[8].neg()};
		return
			((edges[4].length  != 0) << 3) |
			((edges[10].length != 0) << 2) |
			((edges[6].length  != 0) << 1) |
			((edges[8].length  != 0) << 0);
	case 2:
		out_verts[0] =  {edges[1].pos(), edges[5].pos(), edges[11].neg()};
		out_verts[1] =  {edges[3].pos(), edges[7].pos(), edges[11].pos()};
		out_verts[2] = {edges[3].neg(), edges[6].pos(), edges[10].pos()};
		out_verts[3] = {edges[1].neg(), edges[4].pos(), edges[10].neg()};
		return
			((edges[1].length  != 0) << 3) |
			((edges[11].length != 0) << 2) |
			((edges[3].length  != 0) << 1) |
			((edges[10].length != 0) << 0);
	case 3:
		out_verts[0] = {edges[0].neg(), edges[4].neg(), edges[8].neg()};
		out_verts[1] = {edges[2].neg(), edges[6].neg(), edges[8].pos()};
		out_verts[2] = {edges[2].pos(), edges[7].neg(), edges[9].pos()};
		out_verts[3] = {edges[0].pos(), edges[5].neg(), edges[9].neg()};
		return
			((edges[0].length != 0) << 3) |
			((edges[8].length != 0) << 2) |
			((edges[2].length != 0) << 1) |
			((edges[9].length != 0) << 0);
	case 4:
		out_verts[0] = {edges[2].neg(), edges[6].neg(), edges[8].pos()};
		out_verts[1] = {edges[3].neg(), edges[6].pos(), edges[10].pos()};
		out_verts[2] = {edges[3].pos(), edges[7].pos(), edges[11].pos()};
		out_verts[3] = {edges[2].pos(), edges[7].neg(), edges[9].pos()};
		return
			((edges[2].length != 0) << 3) |
			((edges[6].length != 0) << 2) |
			((edges[3].length != 0) << 1) |
			((edges[7].length != 0) << 0);
	case 5:
		out_verts[0] = {edges[0].pos(), edges[5].neg(), edges[9].neg()};
		out_verts[1] = {edges[1].pos(), edges[5].pos(), edges[11].neg()};
		out_verts[2] = {edges[1].neg(), edges[4].pos(), edges[10].neg()};
		out_verts[3] = {edges[0].neg(), edges[4].neg(), edges[8].neg()};
		return
			((edges[0].length != 0) << 3) |
			((edges[5].length != 0) << 2) |
			((edges[1].length != 0) << 1) |
			((edges[4].length != 0) << 0);
	default:
		die("Cube has 6 faces only");
		return 0;
	}
}

bool CubeData::is_solid() const
{
	if (material == 0)
		return false;

	const CubeEdge solid_edge(0, 8);
	for (int i = 0; i < 12; i++) {
		if (edges[i] != solid_edge)
			return false;
	}
	return true;
}
