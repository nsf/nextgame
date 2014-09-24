#include "Render/DeferredShading.h"
#include "Core/Defer.h"

DeferredShading::DeferredShading(const Vec2i &newsize)
{
	sphere = create_sphere_mesh(3, 1);
	inv_sphere = create_sphere_mesh(3, 1, true);
	quad_stream = create_quad_stream();
	vertex_stream = create_vertex_stream();

	// G-Buffer
	gbuffer.bind();
	const GLenum draw_buffers[] = {
		GL_COLOR_ATTACHMENT0,
		GL_COLOR_ATTACHMENT1,
		GL_COLOR_ATTACHMENT2,
	};
	glDrawBuffers(3, draw_buffers);

	// Shadow map buffers
	shadow_map_rt0 = Texture_DepthArray(1024, 1024, 4, GL_DEPTH_COMPONENT16);
	for (int i = 0; i < 4; i++) {
		shadow_map[i].bind();
		glDrawBuffer(GL_NONE);
		shadow_map[i].attach_layer(GL_DEPTH_ATTACHMENT, shadow_map_rt0, i);
		shadow_map[i].validate();
	}

	set_size(newsize);
}

void DeferredShading::set_size(const Vec2i &newsize)
{
	if (size == newsize)
		return;
	size = newsize;

	gbuf_rt0 = Texture(size.x, size.y, GL_RGBA8);
	gbuf_rt1 = Texture(size.x, size.y, GL_RGBA8);
	gbuf_rt2 = Texture(size.x, size.y, GL_RGB10_A2);
	gbuf_depth = Texture_Depth(size.x, size.y, GL_DEPTH24_STENCIL8);
	hdr_rt0 = Texture_Linear(size.x, size.y, GL_RGB16F);
	for (auto &tex : bright_pass_tex)
		tex = Texture_Linear(size.x/2, size.y/2, GL_RGB16F);
	sun_shadow_rt0 = Texture(size.x, size.y, GL_R8);

	// BaseColor RGB, Metallic
	// Roughness, (3 empty slots here)
	// Normal
	gbuffer.attach(GL_COLOR_ATTACHMENT0, gbuf_rt0);
	gbuffer.attach(GL_COLOR_ATTACHMENT1, gbuf_rt1);
	gbuffer.attach(GL_COLOR_ATTACHMENT2, gbuf_rt2);
	gbuffer.attach(GL_DEPTH_ATTACHMENT, gbuf_depth);
	gbuffer.validate();

	hdr.attach(GL_COLOR_ATTACHMENT0, hdr_rt0);
	hdr.attach(GL_DEPTH_STENCIL_ATTACHMENT, gbuf_depth);
	hdr.validate();

	for (int i = 0; i < 2; i++) {
		bright_pass[i].attach(GL_COLOR_ATTACHMENT0, bright_pass_tex[i]);
		bright_pass[i].validate();
	}

	sun_shadow.attach(GL_COLOR_ATTACHMENT0, sun_shadow_rt0);
	sun_shadow.attach(GL_DEPTH_STENCIL_ATTACHMENT, gbuf_depth);
	sun_shadow.validate();
}

void DeferredShading::draw_shadow_map_stage(Func<void (int)> cb, float factor, float units)
{
	glEnable(GL_POLYGON_OFFSET_FILL);
	glDisable(GL_CULL_FACE);
	glViewport(0, 0, 1024, 1024);
	glPolygonOffset(factor, units);
	DEFER {
		glViewport(0, 0, size.x, size.y);
		glDisable(GL_POLYGON_OFFSET_FILL);
		glEnable(GL_CULL_FACE);
	};
	for (int i = 0; i < 4; i++) {
		shadow_map[i].bind();
		cb(i);
	}
}

void DeferredShading::draw_gbuffer_stage(Func<void ()> cb)
{
	gbuffer.bind();
	glClear(GL_DEPTH_BUFFER_BIT);
	cb();
}

