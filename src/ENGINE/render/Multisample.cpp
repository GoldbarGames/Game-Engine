// Multisample anti-aliasing - see Multisample.h. Backend-agnostic: the GPU
// work is RenderDevice's renderbuffers and BlitFramebuffer.

#include "Multisample.h"
#include "ColorPipeline.h"
#include "../globals.h"
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <map>
#include <sstream>
#include <string>

namespace
{
	int requested = 0;   // renderer.dat `msaa` / KINJO_MSAA (< 2 = off)

	// The multisampled world target, as last built.
	struct Target
	{
		FramebufferHandle fbo;
		RenderbufferHandle color, mask, motion, depth;
		int width = 0;
		int height = 0;
		int samples = 0;
		TextureFormat format = TextureFormat::RGBA8;
		bool hasMotion = false;
	};
	Target target;

	// A build that failed (the framebuffer was incomplete), so it isn't
	// retried every frame; cleared when anything it depends on changes.
	struct FailedBuild { int width = 0, height = 0, samples = 0; TextureFormat format = TextureFormat::RGBA8; bool motion = false; };
	bool failed = false;
	FailedBuild failedBuild;

	std::string lastReport;

	// Say what MSAA is doing whenever that changes (startup, a resize, the
	// editor's PROJECT page).
	void Report(const std::string& text)
	{
		if (text == lastReport)
			return;
		lastReport = text;
		std::cout << "MSAA: " << text << std::endl;
	}

	void ReleaseTarget()
	{
		RenderDevice& device = Device();
		device.DestroyRenderbuffer(target.color);
		device.DestroyRenderbuffer(target.mask);
		device.DestroyRenderbuffer(target.motion);
		device.DestroyRenderbuffer(target.depth);
		if (target.fbo)
			device.DestroyFramebuffer(target.fbo);
		target = Target();
	}

	// The GPU's limit for a format, asked once.
	int MaxSamplesOf(TextureFormat format)
	{
		static std::map<int, int> known;
		auto it = known.find((int)format);
		if (it != known.end())
			return it->second;
		const int most = Device().MaxSamples(format);
		known[(int)format] = most;
		return most;
	}

	// The samples every attachment can share: what was asked for, capped by
	// the GPU's limit for each format.
	int SupportedSamples(TextureFormat colorFormat, bool motion)
	{
		int samples = std::min(requested, MaxSamplesOf(colorFormat));
		samples = std::min(samples, MaxSamplesOf(TextureFormat::R8));
		samples = std::min(samples, MaxSamplesOf(TextureFormat::Depth24Stencil8));
		if (motion)
			samples = std::min(samples, MaxSamplesOf(TextureFormat::RGBA16F));
		return samples;
	}

	const char* FormatName(TextureFormat f)
	{
		return (f == TextureFormat::RGBA16F) ? "RGBA16F" : "RGBA8";
	}
}

void LoadMultisampleSettings()
{
	auto config = ReadRendererConfig();
	requested = 0;
	if (config.count("msaa") > 0)
		requested = std::atoi(config["msaa"].c_str());
	if (const char* e = std::getenv("KINJO_MSAA"))
		requested = std::atoi(e);
	requested = (requested < 2) ? 0 : std::min(requested, 16);
	failed = false;
	if (requested == 0 && !lastReport.empty())
		Report("off");   // only once it was on: games that never use it stay quiet
}

bool EnsureMultisampleWorld(int width, int height, TextureFormat colorFormat, bool motion)
{
	if (requested == 0 || width <= 0 || height <= 0)
	{
		if (target.fbo)
			ReleaseTarget();
		return false;
	}

	const int samples = SupportedSamples(colorFormat, motion);
	if (samples < 2)
	{
		if (target.fbo)
			ReleaseTarget();
		Report(std::string("this GPU can't multisample the world (") + FormatName(colorFormat) + ") - off");
		return false;
	}

	if (target.fbo && target.width == width && target.height == height && target.samples == samples
		&& target.format == colorFormat && target.hasMotion == motion)
		return true;
	if (failed && failedBuild.width == width && failedBuild.height == height && failedBuild.samples == samples
		&& failedBuild.format == colorFormat && failedBuild.motion == motion)
		return false;

	ReleaseTarget();
	RenderDevice& device = Device();
	target.width = width;
	target.height = height;
	target.samples = samples;
	target.format = colorFormat;
	target.hasMotion = motion;
	// The same attachment slots as the world target, so the World pass's draw
	// buffer masks (mask = 1, motion = 2) work unchanged.
	target.fbo = device.CreateFramebuffer();
	target.color = device.CreateRenderbuffer(colorFormat, width, height, samples);
	target.mask = device.CreateRenderbuffer(TextureFormat::R8, width, height, samples);
	target.depth = device.CreateRenderbuffer(TextureFormat::Depth24Stencil8, width, height, samples);
	device.AttachRenderbuffer(target.fbo, Attachment::Color0, target.color);
	device.AttachRenderbuffer(target.fbo, Attachment::Color1, target.mask);
	if (motion)
	{
		target.motion = device.CreateRenderbuffer(TextureFormat::RGBA16F, width, height, samples);
		device.AttachRenderbuffer(target.fbo, Attachment::Color2, target.motion);
	}
	device.AttachRenderbuffer(target.fbo, Attachment::DepthStencil, target.depth);

	std::string error;
	if (!device.IsFramebufferComplete(target.fbo, &error))
	{
		Report("the multisampled world target is incomplete (" + error + ") - off");
		ReleaseTarget();
		failed = true;
		failedBuild = { width, height, samples, colorFormat, motion };
		device.BindFramebuffer(FramebufferHandle());
		return false;
	}
	device.BindFramebuffer(FramebufferHandle());

	std::ostringstream ss;
	ss << samples << "x on the world (" << FormatName(colorFormat) << (motion ? " + motion vectors" : "") << ", "
		<< width << "x" << height << ")";
	if (samples < requested)
		ss << ", " << requested << "x asked for: the GPU's limit";
	Report(ss.str());
	return true;
}

void BindMultisampleWorld()
{
	if (target.fbo)
		Device().BindFramebuffer(target.fbo);
}

void ResolveMultisampleWorld(FramebufferHandle worldTarget)
{
	if (!target.fbo)
		return;
	const unsigned int colors = 1u | 2u | (target.hasMotion ? 4u : 0u);
	Device().BlitFramebuffer(target.fbo, worldTarget, target.width, target.height, colors, true);
}

int MultisampleSamples()
{
	return target.fbo ? target.samples : 0;
}

void ReleaseMultisample()
{
	ReleaseTarget();
	failed = false;
	lastReport.clear();
}
