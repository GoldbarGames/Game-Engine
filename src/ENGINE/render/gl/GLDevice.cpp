#include "GLDevice.h"
#include "../../Shader.h"
#include "../../opengl_includes.h"
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <memory>
#include <vector>

namespace
{
#ifndef __EMSCRIPTEN__
	// ---- the program cache -----------------------------------------------
	//
	// Compiling GLSL is the slowest thing a start-up does: up to half a second
	// for some of DB2's 2D effects, for every program, on every run - a debug
	// context gets no help from the driver's own cache. A linked program can be
	// saved as the driver's binary (glGetProgramBinary) and loaded back
	// (glProgramBinary), so each is compiled once and afterwards read from the
	// game's cache/shaders/. The key is a hash of the program's final source and
	// the driver, so editing a shader (or updating the driver) simply compiles
	// it again. KINJO_SHADER_CACHE=0 turns it off (docs/STARTUP.md).
	struct ProgramCache
	{
		bool checked = false;
		bool usable = false;
		std::string driver;          // vendor | renderer | version: part of every key

		bool Usable()
		{
			if (checked)
				return usable;
			checked = true;
			const char* env = std::getenv("KINJO_SHADER_CACHE");
			GLint formats = 0;
			if (glGetProgramBinary != nullptr && glProgramBinary != nullptr && glProgramParameteri != nullptr)
				glGetIntegerv(GL_NUM_PROGRAM_BINARY_FORMATS, &formats);
			usable = formats > 0 && !(env != nullptr && env[0] == '0');
			if (usable)
			{
				auto text = [](GLenum e)
				{
					const GLubyte* str = glGetString(e);
					return (str != nullptr) ? std::string((const char*)str) : std::string();
				};
				driver = text(GL_VENDOR) + "|" + text(GL_RENDERER) + "|" + text(GL_VERSION);
				std::error_code ec;
				std::filesystem::create_directories("cache/shaders", ec);
				usable = !ec;
			}
			std::cout << "Program cache: " << (usable ? "on (cache/shaders/)" : "off") << std::endl;
			return usable;
		}

		std::string PathFor(std::initializer_list<const char*> sources) const
		{
			uint64_t h = 1469598103934665603ull;                     // FNV-1a
			auto mix = [&](const char* text, size_t n)
			{
				for (size_t i = 0; i < n; i++)
				{
					h ^= (unsigned char)text[i];
					h *= 1099511628211ull;
				}
				h ^= 0xffu;                                          // a separator
				h *= 1099511628211ull;
			};
			mix("KPB1", 4);
			mix(driver.data(), driver.size());
			for (const char* source : sources)
				mix(source, strlen(source));
			char name[64];
			snprintf(name, sizeof(name), "cache/shaders/%016llx.bin", (unsigned long long)h);
			return name;
		}

		// The cached program, or 0 to compile it (none yet, or the driver
		// refuses an old binary after an update).
		GLuint Load(const std::string& path) const
		{
			std::ifstream in(path, std::ios::binary);
			if (!in)
				return 0;
			char magic[4] = { 0 };
			GLenum format = 0;
			uint32_t length = 0;
			in.read(magic, 4);
			in.read((char*)&format, sizeof(format));
			in.read((char*)&length, sizeof(length));
			if (!in || memcmp(magic, "KPB1", 4) != 0 || length == 0 || length > 64u * 1024u * 1024u)
				return 0;
			std::vector<char> data(length);
			if (!in.read(data.data(), length))
				return 0;
			GLuint program = glCreateProgram();
			glProgramBinary(program, format, data.data(), (GLsizei)length);
			GLint ok = 0;
			glGetProgramiv(program, GL_LINK_STATUS, &ok);
			if (!ok)
			{
				glDeleteProgram(program);
				return 0;
			}
			return program;
		}

		void Save(const std::string& path, GLuint program) const
		{
			GLint length = 0;
			glGetProgramiv(program, GL_PROGRAM_BINARY_LENGTH, &length);
			if (length <= 0)
				return;
			std::vector<char> data((size_t)length);
			GLenum format = 0;
			GLsizei written = 0;
			glGetProgramBinary(program, length, &written, &format, data.data());
			if (written <= 0)
				return;
			// Written beside and renamed into place, so a run that dies half way
			// through never leaves half a binary under the real name.
			const std::string temp = path + ".tmp";
			{
				std::ofstream out(temp, std::ios::binary | std::ios::trunc);
				if (!out)
					return;
				const uint32_t n = (uint32_t)written;
				out.write("KPB1", 4);
				out.write((const char*)&format, sizeof(format));
				out.write((const char*)&n, sizeof(n));
				out.write(data.data(), written);
				if (!out)
					return;
			}
			std::error_code ec;
			std::filesystem::rename(temp, path, ec);
		}
	};
	ProgramCache programCache;
#endif

	GLenum ToGL(BufferUsage usage)
	{
		switch (usage)
		{
		case BufferUsage::Dynamic: return GL_DYNAMIC_DRAW;
		case BufferUsage::Stream:  return GL_STREAM_DRAW;
		case BufferUsage::Static:
		default:                   return GL_STATIC_DRAW;
		}
	}

	GLenum ToGL(Primitive primitive)
	{
		return primitive == Primitive::Lines ? GL_LINES : GL_TRIANGLES;
	}

	GLenum ToGL(CompareOp op)
	{
		switch (op)
		{
		case CompareOp::LessEqual: return GL_LEQUAL;
		case CompareOp::Always:    return GL_ALWAYS;
		case CompareOp::Less:
		default:                   return GL_LESS;
		}
	}

	GLenum TargetOf(TextureType type)
	{
		switch (type)
		{
		case TextureType::Cube:      return GL_TEXTURE_CUBE_MAP;
		case TextureType::Tex2DArray: return GL_TEXTURE_2D_ARRAY;
#ifndef __EMSCRIPTEN__
		case TextureType::CubeArray: return GL_TEXTURE_CUBE_MAP_ARRAY;
#endif
		case TextureType::Tex2D:
		default:                     return GL_TEXTURE_2D;
		}
	}

