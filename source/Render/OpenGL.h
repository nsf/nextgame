#pragma once

#include "Core/Vector.h"
#include "Core/HashMap.h"
#include "Core/Error.h"
#include "Core/String.h"
#include "Core/UniquePtr.h"
#include "Math/Vec.h"
#include "Math/Mat.h"
#include <GL/glew.h>
#include <cstdint>

#define voidp_offsetof(type, member) \
	reinterpret_cast<void*>(offsetof(type, member))
#define voidp_offset(size) (void*)(size_t)(size)

#define DEFINE_GL_HANDLE(Name, Free)                         \
struct Name {                                                \
	GLuint id = 0;                                           \
	Name() = default;                                        \
	explicit Name(GLuint id): id(id) { NG_ASSERT(id != 0); } \
	Name(const Name&) = delete;                              \
	Name(Name &&r): id(r.id) { r.id = 0; }                   \
	Name &operator=(const Name&) = delete;                   \
	Name &operator=(Name &&r)                                \
	{                                                        \
		if (id) Free;                                        \
		id = r.id;                                           \
		r.id = 0;                                            \
		return *this;                                        \
	}                                                        \
	~Name() { if (id) Free; }                                \
	operator GLuint() const { return id; }                   \
	GLuint *operator&() { return &id; }                      \
};

DEFINE_GL_HANDLE(GLProgram, glDeleteProgram(id))
DEFINE_GL_HANDLE(GLShader, glDeleteShader(id))
DEFINE_GL_HANDLE(GLTexture, glDeleteTextures(1, &id))
DEFINE_GL_HANDLE(GLFramebuffer, glDeleteFramebuffers(1, &id))
DEFINE_GL_HANDLE(GLBuffer, glDeleteBuffers(1, &id))
DEFINE_GL_HANDLE(GLVertexArray, glDeleteVertexArrays(1, &id))

//----------------------------------------------------------------------
// Shader
//----------------------------------------------------------------------

struct ShaderSection {
	String name;
	String contents;
	Vector<String> files;
};

struct Uniform {
	bool valid = true;
	String name;
	GLint location = -1;

	void set_i(int v) const { glUniform1i(location, v); }
	void set(const Vec3i &v) const { glUniform3iv(location, 1, v.data); }
	void set(const Vec4i &v) const { glUniform4iv(location, 1, v.data); }

	void set(Slice<const float> v) const { glUniform1fv(location, v.length, v.data); }
	void set(float v) const { glUniform1f(location, v); }
	void set(const Vec2 &v) const { glUniform2fv(location, 1, v.data); }
	void set(const Vec3 &v) const { glUniform3fv(location, 1, v.data); }
	void set(const Vec4 &v) const { glUniform4fv(location, 1, v.data); }
	void set(const Mat3 &v) const { glUniformMatrix3fv(location, 1, GL_FALSE, v.data); }
	void set(const Mat4 &v) const { glUniformMatrix4fv(location, 1, GL_FALSE, v.data); }

	void resolve(GLuint programid);
};

struct UniformBlock {
	bool valid = true;
	String name;
	GLuint location = GL_INVALID_INDEX;

	void resolve(GLuint programid);
};

struct UniformInfo {
	String name;
	int offset;
	int length;
	GLenum type;
	int array_stride;
	int matrix_stride;

	template <typename T>
	void set_generic(Slice<uint8_t> dst, const T &v) const
	{
		uint8_t *p = dst.data + offset;
		*(T*)p = v;
	}

	template <typename T>
	void set_generic_array(Slice<uint8_t> dst, Slice<const T> src) const
	{
		NG_ASSERT(src.length <= length);
		uint8_t *p = dst.data + offset;
		for (const T &elem : src) {
			*(T*)p = elem;
			p += array_stride;
		}
	}

	void set(Slice<uint8_t> dst, const Vec3i &v) const { set_generic(dst, v); }
	void set(Slice<uint8_t> dst, const Vec4i &v) const { set_generic(dst, v); }

