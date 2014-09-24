#include "Render/Meshes.h"
#include "Core/Defer.h"
#include <utility>
#include <cmath>

VertexArray create_layer1_mesh(Slice<const V3M1_layer1> vertices)
{
	auto va = VertexArray(GL_STATIC_DRAW);
	va.bind();
	va.vbo.upload(slice_cast<const uint8_t>(vertices.sub()));
	glEnableVertexAttribArray(Shader::POSITION);
	glEnableVertexAttribArray(Shader::MATERIAL);
	glVertexAttribPointer(Shader::POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(V3M1_layer1),
		voidp_offsetof(V3M1_layer1, position));
	glVertexAttribIPointer(Shader::MATERIAL, 1, GL_UNSIGNED_INT, sizeof(V3M1_layer1),
		voidp_offsetof(V3M1_layer1, material));
	va.n = vertices.length;
	return va;
}

VertexArray create_cube_field_mesh(Slice<const V3N3M1> vertices)
{
	auto va = VertexArray(GL_STATIC_DRAW);
	va.bind();
	va.vbo.upload(slice_cast<const uint8_t>(vertices.sub()));
	glEnableVertexAttribArray(Shader::POSITION);
	glEnableVertexAttribArray(Shader::NORMAL);
	glEnableVertexAttribArray(Shader::MATERIAL);
	glVertexAttribPointer(Shader::POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(V3N3M1),
		voidp_offsetof(V3N3M1, position));
	glVertexAttribPointer(Shader::NORMAL, 3, GL_FLOAT, GL_FALSE, sizeof(V3N3M1),
		voidp_offsetof(V3N3M1, normal));
	glVertexAttribIPointer(Shader::MATERIAL, 1, GL_UNSIGNED_INT, sizeof(V3N3M1),
		voidp_offsetof(V3N3M1, material));
	va.n = vertices.length;
	return va;
}

VertexArray create_hermite_field_mesh(Slice<const V3N3M1_terrain> vertices,
	Slice<const uint32_t> indices)
{
	auto va = VertexArray(GL_STATIC_DRAW);
	va.bind();
	va.vbo.upload(slice_cast<const uint8_t>(vertices.sub()));
	va.ibo.upload(slice_cast<const uint8_t>(indices.sub()));
	glEnableVertexAttribArray(Shader::POSITION);
	glEnableVertexAttribArray(Shader::NORMAL);
	glEnableVertexAttribArray(Shader::MATERIAL);
	glVertexAttribPointer(Shader::POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(V3N3M1_terrain),
		voidp_offsetof(V3N3M1_terrain, position));
	glVertexAttribPointer(Shader::NORMAL, 4, GL_INT_2_10_10_10_REV, GL_TRUE, sizeof(V3N3M1_terrain),
		voidp_offsetof(V3N3M1_terrain, normal));
	glVertexAttribIPointer(Shader::MATERIAL, 1, GL_UNSIGNED_BYTE, sizeof(V3N3M1_terrain),
		voidp_offsetof(V3N3M1_terrain, material));
	va.n = indices.length;
	return va;
}

VertexArray create_debug_mesh(Slice<const V3C3> vertices,
	Slice<const uint32_t> indices)
{
	auto va = VertexArray(GL_STATIC_DRAW);
	va.bind();
	va.vbo.upload(slice_cast<const uint8_t>(vertices.sub()));
	va.ibo.upload(slice_cast<const uint8_t>(indices.sub()));
	glEnableVertexAttribArray(Shader::POSITION);
	glEnableVertexAttribArray(Shader::COLOR);
	glVertexAttribPointer(Shader::POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(V3C3),
		voidp_offsetof(V3C3, position));
	glVertexAttribPointer(Shader::COLOR, 3, GL_FLOAT, GL_FALSE, sizeof(V3C3),
		voidp_offsetof(V3C3, color));
	va.n = indices.length;
	return va;
}