	// Block-compressed internal formats, spelled out so the Emscripten GLES
	// headers (which may lack the extension enums) compile too.
	const GLint kRgbaS3tcDxt1 = 0x83F1;
	const GLint kSrgbAlphaS3tcDxt1 = 0x8C4D;
	const GLint kRgbaS3tcDxt5 = 0x83F3;
	const GLint kSrgbAlphaS3tcDxt5 = 0x8C4F;
	const GLint kRedRgtc1 = 0x8DBB;
	const GLint kRgRgtc2 = 0x8DBD;
	const GLint kRgbaBptc = 0x8E8C;
	const GLint kSrgbAlphaBptc = 0x8E8D;

	struct GLFormat { GLint internal; GLenum format; GLenum type; };
	GLFormat FormatOf(TextureFormat f)
	{
		switch (f)
		{
		case TextureFormat::BC1:             return { kRgbaS3tcDxt1, 0, 0 };
		case TextureFormat::BC1_SRGB:        return { kSrgbAlphaS3tcDxt1, 0, 0 };
		case TextureFormat::BC3:             return { kRgbaS3tcDxt5, 0, 0 };
		case TextureFormat::BC3_SRGB:        return { kSrgbAlphaS3tcDxt5, 0, 0 };
		case TextureFormat::BC4:             return { kRedRgtc1, 0, 0 };
		case TextureFormat::BC5:             return { kRgRgtc2, 0, 0 };
		case TextureFormat::BC7:             return { kRgbaBptc, 0, 0 };
		case TextureFormat::BC7_SRGB:        return { kSrgbAlphaBptc, 0, 0 };
		case TextureFormat::SRGB8_A8:        return { GL_SRGB8_ALPHA8, GL_RGBA, GL_UNSIGNED_BYTE };
		case TextureFormat::RGBA16F:         return { GL_RGBA16F, GL_RGBA, GL_HALF_FLOAT };
		case TextureFormat::R8:              return { GL_R8, GL_RED, GL_UNSIGNED_BYTE };
		case TextureFormat::R32F:            return { GL_R32F, GL_RED, GL_FLOAT };
		case TextureFormat::RGBA32UI:        return { GL_RGBA32UI, GL_RGBA_INTEGER, GL_UNSIGNED_INT };
		case TextureFormat::Depth24:         return { GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT, GL_FLOAT };
		case TextureFormat::Depth24Stencil8: return { GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8 };
		case TextureFormat::RGBA8:
		default:                             return { GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE };   // unsized, as the engine always used
		}
	}

	GLenum AttachmentOf(Attachment a)
	{
		switch (a)
		{
		case Attachment::Color1:       return GL_COLOR_ATTACHMENT1;
		case Attachment::Color2:       return GL_COLOR_ATTACHMENT2;
		case Attachment::Depth:        return GL_DEPTH_ATTACHMENT;
		case Attachment::DepthStencil: return GL_DEPTH_STENCIL_ATTACHMENT;
		case Attachment::Color0:
		default:                       return GL_COLOR_ATTACHMENT0;
		}
	}

	// The context's extensions (desktop core profiles list them one by one;
	// WebGL as one string, with or without a "GL_" prefix).
	bool HasExtension(const std::string& name)
	{
		static std::vector<std::string> extensions;
		static bool loaded = false;
		if (!loaded)
		{
			loaded = true;
#ifdef __EMSCRIPTEN__
			const char* all = (const char*)glGetString(GL_EXTENSIONS);
			std::string s = (all != nullptr) ? all : "";
			size_t start = 0;
			while (start < s.size())
			{
				size_t end = s.find(' ', start);
				if (end == std::string::npos) end = s.size();
				if (end > start) extensions.push_back(s.substr(start, end - start));
				start = end + 1;
			}
#else
			GLint count = 0;
			glGetIntegerv(GL_NUM_EXTENSIONS, &count);
			for (GLint i = 0; i < count; i++)
			{
				const char* e = (const char*)glGetStringi(GL_EXTENSIONS, (GLuint)i);
				if (e != nullptr) extensions.push_back(e);
			}
#endif
		}
		for (const std::string& e : extensions)
			if (e == name || e == "GL_" + name)
				return true;
		return false;
	}

	// The hardware's anisotropic filtering limit (1 = none), queried once.
	float MaxAnisotropy()
	{
#ifdef GL_TEXTURE_MAX_ANISOTROPY_EXT
		static const float limit = []()
		{
			GLfloat v = 0.0f;
			glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &v);
			return v > 1.0f ? (float)v : 1.0f;
		}();
		return limit;
#else
		return 1.0f;
#endif
	}

	// Filtering, wrapping, depth comparison and anisotropy of the texture bound to `target`.
	void SetSamplerState(GLenum target, const TextureDesc& desc)
	{
		GLint minFilter = GL_NEAREST, magFilter = GL_NEAREST;
		if (desc.filter == TextureFilter::Linear) { minFilter = GL_LINEAR; magFilter = GL_LINEAR; }
		if (desc.filter == TextureFilter::Trilinear) { minFilter = GL_LINEAR_MIPMAP_LINEAR; magFilter = GL_LINEAR; }
		glTexParameteri(target, GL_TEXTURE_MIN_FILTER, minFilter);
		glTexParameteri(target, GL_TEXTURE_MAG_FILTER, magFilter);

		GLint wrap = GL_CLAMP_TO_EDGE;
		if (desc.wrap == TextureWrap::Repeat) wrap = GL_REPEAT;
#ifndef __EMSCRIPTEN__
		if (desc.wrap == TextureWrap::ClampToBorder)
		{
			wrap = GL_CLAMP_TO_BORDER;
			glTexParameterfv(target, GL_TEXTURE_BORDER_COLOR, glm::value_ptr(desc.borderColor));
		}
#endif
		glTexParameteri(target, GL_TEXTURE_WRAP_S, wrap);
		glTexParameteri(target, GL_TEXTURE_WRAP_T, wrap);
		if (desc.type != TextureType::Tex2D)
			glTexParameteri(target, GL_TEXTURE_WRAP_R, wrap);

		// Depth comparison: shadow samplers return the filtered result of
		// "reference <= stored depth" (hardware PCF) instead of the depth.
		if (desc.depthCompare)
		{
			glTexParameteri(target, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
			glTexParameteri(target, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
		}

#ifdef GL_TEXTURE_MAX_ANISOTROPY_EXT
		if (desc.maxAnisotropy > 1.0f && MaxAnisotropy() > 1.0f)
			glTexParameterf(target, GL_TEXTURE_MAX_ANISOTROPY_EXT, std::min(MaxAnisotropy(), desc.maxAnisotropy));
#endif
	}
}

// ---------------------------------------------------------------- device

RenderDevice& Device()
{
	static std::unique_ptr<RenderDevice> device(new GLDevice());
	return *device;
}

GLDevice::GLDevice()
{
}

// ---------------------------------------------------------------- buffers

BufferHandle GLDevice::CreateBuffer(size_t bytes, const void* data, BufferUsage usage)
{
	GLuint id = 0;
	glGenBuffers(1, &id);
	glBindBuffer(GL_COPY_WRITE_BUFFER, id);
	glBufferData(GL_COPY_WRITE_BUFFER, (GLsizeiptr)bytes, data, ToGL(usage));
	glBindBuffer(GL_COPY_WRITE_BUFFER, 0);
	return BufferHandle(id);
}

BufferHandle GLDevice::CreateIndexBuffer(size_t bytes, const void* data)
{
	// No vertex array bound, so the element binding can't attach to one.
	glBindVertexArray(0);
	GLuint id = 0;
	glGenBuffers(1, &id);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)bytes, data, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	return BufferHandle(id);
}

