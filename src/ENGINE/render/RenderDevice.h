#ifndef RENDER_DEVICE_H
#define RENDER_DEVICE_H
#pragma once

// Engine-internal (not exported). The graphics API boundary: everything the
// engine asks of the GPU goes through RenderDevice, and only its backends
// (render/gl/ today, a Vulkan one later) know the underlying API. No GL types
// appear here.
//
// Shape: resources are created through the device and named by small typed
// handles; drawing is "bind a target, bind a pipeline (program + RenderState),
// bind vertex input and textures, set per-draw values, draw". The GL backend
// executes immediately; handles are the GL object names, so there is no
// lookup cost.

#include "../RenderState.h"
#include <cstddef>
#include <cstdint>
#include <string>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>

// ---- handles ---------------------------------------------------------------
// One type per resource kind so they can't be mixed up. id 0 = none.
#define KINJO_DEVICE_HANDLE(Name)                                   \
	struct Name                                                     \
	{                                                               \
		uint32_t id = 0;                                            \
		Name() = default;                                           \
		explicit Name(uint32_t i) : id(i) {}                        \
		explicit operator bool() const { return id != 0; }          \
		bool operator==(const Name& o) const { return id == o.id; } \
		bool operator!=(const Name& o) const { return id != o.id; } \
	}
KINJO_DEVICE_HANDLE(BufferHandle);
KINJO_DEVICE_HANDLE(TextureHandle);
KINJO_DEVICE_HANDLE(FramebufferHandle);     // 0 = the window
KINJO_DEVICE_HANDLE(VertexArrayHandle);
KINJO_DEVICE_HANDLE(ProgramHandle);
#undef KINJO_DEVICE_HANDLE

struct FenceHandle
{
	void* sync = nullptr;
	explicit operator bool() const { return sync != nullptr; }
};

// ---- descriptions ----------------------------------------------------------
enum class BufferUsage : uint8_t { Static, Dynamic, Stream };

enum class TextureType : uint8_t { Tex2D, Cube, CubeArray };
enum class TextureFormat : uint8_t
{
	RGBA8,
	R8,
	Depth24,          // sampled shadow maps
	Depth24Stencil8,  // framebuffer depth that post-process passes sample
};
enum class TextureFilter : uint8_t { Nearest, Linear, Trilinear };   // Trilinear = linear + mipmaps
enum class TextureWrap : uint8_t { Repeat, ClampToEdge, ClampToBorder };

struct TextureDesc
{
	TextureType type = TextureType::Tex2D;
	TextureFormat format = TextureFormat::RGBA8;
	int width = 1;
	int height = 1;
	int layers = 1;                  // CubeArray: number of cubes
	TextureFilter filter = TextureFilter::Nearest;
	TextureWrap wrap = TextureWrap::ClampToEdge;
	glm::vec4 borderColor = glm::vec4(1.0f);   // ClampToBorder only
	float maxAnisotropy = 1.0f;      // clamped to what the hardware allows
	bool generateMipmaps = false;    // after the initial upload
};

enum class Attachment : uint8_t { Color0, Color1, Depth, DepthStencil };
enum class ReadbackFormat : uint8_t { RGBA8, BGR8 };   // BGR8 = what SDL_SaveBMP / IMG_SavePNG surfaces take
enum class Primitive : uint8_t { Triangles, Lines };

// ---- the device --------------------------------------------------------------
class RenderDevice
{
public:
	virtual ~RenderDevice() = default;

	// Buffers. Index data needs CreateIndexBuffer: WebGL fixes a buffer's role
	// (index vs everything else) the first time it is bound.
	virtual BufferHandle CreateBuffer(size_t bytes, const void* data, BufferUsage usage) = 0;
	virtual BufferHandle CreateIndexBuffer(size_t bytes, const void* data) = 0;
	virtual void UpdateBuffer(BufferHandle buffer, size_t offset, size_t bytes, const void* data) = 0;
	// Re-specify the whole store (lets the driver orphan the old one).
	virtual void ReplaceBuffer(BufferHandle buffer, size_t bytes, const void* data, BufferUsage usage) = 0;
	// A buffer mapped for CPU writes for its whole life (coherent). Returns the
	// mapping, or nullptr if the backend can't (the buffer is then not created).
	virtual void* CreatePersistentBuffer(size_t bytes, BufferHandle& out) = 0;
	virtual void DestroyBuffer(BufferHandle& buffer) = 0;
	virtual void BindUniformBuffer(unsigned int binding, BufferHandle buffer) = 0;

