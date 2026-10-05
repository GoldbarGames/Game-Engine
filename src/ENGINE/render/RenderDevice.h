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
KINJO_DEVICE_HANDLE(QueryHandle);
#undef KINJO_DEVICE_HANDLE

struct FenceHandle
{
	void* sync = nullptr;
	explicit operator bool() const { return sync != nullptr; }
};

// ---- descriptions ----------------------------------------------------------
enum class BufferUsage : uint8_t { Static, Dynamic, Stream };

enum class TextureType : uint8_t { Tex2D, Cube, CubeArray, Tex2DArray };
enum class TextureFormat : uint8_t
{
	RGBA8,
	SRGB8_A8,         // colour art in a linear-workflow project: sampling returns linear values
	RGBA16F,          // HDR render target (linear-workflow world)
	R8,
	R32F,             // one float channel (linear view depth for ambient occlusion)
	RGBA32UI,         // four unsigned ints per texel, read with texelFetch (clustered light lists)
	Depth24,          // sampled shadow maps
	Depth24Stencil8,  // framebuffer depth that post-process passes sample
	// Block-compressed (4x4-texel blocks), uploaded pre-encoded from KTX2 files
	// (CreateTextureLevels). Check SupportsTextureFormat first.
	BC1,              // RGB + 1-bit alpha, 8 bytes a block
	BC1_SRGB,
	BC3,              // RGBA, 16 bytes a block
	BC3_SRGB,
	BC4,              // one channel (R), 8 bytes a block: masks, roughness, occlusion
	BC5,              // two channels (RG), 16 bytes a block: normal maps (z rebuilt in the shader)
	BC7,              // RGBA, 16 bytes a block, the best quality
	BC7_SRGB,
};

inline bool IsBlockCompressed(TextureFormat f)
{
	return f >= TextureFormat::BC1 && f <= TextureFormat::BC7_SRGB;
}

// Bytes in one 4x4 block of a block-compressed format.
inline int BlockBytes(TextureFormat f)
{
	return (f == TextureFormat::BC1 || f == TextureFormat::BC1_SRGB || f == TextureFormat::BC4) ? 8 : 16;
}

enum class TextureFilter : uint8_t { Nearest, Linear, Trilinear };   // Trilinear = linear + mipmaps
enum class TextureWrap : uint8_t { Repeat, ClampToEdge, ClampToBorder };

struct TextureDesc
{
	TextureType type = TextureType::Tex2D;
	TextureFormat format = TextureFormat::RGBA8;
	int width = 1;
	int height = 1;
	int layers = 1;                  // CubeArray: number of cubes; Tex2DArray: number of layers
	bool depthCompare = false;       // depth formats: shadow samplers compare (hardware PCF)
	TextureFilter filter = TextureFilter::Nearest;
	TextureWrap wrap = TextureWrap::ClampToEdge;
	glm::vec4 borderColor = glm::vec4(1.0f);   // ClampToBorder only
	float maxAnisotropy = 1.0f;      // clamped to what the hardware allows
	bool generateMipmaps = false;    // after the initial upload
	int mipLevels = 1;               // Tex2D: allocate this many levels (render targets drawn per level)
};

// One level of a texture's mip chain, as stored: `bytes` of texels in the
// format's layout (tightly packed rows, or 4x4 blocks in row order).
struct TextureLevel
{
	const void* data = nullptr;
	size_t bytes = 0;
};

// One indexed indirect draw, as the GPU reads it (DrawIndexedIndirect / VkDrawIndexedIndirectCommand).
struct DrawIndexedIndirectCommand
{
	uint32_t indexCount;
	uint32_t instanceCount;
	uint32_t firstIndex;
	int32_t baseVertex;
	uint32_t baseInstance;
};
static_assert(sizeof(DrawIndexedIndirectCommand) == 20, "indirect commands are 5 words");

enum GpuBarrierBit : unsigned int
{
	BarrierStorage = 1u,          // later shader storage reads/writes
	BarrierIndirect = 2u,         // later indirect draw parameters
	BarrierVertexAttributes = 4u, // later vertex attribute fetches
	BarrierImage = 8u,            // later image loads/stores (BindImage)
	BarrierTextureFetch = 16u,    // later texture sampling of image-written textures
	BarrierBufferRead = 32u,      // a later ReadBuffer of shader-written buffers
};

enum class ImageAccess : uint8_t { Read, Write, ReadWrite };