void GLDevice::UpdateBuffer(BufferHandle buffer, size_t offset, size_t bytes, const void* data)
{
	glBindBuffer(GL_COPY_WRITE_BUFFER, buffer.id);
	glBufferSubData(GL_COPY_WRITE_BUFFER, (GLintptr)offset, (GLsizeiptr)bytes, data);
	glBindBuffer(GL_COPY_WRITE_BUFFER, 0);
}

void GLDevice::ReplaceBuffer(BufferHandle buffer, size_t bytes, const void* data, BufferUsage usage)
{
	glBindBuffer(GL_COPY_WRITE_BUFFER, buffer.id);
	glBufferData(GL_COPY_WRITE_BUFFER, (GLsizeiptr)bytes, data, ToGL(usage));
	glBindBuffer(GL_COPY_WRITE_BUFFER, 0);
}

void* GLDevice::CreatePersistentBuffer(size_t bytes, BufferHandle& out)
{
	out = BufferHandle();
#ifndef __EMSCRIPTEN__
	if (!SupportsPersistentMapping())
		return nullptr;
	GLuint id = 0;
	glGenBuffers(1, &id);
	glBindBuffer(GL_COPY_WRITE_BUFFER, id);
	const GLbitfield flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
	glBufferStorage(GL_COPY_WRITE_BUFFER, (GLsizeiptr)bytes, nullptr, flags);
	void* mapped = glMapBufferRange(GL_COPY_WRITE_BUFFER, 0, (GLsizeiptr)bytes, flags);
	glBindBuffer(GL_COPY_WRITE_BUFFER, 0);
	if (mapped == nullptr)
	{
		glDeleteBuffers(1, &id);   // immutable storage: no glBufferData fallback on this one
		return nullptr;
	}
	out = BufferHandle(id);
	return mapped;
#else
	(void)bytes;
	return nullptr;
#endif
}

void GLDevice::DestroyBuffer(BufferHandle& buffer)
{
	if (buffer.id != 0)
	{
		GLuint id = buffer.id;
		glDeleteBuffers(1, &id);   // deleting a mapped buffer unmaps it
	}
	buffer = BufferHandle();
}

void GLDevice::BindUniformBuffer(unsigned int binding, BufferHandle buffer)
{
	glBindBufferBase(GL_UNIFORM_BUFFER, binding, buffer.id);
}

// ---------------------------------------------------------------- vertex input

VertexArrayHandle GLDevice::CreateVertexArray()
{
	GLuint id = 0;
	glGenVertexArrays(1, &id);
	return VertexArrayHandle(id);
}

void GLDevice::DestroyVertexArray(VertexArrayHandle& vao)
{
	if (vao.id != 0)
	{
		GLuint id = vao.id;
		glDeleteVertexArrays(1, &id);
	}
	vao = VertexArrayHandle();
}

void GLDevice::SetVertexAttribute(VertexArrayHandle vao, unsigned int location, BufferHandle buffer,
	int components, size_t stride, size_t offset, unsigned int divisor)
{
	glBindVertexArray(vao.id);
	glBindBuffer(GL_ARRAY_BUFFER, buffer.id);
	glVertexAttribPointer(location, components, GL_FLOAT, GL_FALSE, (GLsizei)stride, (void*)offset);
	glEnableVertexAttribArray(location);
	glVertexAttribDivisor(location, divisor);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void GLDevice::DisableVertexAttribute(VertexArrayHandle vao, unsigned int location)
{
	glBindVertexArray(vao.id);
	glDisableVertexAttribArray(location);
	glBindVertexArray(0);
}

void GLDevice::SetIndexBuffer(VertexArrayHandle vao, BufferHandle buffer)
{
	glBindVertexArray(vao.id);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer.id);
	// Unbind the VAO first: unbinding the element buffer while it is bound
	// would remove the association from the VAO.
	glBindVertexArray(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void GLDevice::BindVertexArray(VertexArrayHandle vao)
{
	glBindVertexArray(vao.id);
}

// ---------------------------------------------------------------- textures

TextureHandle GLDevice::CreateTexture(const TextureDesc& desc, const void* rgbaPixels)
{
	const GLenum target = TargetOf(desc.type);
	const GLFormat fmt = FormatOf(desc.format);

	GLuint id = 0;
	glGenTextures(1, &id);
	glBindTexture(target, id);
	unit0Known = false;   // that bind replaced whatever unit 0 held

	SetSamplerState(target, desc);

	switch (desc.type)
	{
	case TextureType::Cube:
		for (int face = 0; face < 6; face++)
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, fmt.internal, desc.width, desc.height, 0,
				fmt.format, fmt.type, nullptr);
		break;
	case TextureType::Tex2DArray:
		glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, fmt.internal, desc.width, desc.height, desc.layers, 0,
			fmt.format, fmt.type, nullptr);
		break;
#ifndef __EMSCRIPTEN__
	case TextureType::CubeArray:
		glTexImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 0, fmt.internal, desc.width, desc.height, desc.layers * 6, 0,
			fmt.format, fmt.type, nullptr);
		break;
#endif
	case TextureType::Tex2D:
	default:
		glTexImage2D(GL_TEXTURE_2D, 0, fmt.internal, desc.width, desc.height, 0, fmt.format, fmt.type, rgbaPixels);
		if (desc.mipLevels > 1)
		{
			// Storage for every level up front, so each can be a render target;
			// MAX_LEVEL makes the texture complete without levels past it.
			for (int level = 1; level < desc.mipLevels; level++)
			{
				const int w = std::max(1, desc.width >> level);
				const int h = std::max(1, desc.height >> level);
				glTexImage2D(GL_TEXTURE_2D, level, fmt.internal, w, h, 0, fmt.format, fmt.type, nullptr);
			}
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, desc.mipLevels - 1);
		}
		break;
	}

	if (desc.generateMipmaps)
		glGenerateMipmap(target);

	glBindTexture(target, 0);
	return TextureHandle(id);
}

