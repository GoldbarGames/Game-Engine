#include "DataTexture.h"
#include "render/RenderDevice.h"
#include "render/TextureFiles.h"

#include <glm/gtc/packing.hpp>
#include <cstdint>
#include <iostream>
#include <vector>

namespace
{
	TextureFormat DeviceFormat(DataFormat f)
	{
		switch (f)
		{
		case DataFormat::R8:      return TextureFormat::R8;
		case DataFormat::RG8:     return TextureFormat::RG8;
		case DataFormat::RGBA8:   return TextureFormat::RGBA8;
		case DataFormat::R16F:    return TextureFormat::R16F;
		case DataFormat::RG16F:   return TextureFormat::RG16F;
		case DataFormat::RGBA16F: return TextureFormat::RGBA16F;
		case DataFormat::R32F:    return TextureFormat::R32F;
		case DataFormat::RG32F:   return TextureFormat::RG32F;
		case DataFormat::RGBA32F:
		default:                  return TextureFormat::RGBA32F;
		}
	}

	bool IsHalf(DataFormat f)
	{
		return f == DataFormat::R16F || f == DataFormat::RG16F || f == DataFormat::RGBA16F;
	}
}

int DataTexture::Channels(DataFormat f)
{
	switch (f)
	{
	case DataFormat::R8: case DataFormat::R16F: case DataFormat::R32F: return 1;
	case DataFormat::RG8: case DataFormat::RG16F: case DataFormat::RG32F: return 2;
	default: return 4;
	}
}

bool DataTexture::IsFloat(DataFormat f)
{
	return f != DataFormat::R8 && f != DataFormat::RG8 && f != DataFormat::RGBA8;
}

DataTexture::DataTexture(const DataTextureDesc& d, const void* data) : desc(d)
{
	Create(data);
}

DataTexture::~DataTexture()
{
	if (id != 0)
	{
		TextureHandle handle(id);
		Device().DestroyTexture(handle);
	}
}

void DataTexture::Create(const void* data)
{
	if (desc.width < 1 || desc.height < 1 || desc.layers < 0)
	{
		std::cout << "DataTexture: no texture of " << desc.width << " x " << desc.height << std::endl;
		return;
	}
	TextureDesc t;
	t.type = desc.layers > 0 ? TextureType::Tex2DArray : TextureType::Tex2D;
	t.format = DeviceFormat(desc.format);
	t.width = desc.width;
	t.height = desc.height;
	t.layers = desc.layers > 0 ? desc.layers : 1;
	bool mips = desc.mipmaps && desc.linear;
#ifdef __EMSCRIPTEN__
	if (mips && IsFloat(desc.format))
	{
		std::cout << "DataTexture: no mip chain for a float texture on the web; filtering without" << std::endl;
		mips = false;
	}
#endif
	t.filter = !desc.linear ? TextureFilter::Nearest : mips ? TextureFilter::Trilinear : TextureFilter::Linear;
	t.wrap = desc.repeat ? TextureWrap::Repeat : TextureWrap::ClampToEdge;
	t.generateMipmaps = mips;
	if (mips && desc.anisotropic)
		t.maxAnisotropy = TextureAnisotropy();

	// The 16-bit formats upload as halves: convert the game's floats
	std::vector<uint16_t> halves;
	const void* upload = data;
	if (data != nullptr && IsHalf(desc.format))
	{
		const size_t count = static_cast<size_t>(desc.width) * desc.height * t.layers * Channels(desc.format);
		const float* f = static_cast<const float*>(data);
		halves.resize(count);
		for (size_t i = 0; i < count; i++)
			halves[i] = glm::packHalf1x16(f[i]);
		upload = halves.data();
	}
	id = Device().CreateTexture(t, upload).id;
}

void DataTexture::Update(const void* data)
{
	if (id == 0 || data == nullptr)
		return;
	if (desc.layers > 0 || desc.mipmaps)
	{
		// An array (or a mip chain) is made again: the device updates only 2D level 0
		TextureHandle handle(id);
		Device().DestroyTexture(handle);
		id = 0;
		Create(data);
		return;
	}
	std::vector<uint16_t> halves;
	const void* upload = data;
	if (IsHalf(desc.format))
	{
		const size_t count = static_cast<size_t>(desc.width) * desc.height * Channels(desc.format);
		const float* f = static_cast<const float*>(data);
		halves.resize(count);
		for (size_t i = 0; i < count; i++)
			halves[i] = glm::packHalf1x16(f[i]);
		upload = halves.data();
	}
	Device().UpdateTexture(TextureHandle(id), DeviceFormat(desc.format), 0, 0, desc.width, desc.height, upload);
}

void DataTexture::Bind(unsigned int unit) const
{
	Device().BindTexture(unit, TextureHandle(id), desc.layers > 0 ? TextureType::Tex2DArray : TextureType::Tex2D);
}
