#include "Render/DDS.h"
#include "OS/IO.h"
#include "Core/Defer.h"
#include "Math/Vec.h"

enum DDSPFFlags : uint32_t {
	// Texture contains alpha data; 'a_bit_mask' contains valid data.
	PF_ALPHA_PIXELS = 0x1,

	// Used in some older DDS files for alpha channel only uncompressed data.
	PF_ALPHA = 0x2,

	// Texture contains compressed RGB data; 'four_cc' contains valid data.
	PF_FOUR_CC = 0x4,

	// Texture contains uncompressed RGB data; 'rgb_bit_count' and the RGB masks
	// ('r_bit_mask', 'g_bit_mask', 'b_bit_mask') contain valid data.
	PF_RGB = 0x40,

	// Used in some older DDS files for YUV uncompressed data.
	PF_YUV = 0x200,

	// Used in some older DDS files for single channel color uncompressed data.
	PF_LUMINANCE = 0x20000,

	// Custom flag used by nvidia-texture-tools.
	PF_NORMAL = 0x80000000,
};

struct DDSPixelFormat {
	uint32_t   size; // must be 32
	DDSPFFlags flags;

	// Four-character codes for specifying compressed or custom
	// formats. Possible values include: DXT1, DXT2, DXT3, DXT4, or DXT5.
	uint32_t four_cc;
	uint32_t rgb_bit_count;
	uint32_t r_bit_mask;
	uint32_t g_bit_mask;
	uint32_t b_bit_mask;
	uint32_t a_bit_mask;
};

enum DDSFlags : uint32_t {
	F_CAPS         = 0x1,      // Required in all files.
	F_HEIGHT       = 0x2,      // Required in all files.
	F_WIDTH        = 0x4,      // Required in all files.
	F_PITCH        = 0x8,      // When pitch is provided for uncompressed texture.
	F_PIXEL_FORMAT = 0x1000,   // Required in all files.
	F_MIPMAP_COUNT = 0x20000,  // Required in a mipmapped texture.
	F_LINEAR_SIZE  = 0x80000,  // When pitch is provided for compressed texture.
	F_DEPTH        = 0x800000, // Required in a depth texture.
};

enum DDSCaps : uint32_t {
	// Optional; must be used on any file that contains more than one surface (a
	// mipmap, a cubic environment map, or mipmapped volume texture).
	C_COMPLEX = 0x8,

	// Optional; should be used for a mipmap.
	C_MIPMAP  = 0x400000,

	// Required
	C_TEXTURE = 0x1000,
};


enum DDSCaps2 : uint32_t {
	C2_CUBEMAP            = 0x200,
	C2_CUBEMAP_POSITIVE_X = 0x400,
	C2_CUBEMAP_NEGATIVE_X = 0x800,
	C2_CUBEMAP_POSITIVE_Y = 0x1000,
	C2_CUBEMAP_NEGATIVE_Y = 0x2000,
	C2_CUBEMAP_POSITIVE_Z = 0x4000,
	C2_CUBEMAP_NEGATIVE_Z = 0x8000,
	C2_VOLUME             = 0x200000,
};

const uint32_t DDS_MAGIC = 0x20534444;

struct DDSHeader {
	uint32_t       magic;
	uint32_t       size; // must be 124
	DDSFlags       flags;
	uint32_t       height;
	uint32_t       width;

	// The pitch or number of bytes per scan line in an uncompressed texture;
	// the total number of bytes in the top level texture for a compressed
	// texture.
	uint32_t       pitch_or_linear_size;
	uint32_t       depth;
	uint32_t       mipmap_count;
	uint32_t       reserved1[11];
	DDSPixelFormat pixel_format;
	DDSCaps        caps;
	DDSCaps2       caps2;
	uint32_t       caps3;
	uint32_t       caps4;
	uint32_t       reserved2;
};

