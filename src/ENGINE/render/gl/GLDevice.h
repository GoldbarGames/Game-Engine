#ifndef GL_DEVICE_H
#define GL_DEVICE_H
#pragma once

#include "../RenderDevice.h"

// The OpenGL (desktop 3.3-4.6 / WebGL2) implementation of RenderDevice.
// Handles are GL object names. Everything is executed immediately.
class GLDevice : public RenderDevice
{
public:
	GLDevice();

	BufferHandle CreateBuffer(size_t bytes, const void* data, BufferUsage usage) override;
	BufferHandle CreateIndexBuffer(size_t bytes, const void* data) override;
	void UpdateBuffer(BufferHandle buffer, size_t offset, size_t bytes, const void* data) override;
	void ReplaceBuffer(BufferHandle buffer, size_t bytes, const void* data, BufferUsage usage) override;
	void* CreatePersistentBuffer(size_t bytes, BufferHandle& out) override;
	void DestroyBuffer(BufferHandle& buffer) override;
	void BindUniformBuffer(unsigned int binding, BufferHandle buffer) override;

	VertexArrayHandle CreateVertexArray() override;
	void DestroyVertexArray(VertexArrayHandle& vao) override;
	void SetVertexAttribute(VertexArrayHandle vao, unsigned int location, BufferHandle buffer,
		int components, size_t stride, size_t offset, unsigned int divisor) override;
	void DisableVertexAttribute(VertexArrayHandle vao, unsigned int location) override;
	void SetIndexBuffer(VertexArrayHandle vao, BufferHandle buffer) override;
	void BindVertexArray(VertexArrayHandle vao) override;

	TextureHandle CreateTexture(const TextureDesc& desc, const void* rgbaPixels) override;
	void DestroyTexture(TextureHandle& texture) override;
	void BindTexture(unsigned int unit, TextureHandle texture, TextureType type) override;

	FramebufferHandle CreateFramebuffer() override;
	void DestroyFramebuffer(FramebufferHandle& framebuffer) override;
	void AttachTexture(FramebufferHandle framebuffer, Attachment attachment, TextureHandle texture,
		TextureType type, int layer) override;
	void SetDrawBuffers(FramebufferHandle framebuffer, int colorCount) override;
	void SetBoundDrawBuffers(int colorCount) override;
	bool IsFramebufferComplete(FramebufferHandle framebuffer, std::string* error) override;

	void BindFramebuffer(FramebufferHandle framebuffer) override;
	void SetViewport(int x, int y, int width, int height) override;
	void Clear(bool color, bool depth, const glm::vec4& clearColor) override;
	void Draw(VertexArrayHandle vao, Primitive primitive, int first, int count, int instances) override;
	void DrawIndexed(VertexArrayHandle vao, Primitive primitive, int indexCount, int instances) override;

	ProgramHandle CreateProgram(const char* vertexSource, const char* fragmentSource, std::string& log) override;
	void DestroyProgram(ProgramHandle& program) override;
	void UseProgram(ProgramHandle program) override;
	int UniformLocation(ProgramHandle program, const char* name) override;
	bool HasUniformBlock(ProgramHandle program, const char* name) override;
	void SetUniformBlockBinding(ProgramHandle program, const char* name, unsigned int binding) override;
	int UniformBlockSize(ProgramHandle program, const char* blockName) override;
	bool UniformOffset(ProgramHandle program, const char* name, int& offset, int& arrayStride) override;
	void SetUniform(int location, int value) override;
	void SetUniform(int location, float value) override;
	void SetUniform(int location, const glm::vec2& value) override;
	void SetUniform(int location, const glm::vec3& value) override;
	void SetUniform(int location, const glm::vec4& value) override;
	void SetUniform(int location, const glm::mat3& value) override;
	void SetUniform(int location, const glm::mat4& value) override;
	void SetUniformArray(int location, const int* values, int count) override;
	void SetUniformArray(int location, const float* values, int count) override;
	void SetUniformArray(int location, const glm::vec3* values, int count) override;

	void ApplyState(const RenderState& state, bool force) override;
	const RenderState& CurrentState() const override { return state; }

	FenceHandle InsertFence() override;
	void WaitFence(FenceHandle& fence) override;
	void DeleteFence(FenceHandle& fence) override;
	void Finish() override;

	void ReadPixels(int x, int y, int width, int height, ReadbackFormat format, void* out) override;

	void PushDebugGroup(const char* name) override;
	void PopDebugGroup() override;

	bool SupportsCubeMapArrays() const override;
	bool SupportsPersistentMapping() const override;

private:
	RenderState state;
	// Texture-bind cache for unit 0 (where nearly every sprite binds), with
	// the same semantics the old Texture::UseTexture cache had. Invalidated
	// whenever the device itself binds a texture for another reason.
	unsigned int boundUnit0 = 0;
	bool unit0Known = false;
};

#endif
