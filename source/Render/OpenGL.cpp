#include "Render/OpenGL.h"
#include "Render/LuaLoadShader.h"
#include "Script/Lua.h"
#include "Core/Defer.h"

void Uniform::resolve(GLuint programid)
{
	valid = true;
	location = glGetUniformLocation(programid, name.c_str());
}

void UniformBlock::resolve(GLuint programid)
{
	valid = true;
	location = glGetUniformBlockIndex(programid, name.c_str());
}

//----------------------------------------------------------------------
// Shader
//----------------------------------------------------------------------

void Shader::bind()
{
	glUseProgram(handles.program);
	last_bound = this;
}

Uniform *Shader::get_uniform(const char *name)
{
	for (UniquePtr<Uniform> &u : uniforms) {
		if (u->name == name)
			return u.get();
	}

	// not found
	Uniform *out = nullptr;
	auto u = make_unique<Uniform>();
	u->name = name;
	u->resolve(handles.program);
	if (u->location == -1)
		warn("failed to resolve a shader uniform: '%s'", name);
	out = u.get();
	uniforms.append(std::move(u));
	return out;
}

UniformBlock *Shader::get_uniform_block(const char *name)
{
	for (UniquePtr<UniformBlock> &ub : uniform_blocks) {
		if (ub->name == name)
			return ub.get();
	}

	// not found
	UniformBlock *out = nullptr;
	auto ub = make_unique<UniformBlock>();
	ub->name = name;
	ub->resolve(handles.program);
	if (ub->location == GL_INVALID_INDEX)
		warn("failed to resolve a shader uniform block: '%s'", name);
	out = ub.get();
	uniform_blocks.append(std::move(ub));
	return out;
}

void Shader::invalidate()
{
	valid = false;
	for (UniquePtr<Uniform> &u : uniforms)
		u->valid = false;
	for (UniquePtr<UniformBlock> &u : uniform_blocks)
		u->valid = false;
}

static void error_if_bad_shader_status(const ShaderSection &section,
	GLuint shader, GLenum param, Error *err)
{
	GLint status, length;

	glGetShaderiv(shader, param, &status);
	if (status != GL_TRUE) {
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
		String buf(length-1);
		glGetShaderInfoLog(shader, length, nullptr, buf.data());

		String buf2;
		char *p = buf.data(), *e = nullptr;
		while (true) {
			int n = strtol(p, &e, 10);
			if (p != e) {
				buf2.append(section.files[n]);
				p = e;
			}
			while (*p != '\0' && *p != '\n')
				buf2.append(*p++);
			if (*p == '\0')
				break;
			buf2.append(*p++);
		}
		err->set("Shader compilation error [%s]:\n%s",
			section.name.c_str(), buf2.c_str());
	}
}

static void error_if_bad_program_status(GLuint shader, GLenum param, Error *err)
{
	GLint status, length;

	glGetProgramiv(shader, param, &status);
	if (status != GL_TRUE) {
		glGetProgramiv(shader, GL_INFO_LOG_LENGTH, &length);
		Vector<char> buf(length);
		glGetProgramInfoLog(shader, length, nullptr, buf.data());
		err->set("Program error: %s", buf.data());
	}
}

static void gl_shader_source(GLuint shader, Slice<const char> data)
{
	const GLchar *s = data.data;
	const GLint len = data.length;
	glShaderSource(shader, 1, &s, &len);
}

static GLenum section_name_to_shader_type(Slice<const char> name)
{
	if (name == "VS") {
		return GL_VERTEX_SHADER;
	} else if (name == "FS") {
		return GL_FRAGMENT_SHADER;
	} else if (name == "GS") {
		return GL_GEOMETRY_SHADER;
	}
	die("Unknown shader type: %.*s", name.length, name.data);
	return 0;
}

