// Clustered forward lighting - see ClusteredLights.h. Backend-agnostic: GPU
// work goes through RenderDevice.

#include "ClusteredLights.h"
#include "ColorPipeline.h"
#include "RenderDevice.h"
#include "ProgramEvents.h"
#include "RenderViews.h"
#include "../UniformBlocks.h"
#include "../UniformBufferCache.h"
#include "../globals.h"
#include <glm/glm.hpp>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <unordered_map>

namespace
{
	// The grid: screen tiles x depth slices. Fixed, so it costs the same at any
	// resolution (tiles just get bigger).
	const int kTilesX = 16;
	const int kTilesY = 9;
	const int kSlices = 24;
	const int kClusters = kTilesX * kTilesY * kSlices;
	const int kTextureWidth = 1024;      // texels per row of the light texture
	const float kMinSliceNear = 1.0f;    // slices start here, not at a 0.1 near plane

	// --- settings -----------------------------------------------------------
	bool projectOn = true;   // renderer.dat `clusteredLights`
	int forced = -1;         // KINJO_CLUSTERS
	bool debugView = false;  // KINJO_CLUSTER_DEBUG
	bool verifyAll = false;  // KINJO_CLUSTER_VERIFY: every light in every cluster (brute force, for comparison)

	// --- state --------------------------------------------------------------
	// Three textures in rotation (per view, below): new data goes into the next
	// one, never into one the GPU may still be reading for a frame in flight
	// (overwriting that stalls the pipeline until those draws finish).
	const int kRing = 3;
	std::vector<uint32_t> texels;     // what this frame uploads (4 uints per texel)
	std::vector<uint32_t> counts;       // per cluster
	std::vector<uint32_t> pairCluster;  // (cluster, light) for every light reaching a cluster,
	std::vector<uint32_t> pairLight;    // in light order

	std::vector<float> inputKey;      // what this build is made from (lights and camera)
	int builds = 0;

	struct Bounds
	{
		int s0 = 0, s1 = -1;   // slice range (empty when s1 < s0)
		int x0 = 0, x1 = 0, y0 = 0, y1 = 0;
	};

	// std140 mirror of the GLSL "Clusters" block (shaders/lights.glsl).
	struct ClustersBlockData
	{
		glm::ivec4 grid;     // tiles x, tiles y, slices, 1 = on
		glm::vec4 params;    // slice scale, slice bias, 1 / width, 1 / height
		glm::ivec4 layout;   // texture width, first table texel, first index texel, debug
	};
	static_assert(sizeof(ClustersBlockData) == 48, "ClustersBlockData must match shaders/lights.glsl");
	const UniformBlockMember kClustersMembers[] = {
		{ "clusterGrid", offsetof(ClustersBlockData, grid), false },
		{ "clusterParams", offsetof(ClustersBlockData, params), false },
		{ "clusterLayout", offsetof(ClustersBlockData, layout), false },
	};
	UniformBufferCache clusterBlocks(UniformBlock::Clusters, sizeof(ClustersBlockData), 2);

	// One set per split-screen view (render/RenderViews.h): each view's
	// clusters follow its own camera, so its cache and ring are its own too.
	struct ClusterView
	{
		bool active = false;
		TextureHandle ring[kRing];
		int ringRows[kRing] = {};
		int current = 0;                  // the one lit shaders read
		// Each cluster's view-space box, for the sphere tests. Depends only on the
		// projection, planes and target size, so it is rebuilt only when they change.
		std::vector<glm::vec3> clusterMin, clusterMax;
		std::vector<float> boxKey;
		// What the last build was made from: unchanged lights and camera (a still
		// view) skip the whole build.
		std::vector<float> lastInputKey;
		ClustersBlockData blockData = {};
	};
	ClusterView clusterViews[kMaxRenderViews];
	ClusterView* cv = &clusterViews[0];   // the view drawing now (SetLightClusterView)

	struct ProgramLocs { bool hasBlock = false; int sampler = -1; };
	std::unordered_map<unsigned int, ProgramLocs> locsByProgram;