	void set(Slice<uint8_t> dst, float v) const       { set_generic(dst, v); }
	void set(Slice<uint8_t> dst, const Vec2 &v) const  { set_generic(dst, v); }
	void set(Slice<uint8_t> dst, const Vec3 &v) const { set_generic(dst, v); }
	void set(Slice<uint8_t> dst, const Vec4 &v) const { set_generic(dst, v); }
	void set(Slice<uint8_t> dst, const Mat3 &v) const
	{
		const uint8_t *p = dst.data + offset;
		*(Vec3*)p = *(Vec3*)(v.data + 0);
		p += matrix_stride;
		*(Vec3*)p = *(Vec3*)(v.data + 3);
		p += matrix_stride;
		*(Vec3*)p = *(Vec3*)(v.data + 6);
	}
	void set(Slice<uint8_t> dst, const Mat4 &v) const
	{
		NG_ASSERT(matrix_stride == sizeof(v[0]) * 4);
		set_generic(dst, v);
	}
	void set(Slice<uint8_t> dst, Slice<const Mat4> vs) const
	{
		NG_ASSERT(matrix_stride == sizeof(vs[0][0]) * 4);
		NG_ASSERT(array_stride == sizeof(vs[0]));
		uint8_t *p = dst.data + offset;
		std::memcpy(p, vs.data, vs.byte_length());
	}
};

struct UniformBlockInfo {
	String name;
	int size;
	Vector<UniformInfo> info;

	const UniformInfo *get_uniform_info(const char *name) const
	{
		for (const UniformInfo &uinfo : info) {
			if (uinfo.name == name)
				return &uinfo;
		}
		return nullptr;
	}
};

struct ShaderHandles {
	GLProgram program;
	Vector<GLShader> shaders;

	ShaderHandles() = default;
	explicit ShaderHandles(Slice<const ShaderSection> sections,
		Error *err = &DefaultError);
};


struct Shader {
	enum {
		POSITION,
		COLOR,
		TEXCOORD,
		NORMAL,
		MATERIAL,
	};
	enum {
		AUX0,
		AUX1,
		AUX2,
		AUX3,
	};
	enum {
		OUT0,
		OUT1,
		OUT2,
		OUT3,
	};

	bool valid;
	ShaderHandles handles;
	Vector<UniquePtr<Uniform>> uniforms;
	Vector<UniquePtr<UniformBlock>> uniform_blocks;

	void bind();
	Uniform *get_uniform(const char *name);
	UniformBlock *get_uniform_block(const char *name);

	void invalidate();

	static Shader *last_bound;
};

Vector<UniformBlockInfo> get_uniform_blocks_info(GLuint program);
void dump_uniform_blocks_info(Slice<const UniformBlockInfo> infos);
String gl_type_to_string(GLenum type);

//----------------------------------------------------------------------
// Texture
//----------------------------------------------------------------------

struct Texture {
	GLTexture id;
	GLenum target;

	void bind(int unit) const;

	Texture() = default;
	Texture(GLTexture id, GLenum target): id(std::move(id)), target(target) {}
	Texture(int w, int h, GLenum internal_format);
};

Texture Texture_Linear(int w, int h, GLenum internal_format);
Texture Texture_Depth(int w, int h, GLenum internal_format);
Texture Texture_DepthArray(int w, int h, int num, GLenum internal_format);
Texture Texture_FromData(Slice<const uint8_t> data, int w, int h,
	GLenum internal_format, GLenum format, GLenum type);

//----------------------------------------------------------------------
// Framebuffer
//----------------------------------------------------------------------

struct Framebuffer {
	GLFramebuffer id;

	void attach(GLenum attachment, const Texture &t) const;
	void attach_layer(GLenum attachment, const Texture &t, int layer) const;
	void validate(Error *err = &DefaultError) const;

	void bind() const;

	Framebuffer();
};

void unbind_framebuffer();

//----------------------------------------------------------------------
// Buffer
//----------------------------------------------------------------------

