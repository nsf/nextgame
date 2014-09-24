#pragma once

#include "Geometry/VertexFormats.h"
#include "Render/OpenGL.h"

VertexArray create_layer1_mesh(Slice<const V3M1_layer1> vertices);

VertexArray create_cube_field_mesh(Slice<const V3N3M1> vertices);
VertexArray create_hermite_field_mesh(Slice<const V3N3M1_terrain> vertices,
	Slice<const uint32_t> indices);
VertexArray create_debug_mesh(Slice<const V3C3> vertices,
	Slice<const uint32_t> indices);
VertexArray create_sky_dome_mesh(int hres, int vres, float radius);
VertexArray create_sphere_mesh(int subdiv, float radius, bool ccw_winding = false);
VertexArray create_pixels_mesh(int w, int h);

VertexArray create_quad_stream();
void upload_quads(VertexArray &vao, const Slice<V2T2C3> quads);
void draw_quads(VertexArray &vao, const Slice<V2T2C3> quads);

VertexArray create_quad_stream_alpha();
void upload_quads(VertexArray &vao, const Slice<V2T2C4> quads);
void draw_quads(VertexArray &vao, const Slice<V2T2C4> quads);

VertexArray create_vertex_stream();
void upload_vertices(VertexArray &vao, const Slice<Vec3> vertices);
void draw_vertices(VertexArray &vao, const Slice<Vec3> vertices);