TextureHandle GLDevice::CreateTextureLevels(const TextureDesc& desc, const TextureLevel* levels, int levelCount)
{
	if (levels == nullptr || levelCount < 1 || desc.type != TextureType::Tex2D)
		return TextureHandle();
	const GLFormat fmt = FormatOf(desc.format);
	const bool compressed = IsBlockCompressed(desc.format);

	GLuint id = 0;
	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);
	unit0Known = false;   // that bind replaced whatever unit 0 held

	TextureDesc sampler = desc;
	const bool generate = desc.generateMipmaps && levelCount == 1 && !compressed;
	if (sampler.filter == TextureFilter::Trilinear && levelCount == 1 && !generate)
		sampler.filter = TextureFilter::Linear;   // no levels to blend between
	SetSamplerState(GL_TEXTURE_2D, sampler);

	// Rows of RGBA8 levels are tightly packed (4-byte texels: any alignment works).
	for (int level = 0; level < levelCount; level++)
	{
		const int w = std::max(1, desc.width >> level);
		const int h = std::max(1, desc.height >> level);
		if (compressed)
			glCompressedTexImage2D(GL_TEXTURE_2D, level, (GLenum)fmt.internal, w, h, 0,
				(GLsizei)levels[level].bytes, levels[level].data);
		else
			glTexImage2D(GL_TEXTURE_2D, level, fmt.internal, w, h, 0, fmt.format, fmt.type, levels[level].data);
	}
	// A chain that stops before 1x1 is still complete up to its last level.
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, generate ? 1000 : levelCount - 1);
	if (generate)
		glGenerateMipmap(GL_TEXTURE_2D);

	glBindTexture(GL_TEXTURE_2D, 0);
	return TextureHandle(id);
}

bool GLDevice::SupportsTextureFormat(TextureFormat format) const
{
	switch (format)
	{
#ifdef __EMSCRIPTEN__
	case TextureFormat::BC1:
	case TextureFormat::BC3:
		return HasExtension("WEBGL_compressed_texture_s3tc");
	case TextureFormat::BC1_SRGB:
	case TextureFormat::BC3_SRGB:
		return HasExtension("WEBGL_compressed_texture_s3tc_srgb");
	case TextureFormat::BC4:
	case TextureFormat::BC5:
		return HasExtension("EXT_texture_compression_rgtc");
	case TextureFormat::BC7:
	case TextureFormat::BC7_SRGB:
		return HasExtension("EXT_texture_compression_bptc");
#else
	case TextureFormat::BC1:
	case TextureFormat::BC3:
		return HasExtension("GL_EXT_texture_compression_s3tc");
	case TextureFormat::BC1_SRGB:
	case TextureFormat::BC3_SRGB:
		return HasExtension("GL_EXT_texture_compression_s3tc")
			&& (HasExtension("GL_EXT_texture_sRGB") || HasExtension("GL_EXT_texture_compression_s3tc_srgb"));
	case TextureFormat::BC4:
	case TextureFormat::BC5:
		return true;   // RGTC: core since GL 3.0
	case TextureFormat::BC7:
	case TextureFormat::BC7_SRGB:
		return ShaderProgram::glslVersion >= 420 || HasExtension("GL_ARB_texture_compression_bptc");
#endif
	default:
		return true;
	}
}

void GLDevice::DestroyTexture(TextureHandle& texture)
{
	if (texture.id != 0)
	{
		GLuint id = texture.id;
		glDeleteTextures(1, &id);
		if (unit0Known && boundUnit0 == id)
			unit0Known = false;
	}
	texture = TextureHandle();
}

void GLDevice::BindTexture(unsigned int unit, TextureHandle texture, TextureType type)
{
	// Invariant kept for code outside the device: the active unit is 0
	// whenever the device returns.
	if (unit == 0 && type == TextureType::Tex2D)
	{
		if (unit0Known && boundUnit0 == texture.id)
			return;
		glBindTexture(GL_TEXTURE_2D, texture.id);
		boundUnit0 = texture.id;
		unit0Known = true;
		return;
	}
	glActiveTexture(GL_TEXTURE0 + unit);
	glBindTexture(TargetOf(type), texture.id);
	glActiveTexture(GL_TEXTURE0);
}

// ---------------------------------------------------------------- framebuffers

FramebufferHandle GLDevice::CreateFramebuffer()
{
	GLuint id = 0;
	glGenFramebuffers(1, &id);
	return FramebufferHandle(id);
}