VertexArray create_sky_dome_mesh(int hres, int vres, float radius)
{
	const float hstep = MATH_2PI / hres;
	const float vstep = MATH_HALF_PI / vres;

	Vector<Vec3> verts((1 + hres * vres) * 1);
	int vi = 0;

	// top point of the sphere
	verts[vi++] = {0, 0, radius};

	// upper half of the sphere
	for (int j = vres-1; j >= 0; j--) {
	for (int i = 0; i < hres; i++) {
		const float hstepi = hstep * i;
		const float vstepj = vstep * j;
		const float hr = radius * 8.0f;
		const float vr = radius * 2.0f;
		verts[vi++] = Vec3(
			 cosf(hstepi) * cosf(vstepj) * hr,
			-sinf(hstepi) * cosf(vstepj) * hr,
			 sinf(vstepj) * vr
		) - Vec3_Z(1);
	}}

	Vector<uint32_t> inds((hres * 3 + (hres * (vres-1) * 6)) * 1);
	int ii = 0;

	// top cap of the sphere
	for (int i = 0; i < hres; i++) {
		inds[ii++] = 0;
		inds[ii++] = i+1;
		inds[ii++] = i+2;
	}
	inds[ii-1] = 1; // loop vertex

	// upper half of the sphere
	for (int j = 0; j < vres-1; j++) {
	for (int i = 0; i < hres; i++) {
		const int base = 1;
		int n = 1;
		if (i == hres-1)
			n = -i;
		const int tl = base + j*hres + i;
		const int tr = tl+n;
		const int bl = tl+hres;
		const int br = tr+hres;
		inds[ii++] = tl;
		inds[ii++] = bl;
		inds[ii++] = br;
		inds[ii++] = tr;
		inds[ii++] = tl;
		inds[ii++] = br;
	}}

	auto va = VertexArray(GL_STATIC_DRAW);
	va.bind();
	va.vbo.upload(slice_cast<const uint8_t>(verts.sub()));
	va.ibo.upload(slice_cast<const uint8_t>(inds.sub()));
	glEnableVertexAttribArray(Shader::POSITION);
	glVertexAttribPointer(Shader::POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(Vec3), 0);
	va.n = inds.length();
	return va;
}