void DeferredShading::draw_lighting_stage(Func<void ()> cb)
{
	glEnable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);
	DEFER {
		glDisable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);
		glDepthMask(GL_TRUE);
	};
	glBlendFunc(GL_ONE, GL_ONE);
	hdr.bind();
	cb();
}

void DeferredShading::draw_sky_stage(Func<void ()> cb)
{
	glDepthMask(GL_FALSE);
	glDepthFunc(GL_EQUAL);
	DEFER {
		glDepthMask(GL_TRUE);
		glDepthFunc(GL_LESS);
	};
	hdr.bind();
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);
	cb();
}

void DeferredShading::bright_pass_stage(Func<void ()> cb, int i)
{
	bright_pass[i].bind();
	glDisable(GL_DEPTH_TEST);
	glViewport(0, 0, size.x/2, size.y/2);
	DEFER {
		glEnable(GL_DEPTH_TEST);
		glViewport(0, 0, size.x, size.y);
	};
	cb();
}

void DeferredShading::draw_post_process_stage(Func<void ()> cb)
{
	glDisable(GL_DEPTH_TEST);
	DEFER {
		glEnable(GL_DEPTH_TEST);
	};
	unbind_framebuffer();
	cb();
}

void DeferredShading::draw_transparent_stage(Func<void ()> cb)
{
	glEnable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	DEFER {
		glDisable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);
	};
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	unbind_framebuffer();
	cb();
}

void DeferredShading::mark_frustums(Slice<const Frustum> fs)
{
	glEnable(GL_STENCIL_TEST);
	glDisable(GL_CULL_FACE);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	glDepthMask(GL_FALSE);
	DEFER {
		glDisable(GL_STENCIL_TEST);
		glEnable(GL_CULL_FACE);
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		glDepthMask(GL_TRUE);
	};
	glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_REPLACE, GL_KEEP);
	glStencilOpSeparate(GL_BACK, GL_KEEP, GL_KEEP, GL_KEEP);

	sun_shadow.bind();
	glClear(GL_STENCIL_BUFFER_BIT);

	BIND_SHADER(stencil);
	for (int i = fs.length-1; i >= 0; i--) {
		const Frustum &f = fs[i];
		glStencilFunc(GL_ALWAYS, i+1, 0xFF);
		draw_frustum(f);
	}
}

void DeferredShading::draw_sun_shadows(Func<void (int)> cb)
{
	glEnable(GL_STENCIL_TEST);
	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);
	DEFER {
		glDisable(GL_STENCIL_TEST);
		glEnable(GL_DEPTH_TEST);
		glDepthMask(GL_TRUE);
	};

	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
	sun_shadow.bind();
	glClearColor(1, 1, 1, 1);
	glClear(GL_COLOR_BUFFER_BIT);
	for (int i = 0; i < 4; i++) {
		glStencilFunc(GL_EQUAL, i+1, 0xFF);
		cb(i);
	}
}

void DeferredShading::draw_sun_shadows_debug(Func<void (int)> cb)
{
	glEnable(GL_BLEND);
	glEnable(GL_STENCIL_TEST);
	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);
	DEFER {
		glDisable(GL_BLEND);
		glDisable(GL_STENCIL_TEST);
		glEnable(GL_DEPTH_TEST);
		glDepthMask(GL_TRUE);
	};

	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
	glBlendFunc(GL_ONE, GL_ONE);
	hdr.bind();
	for (int i = 0; i < 4; i++) {
		glStencilFunc(GL_EQUAL, i+1, 0xFF);
		cb(i);
	}
}

