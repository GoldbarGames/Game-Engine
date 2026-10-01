#include "UniformBufferCache.h"
#include "render/RenderDevice.h"
#include <cstring>
#include <iostream>
#include <set>
#include <string>

namespace
{
	// Function-local so caches constructed at static-init time in other files
	// can register safely.
	std::vector<UniformBufferCache*>& Registry()
	{
		static std::vector<UniformBufferCache*> caches;
		return caches;
	}

	// Which buffer each binding point currently holds, to skip redundant binds.
	const int kMaxBindings = 16;
	unsigned int boundAt[kMaxBindings] = { 0 };

	uint64_t HashBytes(const void* data, size_t size)
	{
		// FNV-1a: fast, and collisions are caught by the byte compare anyway.
		const unsigned char* p = static_cast<const unsigned char*>(data);
		uint64_t h = 1469598103934665603ull;
		for (size_t i = 0; i < size; i++)
		{
			h ^= p[i];
			h *= 1099511628211ull;
		}
		return h;
	}
}

UniformBufferCache::UniformBufferCache(unsigned int binding, size_t blockSize, int slotCount)
	: binding(binding), size(blockSize), slots(slotCount > 0 ? slotCount : 1)
{
	Registry().push_back(this);
}

void UniformBufferCache::Bind(const void* data)
{
	const uint64_t hash = HashBytes(data, size);

	int slot = -1;
	auto found = slotByHash.find(hash);
	if (found != slotByHash.end() && std::memcmp(slots[found->second].bytes.data(), data, size) == 0)
		slot = found->second;

	if (slot < 0)
	{
		slot = nextSlot;
		nextSlot = (nextSlot + 1) % (int)slots.size();

		Slot& s = slots[slot];
		if (s.filled)
		{
			auto old = slotByHash.find(s.hash);
			if (old != slotByHash.end() && old->second == slot)
				slotByHash.erase(old);
		}
		s.bytes.assign(static_cast<const unsigned char*>(data), static_cast<const unsigned char*>(data) + size);
		s.hash = hash;
		s.filled = true;
		slotByHash[hash] = slot;

		if (s.ubo == 0)
			s.ubo = Device().CreateBuffer(size, data, BufferUsage::Dynamic).id;
		else
			Device().UpdateBuffer(BufferHandle(s.ubo), 0, size, data);
	}

	const unsigned int ubo = slots[slot].ubo;
	if (binding >= (unsigned int)kMaxBindings || boundAt[binding] != ubo)
	{
		Device().BindUniformBuffer(binding, BufferHandle(ubo));
		if (binding < (unsigned int)kMaxBindings)
			boundAt[binding] = ubo;
	}
}

void UniformBufferCache::Release()
{
	for (Slot& s : slots)
	{
		BufferHandle buffer(s.ubo);
		Device().DestroyBuffer(buffer);
		s = Slot();
	}
	slotByHash.clear();
	nextSlot = 0;
}

void UniformBufferCache::ReleaseAll()
{
	for (UniformBufferCache* cache : Registry())
		cache->Release();
	for (unsigned int& b : boundAt)
		b = 0;
}

void CheckUniformBlockLayout(unsigned int program, const char* blockName,
	const UniformBlockMember* members, size_t memberCount, size_t mirrorSize)
{
	static std::set<std::string> checked;
	if (checked.count(blockName) != 0)
		return;

	RenderDevice& device = Device();
	const ProgramHandle handle(program);
	if (!device.HasUniformBlock(handle, blockName))
		return;
	checked.insert(blockName);

	int problems = 0;
	const int dataSize = device.UniformBlockSize(handle, blockName);
	if (dataSize > 0 && (size_t)dataSize > mirrorSize)
	{
		std::cout << "ERROR: uniform block " << blockName << " is " << dataSize
			<< " bytes in the shader but its C++ mirror is " << mirrorSize << std::endl;
		problems++;
	}

	for (size_t i = 0; i < memberCount; i++)
	{
		const UniformBlockMember& m = members[i];
		int offset = -1, stride = 0;
		if (!device.UniformOffset(handle, m.name, offset, stride))
		{
			std::cout << "ERROR: uniform block " << blockName << " has no member " << m.name << std::endl;
			problems++;
			continue;
		}
		if ((size_t)offset != m.offset)
		{
			std::cout << "ERROR: uniform block " << blockName << " member " << m.name
				<< " is at offset " << offset << " in the shader but " << m.offset << " in C++" << std::endl;
			problems++;
		}
		if (m.isArray && stride != 16)
		{
			std::cout << "ERROR: uniform block " << blockName << " member " << m.name
				<< " has array stride " << stride << " (C++ mirror assumes 16)" << std::endl;
			problems++;
		}
	}

	if (problems == 0)
		std::cout << "Uniform block " << blockName << " layout verified (" << memberCount << " members)" << std::endl;
}
