#include "RenderPass.h"
#include "Shader.h"
#include "render/RenderDevice.h"
#include <cstdlib>
#include <iostream>
#include <set>
#include <string>
#include <vector>

namespace
{
	struct PassRecord
	{
		std::string name;
		int depth;
		TargetSet reads;
		TargetSet writes;
	};

	// Targets that legitimately carry data from earlier frames, so reading them
	// before this frame writes them is fine.
	const TargetSet kPersistent = Targets(RenderTarget::ShadowMap, RenderTarget::PointShadowMaps,
		RenderTarget::PrevMainColor, RenderTarget::PrevCutsceneColor);

	TargetSet writtenThisFrame = 0;
	int depth = 0;
	int frameNumber = 0;
	std::vector<PassRecord> frame;

	bool DumpRequested()
	{
		static const bool on = []()
		{
			const char* v = std::getenv("KINJO_DUMP_FRAME");
			return v != nullptr && v[0] == '1';
		}();
		return on;
	}

	std::string Describe(TargetSet set)
	{
		std::string out;
		for (int i = 0; i < (int)RenderTarget::Count; i++)
		{
			if (set & (1u << i))
			{
				if (!out.empty()) out += ", ";
				out += RenderTargetName((RenderTarget)i);
			}
		}
		return out.empty() ? "-" : out;
	}
}

const char* RenderTargetName(RenderTarget target)
{
	switch (target)
	{
	case RenderTarget::Backbuffer:        return "Backbuffer";
	case RenderTarget::ShadowMap:         return "ShadowMap";
	case RenderTarget::PointShadowMaps:   return "PointShadowMaps";
	case RenderTarget::MainColor:         return "MainColor";
	case RenderTarget::MainDepth:         return "MainDepth";
	case RenderTarget::CharacterMask:     return "CharacterMask";
	case RenderTarget::CutsceneColor:     return "CutsceneColor";
	case RenderTarget::PrevMainColor:     return "PrevMainColor";
	case RenderTarget::PrevCutsceneColor: return "PrevCutsceneColor";
	default:                              return "?";
	}
}

void BeginFramePasses()
{
	writtenThisFrame = 0;
	depth = 0;
	frame.clear();
}

void RunPass(const char* name, TargetSet reads, TargetSet writes, const std::function<void()>& body)
{
	// Ordering check: report each (pass, target) problem once per run.
	const TargetSet unwritten = reads & ~writtenThisFrame & ~kPersistent;
	if (unwritten != 0)
	{
		static std::set<std::string> reported;
		const std::string key = std::string(name) + ":" + std::to_string(unwritten);
		if (reported.insert(key).second)
			std::cout << "WARNING: pass " << name << " reads " << Describe(unwritten)
				<< " before any earlier pass this frame wrote it" << std::endl;
	}

	frame.push_back({ name, depth, reads, writes });

	// A GPU-capture label (no-op where the backend can't).
	Device().PushDebugGroup(name);

	depth++;
	body();
	depth--;

	Device().PopDebugGroup();

	writtenThisFrame |= writes;
}

void EndFramePasses()
{
	frameNumber++;
	// Dump one settled frame (not the first, which also does first-use setup).
	if (DumpRequested() && frameNumber == 30)
	{
		std::cout << "Frame passes (frame " << frameNumber << "):" << std::endl;
		for (const PassRecord& p : frame)
		{
			std::cout << "  " << std::string(p.depth * 2, ' ') << p.name
				<< "  reads [" << Describe(p.reads) << "]  writes [" << Describe(p.writes) << "]" << std::endl;
		}
	}
}