ShaderHandles::ShaderHandles(Slice<const ShaderSection> sections, Error *err)
{
	if (sections.length == 0) {
		err->set("Can't load a shader without sections");
		return;
	}

	// shaders array
	Vector<GLShader> shaders;
	shaders.reserve(sections.length);
	for (const auto &s : sections) {
		const GLenum type = section_name_to_shader_type(s.name);
		shaders.pappend(glCreateShader(type));
		GLShader &shader = shaders.last();

		gl_shader_source(shader, s.contents);
		glCompileShader(shader);
		error_if_bad_shader_status(s, shader, GL_COMPILE_STATUS, err);
		if (*err)
			return;
	}

	GLProgram program(glCreateProgram());
	NG_ASSERT(program != 0);
	DEFER_NAMED(free_program) { glDeleteProgram(program); };

	for (auto &id : shaders)
		glAttachShader(program, id);

	glBindAttribLocation(program, Shader::POSITION, "NG_Position");
	glBindAttribLocation(program, Shader::COLOR,    "NG_Color");
	glBindAttribLocation(program, Shader::TEXCOORD, "NG_TexCoord");
	glBindAttribLocation(program, Shader::NORMAL,   "NG_Normal");
	glBindAttribLocation(program, Shader::MATERIAL, "NG_Material");

	glBindAttribLocation(program, Shader::AUX0, "NG_Aux0");
	glBindAttribLocation(program, Shader::AUX1, "NG_Aux1");
	glBindAttribLocation(program, Shader::AUX2, "NG_Aux2");
	glBindAttribLocation(program, Shader::AUX3, "NG_Aux3");

	glBindFragDataLocation(program, Shader::OUT0, "NG_Out0");
	glBindFragDataLocation(program, Shader::OUT1, "NG_Out1");
	glBindFragDataLocation(program, Shader::OUT2, "NG_Out2");
	glBindFragDataLocation(program, Shader::OUT3, "NG_Out3");
	glLinkProgram(program);
	error_if_bad_program_status(program, GL_LINK_STATUS, err);
	if (*err)
		return;

	glValidateProgram(program);
	error_if_bad_program_status(program, GL_VALIDATE_STATUS, err);
	if (*err)
		return;

	this->program = std::move(program);
	this->shaders = std::move(shaders);
}

Shader *Shader::last_bound = nullptr;

Vector<UniformBlockInfo> get_uniform_blocks_info(GLuint program)
{
	GLint n_blocks;
	glGetProgramiv(program, GL_ACTIVE_UNIFORM_BLOCKS, &n_blocks);
	Vector<UniformBlockInfo> ubinfos(n_blocks);

	for (int i = 0; i < n_blocks; i++) {
		UniformBlockInfo &ubinfo = ubinfos[i];

		GLint name_length, n_uniforms, size;
		glGetActiveUniformBlockiv(program, i, GL_UNIFORM_BLOCK_NAME_LENGTH, &name_length);
		glGetActiveUniformBlockiv(program, i, GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS, &n_uniforms);
		glGetActiveUniformBlockiv(program, i, GL_UNIFORM_BLOCK_DATA_SIZE, &size);
		ubinfo.size = size;

		String &name = ubinfo.name;
		name.resize(name_length-1);
		glGetActiveUniformBlockName(program, i, name.length()+1, nullptr, name.data());

		ubinfo.info.resize(n_uniforms);
		Vector<GLuint> indices(n_uniforms);
		Vector<GLint> offsets(n_uniforms), sizes(n_uniforms),
			name_lengths(n_uniforms), types(n_uniforms),
			array_strides(n_uniforms), matrix_strides(n_uniforms);
		glGetActiveUniformBlockiv(program, i, GL_UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES, (GLint*)indices.data());
		glGetActiveUniformsiv(program, n_uniforms, indices.data(), GL_UNIFORM_OFFSET, offsets.data());
		glGetActiveUniformsiv(program, n_uniforms, indices.data(), GL_UNIFORM_SIZE, sizes.data());
		glGetActiveUniformsiv(program, n_uniforms, indices.data(), GL_UNIFORM_NAME_LENGTH, name_lengths.data());
		glGetActiveUniformsiv(program, n_uniforms, indices.data(), GL_UNIFORM_TYPE, types.data());
		glGetActiveUniformsiv(program, n_uniforms, indices.data(), GL_UNIFORM_ARRAY_STRIDE, array_strides.data());
		glGetActiveUniformsiv(program, n_uniforms, indices.data(), GL_UNIFORM_MATRIX_STRIDE, matrix_strides.data());

		for (int j = 0; j < n_uniforms; j++) {
			UniformInfo &uinfo = ubinfo.info[j];
			String &name = uinfo.name;
			name.resize(name_lengths[j]-1);
			glGetActiveUniformName(program, indices[j], name.length()+1, nullptr, name.data());

			uinfo.offset = offsets[j];
			uinfo.length = sizes[j];
			uinfo.type = types[j];
			uinfo.array_stride = array_strides[j];
			uinfo.matrix_stride = matrix_strides[j];
		}
	}

	return ubinfos;
}