struct Buffer {
	GLBuffer id;
	GLenum target = GL_INVALID_ENUM;
	GLenum usage = GL_INVALID_ENUM;
	int size = 0;

	Buffer() = default;
	explicit Buffer(GLenum target, GLenum usage = GL_STATIC_DRAW);

	void reserve(int size);
	void upload(Slice<const uint8_t> data);
	void sub_upload(int offset, Slice<const uint8_t> data);

	void bind() const;
	void bind_as(GLenum target) const;
	void bind_to(GLuint index) const;

	Slice<uint8_t> map(GLenum access) const;
	void unmap() const;
};

//----------------------------------------------------------------------
// VertexArray
//----------------------------------------------------------------------

struct VertexArray {
	GLVertexArray id;
	Buffer vbo;
	Buffer ibo;
	int n = 0;

	VertexArray() = default;
	explicit VertexArray(GLenum usage);

	void bind() const;
};

//----------------------------------------------------------------------
// ResourceCache
//----------------------------------------------------------------------

struct ResourceCache {
	HashMap<String, UniquePtr<Shader>> shaders;
	Vector<UniformBlockInfo> ub_info;

	NG_DELETE_COPY_AND_MOVE(ResourceCache);
	ResourceCache();
	~ResourceCache();

	void update_ub_info();
	Shader *get_shader(const char *name);
	const UniformBlockInfo *get_ub_info(const char *name);
};

extern ResourceCache *NG_ResourceCache;

#define MAP_UNIFORM_BLOCK(name, value, code)           \
do {                                                   \
	static const UniformBlockInfo *__ubinfo = nullptr; \
	if (!__ubinfo)                                     \
		__ubinfo = NG_ResourceCache->get_ub_info(#name); \
	(value).reserve(__ubinfo->size);                   \
	auto __data = (value).map(GL_WRITE_ONLY);          \
	code;                                              \
	(value).unmap();                                   \
} while (0)

#define SET_UNIFORM_IN_BLOCK(name, ...)         \
do {                                            \
	static const UniformInfo *info = nullptr;   \
	if (!info)                                  \
		info = __ubinfo->get_uniform_info(#name); \
	info->set(__data, __VA_ARGS__);             \
} while (0);

#define BIND_SHADER(name) \
do { \
	static Shader *s = nullptr; \
	if (!s || !s->valid) \
		s = NG_ResourceCache->get_shader(#name); \
	s->bind(); \
} while (0)

#define SET_UNIFORM(name, ...) \
do { \
	static Uniform *u = nullptr; \
	if (!u) \
		u = Shader::last_bound->get_uniform(#name); \
	if (!u->valid) \
		u->resolve(Shader::last_bound->handles.program); \
	if (u->location != -1) \
		u->set(__VA_ARGS__); \
} while (0)

#define SET_UNIFORM_I(name, ...) \
do { \
	static Uniform *u = nullptr; \
	if (!u) \
		u = Shader::last_bound->get_uniform(#name); \
	if (!u->valid) \
		u->resolve(Shader::last_bound->handles.program); \
	if (u->location != -1) \
		u->set_i(__VA_ARGS__); \
} while (0)

#define SET_UNIFORM_TEXTURE(name, value, binding) \
do { \
	static Uniform *u = nullptr; \
	(value).bind(binding); \
	if (!u) \
		u = Shader::last_bound->get_uniform(#name); \
	if (!u->valid) \
		u->resolve(Shader::last_bound->handles.program); \
	if (u->location != -1) \
		u->set_i(binding); \
} while (0)

#define SET_UNIFORM_BLOCK(name, value, binding)        \
do {                                                   \
	static UniformBlock *b = nullptr;            \
	(value).bind_to(binding);                           \
	if (!b)                                            \
		b = Shader::last_bound->get_uniform_block(#name); \
	if (!b->valid) \
		b->resolve(Shader::last_bound->handles.program); \
	if (b->location != GL_INVALID_INDEX) \
		glUniformBlockBinding( \
			Shader::last_bound->handles.program, \
			b->location, binding); \
} while (0)
