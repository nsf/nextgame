uniform PerFrame {
	mat4 PF_ViewProjection;
	mat4 PF_ShadowViewProjection[4];
	mat4 PF_ShadowViewProjectionBiased[4];
	mat3 PF_InverseView;
	vec2 PF_HalfPlaneSize;
	vec3 PF_CameraPosition;
	vec2 PF_ProjectionRatio;
	float PF_AspectRatio;
};