void dump_uniform_blocks_info(Slice<const UniformBlockInfo> infos)
{
	for (const UniformBlockInfo &ubinfo : infos) {
		printf("= Uniform Block: %s (uniforms: %d, size: %d)\n", ubinfo.name.c_str(), ubinfo.info.length(), ubinfo.size);
		for (const UniformInfo &uinfo : ubinfo.info) {
			printf("== Uniform: %s, offset: %d, length: %d, type: %s, array stride: %d, matrix stride: %d\n",
				uinfo.name.c_str(), uinfo.offset, uinfo.length,
				gl_type_to_string(uinfo.type).c_str(),
				uinfo.array_stride, uinfo.matrix_stride);
		}
	}
}

String gl_type_to_string(GLenum type)
{
	switch (type) {
	case GL_FLOAT: return "float";
	case GL_FLOAT_VEC2: return "vec2";
	case GL_FLOAT_VEC3: return "vec3";
	case GL_FLOAT_VEC4: return "vec4";
	case GL_DOUBLE: return "double";
	case GL_DOUBLE_VEC2: return "dvec2";
	case GL_DOUBLE_VEC3: return "dvec3";
	case GL_DOUBLE_VEC4: return "dvec4";
	case GL_INT: return "int";
	case GL_INT_VEC2: return "ivec2";
	case GL_INT_VEC3: return "ivec3";
	case GL_INT_VEC4: return "ivec4";
	case GL_UNSIGNED_INT: return "unsigned int";
	case GL_UNSIGNED_INT_VEC2: return "uvec2";
	case GL_UNSIGNED_INT_VEC3: return "uvec3";
	case GL_UNSIGNED_INT_VEC4: return "uvec4";
	case GL_BOOL: return "bool";
	case GL_BOOL_VEC2: return "bvec2";
	case GL_BOOL_VEC3: return "bvec3";
	case GL_BOOL_VEC4: return "bvec4";
	case GL_FLOAT_MAT2: return "mat2";
	case GL_FLOAT_MAT3: return "mat3";
	case GL_FLOAT_MAT4: return "mat4";
	case GL_FLOAT_MAT2x3: return "mat2x3";
	case GL_FLOAT_MAT2x4: return "mat2x4";
	case GL_FLOAT_MAT3x2: return "mat3x2";
	case GL_FLOAT_MAT3x4: return "mat3x4";
	case GL_FLOAT_MAT4x2: return "mat4x2";
	case GL_FLOAT_MAT4x3: return "mat4x3";
	case GL_DOUBLE_MAT2: return "dmat2";
	case GL_DOUBLE_MAT3: return "dmat3";
	case GL_DOUBLE_MAT4: return "dmat4";
	case GL_DOUBLE_MAT2x3: return "dmat2x3";
	case GL_DOUBLE_MAT2x4: return "dmat2x4";
	case GL_DOUBLE_MAT3x2: return "dmat3x2";
	case GL_DOUBLE_MAT3x4: return "dmat3x4";
	case GL_DOUBLE_MAT4x2: return "dmat4x2";
	case GL_DOUBLE_MAT4x3: return "dmat4x3";
	case GL_SAMPLER_1D: return "sampler1D";
	case GL_SAMPLER_2D: return "sampler2D";
	case GL_SAMPLER_3D: return "sampler3D";
	case GL_SAMPLER_CUBE: return "samplerCube";
	case GL_SAMPLER_1D_SHADOW: return "sampler1DShadow";
	case GL_SAMPLER_2D_SHADOW: return "sampler2DShadow";
	case GL_SAMPLER_1D_ARRAY: return "sampler1DArray";
	case GL_SAMPLER_2D_ARRAY: return "sampler2DArray";
	case GL_SAMPLER_1D_ARRAY_SHADOW: return "sampler1DArrayShadow";
	case GL_SAMPLER_2D_ARRAY_SHADOW: return "sampler2DArrayShadow";
	case GL_SAMPLER_2D_MULTISAMPLE: return "sampler2DMS";
	case GL_SAMPLER_2D_MULTISAMPLE_ARRAY: return "sampler2DMSArray";
	case GL_SAMPLER_CUBE_SHADOW: return "samplerCubeShadow";
	case GL_SAMPLER_BUFFER: return "samplerBuffer";
	case GL_SAMPLER_2D_RECT: return "sampler2DRect";
	case GL_SAMPLER_2D_RECT_SHADOW: return "sampler2DRectShadow";
	case GL_INT_SAMPLER_1D: return "isampler1D";
	case GL_INT_SAMPLER_2D: return "isampler2D";
	case GL_INT_SAMPLER_3D: return "isampler3D";
	case GL_INT_SAMPLER_CUBE: return "isamplerCube";
	case GL_INT_SAMPLER_1D_ARRAY: return "isampler1DArray";
	case GL_INT_SAMPLER_2D_ARRAY: return "isampler2DArray";
	case GL_INT_SAMPLER_2D_MULTISAMPLE: return "isampler2DMS";
	case GL_INT_SAMPLER_2D_MULTISAMPLE_ARRAY: return "isampler2DMSArray";
	case GL_INT_SAMPLER_BUFFER: return "isamplerBuffer";
	case GL_INT_SAMPLER_2D_RECT: return "isampler2DRect";
	case GL_UNSIGNED_INT_SAMPLER_1D: return "usampler1D";
	case GL_UNSIGNED_INT_SAMPLER_2D: return "usampler2D";
	case GL_UNSIGNED_INT_SAMPLER_3D: return "usampler3D";
	case GL_UNSIGNED_INT_SAMPLER_CUBE: return "usamplerCube";
	case GL_UNSIGNED_INT_SAMPLER_1D_ARRAY: return "usampler2DArray";
	case GL_UNSIGNED_INT_SAMPLER_2D_ARRAY: return "usampler2DArray";
	case GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE: return "usampler2DMS";
	case GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY: return "usampler2DMSArray";
	case GL_UNSIGNED_INT_SAMPLER_BUFFER: return "usamplerBuffer";
	case GL_UNSIGNED_INT_SAMPLER_2D_RECT: return "usampler2DRect";
	default:
		return "<unknown>";
	}
}

