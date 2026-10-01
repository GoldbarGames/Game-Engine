#include "GLDevice.h"
#include "../../Shader.h"
#include "../../opengl_includes.h"
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <memory>
#include <vector>

namespace
{
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
#ifndef __EMSCRIPTEN__
		case TextureType::CubeArray: return GL_TEXTURE_CUBE_MAP_ARRAY;
#endif
		case TextureType::Tex2D:
		default:                     return GL_TEXTURE_2D;
		}
	}

	struct GLFormat { GLint internal; GLenum format; GLenum type; };
	GLFormat FormatOf(TextureFormat f)
	{
		switch (f)
		{
		case TextureFormat::R8:              return { GL_R8, GL_RED, GL_UNSIGNED_BYTE };
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
		case Attachment::Depth:        return GL_DEPTH_ATTACHMENT;
		case Attachment::DepthStencil: return GL_DEPTH_STENCIL_ATTACHMENT;
		case Attachment::Color0:
		default:                       return GL_COLOR_ATTACHMENT0;
		}
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

#ifdef GL_TEXTURE_MAX_ANISOTROPY_EXT
	if (desc.maxAnisotropy > 1.0f)
	{
		GLfloat maxAniso = 0.0f;
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAniso);
		if (maxAniso > 1.0f)
			glTexParameterf(target, GL_TEXTURE_MAX_ANISOTROPY_EXT,
				maxAniso < desc.maxAnisotropy ? maxAniso : desc.maxAnisotropy);
	}
#endif

	switch (desc.type)
	{
	case TextureType::Cube:
		for (int face = 0; face < 6; face++)
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, fmt.internal, desc.width, desc.height, 0,
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
		break;
	}

	if (desc.generateMipmaps)
		glGenerateMipmap(target);

	glBindTexture(target, 0);
	return TextureHandle(id);
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
	TextureType type, int layer)
{
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.id);
	const GLenum point = AttachmentOf(attachment);
	switch (type)
	{
	case TextureType::Cube:
		glFramebufferTexture2D(GL_FRAMEBUFFER, point, GL_TEXTURE_CUBE_MAP_POSITIVE_X + layer, texture.id, 0);
		break;
	case TextureType::CubeArray:
		glFramebufferTextureLayer(GL_FRAMEBUFFER, point, texture.id, 0, layer);
		break;
	case TextureType::Tex2D:
	default:
		glFramebufferTexture2D(GL_FRAMEBUFFER, point, GL_TEXTURE_2D, texture.id, 0);
		break;
	}
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

// ---------------------------------------------------------------- programs

ProgramHandle GLDevice::CreateProgram(const char* vertexSource, const char* fragmentSource, std::string& log)
{
	log.clear();
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