	TextureHandle MakeTexture(int height)
	{
		TextureDesc desc;
		desc.format = TextureFormat::RGBA32UI;
		desc.width = kTextureWidth;
		desc.height = height;
		desc.filter = TextureFilter::Nearest;   // integer textures can't filter
		desc.wrap = TextureWrap::ClampToEdge;
		return Device().CreateTexture(desc);
	}

	uint32_t Bits(float f)
	{
		uint32_t u;
		std::memcpy(&u, &f, sizeof(u));
		return u;
	}

	void PutTexel(int texel, uint32_t a, uint32_t b, uint32_t c, uint32_t d)
	{
		uint32_t* t = &texels[(size_t)texel * 4];
		t[0] = a; t[1] = b; t[2] = c; t[3] = d;
	}

	// The view-space box around a light's sphere, projected: the screen tiles it
	// can touch (conservative: the box's corners bound the sphere's outline).
	void TileBounds(const glm::vec3& c, float r, float depthMin, float nearPlane, const glm::mat4& projection,
		int width, int height, Bounds& b)
	{
		if (depthMin <= nearPlane)
		{
			// The sphere reaches the camera: it can cover any part of the screen.
			b.x0 = 0; b.x1 = kTilesX - 1;
			b.y0 = 0; b.y1 = kTilesY - 1;
			return;
		}
		float lo[2] = { 1e30f, 1e30f };
		float hi[2] = { -1e30f, -1e30f };
		for (int i = 0; i < 8; i++)
		{
			const glm::vec4 corner(c.x + ((i & 1) ? r : -r), c.y + ((i & 2) ? r : -r), c.z + ((i & 4) ? r : -r), 1.0f);
			const glm::vec4 clip = projection * corner;
			const float w = std::max(clip.w, 1e-6f);
			for (int a = 0; a < 2; a++)
			{
				lo[a] = std::min(lo[a], clip[a] / w);
				hi[a] = std::max(hi[a], clip[a] / w);
			}
		}
		// A couple of pixels of margin: the shader picks tiles from pixel
		// centres, and the camera's sub-pixel jitter moves things by up to half
		// a pixel.
		const float marginX = 4.0f / (float)width, marginY = 4.0f / (float)height;
		auto tile = [](float ndc, int tiles) { return (int)std::floor((ndc * 0.5f + 0.5f) * (float)tiles); };
		b.x0 = std::max(tile(lo[0] - marginX, kTilesX), 0);
		b.x1 = std::min(tile(hi[0] + marginX, kTilesX), kTilesX - 1);
		b.y0 = std::max(tile(lo[1] - marginY, kTilesY), 0);
		b.y1 = std::min(tile(hi[1] + marginY, kTilesY), kTilesY - 1);
		if (b.x0 > b.x1 || b.y0 > b.y1)
			b.s1 = b.s0 - 1;   // entirely off screen
	}
}

void LoadClusteredLightSettings()
{
	auto config = ReadRendererConfig();
	projectOn = !(config.count("clusteredLights") > 0 && config["clusteredLights"] == "0");
	if (const char* e = std::getenv("KINJO_CLUSTERS"))
		forced = (e[0] == '0') ? 0 : (e[0] == '1') ? 1 : -1;
	if (const char* d = std::getenv("KINJO_CLUSTER_DEBUG"))
		debugView = (d[0] == '1');
	if (const char* v = std::getenv("KINJO_CLUSTER_VERIFY"))
		verifyAll = (v[0] == '1');
	if (verifyAll)
		std::cout << "Clustered lights: KINJO_CLUSTER_VERIFY - every light in every cluster (brute force)" << std::endl;
	if (forced >= 0 || debugView)
		std::cout << "Clustered lights: " << (ClusteredLightsWanted() ? "on" : "off")
			<< (forced >= 0 ? " (KINJO_CLUSTERS)" : "") << (debugView ? " (KINJO_CLUSTER_DEBUG: light-count view)" : "")
			<< std::endl;
}

void SetClusterDebugView(bool on)
{
	debugView = on;
	// An unchanged frame reuses the last build, block included: flip it in place.
	for (ClusterView& view : clusterViews)
		view.blockData.layout.w = on ? 1 : 0;
}

bool ClusterDebugView()
{
	return debugView;
}

bool ClusteredLightsWanted()
{
	return (forced >= 0) ? (forced == 1) : projectOn;
}

