struct MaterialFace {
	float BaseColorTexture;
	float NormalTexture;
	float MetallicTexture;
	float RoughnessTexture;

	vec3 BaseColor;
	float Scale;

	float Metallic;
	float Roughness;
	float BumpScale;
	float _Pad1;
};

struct Material {
	MaterialFace Top;
	MaterialFace Side;
};

layout(std140) uniform Materials {
	Material M_Materials[256];
};