VertexArray create_sphere_mesh(int subdiv, float radius, bool ccw_winding)
{
	const float t = (1.0f + sqrtf(5.0f)) / 2.0f;
	const float inv_s = 1.0f / sqrtf(1 + t*t);

	struct Edge {
		int v0, v1;

		// when edge is being split, these are set to edge indices within edges2
		// array
		int e0, e1;
		Edge() = default;
		Edge(int v0, int v1): v0(v0), v1(v1) {}
	};

	struct Triangle {
		int edge[3];
		bool positive[3];

		Triangle() = default;
		Triangle(int a, int b, int c, bool d, bool e, bool f) {
			edge[0] = a;
			edge[1] = b;
			edge[2] = c;
			positive[0] = d;
			positive[1] = e;
			positive[2] = f;
		}
	};

	Vector<Vec3> verts(12);
	Vector<uint32_t> inds;
	Vector<Edge> edges;
	Vector<Edge> edges2;
	Vector<Triangle> tris;
	Vector<Triangle> tris2;

	auto split_edge = [&](Edge &e) {
		int newv = verts.length();
		verts.append((verts[e.v0] + verts[e.v1]) / Vec3(2));
		verts[newv] = normalize(verts[newv]);
		e.e0 = edges2.length();
		edges2.append({e.v0, newv});
		e.e1 = edges2.length();
		edges2.append({newv, e.v1});
	};

	auto split_triangle = [&](const Triangle &t) {
		int v0 = edges2[edges[t.edge[0]].e0].v1;
		int v1 = edges2[edges[t.edge[1]].e0].v1;
		int v2 = edges2[edges[t.edge[2]].e0].v1;

		int e0 = edges2.length();
		edges2.append({v0, v1});
		int e1 = edges2.length();
		edges2.append({v1, v2});
		int e2 = edges2.length();
		edges2.append({v2, v0});

		tris2.append({e0, e1, e2, true, true, true});

		bool p0 = t.positive[0];
		bool p1 = t.positive[1];
		bool p2 = t.positive[2];
		const Edge &te0 = edges[t.edge[0]];
		const Edge &te1 = edges[t.edge[1]];
		const Edge &te2 = edges[t.edge[2]];
		tris2.append({p0 ? te0.e0 : te0.e1, e2, p2 ? te2.e1 : te2.e0,
			p0, false, p2});
		tris2.append({p0 ? te0.e1 : te0.e0, p1 ? te1.e0 : te1.e1, e0,
			p0, p1, false});
		tris2.append({e1, p1 ? te1.e1 : te1.e0, p2 ? te2.e0 : te2.e1,
			false, p1, p2});
	};

	verts[0]  = { t,  1,  0};
	verts[1]  = {-t,  1,  0};
	verts[2]  = { t, -1,  0};
	verts[3]  = {-t, -1,  0};
	verts[4]  = { 1,  0,  t};
	verts[5]  = { 1,  0, -t};
	verts[6]  = {-1,  0,  t};
	verts[7]  = {-1,  0, -t};
	verts[8]  = { 0,  t,  1};
	verts[9]  = { 0, -t,  1};
	verts[10] = { 0,  t, -1};
	verts[11] = { 0, -t, -1};
	for (auto &v : verts)
		v *= Vec3(inv_s);

	edges.append({
		Edge(0, 8),
		Edge(0, 5),
		Edge(0, 4),
		Edge(0, 10),
		Edge(0, 2),
		Edge(1, 10),
		Edge(1, 7),
		Edge(1, 6),
		Edge(1, 8),
		Edge(1, 3),
		Edge(2, 9),
		Edge(2, 4),
		Edge(2, 11),
		Edge(2, 5),
		Edge(3, 9),
		Edge(3, 11),
		Edge(3, 7),
		Edge(3, 6),
		Edge(4, 8),
		Edge(4, 9),
		Edge(4, 6),
		Edge(5, 10),
		Edge(5, 7),
		Edge(5, 11),
		Edge(6, 9),
		Edge(6, 8),
		Edge(7, 11),
		Edge(7, 10),
		Edge(8, 10),
		Edge(9, 11),
	});

	tris.append({
		Triangle( 0, 18,  2, true,  false, false),
		Triangle( 5, 27,  6, true,  false, false),
		Triangle(10, 29, 12, true,  true,  false),
		Triangle(16,  9,  6, false, false, true),
		Triangle( 1, 21,  3, true,  true,  false),
		Triangle(14, 24, 17, true,  false, false),
		Triangle(15, 29, 14, true,  false, false),
		Triangle(25, 20, 18, false, false, true),
		Triangle(11, 19, 10, true,  true,  false),
		Triangle(16, 26, 15, true,  true,  false),
		Triangle(11,  4,  2, false, false, true),
		Triangle(19, 20, 24, false, true,  true),
		Triangle(12, 23, 13, true,  false, false),
		Triangle( 3, 28,  0, true,  false, false),
		Triangle( 1,  4, 13, false, true,  true),
		Triangle(21, 22, 27, false, true,  true),
		Triangle( 7, 25,  8, true,  true,  false),
		Triangle( 8, 28,  5, true,  true,  false),
		Triangle( 7, 9,  17, false, true,  true),
		Triangle(26, 22, 23, false, false, true),
	});

	for (int i = 0; i < subdiv; i++) {
		for (auto &e : edges)
			split_edge(e);
		for (const auto &t : tris)
			split_triangle(t);
		std::swap(edges, edges2);
		std::swap(tris, tris2);
		edges2.clear();
		tris2.clear();
	}

	for (const auto &t : tris) {
		for (int i = 0; i < 3; i++) {
			const Edge &e = edges[t.edge[i]];
			if (t.positive[i])
				inds.append(e.v0);
			else
				inds.append(e.v1);
		}
	}
	if (ccw_winding) {
		for (int i = 0; i < inds.length(); i += 3)
			std::swap(inds[i+1], inds[i+2]);
	}
	for (Vec3 &v : verts)
		v *= Vec3(radius);

	auto va = VertexArray(GL_STATIC_DRAW);
	va.bind();
	va.vbo.upload(slice_cast<const uint8_t>(verts.sub()));
	va.ibo.upload(slice_cast<const uint8_t>(inds.sub()));
	glEnableVertexAttribArray(Shader::POSITION);
	glVertexAttribPointer(Shader::POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(Vec3), 0);
	va.n = inds.length();
	return va;
}

VertexArray create_pixels_mesh(int w, int h)
{
	struct Vec2s { short x, y; };
	Vector<Vec2s> vertices;
	for (int y = 0; y < h; y++) {
	for (int x = 0; x < w; x++) {
		vertices.append({(short)x, (short)y});
	}}

	auto va = VertexArray(GL_STATIC_DRAW);
	va.bind();
	va.vbo.upload(slice_cast<const uint8_t>(vertices.sub()));
	glEnableVertexAttribArray(Shader::POSITION);
	glVertexAttribIPointer(Shader::POSITION, 1, GL_SHORT, sizeof(Vec2s), 0);
	va.n = vertices.length();
	return va;
}