//----------------------------------------------------------------------
// Texture
//----------------------------------------------------------------------

void Texture::bind(int unit) const
{
	glActiveTexture(GL_TEXTURE0 + unit);
	glBindTexture(target, id);
}

Texture::Texture(int w, int h, GLenum internal_format): target(GL_TEXTURE_2D)
{
	glGenTextures(1, &id);
	NG_ASSERT(id != 0);
	glBindTexture(GL_TEXTURE_2D, id);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	glTexImage2D(GL_TEXTURE_2D, 0, internal_format, w, h,
		0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
}

Texture Texture_FromData(Slice<const uint8_t> data, int w, int h,
	GLenum internal_format, GLenum format, GLenum type)
{
	GLTexture id;
	glGenTextures(1, &id);
	NG_ASSERT(id != 0);
	glBindTexture(GL_TEXTURE_2D, id);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_REPEAT);

	glTexImage2D(GL_TEXTURE_2D, 0, internal_format, w, h,
		0, format, type, data.data);
	return {std::move(id), GL_TEXTURE_2D};
}


Texture Texture_Linear(int w, int h, GLenum internal_format)
{
	Texture t = Texture(w, h, internal_format);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	return t;
}

Texture Texture_Depth(int w, int h, GLenum internal_format)
{
	GLTexture id;
	glGenTextures(1, &id);
	NG_ASSERT(id != 0);
	glBindTexture(GL_TEXTURE_2D, id);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	const float clamp_color[] = {1.0, 1.0, 1.0, 1.0};
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, clamp_color);
	if (internal_format == GL_DEPTH_COMPONENT16) {
		warn("TODO: Using ugly hack here");
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LESS);
	}
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
	//glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_INTENSITY);
	glTexImage2D(GL_TEXTURE_2D, 0, internal_format, w, h,
		0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);
	return {std::move(id), GL_TEXTURE_2D};
}

Texture Texture_DepthArray(int w, int h, int num, GLenum internal_format)
{
	GLTexture id;
	glGenTextures(1, &id);
	NG_ASSERT(id != 0);
	glBindTexture(GL_TEXTURE_2D_ARRAY, id);

	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	const float clamp_color[] = {1.0, 1.0, 1.0, 1.0};
	glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, clamp_color);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_FUNC, GL_LESS);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
	glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, internal_format, w, h, num,
		0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);
	return {std::move(id), GL_TEXTURE_2D_ARRAY};
}