	// Vertex input
	virtual VertexArrayHandle CreateVertexArray() = 0;
	virtual void DestroyVertexArray(VertexArrayHandle& vao) = 0;
	// Float attribute `location` reads `components` floats per vertex (or per
	// instance when divisor > 0) from `buffer` at offset + i * stride.
	virtual void SetVertexAttribute(VertexArrayHandle vao, unsigned int location, BufferHandle buffer,
		int components, size_t stride, size_t offset, unsigned int divisor = 0) = 0;
	virtual void DisableVertexAttribute(VertexArrayHandle vao, unsigned int location) = 0;
	virtual void SetIndexBuffer(VertexArrayHandle vao, BufferHandle buffer) = 0;
	// Leave `vao` bound (Draw/DrawIndexed bind their own; this is for the few
	// places that need one bound outside a draw, e.g. shader validation on macOS).
	virtual void BindVertexArray(VertexArrayHandle vao) = 0;

	// Textures
	virtual TextureHandle CreateTexture(const TextureDesc& desc, const void* rgbaPixels = nullptr) = 0;
	virtual void DestroyTexture(TextureHandle& texture) = 0;
	virtual void BindTexture(unsigned int unit, TextureHandle texture, TextureType type = TextureType::Tex2D) = 0;

	// Framebuffers. `layer` selects a cube face (Cube) or face-layer (CubeArray).
	virtual FramebufferHandle CreateFramebuffer() = 0;
	virtual void DestroyFramebuffer(FramebufferHandle& framebuffer) = 0;
	virtual void AttachTexture(FramebufferHandle framebuffer, Attachment attachment, TextureHandle texture,
		TextureType type = TextureType::Tex2D, int layer = 0) = 0;
	// How many colour attachments draws write (0 = depth-only).
	virtual void SetDrawBuffers(FramebufferHandle framebuffer, int colorCount) = 0;
	// The same for whichever framebuffer is bound (a draw that adds a second
	// output mid-pass, like the character mask), leaving it bound.
	virtual void SetBoundDrawBuffers(int colorCount) = 0;
	virtual bool IsFramebufferComplete(FramebufferHandle framebuffer, std::string* error = nullptr) = 0;

	// Commands
	virtual void BindFramebuffer(FramebufferHandle framebuffer) = 0;
	virtual void SetViewport(int x, int y, int width, int height) = 0;
	virtual void Clear(bool color, bool depth, const glm::vec4& clearColor = glm::vec4(0.0f)) = 0;
	virtual void Draw(VertexArrayHandle vao, Primitive primitive, int first, int count, int instances = 0) = 0;
	virtual void DrawIndexed(VertexArrayHandle vao, Primitive primitive, int indexCount, int instances = 0) = 0;

	// Programs
	virtual ProgramHandle CreateProgram(const char* vertexSource, const char* fragmentSource, std::string& log) = 0;
	virtual void DestroyProgram(ProgramHandle& program) = 0;
	virtual void UseProgram(ProgramHandle program) = 0;
	virtual int UniformLocation(ProgramHandle program, const char* name) = 0;
	virtual bool HasUniformBlock(ProgramHandle program, const char* name) = 0;
	virtual void SetUniformBlockBinding(ProgramHandle program, const char* name, unsigned int binding) = 0;
	// Layout introspection for the startup block check (-1 if unknown).
	virtual int UniformBlockSize(ProgramHandle program, const char* blockName) = 0;
	virtual bool UniformOffset(ProgramHandle program, const char* name, int& offset, int& arrayStride) = 0;
	// Values for the currently used program; location -1 is ignored.
	virtual void SetUniform(int location, int value) = 0;
	virtual void SetUniform(int location, float value) = 0;
	virtual void SetUniform(int location, const glm::vec2& value) = 0;
	virtual void SetUniform(int location, const glm::vec3& value) = 0;
	virtual void SetUniform(int location, const glm::vec4& value) = 0;
	virtual void SetUniform(int location, const glm::mat3& value) = 0;
	virtual void SetUniform(int location, const glm::mat4& value) = 0;
	virtual void SetUniformArray(int location, const int* values, int count) = 0;
	virtual void SetUniformArray(int location, const float* values, int count) = 0;
	virtual void SetUniformArray(int location, const glm::vec3* values, int count) = 0;

	// Fixed-function state (see RenderState.h); applies only what differs from
	// the tracked state unless `force`.
	virtual void ApplyState(const RenderState& state, bool force) = 0;
	virtual const RenderState& CurrentState() const = 0;

	// Synchronisation
	virtual FenceHandle InsertFence() = 0;
	virtual void WaitFence(FenceHandle& fence) = 0;    // and delete it
	virtual void DeleteFence(FenceHandle& fence) = 0;
	virtual void Finish() = 0;

	// Readback of the bound framebuffer, tightly packed, bottom-up rows.
	virtual void ReadPixels(int x, int y, int width, int height, ReadbackFormat format, void* out) = 0;

	// GPU-capture labels (RenderDoc / Nsight); no-ops where unsupported.
	virtual void PushDebugGroup(const char* name) = 0;
	virtual void PopDebugGroup() = 0;

	// Capabilities
	virtual bool SupportsCubeMapArrays() const = 0;
	virtual bool SupportsPersistentMapping() const = 0;
};

// The active device. Created on first use (the GL backend, the only one
// today); needs a current graphics context.
RenderDevice& Device();

#endif