void DeferredShading::draw_box(const Vec3 &min, const Vec3 &max)
{
	const Vec3 vs[] = {
		Vec3(min.x, min.y, min.z),
		Vec3(max.x, min.y, min.z),
		Vec3(min.x, max.y, min.z),
		Vec3(max.x, max.y, min.z),
		Vec3(min.x, min.y, max.z),
		Vec3(max.x, min.y, max.z),
		Vec3(min.x, max.y, max.z),
		Vec3(max.x, max.y, max.z),
	};

	// quad vertices
	// a b
	// c d
	// (when quad normal looks towards viewer)
	auto quad = [&](const Vec3 &a, const Vec3 &b, const Vec3 &c, const Vec3 &d) {
		vertex_buf.append({b, a, c});
		vertex_buf.append({d, b, c});
	};

	vertex_buf.clear();
	quad(vs[5], vs[4], vs[1], vs[0]);
	quad(vs[4], vs[6], vs[0], vs[2]);
	quad(vs[6], vs[7], vs[2], vs[3]);
	quad(vs[7], vs[5], vs[3], vs[1]);
	quad(vs[7], vs[6], vs[5], vs[4]);
	quad(vs[1], vs[0], vs[3], vs[2]);

	quad(vs[4], vs[5], vs[0], vs[1]);
	quad(vs[6], vs[4], vs[2], vs[0]);
	quad(vs[7], vs[6], vs[3], vs[2]);
	quad(vs[5], vs[7], vs[1], vs[3]);
	quad(vs[6], vs[7], vs[4], vs[5]);
	quad(vs[0], vs[1], vs[2], vs[3]);

	draw_vertices(vertex_stream, vertex_buf);
}

void DeferredShading::draw_frustum(const Frustum &f)
{
	Vec3 far[4];
	copy(Slice<Vec3>(far), Slice<const Vec3>(f.far));
	Vec3 near[4];
	copy(Slice<Vec3>(near), Slice<const Vec3>(f.near));
	Vec3 middle(0);
	for (int i = 0; i < 4; i++) {
		middle += near[i];
		middle += far[i];
	}
	middle /= Vec3(8);

	for (int i = 0; i < 4; i++) {
		Vec3 tmp;

		tmp = near[i] - middle;
		tmp *= Vec3(0.9);
		near[i] = middle + tmp;

		tmp = far[i] - middle;
		tmp *= Vec3(0.9);
		far[i] = middle + tmp;
	}

	/*
	for (int i = 0; i < 4; i++) {
		const Vec3 dir = Normalize(near[i] - far[i]);
		near[i] = far[i] + dir * Vec3(1500.0);
	}
	*/
	vertex_buf.clear();
	// quad vertices
	// a b
	// c d
	// (when quad normal looks towards viewer)
	auto quad = [&](const Vec3 &a, const Vec3 &b, const Vec3 &c, const Vec3 &d) {
		vertex_buf.append({b, a, c});
		vertex_buf.append({d, b, c});
	};

	// far
	quad(far[FPC_TOP_LEFT], far[FPC_TOP_RIGHT],
		far[FPC_BOTTOM_LEFT], far[FPC_BOTTOM_RIGHT]);
	// near
	quad(near[FPC_TOP_RIGHT], near[FPC_TOP_LEFT],
		near[FPC_BOTTOM_RIGHT], far[FPC_BOTTOM_LEFT]);
	// left
	quad(near[FPC_TOP_LEFT], far[FPC_TOP_LEFT],
		near[FPC_BOTTOM_LEFT], far[FPC_BOTTOM_LEFT]);
	// right
	quad(far[FPC_TOP_RIGHT], near[FPC_TOP_RIGHT],
		far[FPC_BOTTOM_RIGHT], near[FPC_BOTTOM_RIGHT]);
	// bottom
	quad(far[FPC_BOTTOM_LEFT], far[FPC_BOTTOM_RIGHT],
		near[FPC_BOTTOM_LEFT], near[FPC_BOTTOM_RIGHT]);
	// top
	quad(near[FPC_TOP_LEFT], near[FPC_TOP_RIGHT],
		far[FPC_TOP_LEFT], far[FPC_TOP_RIGHT]);
	draw_vertices(vertex_stream, vertex_buf);
}

void DeferredShading::draw_fullscreen_quad()
{
	quad_buf.clear();
	append_quad(quad_buf, Rect(-1, 1, 2, -2));
	draw_quads(quad_stream, quad_buf);
}
