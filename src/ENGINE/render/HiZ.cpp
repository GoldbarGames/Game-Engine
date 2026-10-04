// Hi-Z pyramid - see HiZ.h. Backend-agnostic: GPU work goes through RenderDevice.

#include "HiZ.h"
#include "../ShaderSources.h"
#include <algorithm>
#include <iostream>

namespace
{
	unsigned int program = 0;
	bool programTried = false;
	struct Locations { int sourceWidth, sourceHeight, targetWidth, targetHeight, fromDepth, depthTexture; } loc;

	TextureHandle pyramid;
	int pyramidWidth = 0, pyramidHeight = 0, levels = 0;

	bool EnsureProgram()
	{
		if (programTried)
			return program != 0;
		programTried = true;
		if (!Device().SupportsGpuDriven())
			return false;
		program = CreateComputeProgramFromFile("data/shaders/hiz_build.comp");
		if (program == 0)
			return false;
		RenderDevice& device = Device();
		const ProgramHandle p(program);
		loc.sourceWidth = device.UniformLocation(p, "draw.sourceWidth");
		loc.sourceHeight = device.UniformLocation(p, "draw.sourceHeight");
		loc.targetWidth = device.UniformLocation(p, "draw.targetWidth");
		loc.targetHeight = device.UniformLocation(p, "draw.targetHeight");
		loc.fromDepth = device.UniformLocation(p, "draw.fromDepth");
		loc.depthTexture = device.UniformLocation(p, "depthTexture");
		return true;
	}

	void EnsurePyramid(int width, int height)
	{
		if (pyramid && pyramidWidth == width && pyramidHeight == height)
			return;
		RenderDevice& device = Device();
		device.DestroyTexture(pyramid);
		levels = 1;
		while ((std::max(width, height) >> levels) > 0)
			levels++;
		TextureDesc desc;
		desc.format = TextureFormat::R32F;
		desc.width = width;
		desc.height = height;
		// A mipmapped filter, so every level is part of the texture (texelFetch
		// on a level past a non-mipmapped texture's base level is undefined).
		desc.filter = TextureFilter::Trilinear;
		desc.wrap = TextureWrap::ClampToEdge;
		desc.mipLevels = levels;
		pyramid = device.CreateTexture(desc);
		pyramidWidth = width;
		pyramidHeight = height;
	}
}

bool BuildHiZ(TextureHandle depth, int width, int height)
{
	if (!depth || width <= 0 || height <= 0 || !EnsureProgram())
		return false;
	EnsurePyramid(width, height);

	RenderDevice& device = Device();
	device.UseProgram(ProgramHandle(program));
	device.BindTexture(0, depth);
	device.SetUniform(loc.depthTexture, 0);
	int sw = width, sh = height;
	for (int level = 0; level < levels; level++)
	{
		const int tw = std::max(1, width >> level);
		const int th = std::max(1, height >> level);
		// Level 0 reads the depth texture; the source image is bound but unused.
		device.BindImage(0, pyramid, std::max(level - 1, 0), ImageAccess::Read, TextureFormat::R32F);
		device.BindImage(1, pyramid, level, ImageAccess::Write, TextureFormat::R32F);
		device.SetUniform(loc.fromDepth, level == 0 ? 1 : 0);
		device.SetUniform(loc.sourceWidth, sw);
		device.SetUniform(loc.sourceHeight, sh);
		device.SetUniform(loc.targetWidth, tw);
		device.SetUniform(loc.targetHeight, th);
		device.Dispatch((unsigned int)((tw + 7) / 8), (unsigned int)((th + 7) / 8));
		device.GpuBarrier(BarrierImage);
		sw = tw;
		sh = th;
	}
	device.GpuBarrier(BarrierTextureFetch);
	return true;
}

TextureHandle HiZTexture()
{
	return pyramid;
}

int HiZLevels()
{
	return levels;
}
