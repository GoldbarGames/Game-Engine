// Texture files - see TextureFiles.h. Backend-agnostic.

#include "TextureFiles.h"
#include "../globals.h"
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <unordered_map>

namespace
{
	const unsigned char kKtx2Identifier[12] = { 0xAB, 'K', 'T', 'X', ' ', '2', '0', 0xBB, '\r', '\n', 0x1A, '\n' };
	const size_t kKtx2HeaderBytes = 80;   // identifier, header, index
	const size_t kKtx2LevelBytes = 24;    // byteOffset, byteLength, uncompressedByteLength

	uint32_t ReadU32(const unsigned char* p)
	{
		return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
	}

	uint64_t ReadU64(const unsigned char* p)
	{
		return (uint64_t)ReadU32(p) | ((uint64_t)ReadU32(p + 4) << 32);
	}

	// VkFormat values (the KTX2 header names formats by Vulkan's enum).
	bool FormatFromVulkan(uint32_t vkFormat, TextureFormat& out)
	{
		switch (vkFormat)
		{
		case 37:  out = TextureFormat::RGBA8; return true;      // R8G8B8A8_UNORM
		case 43:  out = TextureFormat::SRGB8_A8; return true;   // R8G8B8A8_SRGB
		case 131:                                               // BC1_RGB_UNORM (read as RGBA)
		case 133: out = TextureFormat::BC1; return true;        // BC1_RGBA_UNORM
		case 132:
		case 134: out = TextureFormat::BC1_SRGB; return true;
		case 137: out = TextureFormat::BC3; return true;
		case 138: out = TextureFormat::BC3_SRGB; return true;
		case 139: out = TextureFormat::BC4; return true;        // BC4_UNORM
		case 141: out = TextureFormat::BC5; return true;        // BC5_UNORM
		case 145: out = TextureFormat::BC7; return true;
		case 146: out = TextureFormat::BC7_SRGB; return true;
		default:  return false;
		}
	}

	float anisotropy = 8.0f;

	// Never destroyed: textures can outlive static destruction (at exit).
	std::unordered_map<const Texture*, TextureFormat>& Formats()
	{
		static auto* formats = new std::unordered_map<const Texture*, TextureFormat>();
		return *formats;
	}

	std::string Lower(std::string s)
	{
		for (char& c : s)
			c = (char)tolower((unsigned char)c);
		return s;
	}
}

bool IsKtx2(const void* bytes, size_t size)
{
	return bytes != nullptr && size >= sizeof(kKtx2Identifier)
		&& memcmp(bytes, kKtx2Identifier, sizeof(kKtx2Identifier)) == 0;
}

bool IsKtx2Path(const std::string& path)
{
	return path.size() > 5 && Lower(path.substr(path.size() - 5)) == ".ktx2";
}

bool ParseKtx2(const unsigned char* b, size_t size, Ktx2Image& out, std::string& error)
{
	if (!IsKtx2(b, size) || size < kKtx2HeaderBytes)
	{
		error = "not a KTX2 file";
		return false;
	}
	const uint32_t vkFormat = ReadU32(b + 12);
	const uint32_t width = ReadU32(b + 20);
	const uint32_t height = ReadU32(b + 24);
	const uint32_t depth = ReadU32(b + 28);
	const uint32_t layers = ReadU32(b + 32);
	const uint32_t faces = ReadU32(b + 36);
	const uint32_t levelCount = ReadU32(b + 40);
	const uint32_t supercompression = ReadU32(b + 44);

	if (supercompression != 0)
	{
		error = "supercompressed (Basis Universal / Zstandard / zlib), which the engine doesn't decode: "
			"save it with plain BCn blocks (utils/texconvert.py)";
		return false;
	}
	if (depth > 1 || layers > 1 || faces != 1)
	{
		error = "3D, array and cube-map KTX2 textures aren't supported";
		return false;
	}
	if (width == 0 || height == 0 || width > 16384 || height > 16384)
	{
		error = "bad size " + std::to_string(width) + "x" + std::to_string(height);
		return false;
	}
	TextureFormat format;
	if (!FormatFromVulkan(vkFormat, format))
	{
		error = "unsupported format (VkFormat " + std::to_string(vkFormat)
			+ "); the engine reads BC1, BC3, BC4, BC5, BC7 and R8G8B8A8";
		return false;
	}

	// levelCount 0 means "make the mips at load time": one stored level.
	uint32_t levels = std::max(levelCount, 1u);
	if (kKtx2HeaderBytes + (size_t)levels * kKtx2LevelBytes > size)
	{
		error = "truncated level index";
		return false;
	}
	// Ignore any levels past 1x1 (they would make the GL texture incomplete).
	uint32_t fullChain = 1;
	while ((std::max(width, height) >> fullChain) > 0)
		fullChain++;
	levels = std::min(levels, fullChain);

	out = Ktx2Image();
	out.format = format;
	out.width = (int)width;
	out.height = (int)height;
	const bool compressed = IsBlockCompressed(format);
	for (uint32_t i = 0; i < levels; i++)
	{
		const unsigned char* entry = b + kKtx2HeaderBytes + i * kKtx2LevelBytes;
		const uint64_t offset = ReadU64(entry);
		const uint64_t length = ReadU64(entry + 8);
		const uint64_t w = std::max(1u, width >> i);
		const uint64_t h = std::max(1u, height >> i);
		const uint64_t expected = compressed ? ((w + 3) / 4) * ((h + 3) / 4) * (uint64_t)BlockBytes(format)
			: w * h * 4;
		if (offset > size || length > size - offset || length < expected)
		{
			error = "level " + std::to_string(i) + " is truncated";
			return false;
		}
		TextureLevel level;
		level.data = b + offset;
		level.bytes = (size_t)expected;
		out.levels.push_back(level);
	}
	return true;
}

