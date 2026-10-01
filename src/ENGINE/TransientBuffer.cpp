#include "TransientBuffer.h"
#include "render/RenderDevice.h"
#include <cstring>
#include <iostream>

namespace
{
	const int kTransientFrames = 3;
#ifdef __EMSCRIPTEN__
	const size_t kSliceBytes = 2u * 1024u * 1024u;
#else
	// One frame's worth. A busy DB2 frame streams well under 1 MB (glyph and
	// sprite instances, overlay quads, a few thousand particles).
	const size_t kSliceBytes = 8u * 1024u * 1024u;
#endif

	BufferHandle ring;
	unsigned char* mapped = nullptr;   // persistent mapping where the device supports it
	bool initFailed = false;
	bool warnedOverflow = false;

	int slice = 0;      // current frame's slice
	size_t head = 0;    // bytes used in it
	FenceHandle fences[kTransientFrames];

	bool Ensure()
	{
		if (ring)
			return true;
		if (initFailed)
			return false;

		RenderDevice& device = Device();
		const size_t total = kSliceBytes * kTransientFrames;

		// Persistent + coherent mapping (GL 4.4 / ARB_buffer_storage): write
		// straight into GPU-visible memory, no per-upload driver call.
		mapped = (unsigned char*)device.CreatePersistentBuffer(total, ring);
		if (mapped == nullptr)
			ring = device.CreateBuffer(total, nullptr, BufferUsage::Stream);
		if (!ring)
		{
			initFailed = true;
			std::cout << "ERROR: transient buffer could not be created; streaming uses per-feature buffers" << std::endl;
			return false;
		}

		std::cout << "Transient buffer: " << kTransientFrames << " x " << (kSliceBytes >> 20) << " MB, "
			<< (mapped != nullptr ? "persistently mapped" : "glBufferSubData") << std::endl;
		return true;
	}
}

TransientAlloc TransientUpload(const void* data, size_t bytes, size_t align)
{
	TransientAlloc result;
	if (bytes == 0 || bytes > kSliceBytes || !Ensure())
		return result;

	size_t start = (head + align - 1) / align * align;
	if (start + bytes > kSliceBytes)
	{
		// The frame outgrew its slice. Rare: wait for the GPU to finish
		// everything (including this frame's earlier draws from this slice),
		// then reuse the slice from the start.
		if (!warnedOverflow)
		{
			std::cout << "WARNING: transient buffer slice (" << (kSliceBytes >> 20)
				<< " MB) full this frame; stalling to reuse it" << std::endl;
			warnedOverflow = true;
		}
		Device().Finish();
		start = 0;
	}

	const size_t offset = (size_t)slice * kSliceBytes + start;
	if (mapped != nullptr)
		std::memcpy(mapped + offset, data, bytes);
	else
		Device().UpdateBuffer(ring, offset, bytes, data);

	head = start + bytes;
	result.buffer = ring.id;
	result.offset = offset;
	return result;
}

void TransientEndFrame()
{
	if (!ring)
		return;
	if (mapped != nullptr)
	{
		RenderDevice& device = Device();
		fences[slice] = device.InsertFence();
		slice = (slice + 1) % kTransientFrames;
		device.WaitFence(fences[slice]);   // the GPU must be done with this slice before it's rewritten
	}
	else
	{
		// UpdateBuffer path: the driver orders writes against earlier draws.
		slice = (slice + 1) % kTransientFrames;
	}
	head = 0;
}

void TransientRelease()
{
	RenderDevice& device = Device();
	for (int s = 0; s < kTransientFrames; s++)
		device.DeleteFence(fences[s]);
	device.DestroyBuffer(ring);   // also unmaps a persistent buffer
	mapped = nullptr;
	head = 0;
	slice = 0;
}