//----------------------------------------------------------------------
// Framebuffer
//----------------------------------------------------------------------

void Framebuffer::attach(GLenum attachment, const Texture &t) const
{
	bind();
	glFramebufferTexture(GL_FRAMEBUFFER, attachment, t.id, 0);
}

void Framebuffer::attach_layer(GLenum attachment, const Texture &t, int layer) const
{
	bind();
	glFramebufferTextureLayer(GL_FRAMEBUFFER, attachment, t.id, 0, layer);
}

void Framebuffer::validate(Error *err) const
{
	bind();
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE)
		err->set("Incomplete framebuffer");
}

void Framebuffer::bind() const
{
	glBindFramebuffer(GL_FRAMEBUFFER, id);
}

Framebuffer::Framebuffer()
{
	glGenFramebuffers(1, &id);
	NG_ASSERT(id != 0);
}

void unbind_framebuffer()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

//----------------------------------------------------------------------
// Buffer
//----------------------------------------------------------------------

void Buffer::reserve(int size)
{
	if (this->size < size) {
		bind();
		glBufferData(target, size, nullptr, usage);
		this->size = size;
	}
}

void Buffer::upload(Slice<const uint8_t> data)
{
	bind();
	glBufferData(target, data.length, data.data, usage);
	size = data.length;
}

void Buffer::sub_upload(int offset, Slice<const uint8_t> data)
{
	bind();
	glBufferSubData(target, offset, data.length, data.data);
}

void Buffer::bind() const
{
	glBindBuffer(target, id);
}

void Buffer::bind_as(GLenum target) const
{
	glBindBuffer(target, id);
}

void Buffer::bind_to(GLuint index) const
{
	glBindBufferBase(target, index, id);
}

Buffer::Buffer(GLenum target, GLenum usage)
{
	glGenBuffers(1, &id);
	NG_ASSERT(id != 0);
	this->target = target;
	this->usage = usage;
	this->size = 0;
}

Slice<uint8_t> Buffer::map(GLenum access) const
{
	bind();
	return Slice<uint8_t>((uint8_t*)glMapBuffer(target, access), size);
}

void Buffer::unmap() const
{
	glUnmapBuffer(target);
}

//----------------------------------------------------------------------
// VertexArray
//----------------------------------------------------------------------

void VertexArray::bind() const
{
	NG_ASSERT((GLint)id != 0);
	glBindVertexArray(id);
}

VertexArray::VertexArray(GLenum usage)
{
	vbo = Buffer(GL_ARRAY_BUFFER, usage);
	ibo = Buffer(GL_ELEMENT_ARRAY_BUFFER, usage);
	n = 0;
	glGenVertexArrays(1, &id);
	NG_ASSERT(id != 0);
}

//----------------------------------------------------------------------
// ResourceCache
//----------------------------------------------------------------------

ResourceCache::ResourceCache()
{
	if (NG_ResourceCache)
		die("There can only be one ResourceCache");
	NG_ResourceCache = this;
}

ResourceCache::~ResourceCache()
{
	NG_ResourceCache = nullptr;
}

void ResourceCache::update_ub_info()
{
	BIND_SHADER(UNIFORM_BLOCKS);
	ub_info = get_uniform_blocks_info(Shader::last_bound->handles.program);
}

Shader *ResourceCache::get_shader(const char *name)
{
	UniquePtr<Shader> *ps = shaders.get(name);
	if (ps && (*ps)->valid)
		return ps->get();

	Shader *out;
	if (ps) {
		out = ps->get();
	} else {
		UniquePtr<Shader> shader = make_unique<Shader>();
		out = shader.get();
		shaders.insert(name, std::move(shader));
	}

	auto sections = load_shader_sections(NG_LuaVM->L, name);
	out->handles = ShaderHandles(sections);
	out->valid = true;
	return out;
}

const UniformBlockInfo *ResourceCache::get_ub_info(const char *name)
{
	for (const UniformBlockInfo &ubinfo : ub_info) {
		if (ubinfo.name == name)
			return &ubinfo;
	}
	die("UniformBlockInfo '%s' not found", name);
	return nullptr;
}

ResourceCache *NG_ResourceCache = nullptr;

NG_LUA_API void NG_Render_InvalidateAllShaders()
{
	for (auto kv : NG_ResourceCache->shaders)
		kv.value->invalidate();
}