VertexArray create_quad_stream()
{
	auto vao = VertexArray(GL_STREAM_DRAW);
	vao.bind();
	vao.vbo.bind();
	glEnableVertexAttribArray(Shader::POSITION);
	glEnableVertexAttribArray(Shader::TEXCOORD);
	glEnableVertexAttribArray(Shader::COLOR);
	glVertexAttribPointer(Shader::POSITION, 2, GL_FLOAT, GL_FALSE, sizeof(V2T2C3),
		voidp_offsetof(V2T2C3, position));
	glVertexAttribPointer(Shader::TEXCOORD, 2, GL_FLOAT, GL_FALSE, sizeof(V2T2C3),
		voidp_offsetof(V2T2C3, texcoord));
	glVertexAttribPointer(Shader::COLOR, 3, GL_FLOAT, GL_FALSE, sizeof(V2T2C3),
		voidp_offsetof(V2T2C3, color));
	return vao;
}

void upload_quads(VertexArray &vao, const Slice<V2T2C3> quads)
{
	if (quads.length > 0 && vao.vbo.size >= quads.byte_length()) {
		vao.vbo.sub_upload(0, slice_cast<const uint8_t>(quads));
	} else if (quads.length > 0) {
		vao.vbo.upload(slice_cast<const uint8_t>(quads));
	}
	vao.n = quads.length;
}

void draw_quads(VertexArray &vao, const Slice<V2T2C3> quads)
{
	upload_quads(vao, quads);
	if (quads.length > 0) {
		vao.bind();
		glDrawArrays(GL_TRIANGLES, 0, vao.n);
	}
}

VertexArray create_quad_stream_alpha()
{
	auto vao = VertexArray(GL_STREAM_DRAW);
	vao.bind();
	vao.vbo.bind();
	glEnableVertexAttribArray(Shader::POSITION);
	glEnableVertexAttribArray(Shader::TEXCOORD);
	glEnableVertexAttribArray(Shader::COLOR);
	glVertexAttribPointer(Shader::POSITION, 2, GL_FLOAT, GL_FALSE, sizeof(V2T2C4),
		voidp_offsetof(V2T2C4, position));
	glVertexAttribPointer(Shader::TEXCOORD, 2, GL_FLOAT, GL_FALSE, sizeof(V2T2C4),
		voidp_offsetof(V2T2C4, texcoord));
	glVertexAttribPointer(Shader::COLOR, 4, GL_FLOAT, GL_FALSE, sizeof(V2T2C4),
		voidp_offsetof(V2T2C4, color));
	return vao;
}

void upload_quads(VertexArray &vao, const Slice<V2T2C4> quads)
{
	if (quads.length > 0 && vao.vbo.size >= quads.byte_length()) {
		vao.vbo.sub_upload(0, slice_cast<const uint8_t>(quads));
	} else if (quads.length > 0) {
		vao.vbo.upload(slice_cast<const uint8_t>(quads));
	}
	vao.n = quads.length;
}

void draw_quads(VertexArray &vao, const Slice<V2T2C4> quads)
{
	upload_quads(vao, quads);
	if (quads.length > 0) {
		vao.bind();
		glDrawArrays(GL_TRIANGLES, 0, vao.n);
	}
}

VertexArray create_vertex_stream()
{
	auto vao = VertexArray(GL_STREAM_DRAW);
	vao.bind();
	vao.vbo.bind();
	glEnableVertexAttribArray(Shader::POSITION);
	glVertexAttribPointer(Shader::POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(Vec3), nullptr);
	return vao;
}

void upload_vertices(VertexArray &vao, const Slice<Vec3> vertices)
{
	if (vertices.length == 0)
		return;

	if (vao.vbo.size >= vertices.byte_length()) {
		vao.vbo.sub_upload(0, slice_cast<const uint8_t>(vertices));
	} else {
		vao.vbo.upload(slice_cast<const uint8_t>(vertices));
	}
	vao.n = vertices.length;
}

void draw_vertices(VertexArray &vao, const Slice<Vec3> vertices)
{
	upload_vertices(vao, vertices);
	if (vertices.length > 0) {
		vao.bind();
		glDrawArrays(GL_TRIANGLES, 0, vao.n);
	}
}
