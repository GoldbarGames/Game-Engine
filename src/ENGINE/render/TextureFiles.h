#ifndef TEXTURE_FILES_H
#define TEXTURE_FILES_H
#pragma once

// Engine-internal (not exported). Texture files (Phase 1.5 item 10):
//
// - KTX2 containers: a texture's whole mip chain, built offline, either
//   GPU-compressed (BC1/BC3/BC4/BC5/BC7) or plain RGBA8. A compressed texture
//   loads with no decoding, takes a quarter (BC7) of the memory, and its mips
//   are filtered properly (sRGB-correct) instead of by the driver.
//   utils/texconvert.py makes them from PNGs.
// - Which file an image request loads: `name.ktx2` beside a requested
//   `name.png` (any image extension) wins when it is at least as new, so a
//   project switches to compressed textures by converting its files, with no
//   scene or code changes.
// - The project's texture settings (renderer.dat `anisotropy`).

#include "RenderDevice.h"
#include <string>
#include <vector>

class Texture;

struct Ktx2Image
{
	TextureFormat format = TextureFormat::RGBA8;   // as stored (sRGB or not)
	int width = 0;
	int height = 0;
	std::vector<TextureLevel> levels;              // base level first; pointing into the file's bytes
};

bool IsKtx2(const void* bytes, size_t size);
bool IsKtx2Path(const std::string& path);
// Parse a KTX2 file held in memory. Supported: 2D, no supercompression,
// BC1/BC3/BC4/BC5/BC7 (unorm or sRGB) and R8G8B8A8. `error` says why not.
bool ParseKtx2(const unsigned char* bytes, size_t size, Ktx2Image& out, std::string& error);

// `format` read in the requested colour space: the sRGB variant of a colour
// format for a linear-workflow colour texture, the plain one otherwise
// (gamma-space shaders read the stored values as they are). BC4/BC5 have none.
TextureFormat WithColorSpace(TextureFormat format, bool srgb);
const char* TextureFormatName(TextureFormat format);

// The whole file, or false if it can't be read.
bool ReadFileBytes(const std::string& path, std::vector<unsigned char>& out);

// The file a request for `path` loads from (see the top of this file).
std::string PreferredTextureFile(const std::string& path);

// renderer.dat `anisotropy <1..16>` (default 8): anisotropic filtering of
// smooth (mipmapped) textures, clamped to what the GPU allows. Read at startup.
void LoadTextureSettings();
float TextureAnisotropy();

// Formats of textures loaded from KTX2 files, by Texture (a two-channel BC5
// normal map stores only x and y; shaders rebuild z).
void RememberTextureFormat(const Texture* texture, TextureFormat format);
void ForgetTextureFormat(const Texture* texture);
bool IsTwoChannelTexture(const Texture* texture);

#endif
