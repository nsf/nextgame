#include "Render/PNG.h"
#include "Core/Vector.h"
#include "Core/Defer.h"
#include <png.h>
#include <cstdint>

static void user_error(png_structp png_ptr, png_const_charp msg)
{
	auto err = (Error*)png_get_user_chunk_ptr(png_ptr);
	err->set("PNG loader failure: %s", msg);
	longjmp(png_jmpbuf(png_ptr), 1);
}

static void PNGAPI user_read(png_structp png_ptr, png_bytep data, png_size_t length)
{
	uint8_t **ptr = (uint8_t**)png_get_io_ptr(png_ptr);
	memcpy(data, *ptr, length);
	*ptr = *ptr + length;
}

Vector<uint8_t> load_data_from_png_memory(Slice<const uint8_t> p,
	int *w, int *h, Error *err)
{
	const png_byte *pngb = p.data;
	if (png_sig_cmp(pngb, 0, 8)) {
		err->set("PNG loader failure: invalid png signature");
		return {};
	}

	// skip signature
	pngb += 8;

	png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
		err, user_error, nullptr);
	if (!png_ptr) {
		err->set("PNG loader failure: cannot create png read struct");
		return {};
	}

	png_infop info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
		png_destroy_read_struct(&png_ptr, nullptr, nullptr);
		err->set("PNG loader failure: cannot create png info struct");
		return {};
	}
	Vector<uint8_t*> pointers;
	DEFER {
		png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
	};

	Vector<uint8_t> data;
	if (setjmp(png_jmpbuf(png_ptr)))
		return {};

	png_set_read_fn(png_ptr, &pngb, user_read);
	png_set_sig_bytes(png_ptr, 8);
	png_read_info(png_ptr, info_ptr);

	png_uint_32 width = 0;
	png_uint_32 height = 0;
	int bpp = 0;
	int colortype = 0;

	png_get_IHDR(png_ptr, info_ptr, &width, &height, &bpp, &colortype,
		nullptr, nullptr, nullptr);

	// basically, we convert here every possible variant to the 32-bit RGBA.
	if (colortype == PNG_COLOR_TYPE_PALETTE)
		png_set_palette_to_rgb(png_ptr);

	if (bpp < 8) {
		if (colortype == PNG_COLOR_TYPE_GRAY || colortype == PNG_COLOR_TYPE_GRAY_ALPHA)
			png_set_expand_gray_1_2_4_to_8(png_ptr);
		else
			png_set_packing(png_ptr);
	}

	if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
		png_set_tRNS_to_alpha(png_ptr);

	if (bpp == 16)
		png_set_strip_16(png_ptr);

	if (colortype == PNG_COLOR_TYPE_GRAY || colortype == PNG_COLOR_TYPE_GRAY_ALPHA)
		png_set_gray_to_rgb(png_ptr);

	if (colortype != PNG_COLOR_TYPE_RGBA)
		png_set_add_alpha(png_ptr, 0xFF, PNG_FILLER_AFTER);

	png_read_update_info(png_ptr, info_ptr);
	png_get_IHDR(png_ptr, info_ptr, &width, &height, &bpp, &colortype,
		nullptr, nullptr, nullptr);

	data.resize(width * height * 4);
	*w = width;
	*h = height;

	pointers.resize(height);
	uint8_t *buffer = data.data();
	for (png_uint_32 i = 0; i < height; ++i) {
		pointers[i] = buffer;
		buffer += width * 4;
	}

	png_read_image(png_ptr, pointers.data());
	png_read_end(png_ptr, nullptr);
	return data;
}

Texture load_texture_from_png_memory(Slice<const uint8_t> data, Error *err)
{
	int w, h;
	Vector<uint8_t> pngdata = load_data_from_png_memory(data, &w, &h, err);
	if (*err)
		return Texture();

	GLTexture id;
	glGenTextures(1, &id);
	NG_ASSERT(id != 0);
	glBindTexture(GL_TEXTURE_2D, id);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_REPEAT);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h,
		0, GL_RGBA, GL_UNSIGNED_BYTE, pngdata.data());
	return {std::move(id), GL_TEXTURE_2D};
}
