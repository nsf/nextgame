#pragma once

#include "Math/Vec.h"
#include "Core/String.h"
#include "Render/OpenGL.h"
#include <cstdint>

struct MaterialFace {
	float base_color_texture;
	float normal_texture;
	float metallic_texture;
	float roughness_texture;

	Vec3 base_color;
	float scale;

	float metallic;
	float roughness;
	float bump_scale;
	uint32_t _pad1;
};

static_assert(sizeof(MaterialFace) == 4*12, "");

struct Material {
	MaterialFace top;
	MaterialFace side;
};

// Doing static asserts there, just to make sure I'm not going out of the limit.
// Uniform buffers can handle only 64 4-byte parameters per material
// (assuming 256 materials). In other words, the typical limit is 64kb of memory.
static_assert(sizeof(Material) == 4*24, "");

struct MaterialBuffer {
	Buffer buffer;
	Vector<Material> materials;
	Vector<String> bc1_textures; // DXT1 aka BC1 - RGB textures (color)
	Vector<String> bc4_textures; // ATI1 aka BC4 - R textures (roughness, metallic)
	Vector<String> bc5_textures; // 3Dc/ATI2 aka BC5 - RG textures (normal)

	MaterialBuffer();

	void upload_materials();
	void dump(int n) const;
};
