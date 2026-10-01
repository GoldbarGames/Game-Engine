#ifndef UNIFORM_BUFFER_CACHE_H
#define UNIFORM_BUFFER_CACHE_H
#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>
#include <unordered_map>

// Engine-internal (not exported). A few uniform buffers for one block binding
// point, keyed by content: Bind(data) rebinds a buffer that already holds
// exactly these bytes, otherwise refills the least-recently-filled one. A
// block whose contents alternate between a handful of values (cameras,
// materials, a scene's lighting) is then rebound instead of re-uploaded on
// every draw. GL keeps earlier draws that read a refilled buffer correct.
class UniformBufferCache
{
public:
	UniformBufferCache(unsigned int binding, size_t blockSize, int slotCount);

	// Make `data` (blockSize bytes) the contents of this block's binding point.
	void Bind(const void* data);

	// Delete every cache's GL buffers (call while the context is alive).
	static void ReleaseAll();

private:
	struct Slot
	{
		unsigned int ubo = 0;
		uint64_t hash = 0;
		bool filled = false;
		std::vector<unsigned char> bytes;
	};

	void Release();

	unsigned int binding;
	size_t size;
	std::vector<Slot> slots;
	std::unordered_map<uint64_t, int> slotByHash;
	int nextSlot = 0;
};

// One member of a uniform block's C++ mirror, for CheckUniformBlockLayout.
struct UniformBlockMember
{
	const char* name;   // GLSL name; arrays as "name[0]"
	size_t offset;      // offsetof in the C++ mirror
	bool isArray;       // arrays must also have the std140 16-byte stride
};

// Compare a block's layout as the driver reports it against its C++ mirror and
// log every mismatch (once per block name, on the first program declaring it).
// A wrong offset otherwise shows up only as subtly wrong lighting.
void CheckUniformBlockLayout(unsigned int program, const char* blockName,
	const UniformBlockMember* members, size_t memberCount, size_t mirrorSize);

#endif