void BuildLightClusters(const std::vector<ClusterLight>& lights, const glm::mat4& view,
	const glm::mat4& projection, float nearPlane, float farPlane, int width, int height)
{
	if (!ClusteredLightsWanted() || width <= 0 || height <= 0)
	{
		DisableLightClusters();
		return;
	}

	// Depth slices, exponential: slice = log(depth) * scale + bias, so each is
	// a constant ratio deeper than the last (as far tiles cover more world).
	const float zNear = std::max(nearPlane, kMinSliceNear);
	const float zFar = std::max(farPlane, zNear * 2.0f);
	const float scale = (float)kSlices / std::log(zFar / zNear);
	const float bias = -std::log(zNear) * scale;
	const int lightCount = (int)lights.size();

	// 0. Nothing moved: the clusters already uploaded still hold.
	inputKey.clear();
	inputKey.insert(inputKey.end(), &view[0][0], &view[0][0] + 16);
	inputKey.insert(inputKey.end(), &projection[0][0], &projection[0][0] + 16);
	inputKey.push_back(nearPlane);
	inputKey.push_back(farPlane);
	inputKey.push_back((float)width);
	inputKey.push_back((float)height);
	for (const ClusterLight& l : lights)
	{
		const float v[16] = { l.pos.x, l.pos.y, l.pos.z, l.range, l.color.r, l.color.g, l.color.b, l.intensity,
			l.dir.x, l.dir.y, l.dir.z, l.cosOuter, l.cosInner, l.spot ? 1.0f : 0.0f, (float)l.shadow, 0.0f };
		inputKey.insert(inputKey.end(), v, v + 16);
	}
	if (cv->active && inputKey.size() == cv->lastInputKey.size()
		&& std::memcmp(inputKey.data(), cv->lastInputKey.data(), inputKey.size() * sizeof(float)) == 0)
	{
		clusterBlocks.Bind(&cv->blockData);
		return;
	}
	cv->lastInputKey.swap(inputKey);

	// 1. The clusters' view-space boxes (only when the projection changed).
	// For a perspective projection, view x at depth d for NDC x is
	// d * (ndc + P[2][0]) / P[0][0] (likewise y); a couple of pixels of margin
	// cover pixel centres and the sub-pixel jitter TAA adds when drawing.
	{
		std::vector<float> key(&projection[0][0], &projection[0][0] + 16);
		key.push_back(nearPlane);
		key.push_back(farPlane);
		key.push_back((float)width);
		key.push_back((float)height);
		if (key != cv->boxKey)
		{
			cv->boxKey.swap(key);
			cv->clusterMin.resize(kClusters);
			cv->clusterMax.resize(kClusters);
			const float marginX = 4.0f / (float)width, marginY = 4.0f / (float)height;
			for (int sl = 0; sl < kSlices; sl++)
			{
				// Slice 0 also takes everything nearer than zNear; the last, everything past zFar.
				const float d0 = (sl == 0) ? 0.0f : std::exp(((float)sl - bias) / scale) * 0.999f;
				const float d1 = (sl == kSlices - 1) ? 1.0e30f : std::exp(((float)(sl + 1) - bias) / scale) * 1.001f;
				const float dFar = std::min(d1, std::max(zFar, farPlane) * 4.0f);   // a finite box for the x/y extent
				for (int ty = 0; ty < kTilesY; ty++)
				{
					const float ny0 = -1.0f + 2.0f * ty / kTilesY - marginY, ny1 = -1.0f + 2.0f * (ty + 1) / kTilesY + marginY;
					for (int tx = 0; tx < kTilesX; tx++)
					{
						const float nx0 = -1.0f + 2.0f * tx / kTilesX - marginX, nx1 = -1.0f + 2.0f * (tx + 1) / kTilesX + marginX;
						glm::vec3 lo(1e30f), hi(-1e30f);
						for (int c = 0; c < 4; c++)
						{
							const float d = (c & 2) ? dFar : d0;
							const float x = d * (((c & 1) ? nx1 : nx0) + projection[2][0]) / projection[0][0];
							const float y0 = d * (ny0 + projection[2][1]) / projection[1][1];
							const float y1 = d * (ny1 + projection[2][1]) / projection[1][1];
							lo.x = std::min(lo.x, x); hi.x = std::max(hi.x, x);
							lo.y = std::min(lo.y, std::min(y0, y1)); hi.y = std::max(hi.y, std::max(y0, y1));
						}
						lo.z = -d1;
						hi.z = -d0;
						const int k = (sl * kTilesY + ty) * kTilesX + tx;
						cv->clusterMin[k] = lo;
						cv->clusterMax[k] = hi;
					}
				}
			}
		}
	}
	auto sliceOf = [&](float depth)
	{
		if (depth <= zNear)
			return 0;
		return std::min(std::max((int)std::floor(std::log(depth) * scale + bias), 0), kSlices - 1);
	};

	// 2. Each light against the clusters its sphere's screen rectangle and
	// depth range cover - then the sphere against each cluster's box.
	counts.assign(kClusters, 0);
	uint32_t* countOf = counts.data();
	const glm::vec3* boxMin = cv->clusterMin.data();
	const glm::vec3* boxMax = cv->clusterMax.data();
	pairCluster.clear();
	pairLight.clear();
	for (int i = 0; i < lightCount; i++)
	{
		const ClusterLight& l = lights[i];
		if (l.range <= 0.0f)
			continue;
		const glm::vec3 c = glm::vec3(view * glm::vec4(l.pos, 1.0f));
		const float depth = -c.z;
		const float depthMin = depth - l.range;
		const float depthMax = depth + l.range;
		if (verifyAll)
		{
			for (int k = 0; k < kClusters; k++)
			{
				countOf[k]++;
				pairCluster.push_back((uint32_t)k);
				pairLight.push_back((uint32_t)i);
			}
			continue;
		}
		if (depthMax <= nearPlane)
			continue;   // wholly behind the camera
		Bounds b;
		// One slice of margin each way (the box test below decides): the
		// shader's log may land a pixel right at a boundary on the other side.
		b.s0 = std::max(sliceOf(depthMin) - 1, 0);
		b.s1 = std::min(sliceOf(depthMax) + 1, kSlices - 1);
		TileBounds(c, l.range, depthMin, nearPlane, projection, width, height, b);
		const float r2 = l.range * l.range;
		for (int sl = b.s0; sl <= b.s1; sl++)
			for (int y = b.y0; y <= b.y1; y++)
				for (int x = b.x0; x <= b.x1; x++)
				{
					const int k = (sl * kTilesY + y) * kTilesX + x;
					const glm::vec3 nearest = glm::clamp(c, boxMin[k], boxMax[k]);
					const glm::vec3 d = c - nearest;
					if (d.x * d.x + d.y * d.y + d.z * d.z > r2)
						continue;
					countOf[k]++;
					pairCluster.push_back((uint32_t)k);
					pairLight.push_back((uint32_t)i);
				}
	}

	// 3. One texture: light records (4 texels each), the cluster table (each
	// cluster's first index and count), the lists (4 indices per texel) -
	// lights in order, so every list is ascending (points before spots, as
	// the shaders sum them).
	const uint32_t total = (uint32_t)pairCluster.size();
	const int tableStart = lightCount * 4;
	const int indexStart = tableStart + kClusters;
	const int texelCount = indexStart + (int)((total + 3) / 4);
	const int rows = (texelCount + kTextureWidth - 1) / kTextureWidth;
	texels.assign((size_t)rows * kTextureWidth * 4, 0u);
	for (int i = 0; i < lightCount; i++)
	{
		const ClusterLight& l = lights[i];
		PutTexel(i * 4 + 0, Bits(l.pos.x), Bits(l.pos.y), Bits(l.pos.z), Bits(l.range));
		PutTexel(i * 4 + 1, Bits(l.color.r), Bits(l.color.g), Bits(l.color.b), Bits(l.intensity));
		PutTexel(i * 4 + 2, Bits(l.dir.x), Bits(l.dir.y), Bits(l.dir.z), Bits(l.cosOuter));
		PutTexel(i * 4 + 3, Bits(l.cosInner), l.spot ? 1u : 0u, (uint32_t)(l.shadow + 1), 0u);
	}
	uint32_t* t = texels.data();
	uint32_t first = 0;
	int busiest = 0, nonEmpty = 0;
	for (int k = 0; k < kClusters; k++)
	{
		uint32_t* entry = t + (size_t)(tableStart + k) * 4;
		entry[0] = first;
		entry[1] = countOf[k];
		busiest = std::max(busiest, (int)countOf[k]);
		nonEmpty += (countOf[k] > 0) ? 1 : 0;
		countOf[k] = first;   // now each cluster's next free slot
		first += entry[1];
	}
	uint32_t* list = t + (size_t)indexStart * 4;
	const uint32_t* pc = pairCluster.data();
	const uint32_t* pl = pairLight.data();
	for (uint32_t p = 0; p < total; p++)
		list[countOf[pc[p]]++] = pl[p];

	if (debugView && (builds++ % 300) == 0)
		std::cout << "Clustered lights: " << lightCount << " lights, " << total << " cluster entries; "
			<< nonEmpty << " of " << kClusters << " clusters lit, " << (nonEmpty ? (float)total / nonEmpty : 0.0f)
			<< " lights each on average, " << busiest << " at most" << std::endl;

	// 4. Upload into the next texture of the ring.
	{
		RenderDevice& device = Device();
		const int next = (cv->current + 1) % kRing;
		if (!cv->ring[next] || rows > cv->ringRows[next])
		{
			if (cv->ring[next])
				device.DestroyTexture(cv->ring[next]);
			int height = 8;
			while (height < rows)
				height *= 2;
			cv->ring[next] = MakeTexture(height);
			cv->ringRows[next] = height;
		}
		device.UpdateTexture(cv->ring[next], TextureFormat::RGBA32UI, 0, 0, kTextureWidth, rows, texels.data());
		cv->current = next;
	}

	cv->blockData.grid = glm::ivec4(kTilesX, kTilesY, kSlices, 1);
	cv->blockData.params = glm::vec4(scale, bias, 1.0f / (float)width, 1.0f / (float)height);
	cv->blockData.layout = glm::ivec4(kTextureWidth, tableStart, indexStart, debugView ? 1 : 0);
	cv->active = true;
	clusterBlocks.Bind(&cv->blockData);
}

