#pragma once

#include "Math/Frustum.h"
#include "Render/ArHosekSkyModel.h"
#include "Geometry/Quads.h"
#include "Render/OpenGL.h"
#include "Render/Meshes.h"
#include "Core/Func.h"
#include "Math/Vec.h"

struct PointLight {
	Vec3 position;
	Vec3 color;
	float radius;
};

struct DeferredShading {
	Texture gbuf_rt0;
	Texture gbuf_rt1;
	Texture gbuf_rt2;
	Texture gbuf_depth;
	Framebuffer gbuffer;

	Texture hdr_rt0;
	Framebuffer hdr;

	Texture bright_pass_tex[2];
	Framebuffer bright_pass[2];

	Texture shadow_map_rt0;
	Framebuffer shadow_map[4];

	Texture sun_shadow_rt0;
	Framebuffer sun_shadow;

	Vector<V2T2C3> quad_buf;
	VertexArray quad_stream;
	VertexArray sphere;
	VertexArray inv_sphere;

	Vector<Vec3> vertex_buf;
	VertexArray vertex_stream;

	Vector<PointLight> point_lights;
	ArHosekSkyModelState sky_state;

	Vec2i size = Vec2i(-1);

	NG_DELETE_COPY_AND_MOVE(DeferredShading);
	DeferredShading(const Vec2i &size);

	void set_size(const Vec2i &size);

	void draw_shadow_map_stage(Func<void (int)> cb, float factor, float units);
	void draw_gbuffer_stage(Func<void ()> cb);
	void draw_lighting_stage(Func<void ()> cb);
	void draw_sky_stage(Func<void ()> cb);
	void bright_pass_stage(Func<void ()> cb, int i);
	void draw_post_process_stage(Func<void ()> cb);
	void draw_transparent_stage(Func<void ()> cb);

	void mark_frustums(Slice<const Frustum> fs);
	void draw_sun_shadows(Func<void (int)> cb);
	void draw_sun_shadows_debug(Func<void (int)> cb);

	void draw_box(const Vec3 &min, const Vec3 &max);
	void draw_frustum(const Frustum &f);
	void draw_fullscreen_quad();
};