TextureFormat WithColorSpace(TextureFormat format, bool srgb)
{
	switch (format)
	{
	case TextureFormat::RGBA8:
	case TextureFormat::SRGB8_A8: return srgb ? TextureFormat::SRGB8_A8 : TextureFormat::RGBA8;
	case TextureFormat::BC1:
	case TextureFormat::BC1_SRGB: return srgb ? TextureFormat::BC1_SRGB : TextureFormat::BC1;
	case TextureFormat::BC3:
	case TextureFormat::BC3_SRGB: return srgb ? TextureFormat::BC3_SRGB : TextureFormat::BC3;
	case TextureFormat::BC7:
	case TextureFormat::BC7_SRGB: return srgb ? TextureFormat::BC7_SRGB : TextureFormat::BC7;
	default:                      return format;
	}
}

const char* TextureFormatName(TextureFormat format)
{
	switch (format)
	{
	case TextureFormat::RGBA8:    return "RGBA8";
	case TextureFormat::SRGB8_A8: return "RGBA8 sRGB";
	case TextureFormat::BC1:      return "BC1";
	case TextureFormat::BC1_SRGB: return "BC1 sRGB";
	case TextureFormat::BC3:      return "BC3";
	case TextureFormat::BC3_SRGB: return "BC3 sRGB";
	case TextureFormat::BC4:      return "BC4";
	case TextureFormat::BC5:      return "BC5";
	case TextureFormat::BC7:      return "BC7";
	case TextureFormat::BC7_SRGB: return "BC7 sRGB";
	default:                      return "?";
	}
}

bool ReadFileBytes(const std::string& path, std::vector<unsigned char>& out)
{
	std::ifstream in(path, std::ios::binary | std::ios::ate);
	if (!in.is_open())
		return false;
	const std::streamoff size = in.tellg();
	if (size <= 0)
		return false;
	out.resize((size_t)size);
	in.seekg(0);
	in.read((char*)out.data(), size);
	return in.good();
}

std::string PreferredTextureFile(const std::string& path)
{
	const size_t dot = path.find_last_of('.');
	const size_t slash = path.find_last_of("/\\");
	if (dot == std::string::npos || (slash != std::string::npos && dot < slash) || IsKtx2Path(path))
		return path;
	const std::string ktx2 = path.substr(0, dot) + ".ktx2";
	try
	{
		if (!fs::exists(ktx2))
			return path;
		if (fs::exists(path) && fs::last_write_time(ktx2) < fs::last_write_time(path))
		{
			// The image was edited after it was converted: show the edit.
			static std::unordered_map<std::string, bool> warned;
			if (!warned[path])
			{
				warned[path] = true;
				std::cout << "Texture: " << ktx2 << " is older than " << path
					<< " - loading the image; re-run utils/texconvert.py on it" << std::endl;
			}
			return path;
		}
	}
	catch (...)
	{
		return path;
	}
	return ktx2;
}

void LoadTextureSettings()
{
	auto config = GetMapStringsFromFile("data/config/renderer.dat");
	anisotropy = 8.0f;
	if (config.count("anisotropy") > 0)
	{
		try
		{
			anisotropy = std::max(1.0f, std::min(16.0f, std::stof(config["anisotropy"])));
		}
		catch (...)
		{
		}
	}
}

float TextureAnisotropy()
{
	return anisotropy;
}

void RememberTextureFormat(const Texture* texture, TextureFormat format)
{
	Formats()[texture] = format;
}

void ForgetTextureFormat(const Texture* texture)
{
	if (!Formats().empty())
		Formats().erase(texture);
}

bool IsTwoChannelTexture(const Texture* texture)
{
	auto it = Formats().find(texture);
	return it != Formats().end() && it->second == TextureFormat::BC5;
}