void GLDevice::DestroyFramebuffer(FramebufferHandle& framebuffer)
{
	if (framebuffer.id != 0)
	{
		GLuint id = framebuffer.id;
		glDeleteFramebuffers(1, &id);
	}
	framebuffer = FramebufferHandle();
}

void GLDevice::AttachTexture(FramebufferHandle framebuffer, Attachment attachment, TextureHandle texture,
	TextureType type, int layer, int mipLevel)
{
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.id);
	const GLenum point = AttachmentOf(attachment);
	switch (type)
	{
	case TextureType::Cube:
		glFramebufferTexture2D(GL_FRAMEBUFFER, point, GL_TEXTURE_CUBE_MAP_POSITIVE_X + layer, texture.id, mipLevel);
		break;
	case TextureType::CubeArray:
	case TextureType::Tex2DArray:
		glFramebufferTextureLayer(GL_FRAMEBUFFER, point, texture.id, mipLevel, layer);
		break;
	case TextureType::Tex2D:
	default:
		glFramebufferTexture2D(GL_FRAMEBUFFER, point, GL_TEXTURE_2D, texture.id, mipLevel);
		break;
	}
}

void GLDevice::UpdateTexture(TextureHandle texture, TextureFormat format, int x, int y, int width, int height,
	const void* data)
{
	if (texture.id == 0 || width <= 0 || height <= 0)
		return;
	const GLFormat fmt = FormatOf(format);
	glBindTexture(GL_TEXTURE_2D, texture.id);   // on unit 0, which stays active
	glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, width, height, fmt.format, fmt.type, data);
	boundUnit0 = texture.id;
	unit0Known = true;
}

void GLDevice::GenerateMipmaps(TextureHandle texture)
{
	glBindTexture(GL_TEXTURE_2D, texture.id);
	glGenerateMipmap(GL_TEXTURE_2D);
	boundUnit0 = texture.id;   // left bound on unit 0
	unit0Known = true;
}

void GLDevice::SetDrawBuffers(FramebufferHandle framebuffer, int colorCount)
{
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.id);
	SetBoundDrawBuffers(colorCount);
}

void GLDevice::SetBoundDrawBuffers(int colorCount)
{
	if (colorCount <= 0)
	{
		GLenum none = GL_NONE;
		glDrawBuffers(1, &none);
		glReadBuffer(GL_NONE);
		return;
	}
	const GLenum all[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
	glDrawBuffers(colorCount > 2 ? 2 : colorCount, all);
}

void GLDevice::SetBoundDrawBufferMask(unsigned int attachmentMask)
{
	GLenum buffers[3];
	int count = 0;
	for (int i = 0; i < 3; i++)
	{
		buffers[i] = (attachmentMask & (1u << i)) ? (GLenum)(GL_COLOR_ATTACHMENT0 + i) : (GLenum)GL_NONE;
		if (attachmentMask & (1u << i))
			count = i + 1;   // trailing unused slots are simply left off
	}
	if (count == 0)
	{
		GLenum none = GL_NONE;
		glDrawBuffers(1, &none);
		return;
	}
	glDrawBuffers(count, buffers);
}

bool GLDevice::IsFramebufferComplete(FramebufferHandle framebuffer, std::string* error)
{
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.id);
	const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE && error != nullptr)
		*error = std::to_string(status);
	return status == GL_FRAMEBUFFER_COMPLETE;
}

// ---------------------------------------------------------------- commands

void GLDevice::BindFramebuffer(FramebufferHandle framebuffer)
{
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.id);
}

void GLDevice::SetViewport(int x, int y, int width, int height)
{
	glViewport(x, y, width, height);
}

void GLDevice::Clear(bool color, bool depth, const glm::vec4& clearColor)
{
	GLbitfield mask = 0;
	if (color)
	{
		glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
		mask |= GL_COLOR_BUFFER_BIT;
	}
	if (depth)
		mask |= GL_DEPTH_BUFFER_BIT;
	if (mask != 0)
		glClear(mask);
}

void GLDevice::Draw(VertexArrayHandle vao, Primitive primitive, int first, int count, int instances)
{
	glBindVertexArray(vao.id);
	if (instances > 0)
		glDrawArraysInstanced(ToGL(primitive), first, count, instances);
	else
		glDrawArrays(ToGL(primitive), first, count);
	glBindVertexArray(0);
}

void GLDevice::DrawIndexed(VertexArrayHandle vao, Primitive primitive, int indexCount, int instances)
{
	glBindVertexArray(vao.id);
	if (instances > 0)
		glDrawElementsInstanced(ToGL(primitive), indexCount, GL_UNSIGNED_INT, 0, instances);
	else
		glDrawElements(ToGL(primitive), indexCount, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
}

// ---------------------------------------------------------------- GPU-driven

bool GLDevice::SupportsGpuDriven() const
{
#ifdef __EMSCRIPTEN__
	return false;
#else
	// Compute shaders, storage buffers and multi-draw-indirect are core in 4.3.
	return ShaderProgram::glslVersion >= 430 && glDispatchCompute != nullptr
		&& glMultiDrawElementsIndirect != nullptr && glBindBufferRange != nullptr;
#endif
}

ProgramHandle GLDevice::CreateComputeProgram(const char* source, std::string& log)
{
	log.clear();
#ifdef __EMSCRIPTEN__
	(void)source;
	log = "compute shaders are not available on WebGL";
	return ProgramHandle();
#else
	std::string cachePath;
	if (programCache.Usable())
	{
		cachePath = programCache.PathFor({ "compute", source });
		if (GLuint cached = programCache.Load(cachePath))
			return ProgramHandle(cached);
	}

	GLuint shader = glCreateShader(GL_COMPUTE_SHADER);
	glShaderSource(shader, 1, &source, nullptr);
	glCompileShader(shader);
	GLint ok = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
	if (!ok)
	{
		GLchar info[1024] = { 0 };
		glGetShaderInfoLog(shader, sizeof(info), nullptr, info);
		log = std::string("Error compiling the compute shader: ") + info + "\n";
		glDeleteShader(shader);
		return ProgramHandle();
	}
	GLuint program = glCreateProgram();
	glAttachShader(program, shader);
	if (!cachePath.empty())
		glProgramParameteri(program, GL_PROGRAM_BINARY_RETRIEVABLE_HINT, GL_TRUE);
	glLinkProgram(program);
	glDeleteShader(shader);
	GLint linked = 0;
	glGetProgramiv(program, GL_LINK_STATUS, &linked);
	if (!linked)
	{
		GLchar info[1024] = { 0 };
		glGetProgramInfoLog(program, sizeof(info), nullptr, info);
		log = std::string("Error linking compute program: '") + info + "'\n";
		glDeleteProgram(program);
		return ProgramHandle();
	}
	if (!cachePath.empty())
		programCache.Save(cachePath, program);
	return ProgramHandle(program);
#endif
}

void GLDevice::BindStorageBuffer(unsigned int binding, BufferHandle buffer, size_t offset, size_t bytes)
{
#ifndef __EMSCRIPTEN__
	if (bytes == 0)
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, buffer.id);
	else
		glBindBufferRange(GL_SHADER_STORAGE_BUFFER, binding, buffer.id, (GLintptr)offset, (GLsizeiptr)bytes);
#else
	(void)binding; (void)buffer; (void)offset; (void)bytes;
#endif
}