// Loads texture data from a given location, on failure returns an empty vector.
// Also does additional checks and returns a pointer to the DDS header. The
// pointer points into the data returned in the vector.
static Vector<uint8_t> load_dds_texture(const char *filename,
	DDSHeader **hout, Error *err)
{
	auto data = IO::read_file(filename, err);
	if (*err)
		return {};

	DDSHeader *hptr = (DDSHeader*)data.data();
	if (hptr->magic != DDS_MAGIC) {
		err->set("DDS loader failure (%s): bad magic number", filename);
		return {};
	}

	if (hptr->size != 124) {
		err->set("DDS loader failure (%s): bad size number", filename);
		return {};
	}

	*hout = hptr;
	return data;
}

static inline uint32_t make_four_cc(const char *s)
{
	return
		((uint32_t)s[3] << 24) |
		((uint32_t)s[2] << 16) |
		((uint32_t)s[1] << 8)  |
		((uint32_t)s[0] << 0)  ;
}

Texture load_texture_array_from_dds_files(Slice<String> filenames, Error *err)
{
	if (filenames.length == 0) {
		err->set("One must provide at least one file for a texture array");
		return Texture();
	}

	GLTexture id;
	glGenTextures(1, &id);
	NG_ASSERT(id != 0);

	glBindTexture(GL_TEXTURE_2D_ARRAY, id);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_R, GL_REPEAT);

	DDSHeader *header = nullptr;
	auto data = load_dds_texture(filenames[0].c_str(), &header, err);
	if (*err) {
		return Texture();
	}

	if ((header->pixel_format.flags & PF_FOUR_CC) == 0) {
		err->set("DDS loader failure (%s): only compressed images are supported",
			filenames[0].c_str());
		return Texture();
	}

	int minsize = 1;

	// figure out the format
	GLenum internal_format;
	if (header->pixel_format.four_cc == make_four_cc("DXT1")) {
		internal_format = GL_COMPRESSED_SRGB_S3TC_DXT1_EXT;
		minsize = 8;
	} else if (header->pixel_format.four_cc == make_four_cc("DXT5")) {
		if (header->pixel_format.flags & PF_NORMAL)
			internal_format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
		else
			internal_format = GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT;
		minsize = 16;
	} else if (header->pixel_format.four_cc == make_four_cc("ATI1")) {
		internal_format = GL_COMPRESSED_RED_RGTC1;
		minsize = 8;
	} else if (header->pixel_format.four_cc == make_four_cc("ATI2")) {
		internal_format = GL_COMPRESSED_RG_RGTC2;
		minsize = 16;
	} else {
		err->set("Unsupported pixel format");
		return Texture();
	}

	// allocate storage
	const int mipmap_count = header->mipmap_count;
	const int bytesize = header->pitch_or_linear_size;
	const Vec2i size {(int)header->width, (int)header->height};

	int current_byte_size = bytesize;
	Vec2i current_size = size;
	for (int i = 0; i < mipmap_count; i++) {
		glCompressedTexImage3D(GL_TEXTURE_2D_ARRAY, i, internal_format,
			current_size.x, current_size.y, filenames.length,
			0, current_byte_size * filenames.length, nullptr);
		current_byte_size = max(current_byte_size / 4, minsize);
		current_size /= Vec2i(2);
	}

	for (int i = 0; i < filenames.length; i++) {
		if (i != 0) {
			data = load_dds_texture(filenames[i].c_str(), &header, err);
			if (*err)
				return Texture();
		}

		if ((int)header->width != size.x || (int)header->height != size.y) {
			err->set("Images of the same size are expected within a texture array");
			return Texture();
		}

		int offset = sizeof(DDSHeader);
		auto subdata = data.sub(offset);
		current_byte_size = bytesize;
		current_size = size;
		for (int j = 0; j < mipmap_count; j++) {
			glCompressedTexSubImage3D(GL_TEXTURE_2D_ARRAY, j,
				0, 0, i, current_size.x, current_size.y, 1,
				internal_format, current_byte_size,
				subdata.data);
			offset += current_byte_size;
			subdata = data.sub(offset);
			current_byte_size = max(current_byte_size / 4, minsize);
			current_size /= Vec2i(2);
		}
	}

	return {std::move(id), GL_TEXTURE_2D_ARRAY};
}
