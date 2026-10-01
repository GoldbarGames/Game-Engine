#ifndef TRANSIENT_BUFFER_H
#define TRANSIENT_BUFFER_H
#pragma once

#include <cstddef>

// Engine-internal (not exported). Per-frame streaming memory for data that is
// rewritten every frame - sprite/glyph instance data, overlay vertices,
// weather particles, instanced prop matrices - instead of each feature
// re-uploading its own buffer on every draw.
//
// One GL buffer split into kTransientFrames slices (frames in flight). A frame
// writes into its slice; TransientEndFrame fences it and moves on, waiting
// only if the GPU is still reading the slice about to be reused. On desktop GL
// 4.4+ the buffer is persistently mapped and an upload is a plain memcpy; on
// GL 3.3 / WebGL it falls back to glBufferSubData into the slice.
//
// An allocation is valid for draws issued in the same frame. Point vertex
// attributes at it with the buffer + offset (glVertexAttribPointer offset).

struct TransientAlloc
{
	unsigned int buffer = 0;   // GL buffer object (0 = upload failed)
	size_t offset = 0;         // byte offset of the data within it
	bool Valid() const { return buffer != 0; }
};

// Copy `bytes` into this frame's slice (offset aligned to `align`). Creates
// the ring on first use, so it needs a current GL context.
TransientAlloc TransientUpload(const void* data, size_t bytes, size_t align = 16);

// Call once per frame after the buffer swap: fences the slice just used and
// prepares the next one.
void TransientEndFrame();

// Delete the GL buffer and fences (while the context is alive).
void TransientRelease();

#endif