void GLDevice::Dispatch(unsigned int groupsX, unsigned int groupsY, unsigned int groupsZ)
{
#ifndef __EMSCRIPTEN__
	glDispatchCompute(groupsX, groupsY, groupsZ);
#else
	(void)groupsX; (void)groupsY; (void)groupsZ;
#endif
}

void GLDevice::GpuBarrier(unsigned int barrierBits)
{
#ifndef __EMSCRIPTEN__
	GLbitfield bits = 0;
	if (barrierBits & BarrierStorage) bits |= GL_SHADER_STORAGE_BARRIER_BIT;
	if (barrierBits & BarrierIndirect) bits |= GL_COMMAND_BARRIER_BIT;
	if (barrierBits & BarrierVertexAttributes) bits |= GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT;
	if (barrierBits & BarrierImage) bits |= GL_SHADER_IMAGE_ACCESS_BARRIER_BIT;
	if (barrierBits & BarrierTextureFetch) bits |= GL_TEXTURE_FETCH_BARRIER_BIT;
	if (barrierBits & BarrierBufferRead) bits |= GL_BUFFER_UPDATE_BARRIER_BIT;
	if (bits != 0)
		glMemoryBarrier(bits);
#else
	(void)barrierBits;
#endif
}

void GLDevice::SetVertexAttributeInt(VertexArrayHandle vao, unsigned int location, BufferHandle buffer,
	int components, size_t stride, size_t offset, unsigned int divisor)
{
	glBindVertexArray(vao.id);
	glBindBuffer(GL_ARRAY_BUFFER, buffer.id);
	glVertexAttribIPointer(location, components, GL_UNSIGNED_INT, (GLsizei)stride, (void*)offset);
	glEnableVertexAttribArray(location);
	glVertexAttribDivisor(location, divisor);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void GLDevice::MultiDrawIndexedIndirect(VertexArrayHandle vao, Primitive primitive, BufferHandle commands,
	size_t offset, int drawCount, size_t stride)
{
#ifndef __EMSCRIPTEN__
	if (drawCount <= 0)
		return;
	glBindVertexArray(vao.id);
	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, commands.id);
	glMultiDrawElementsIndirect(ToGL(primitive), GL_UNSIGNED_INT, (const void*)offset, drawCount, (GLsizei)stride);
	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
	glBindVertexArray(0);
#else
	(void)vao; (void)primitive; (void)commands; (void)offset; (void)drawCount; (void)stride;
#endif
}

void GLDevice::BindImage(unsigned int unit, TextureHandle texture, int level, ImageAccess access,
	TextureFormat format)
{
#ifndef __EMSCRIPTEN__
	const GLenum glAccess = (access == ImageAccess::Read) ? GL_READ_ONLY
		: (access == ImageAccess::Write) ? GL_WRITE_ONLY : GL_READ_WRITE;
	GLenum glFormat = GL_RGBA8;
	switch (format)
	{
	case TextureFormat::R32F:     glFormat = GL_R32F; break;
	case TextureFormat::R8:       glFormat = GL_R8; break;
	case TextureFormat::RGBA16F:  glFormat = GL_RGBA16F; break;
	case TextureFormat::RGBA32UI: glFormat = GL_RGBA32UI; break;
	default:                      glFormat = GL_RGBA8; break;
	}
	glBindImageTexture(unit, texture.id, level, GL_FALSE, 0, glAccess, glFormat);
#else
	(void)unit; (void)texture; (void)level; (void)access; (void)format;
#endif
}

void GLDevice::ReadBuffer(BufferHandle buffer, size_t offset, size_t bytes, void* out)
{
#ifndef __EMSCRIPTEN__
	glBindBuffer(GL_COPY_READ_BUFFER, buffer.id);
	glGetBufferSubData(GL_COPY_READ_BUFFER, (GLintptr)offset, (GLsizeiptr)bytes, out);
	glBindBuffer(GL_COPY_READ_BUFFER, 0);
#else
	(void)buffer; (void)offset; (void)bytes; (void)out;
#endif
}

// ---------------------------------------------------------------- programs

