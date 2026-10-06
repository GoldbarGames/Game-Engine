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
	TextureHandle CreateTextureLevels(const TextureDesc& desc, const TextureLevel* levels, int levelCount) override;
	bool SupportsTextureFormat(TextureFormat format) const override;
	void DestroyTexture(TextureHandle& texture) override;
	void UpdateTexture(TextureHandle texture, TextureFormat format, int x, int y, int width, int height,
		const void* data) override;
	void GenerateMipmaps(TextureHandle texture) override;
	void BindTexture(unsigned int unit, TextureHandle texture, TextureType type) override;

	FramebufferHandle CreateFramebuffer() override;
	void DestroyFramebuffer(FramebufferHandle& framebuffer) override;
	void AttachTexture(FramebufferHandle framebuffer, Attachment attachment, TextureHandle texture,
		TextureType type, int layer, int mipLevel) override;
	void SetDrawBuffers(FramebufferHandle framebuffer, int colorCount) override;
	void SetBoundDrawBuffers(int colorCount) override;
	void SetBoundDrawBufferMask(unsigned int attachmentMask) override;
	bool IsFramebufferComplete(FramebufferHandle framebuffer, std::string* error) override;

	RenderbufferHandle CreateRenderbuffer(TextureFormat format, int width, int height, int samples) override;
	void DestroyRenderbuffer(RenderbufferHandle& renderbuffer) override;
	void AttachRenderbuffer(FramebufferHandle framebuffer, Attachment attachment, RenderbufferHandle renderbuffer) override;
	int MaxSamples(TextureFormat format) override;
	void BlitFramebuffer(FramebufferHandle src, FramebufferHandle dst, int width, int height,
		unsigned int colorMask, bool depth) override;

	void BindFramebuffer(FramebufferHandle framebuffer) override;
	void SetViewport(int x, int y, int width, int height) override;
	void Clear(bool color, bool depth, const glm::vec4& clearColor) override;
	void Draw(VertexArrayHandle vao, Primitive primitive, int first, int count, int instances) override;
	void DrawIndexed(VertexArrayHandle vao, Primitive primitive, int indexCount, int instances) override;

	bool SupportsGpuDriven() const override;
	ProgramHandle CreateComputeProgram(const char* source, std::string& log) override;
	void BindStorageBuffer(unsigned int binding, BufferHandle buffer, size_t offset, size_t bytes) override;
	void Dispatch(unsigned int groupsX, unsigned int groupsY, unsigned int groupsZ) override;
	void GpuBarrier(unsigned int barrierBits) override;
	void SetVertexAttributeInt(VertexArrayHandle vao, unsigned int location, BufferHandle buffer,
		int components, size_t stride, size_t offset, unsigned int divisor) override;
	void MultiDrawIndexedIndirect(VertexArrayHandle vao, Primitive primitive, BufferHandle commands,
		size_t offset, int drawCount, size_t stride) override;
	void BindImage(unsigned int unit, TextureHandle texture, int level, ImageAccess access,
		TextureFormat format) override;
	void ReadBuffer(BufferHandle buffer, size_t offset, size_t bytes, void* out) override;

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
	bool SupportsTimestamps() const override;
	QueryHandle CreateQuery() override;
	void DestroyQuery(QueryHandle& query) override;
	void WriteTimestamp(QueryHandle query) override;
	bool ReadTimestamp(QueryHandle query, uint64_t& nanoseconds) override;

	void ReadPixels(int x, int y, int width, int height, ReadbackFormat format, void* out) override;

	void PushDebugGroup(const char* name) override;
	void PopDebugGroup() override;

	bool SupportsCubeMapArrays() const override;
	bool SupportsPersistentMapping() const override;
	bool SupportsFloatRenderTargets() const override;

private:
	RenderState state;
	// Texture-bind cache for unit 0 (where nearly every sprite binds), with
	// the same semantics the old Texture::UseTexture cache had. Invalidated
	// whenever the device itself binds a texture for another reason.
	unsigned int boundUnit0 = 0;
	bool unit0Known = false;
};

#endif
