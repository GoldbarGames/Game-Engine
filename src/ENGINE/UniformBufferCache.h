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

	// Where a block's bytes were put: which buffer, holding what.
	struct Held
	{
		int slot = -1;
		uint64_t hash = 0;
	};

	// Make `data` (blockSize bytes) the contents of this block's binding point;
	// says where it is now.
	Held Bind(const void* data);

	// The same bytes as an earlier Bind, without hashing them again: binds that
	// buffer if it still holds them, and returns false if it has been refilled
	// since (then Bind them again). For a caller that knows its block hasn't
	// changed - the scene's lighting within a pass.
	bool BindAgain(const Held& held);

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
	Held BindHashed(const void* data);

	unsigned int binding;
	size_t size;
	std::vector<Slot> slots;
	std::unordered_map<uint64_t, int> slotByHash;
	int nextSlot = 0;

	// The last bytes bound, and where: the same block bound draw after draw (a
	// sprite's camera) is compared, not hashed and looked up again.
	std::vector<unsigned char> lastBytes;
	Held lastHeld;
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