void DisableLightClusters()
{
	cv->active = false;
	cv->lastInputKey.clear();
	cv->blockData = ClustersBlockData();
	clusterBlocks.Bind(&cv->blockData);
}

void BindLightClusters(unsigned int program)
{
	clusterBlocks.Bind(&cv->blockData);
	if (program == 0)
		return;
	RenderDevice& device = Device();
	auto it = locsByProgram.find(program);
	if (it == locsByProgram.end())
	{
		// Forget a program's entry when it is deleted (GL reuses ids).
		AddProgramDeletedListener([](unsigned int p) { locsByProgram.erase(p); });
		ProgramLocs l;
		l.hasBlock = device.HasUniformBlock(ProgramHandle(program), "Clusters");
		l.sampler = device.UniformLocation(ProgramHandle(program), "clusterData");
		if (l.hasBlock)
			CheckUniformBlockLayout(program, "Clusters", kClustersMembers,
				sizeof(kClustersMembers) / sizeof(kClustersMembers[0]), sizeof(ClustersBlockData));
		it = locsByProgram.emplace(program, l).first;
	}
	if (!it->second.hasBlock || it->second.sampler < 0)
		return;
	// The sampler gets its unit and an integer texture even when off (not
	// sampled then): an integer sampler over the albedo on unit 0, or over no
	// texture, is undefined behaviour the driver may complain about every draw.
	device.SetUniform(it->second.sampler, 14);
	if (!cv->ring[cv->current])
	{
		cv->ring[cv->current] = MakeTexture(8);
		cv->ringRows[cv->current] = 8;
	}
	device.BindTexture(14, cv->ring[cv->current]);
}

void SetLightClusterView(int index)
{
	cv = &clusterViews[(index >= 0 && index < kMaxRenderViews) ? index : 0];
}

void ReleaseClusteredLights()
{
	for (ClusterView& view : clusterViews)
	{
		for (int i = 0; i < kRing; i++)
			if (view.ring[i])
				Device().DestroyTexture(view.ring[i]);
		view = ClusterView();
	}
	cv = &clusterViews[0];
	texels.clear();
	locsByProgram.clear();
}