ProgramHandle GLDevice::CreateProgram(const char* vertexSource, const char* fragmentSource, std::string& log)
{
	log.clear();
#ifndef __EMSCRIPTEN__
	std::string cachePath;
	if (programCache.Usable())
	{
		cachePath = programCache.PathFor({ vertexSource, fragmentSource });
		if (GLuint cached = programCache.Load(cachePath))
			return ProgramHandle(cached);
	}
#endif
	GLuint program = glCreateProgram();
	if (program == 0)
	{
		log = "Error creating shader program!";
		return ProgramHandle();
	}

	auto compile = [&](const char* source, GLenum stage, const char* stageName) -> bool
	{
		GLuint shader = glCreateShader(stage);
		glShaderSource(shader, 1, &source, nullptr);
		glCompileShader(shader);
		GLint ok = 0;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
		if (!ok)
		{
			GLchar info[1024] = { 0 };
			glGetShaderInfoLog(shader, sizeof(info), nullptr, info);
			log += std::string("Error compiling the ") + stageName + " shader: " + info + "\n";
			glDeleteShader(shader);
			return false;
		}
		glAttachShader(program, shader);
		glDeleteShader(shader);   // freed once the program is
		return true;
	};
	const bool vsOk = compile(vertexSource, GL_VERTEX_SHADER, "vertex");
	const bool fsOk = compile(fragmentSource, GL_FRAGMENT_SHADER, "fragment");

	GLint linked = 0;
	if (vsOk && fsOk)
	{
#ifndef __EMSCRIPTEN__
		if (!cachePath.empty())
			glProgramParameteri(program, GL_PROGRAM_BINARY_RETRIEVABLE_HINT, GL_TRUE);
#endif
		glLinkProgram(program);
		glGetProgramiv(program, GL_LINK_STATUS, &linked);
		if (!linked)
		{
			GLchar info[1024] = { 0 };
			glGetProgramInfoLog(program, sizeof(info), nullptr, info);
			log += std::string("Error linking program: '") + info + "'\n";
		}
	}
	if (!linked)
	{
		glDeleteProgram(program);
		return ProgramHandle();
	}

	glValidateProgram(program);
	GLint valid = 0;
	glGetProgramiv(program, GL_VALIDATE_STATUS, &valid);
	if (!valid)
	{
		GLchar info[1024] = { 0 };
		glGetProgramInfoLog(program, sizeof(info), nullptr, info);
		log += std::string("Error validating program: '") + info + "'\n";
	}
#ifndef __EMSCRIPTEN__
	if (!cachePath.empty())
		programCache.Save(cachePath, program);
#endif
	return ProgramHandle(program);
}

void GLDevice::DestroyProgram(ProgramHandle& program)
{
	if (program.id != 0)
		glDeleteProgram(program.id);
	program = ProgramHandle();
}

void GLDevice::UseProgram(ProgramHandle program)
{
	glUseProgram(program.id);
}

int GLDevice::UniformLocation(ProgramHandle program, const char* name)
{
	return glGetUniformLocation(program.id, name);
}

bool GLDevice::HasUniformBlock(ProgramHandle program, const char* name)
{
	return glGetUniformBlockIndex(program.id, name) != GL_INVALID_INDEX;
}

void GLDevice::SetUniformBlockBinding(ProgramHandle program, const char* name, unsigned int binding)
{
	const GLuint index = glGetUniformBlockIndex(program.id, name);
	if (index != GL_INVALID_INDEX)
		glUniformBlockBinding(program.id, index, binding);
}

int GLDevice::UniformBlockSize(ProgramHandle program, const char* blockName)
{
	const GLuint index = glGetUniformBlockIndex(program.id, blockName);
	if (index == GL_INVALID_INDEX)
		return -1;
	GLint size = -1;
	glGetActiveUniformBlockiv(program.id, index, GL_UNIFORM_BLOCK_DATA_SIZE, &size);
	return size;
}

bool GLDevice::UniformOffset(ProgramHandle program, const char* name, int& offset, int& arrayStride)
{
	GLuint index = GL_INVALID_INDEX;
	glGetUniformIndices(program.id, 1, &name, &index);
	if (index == GL_INVALID_INDEX)
		return false;
	GLint o = -1, s = 0;
	glGetActiveUniformsiv(program.id, 1, &index, GL_UNIFORM_OFFSET, &o);
	glGetActiveUniformsiv(program.id, 1, &index, GL_UNIFORM_ARRAY_STRIDE, &s);
	offset = o;
	arrayStride = s;
	return true;
}

void GLDevice::SetUniform(int location, int value) { glUniform1i(location, value); }
void GLDevice::SetUniform(int location, float value) { glUniform1f(location, value); }
void GLDevice::SetUniform(int location, const glm::vec2& value) { glUniform2fv(location, 1, glm::value_ptr(value)); }
void GLDevice::SetUniform(int location, const glm::vec3& value) { glUniform3fv(location, 1, glm::value_ptr(value)); }
void GLDevice::SetUniform(int location, const glm::vec4& value) { glUniform4fv(location, 1, glm::value_ptr(value)); }
void GLDevice::SetUniform(int location, const glm::mat3& value) { glUniformMatrix3fv(location, 1, GL_FALSE, glm::value_ptr(value)); }
void GLDevice::SetUniform(int location, const glm::mat4& value) { glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(value)); }
void GLDevice::SetUniformArray(int location, const int* values, int count) { glUniform1iv(location, count, values); }
void GLDevice::SetUniformArray(int location, const float* values, int count) { glUniform1fv(location, count, values); }
void GLDevice::SetUniformArray(int location, const glm::vec3* values, int count) { glUniform3fv(location, count, glm::value_ptr(values[0])); }

// ---------------------------------------------------------------- state

void GLDevice::ApplyState(const RenderState& s, bool force)
{
	if (force || s.blend != state.blend)
	{
		if (s.blend == BlendMode::Off)
		{
			glDisable(GL_BLEND);
		}
		else
		{
			glEnable(GL_BLEND);
			if (s.blend == BlendMode::Additive)
				glBlendFunc(GL_ONE, GL_ONE);
			else if (s.blend == BlendMode::Premultiplied)
				glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);   // src + dst * (1 - src.a)
			else
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		}
	}
	if (force || s.depthTest != state.depthTest)
	{
		if (s.depthTest) glEnable(GL_DEPTH_TEST);
		else glDisable(GL_DEPTH_TEST);
	}
	if (force || s.depthWrite != state.depthWrite)
		glDepthMask(s.depthWrite ? GL_TRUE : GL_FALSE);
	if (force || s.depthCompare != state.depthCompare)
		glDepthFunc(ToGL(s.depthCompare));
	if (force || s.cull != state.cull)
	{
		if (s.cull == CullMode::None)
		{
			glDisable(GL_CULL_FACE);
		}
		else
		{
			glEnable(GL_CULL_FACE);
			glCullFace(s.cull == CullMode::Front ? GL_FRONT : GL_BACK);
		}
	}
	if (force || s.depthBiasFactor != state.depthBiasFactor || s.depthBiasUnits != state.depthBiasUnits)
	{
		if (s.depthBiasFactor == 0.0f && s.depthBiasUnits == 0.0f)
		{
			glDisable(GL_POLYGON_OFFSET_FILL);
			glPolygonOffset(0.0f, 0.0f);
		}
		else
		{
			glEnable(GL_POLYGON_OFFSET_FILL);
			glPolygonOffset(s.depthBiasFactor, s.depthBiasUnits);
		}
	}
	state = s;
}

