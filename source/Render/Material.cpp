#include "Render/Material.h"

MaterialBuffer::MaterialBuffer(): materials(256)
{
	buffer = Buffer(GL_UNIFORM_BUFFER, GL_STATIC_DRAW);
	buffer.reserve(materials.byte_length());
}

void MaterialBuffer::upload_materials()
{
	buffer.sub_upload(0, slice_cast<const uint8_t>(materials.sub()));
}

void MaterialBuffer::dump(int n) const
{
	printf("Material Pack dump ---------------------------------------------\n");
	printf("BC1 Textures:\n");
	for (const String &s : bc1_textures)
		printf(" - %s\n", s.c_str());
	printf("BC4 Textures:\n");
	for (const String &s : bc4_textures)
		printf(" - %s\n", s.c_str());
	printf("BC5 Textures:\n");
	for (const String &s : bc5_textures)
		printf(" - %s\n", s.c_str());
	auto dump_face = [](const MaterialFace &face) {
		printf(" - Base Color Texture: %f\n", face.base_color_texture);
		printf(" - Normal Texture: %f\n", face.normal_texture);
		printf(" - Metallic Texture: %f\n", face.metallic_texture);
		printf(" - Roughness Texture: %f\n", face.roughness_texture);
		printf(" - Base Color: %f %f %f\n", VEC3(face.base_color));
		printf(" - Scale: %f\n", face.scale);
		printf(" - Metallic: %f\n", face.metallic);
		printf(" - Roughness: %f\n", face.roughness);
	};
	int i = 0;
	for (const Material &m : materials) {
		printf("Material %d\n", i++);
		printf("Top:\n");
		dump_face(m.top);
		printf("Side:\n");
		dump_face(m.side);
		if (i == n)
			break;
	}
	printf("----------------------------------------------------------------\n");
}
