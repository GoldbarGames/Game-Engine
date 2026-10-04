#include "RenderPass.h"
#include "Shader.h"
#include "render/RenderDevice.h"
#include <chrono>
#include <cstdio>
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
		RenderTarget::PrevMainColor, RenderTarget::PrevCutsceneColor, RenderTarget::EnvMaps,
		RenderTarget::TaaHistory);

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

	// --- per-pass timings (KINJO_GPU_TIMINGS=1) --------------------------------
	// GPU time from timestamp queries around each pass (they nest, unlike
	// elapsed-time queries), read 3 frames later so nothing waits on the GPU;
	// CPU time = how long the pass took to record. Averages are printed every
	// reportEvery frames: 60, or KINJO_GPU_TIMINGS=<n> for n >= 2 (short
	// capture runs end before 60 frames).
	const int kTimingFrames = 4;
	int reportEvery = 60;

	bool TimingRequested()
	{
		static const bool on = []()
		{
			const char* v = std::getenv("KINJO_GPU_TIMINGS");
			const int n = (v != nullptr) ? std::atoi(v) : 0;
			const bool want = n >= 1;
			if (n >= 2)
				reportEvery = n;
			if (want && !Device().SupportsTimestamps())
				std::cout << "KINJO_GPU_TIMINGS: this backend has no GPU timestamps" << std::endl;
			else if (want)
				std::cout << "KINJO_GPU_TIMINGS: per-pass timings every " << reportEvery << " frames" << std::endl;
			return want && Device().SupportsTimestamps();
		}();
		return on;
	}

	struct PassTiming
	{
		std::string name;
		int depth = 0;
		QueryHandle begin, end;
		double cpuMs = 0.0;
	};
	std::vector<PassTiming> timingRing[kTimingFrames];
	int timingSlot = 0;
	std::vector<QueryHandle> freeQueries;

	struct TimingStat
	{
		std::string name;
		int depth = 0;
		double gpuMs = 0.0, cpuMs = 0.0;
		int samples = 0;
	};
	std::vector<TimingStat> timingStats;   // in the order passes first appeared
	double frameGpuMs = 0.0;
	int timedFrames = 0;
	int droppedFrames = 0;   // results not ready in time

	QueryHandle TakeQuery()
	{
		if (!freeQueries.empty())
		{
			const QueryHandle q = freeQueries.back();
			freeQueries.pop_back();
			return q;
		}
		return Device().CreateQuery();
	}

	// One frame's results (from kTimingFrames - 1 frames ago) into the averages.
	void CollectTimings(std::vector<PassTiming>& passes)
	{
		RenderDevice& device = Device();
		bool complete = !passes.empty();
		std::vector<double> gpu(passes.size(), 0.0);
		uint64_t first = 0, last = 0;
		for (size_t i = 0; i < passes.size() && complete; i++)
		{
			uint64_t b = 0, e = 0;
			if (!passes[i].begin || !passes[i].end
				|| !device.ReadTimestamp(passes[i].begin, b) || !device.ReadTimestamp(passes[i].end, e))
			{
				complete = false;   // the GPU is that far behind: drop this frame
				break;
			}
			gpu[i] = (double)(e - b) / 1.0e6;
			if (i == 0 || b < first) first = b;
			if (e > last) last = e;
		}
		if (complete)
		{
			for (size_t i = 0; i < passes.size(); i++)
			{
				TimingStat* stat = nullptr;
				for (TimingStat& s : timingStats)
					if (s.name == passes[i].name && s.depth == passes[i].depth)
						stat = &s;
				if (stat == nullptr)
				{
					timingStats.push_back(TimingStat());
					stat = &timingStats.back();
					stat->name = passes[i].name;
					stat->depth = passes[i].depth;
				}
				stat->gpuMs += gpu[i];
				stat->cpuMs += passes[i].cpuMs;
				stat->samples++;
			}
			frameGpuMs += (double)(last - first) / 1.0e6;
			timedFrames++;
		}
		else if (!passes.empty())
		{
			droppedFrames++;
		}
		for (PassTiming& p : passes)
		{
			freeQueries.push_back(p.begin);
			freeQueries.push_back(p.end);
		}
		passes.clear();

		if (timedFrames + droppedFrames >= reportEvery)
		{
			std::cout << "Pass timings (ms per frame, avg of " << timedFrames << " frames";
			if (droppedFrames > 0)
				std::cout << "; " << droppedFrames << " more not ready in time";
			std::cout << ")  GPU / CPU-record:" << std::endl;
			for (const TimingStat& s : timingStats)
			{
				char line[160];
				std::snprintf(line, sizeof(line), "  %-*s%-20s %7.3f / %6.3f%s", s.depth * 2, "", s.name.c_str(),
					s.gpuMs / s.samples, s.cpuMs / s.samples,
					(s.samples < timedFrames) ? "  (not every frame)" : "");
				std::cout << line << std::endl;
			}
			if (timedFrames > 0)
			{
				char total[96];
				std::snprintf(total, sizeof(total), "  GPU frame (first pass to last): %.3f", frameGpuMs / timedFrames);
				std::cout << total << std::endl;
			}
			timingStats.clear();
			frameGpuMs = 0.0;
			timedFrames = 0;
			droppedFrames = 0;
		}
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
	case RenderTarget::WorldHdr:          return "WorldHdr";
	case RenderTarget::Bloom:             return "Bloom";
	case RenderTarget::EnvMaps:           return "EnvMaps";
	case RenderTarget::AoGeometry:        return "AoGeometry";
	case RenderTarget::AoRaw:             return "AoRaw";
	case RenderTarget::AmbientOcclusion:  return "AmbientOcclusion";
	case RenderTarget::TaaHistory:        return "TaaHistory";
	case RenderTarget::TaaOutput:         return "TaaOutput";
	case RenderTarget::MotionVectors:     return "MotionVectors";
	case RenderTarget::LightClusters:     return "LightClusters";
	case RenderTarget::DepthOfField:      return "DepthOfField";
	case RenderTarget::FogVolume:         return "FogVolume";
	case RenderTarget::Reflections:       return "Reflections";
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

	const bool timed = TimingRequested();
	size_t timing = 0;
	std::chrono::steady_clock::time_point cpuStart;
	if (timed)
	{
		std::vector<PassTiming>& passes = timingRing[timingSlot];
		timing = passes.size();
		passes.push_back(PassTiming());
		passes[timing].name = name;
		passes[timing].depth = depth;
		passes[timing].begin = TakeQuery();
		Device().WriteTimestamp(passes[timing].begin);
		cpuStart = std::chrono::steady_clock::now();
	}

	depth++;
	body();
	depth--;

	if (timed)
	{
		PassTiming& p = timingRing[timingSlot][timing];
		p.cpuMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - cpuStart).count();
		p.end = TakeQuery();
		Device().WriteTimestamp(p.end);
	}

	Device().PopDebugGroup();

	writtenThisFrame |= writes;
}

void EndFramePasses()
{
	frameNumber++;
	if (TimingRequested())
	{
		// The next slot holds the frame from kTimingFrames - 1 frames ago.
		timingSlot = (timingSlot + 1) % kTimingFrames;
		CollectTimings(timingRing[timingSlot]);
	}
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