enum class Attachment : uint8_t { Color0, Color1, Depth, DepthStencil, Color2 };
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

	// Textures. `rgbaPixels` (optional) is level 0's texels: for a Tex2DArray,
	// every layer's, one after another. Cube and cube-array textures start empty.
	virtual TextureHandle CreateTexture(const TextureDesc& desc, const void* rgbaPixels = nullptr) = 0;
	// A 2D texture from its stored mip chain (a KTX2 file's levels): levels[0]
	// is desc.width x desc.height and each next one half the size, rounded down
	// (at least 1). Block-compressed formats upload as they are. With a single
	// level, desc.generateMipmaps builds the rest (uncompressed formats only).
	// desc.mipLevels is ignored.
	virtual TextureHandle CreateTextureLevels(const TextureDesc& desc, const TextureLevel* levels, int levelCount) = 0;
	// Whether this GPU can sample `format` (block-compressed formats need
	// hardware support: BC7 needs GL 4.2 or an extension, WebGL extensions).
	virtual bool SupportsTextureFormat(TextureFormat format) const = 0;
	virtual void DestroyTexture(TextureHandle& texture) = 0;
	// Replace a rectangle of a 2D texture's level 0 with `data`, tightly packed
	// in `format`'s texel layout (data streamed each frame, e.g. light lists).
	virtual void UpdateTexture(TextureHandle texture, TextureFormat format, int x, int y, int width, int height,
		const void* data) = 0;
	// Rebuild a 2D texture's mip chain from its level 0 (e.g. after rendering into it).
	virtual void GenerateMipmaps(TextureHandle texture) = 0;
	virtual void BindTexture(unsigned int unit, TextureHandle texture, TextureType type = TextureType::Tex2D) = 0;

	// Framebuffers. `layer` selects a cube face (Cube) or face-layer (CubeArray);
	// `mipLevel` the level of a 2D texture to draw into.
	virtual FramebufferHandle CreateFramebuffer() = 0;
	virtual void DestroyFramebuffer(FramebufferHandle& framebuffer) = 0;
	virtual void AttachTexture(FramebufferHandle framebuffer, Attachment attachment, TextureHandle texture,
		TextureType type = TextureType::Tex2D, int layer = 0, int mipLevel = 0) = 0;
	// How many colour attachments draws write (0 = depth-only).
	virtual void SetDrawBuffers(FramebufferHandle framebuffer, int colorCount) = 0;
	// The same for whichever framebuffer is bound (a draw that adds a second
	// output mid-pass, like the character mask), leaving it bound.
	virtual void SetBoundDrawBuffers(int colorCount) = 0;
	// The same by attachment: bit i enables colour attachment i (fragment
	// output location i); the others are not written. E.g. 0b101 = colour and
	// motion vectors without the character mask in between.
	virtual void SetBoundDrawBufferMask(unsigned int attachmentMask) = 0;
	virtual bool IsFramebufferComplete(FramebufferHandle framebuffer, std::string* error = nullptr) = 0;

	// Commands
	virtual void BindFramebuffer(FramebufferHandle framebuffer) = 0;
	virtual void SetViewport(int x, int y, int width, int height) = 0;
	virtual void Clear(bool color, bool depth, const glm::vec4& clearColor = glm::vec4(0.0f)) = 0;
	virtual void Draw(VertexArrayHandle vao, Primitive primitive, int first, int count, int instances = 0) = 0;
	virtual void DrawIndexed(VertexArrayHandle vao, Primitive primitive, int indexCount, int instances = 0) = 0;

	// GPU-driven rendering (Phase 1.5 item 11): compute programs, storage
	// buffers and indirect draws. Desktop GL 4.3+ only; WebGL2 has none.
	virtual bool SupportsGpuDriven() const = 0;
	virtual ProgramHandle CreateComputeProgram(const char* source, std::string& log) = 0;
	// A storage buffer (std430 `buffer` block) at `binding`: the whole buffer,
	// or `bytes` of it from `offset` (aligned to the device's storage alignment).
	virtual void BindStorageBuffer(unsigned int binding, BufferHandle buffer, size_t offset = 0, size_t bytes = 0) = 0;
	virtual void Dispatch(unsigned int groupsX, unsigned int groupsY = 1, unsigned int groupsZ = 1) = 0;
	// Make compute writes visible to later commands that read the memory as
	// what the bits name (GpuBarrierBit).
	virtual void GpuBarrier(unsigned int barrierBits) = 0;
	// An integer vertex attribute (`in uint` / `in int`: no conversion to float).
	virtual void SetVertexAttributeInt(VertexArrayHandle vao, unsigned int location, BufferHandle buffer,
		int components, size_t stride, size_t offset, unsigned int divisor = 0) = 0;
	// `drawCount` indexed draws (32-bit indices) whose parameters live in
	// `commands` from `offset`: DrawIndexedIndirectCommand records, `stride`
	// bytes apart. Each draws `instanceCount` instances from `baseInstance`,
	// which offsets instanced (divisor > 0) attributes.
	virtual void MultiDrawIndexedIndirect(VertexArrayHandle vao, Primitive primitive, BufferHandle commands,
		size_t offset, int drawCount, size_t stride) = 0;
	// One mip level of a 2D texture as a compute shader image (`layout(binding = unit)
	// uniform image2D`), with `format` its texel layout (R32F, RGBA8, RGBA16F...).
	virtual void BindImage(unsigned int unit, TextureHandle texture, int level, ImageAccess access,
		TextureFormat format) = 0;
	// Copy `bytes` of a buffer back to the CPU. Waits for the GPU if it is still
	// writing them (debug statistics; read a few frames late to avoid the wait).
	virtual void ReadBuffer(BufferHandle buffer, size_t offset, size_t bytes, void* out) = 0;

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

	// GPU timestamps (profiling, KINJO_GPU_TIMINGS): a query records the GPU
	// clock once the commands before it have executed. Read results a few
	// frames later; ReadTimestamp is false until the result is available.
	virtual bool SupportsTimestamps() const = 0;
	virtual QueryHandle CreateQuery() = 0;
	virtual void DestroyQuery(QueryHandle& query) = 0;
	virtual void WriteTimestamp(QueryHandle query) = 0;
	virtual bool ReadTimestamp(QueryHandle query, uint64_t& nanoseconds) = 0;

	// Readback of the bound framebuffer, tightly packed, bottom-up rows.
	virtual void ReadPixels(int x, int y, int width, int height, ReadbackFormat format, void* out) = 0;

	// GPU-capture labels (RenderDoc / Nsight); no-ops where unsupported.
	virtual void PushDebugGroup(const char* name) = 0;
	virtual void PopDebugGroup() = 0;

	// Capabilities
	virtual bool SupportsCubeMapArrays() const = 0;
	virtual bool SupportsPersistentMapping() const = 0;
	virtual bool SupportsFloatRenderTargets() const = 0;   // RGBA16F colour attachments
};

// The active device. Created on first use (the GL backend, the only one
// today); needs a current graphics context.
RenderDevice& Device();

#endif