// ---------------------------------------------------------------- sync, readback, debug

FenceHandle GLDevice::InsertFence()
{
	FenceHandle fence;
#ifndef __EMSCRIPTEN__
	fence.sync = (void*)glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
#endif
	return fence;
}

void GLDevice::WaitFence(FenceHandle& fence)
{
#ifndef __EMSCRIPTEN__
	if (fence.sync != nullptr)
	{
		GLsync s = (GLsync)fence.sync;
		while (glClientWaitSync(s, GL_SYNC_FLUSH_COMMANDS_BIT, 1000000000ull) == GL_TIMEOUT_EXPIRED)
		{
		}
		glDeleteSync(s);
	}
#endif
	fence.sync = nullptr;
}

void GLDevice::DeleteFence(FenceHandle& fence)
{
#ifndef __EMSCRIPTEN__
	if (fence.sync != nullptr)
		glDeleteSync((GLsync)fence.sync);
#endif
	fence.sync = nullptr;
}

void GLDevice::Finish()
{
	glFinish();
}

void GLDevice::ReadPixels(int x, int y, int width, int height, ReadbackFormat format, void* out)
{
	// Rows tightly packed (a BGR row isn't always a multiple of 4 bytes).
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	if (format == ReadbackFormat::RGBA8)
	{
		glReadPixels(x, y, width, height, GL_RGBA, GL_UNSIGNED_BYTE, out);
	}
	else
	{
#ifdef __EMSCRIPTEN__
		// GLES has no GL_BGR: read RGBA and repack.
		std::vector<unsigned char> rgba((size_t)width * height * 4);
		glReadPixels(x, y, width, height, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
		unsigned char* dst = (unsigned char*)out;
		for (size_t i = 0, n = (size_t)width * height; i < n; i++)
		{
			dst[i * 3 + 0] = rgba[i * 4 + 2];
			dst[i * 3 + 1] = rgba[i * 4 + 1];
			dst[i * 3 + 2] = rgba[i * 4 + 0];
		}
#else
		glReadPixels(x, y, width, height, GL_BGR, GL_UNSIGNED_BYTE, out);
#endif
	}
	glPixelStorei(GL_PACK_ALIGNMENT, 4);
}

void GLDevice::PushDebugGroup(const char* name)
{
#ifndef __EMSCRIPTEN__
	if (ShaderProgram::glslVersion >= 430 && glPushDebugGroup != nullptr)
		glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, name);
#else
	(void)name;
#endif
}

void GLDevice::PopDebugGroup()
{
#ifndef __EMSCRIPTEN__
	if (ShaderProgram::glslVersion >= 430 && glPopDebugGroup != nullptr)
		glPopDebugGroup();
#endif
}

// ---------------------------------------------------------------- capabilities

bool GLDevice::SupportsCubeMapArrays() const
{
#ifdef __EMSCRIPTEN__
	return false;
#else
	return ShaderProgram::glslVersion >= 400;
#endif
}

bool GLDevice::SupportsPersistentMapping() const
{
#ifdef __EMSCRIPTEN__
	return false;
#else
	return ShaderProgram::glslVersion >= 440 && glBufferStorage != nullptr;
#endif
}

// ---------------------------------------------------------------- timestamps

bool GLDevice::SupportsTimestamps() const
{
#ifdef __EMSCRIPTEN__
	return false;   // WebGL2 only has them through an optional, often-disabled extension
#else
	return true;    // core since GL 3.3 (ARB_timer_query)
#endif
}

QueryHandle GLDevice::CreateQuery()
{
#ifndef __EMSCRIPTEN__
	GLuint id = 0;
	glGenQueries(1, &id);
	return QueryHandle(id);
#else
	return QueryHandle();
#endif
}

void GLDevice::DestroyQuery(QueryHandle& query)
{
#ifndef __EMSCRIPTEN__
	if (query.id != 0)
	{
		GLuint id = query.id;
		glDeleteQueries(1, &id);
	}
#endif
	query = QueryHandle();
}

void GLDevice::WriteTimestamp(QueryHandle query)
{
#ifndef __EMSCRIPTEN__
	if (query.id != 0)
		glQueryCounter(query.id, GL_TIMESTAMP);
#else
	(void)query;
#endif
}

bool GLDevice::ReadTimestamp(QueryHandle query, uint64_t& nanoseconds)
{
#ifndef __EMSCRIPTEN__
	if (query.id == 0)
		return false;
	GLint available = 0;
	glGetQueryObjectiv(query.id, GL_QUERY_RESULT_AVAILABLE, &available);
	if (!available)
		return false;
	GLuint64 value = 0;
	glGetQueryObjectui64v(query.id, GL_QUERY_RESULT, &value);
	nanoseconds = (uint64_t)value;
	return true;
#else
	(void)query;
	(void)nanoseconds;
	return false;
#endif
}

bool GLDevice::SupportsFloatRenderTargets() const
{
#ifdef __EMSCRIPTEN__
	// WebGL2 renders to RGBA16F only with EXT_color_buffer_float (enabled at
	// context creation when the browser offers it).
	static const bool has = []()
	{
		const char* ext = (const char*)glGetString(GL_EXTENSIONS);
		return ext != nullptr && std::string(ext).find("EXT_color_buffer_float") != std::string::npos;
	}();
	return has;
#else
	return true;   // core since GL 3.0
#endif
}
