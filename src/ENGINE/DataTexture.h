#ifndef DATA_TEXTURE_H
#define DATA_TEXTURE_H
#pragma once

#include "leak_check.h"

// Textures of numbers a game fills itself (since 2026-10-09): zone distance
// fields, noise tables, lookup tables. One to four channels of 8-bit unorm,
// 16-bit float or 32-bit float, as a 2D texture or a 2D texture array, with
// nearest or linear filtering and clamp or repeat. A game reads them in its own
// program (Scene3D::SetModelProgram), binding them in that program's `bind`
// callback with Bind(unit). The game owns and frees them.
//
// Formats: the data is given tightly packed, row after row (and layer after
// layer for an array): bytes for the 8-bit formats, 32-bit floats for both the
// 16- and 32-bit float formats (16-bit ones are converted on upload).
// Linear filtering of the 32-bit formats needs OES_texture_float_linear on the
// web; the 16-bit ones filter everywhere.

enum class DataFormat
{
	R8, RG8, RGBA8,
	R16F, RG16F, RGBA16F,
	R32F, RG32F, RGBA32F,
};

struct DataTextureDesc
{
	int width = 1;
	int height = 1;
	int layers = 0;          // 0: a 2D texture; n: a 2D texture array of n layers
	DataFormat format = DataFormat::R32F;
	bool linear = false;     // linear filtering, or nearest
	bool repeat = false;     // repeat, or clamp to the edge
	bool mipmaps = false;    // a mip chain built on upload (linear only; desktop only for the float formats)
	bool anisotropic = false;   // with mips: the project's anisotropic filtering (renderer.dat `anisotropy`),
	                            // for a texture seen at a slant (a ground detail texture)
};

class KINJO_API DataTexture
{
public:
	DataTexture(const DataTextureDesc& desc, const void* data);
	~DataTexture();
	DataTexture(const DataTexture&) = delete;
	DataTexture& operator=(const DataTexture&) = delete;

	// All of it again, in the same layout and size
	void Update(const void* data);
	// On texture unit `unit` (a game program's sampler2D, or sampler2DArray for
	// an array)
	void Bind(unsigned int unit) const;

	const DataTextureDesc& Desc() const { return desc; }
	bool Valid() const { return id != 0; }

	static int Channels(DataFormat format);
	static bool IsFloat(DataFormat format);

private:
	void Create(const void* data);

	DataTextureDesc desc;
	unsigned int id = 0;
};

#endif
