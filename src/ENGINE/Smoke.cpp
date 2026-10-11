// Smoke and steam - see Smoke.h. The puffs live here, in a file-local list
// (no exported class grows); Scene3D::Update moves them and Game::Render draws
// them in the "Smoke" pass.

#include "Smoke.h"
#include "Scene3D.h"
#include "Renderer.h"
#include "Camera.h"
#include "Shader.h"
#include "RenderState.h"
#include "TransientBuffer.h"
#include "globals.h"
#include "render/RenderDevice.h"
#include "render/ColorPipeline.h"
#include <glm/glm.hpp>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <utility>
#include <vector>

namespace
{
	const int MAX_PUFFS = 4096;

	struct Puff
	{
		SmokePuff birth;           // as emitted: size, life, colour...
		glm::vec3 pos = glm::vec3(0.0f);
		glm::vec3 vel = glm::vec3(0.0f);
		float age = 0.0f;
		float rotation = 0.0f;     // radians, the texture's turn on the quad
		float spin = 0.0f;         // radians a second
		float seed = 0.0f;         // 0..1: which shape, and how it wanders
	};
	std::vector<Puff> puffs;
	glm::vec3 wind(0.0f);
	bool drawOn = true;

	// What the GPU gets per puff: three vec4s (shaders/smoke.vert).
	struct Instance
	{
		glm::vec4 posRadius;       // world position, radius now
		glm::vec4 params;          // rotation, opacity now, steam, seed
		glm::vec4 color;           // albedo (authored sRGB), glow
	};
	std::vector<Instance> instances;
	std::vector<std::pair<float, int>> order;

	ShaderProgram* shader = nullptr;
	VertexArrayHandle vao;
	BufferHandle quad;
	BufferHandle fallback;     // the instances when the transient ring is unavailable
	TextureHandle puffTexture;

	// --- flames and sparks (EmitFlame) -----------------------------------------
	const int MAX_LICKS = 3072;
	struct Lick
	{
		FlameLick birth;
		glm::vec3 pos = glm::vec3(0.0f);
		glm::vec3 vel = glm::vec3(0.0f);
		float age = 0.0f;
		float seed = 0.0f;
	};
	std::vector<Lick> licks;
	// What the GPU gets per lick: four vec4s (shaders/flame.vert).
	struct LickInstance
	{
		glm::vec4 rootLength;      // where its root is, how long it is now
		glm::vec4 dirWidth;        // the way it points (unit), how wide it is now
		glm::vec4 params;          // age / life, brightness, temperature, seed
		glm::vec4 params2;         // spark, unused, unused, unused
	};
	std::vector<LickInstance> lickInstances;
	ShaderProgram* flameShader = nullptr;
	VertexArrayHandle flameVao;
	BufferHandle flameFallback;
	float flameClock = 0.0f;   // seconds: the flames' flicker

	void RenderPuffs(const Renderer& renderer, unsigned int sceneDepth);
	void RenderFlames(const Renderer& renderer, unsigned int sceneDepth);

	unsigned int rngState = 0x9e3779b9u;
	float Rand01()
	{
		rngState ^= rngState << 13;
		rngState ^= rngState >> 17;
		rngState ^= rngState << 5;
		return (float)(rngState & 0xffffff) / (float)0x1000000;
	}

	// --- the puff's shape -------------------------------------------------------
	// Four billows in a 2x2 atlas, each a cauliflower: a cluster of round lumps
	// of different sizes, overlapping, with a fine noise over them and a soft
	// edge, so a puff reads as rolling smoke with lumps in it - and the shader
	// lights the lumps (their slopes are its bumps) - rather than a blurred disc.
	float Hash(int x, int y, int s)
	{
		unsigned int h = (unsigned int)x * 374761393u + (unsigned int)y * 668265263u + (unsigned int)s * 2246822519u;
		h = (h ^ (h >> 13)) * 1274126177u;
		return (float)((h ^ (h >> 16)) & 0xffffff) / (float)0x1000000;
	}
	float Noise(float x, float y, int s)
	{
		const int ix = (int)std::floor(x), iy = (int)std::floor(y);
		float tx = x - ix, ty = y - iy;
		tx = tx * tx * (3.0f - 2.0f * tx);
		ty = ty * ty * (3.0f - 2.0f * ty);
		const float a = Hash(ix, iy, s), b = Hash(ix + 1, iy, s);
		const float c = Hash(ix, iy + 1, s), d = Hash(ix + 1, iy + 1, s);
		return (a + (b - a) * tx) + ((c + (d - c) * tx) - (a + (b - a) * tx)) * ty;
	}

	void MakePuffTexture()
	{
		const int CELL = 128, S = CELL * 2;
		std::vector<unsigned char> px((size_t)S * S * 4, 0);
		for (int v = 0; v < 4; v++)
		{
			const int ox = (v % 2) * CELL, oy = (v / 2) * CELL;
			// The lumps: one big one in the middle, the rest round it.
			struct Lump { float x, y, r; };
			std::vector<Lump> lumps;
			lumps.push_back({ 0.0f, 0.0f, 0.55f });
			for (int k = 0; k < 9; k++)
			{
				const float a = Hash(k, v, 71) * 6.2831853f;
				const float d = 0.15f + 0.42f * Hash(k, v, 72);
				const float r = 0.18f + 0.22f * Hash(k, v, 73);
				lumps.push_back({ std::cos(a) * d, std::sin(a) * d, std::min(r, 0.96f - d) });
			}
			for (int y = 0; y < CELL; y++)
			{
				for (int x = 0; x < CELL; x++)
				{
					const float u = (x + 0.5f) / CELL * 2.0f - 1.0f, w = (y + 0.5f) / CELL * 2.0f - 1.0f;
					const float r = std::sqrt(u * u + w * w);
					float n = 0.0f, amp = 0.5f, freq = 4.0f, total = 0.0f;
					for (int o = 0; o < 4; o++)
					{
						n += amp * Noise(u * freq + 7.3f * v, w * freq - 3.1f * v, 11 + v * 5 + o);
						total += amp;
						amp *= 0.5f;
						freq *= 2.2f;
					}
					n /= total;
					// Each lump a rounded dome; where they overlap the cloud is
					// thicker. Their edges wander with the noise.
					float lumpy = 0.0f;
					for (const Lump& l : lumps)
					{
						const float dx = u - l.x, dy = w - l.y;
						const float q = std::sqrt(dx * dx + dy * dy) / (l.r * (0.8f + 0.4f * n));
						if (q < 1.0f)
							lumpy += std::pow(1.0f - q * q, 1.2f);
					}
					float density = std::min(1.0f, lumpy * 0.75f) * (0.75f + 0.5f * n);
					density *= 1.0f - std::max(0.0f, std::min(1.0f, (r - 0.85f) / 0.12f));
					if (r > 0.97f)
						density = 0.0f;
					const unsigned char b = (unsigned char)(std::max(0.0f, std::min(1.0f, density)) * 255.0f);
					unsigned char* p = &px[((size_t)(oy + y) * S + (ox + x)) * 4];
					p[0] = p[1] = p[2] = b;
					p[3] = 255;
				}
			}
		}
		TextureDesc desc;
		desc.width = S;
		desc.height = S;
		desc.filter = TextureFilter::Trilinear;
		desc.wrap = TextureWrap::ClampToEdge;
		desc.generateMipmaps = true;
		puffTexture = Device().CreateTexture(desc, px.data());
	}

	bool EnsureResources()
	{
		if (shader == nullptr)
			shader = new ShaderProgram(-1, "data/shaders/smoke.vert", "data/shaders/smoke.frag");
		if (vao.id != 0)
			return shader != nullptr && shader->GetID() != 0;

		// The same unit quad the weather draws: corner -0.5..0.5, uv 0..1.
		const float corners[] = {
			-0.5f, -0.5f, 0.0f, 0.0f,
			 0.5f, -0.5f, 1.0f, 0.0f,
			 0.5f,  0.5f, 1.0f, 1.0f,
			-0.5f, -0.5f, 0.0f, 0.0f,
			 0.5f,  0.5f, 1.0f, 1.0f,
			-0.5f,  0.5f, 0.0f, 1.0f,
		};
		RenderDevice& device = Device();
		MakePuffTexture();   // (creating a texture rebinds unit 0: before anything is bound)
		vao = device.CreateVertexArray();
		quad = device.CreateBuffer(sizeof(corners), corners, BufferUsage::Static);
		device.SetVertexAttribute(vao, 0, quad, 2, 4 * sizeof(float), 0);
		device.SetVertexAttribute(vao, 1, quad, 2, 4 * sizeof(float), 2 * sizeof(float));
		fallback = device.CreateBuffer(MAX_PUFFS * sizeof(Instance), nullptr, BufferUsage::Dynamic);
		return shader != nullptr && shader->GetID() != 0;
	}

	bool EnsureFlameResources()
	{
		if (flameShader == nullptr)
			flameShader = new ShaderProgram(-1, "data/shaders/flame.vert", "data/shaders/flame.frag");
		if (flameVao.id == 0 && quad.id != 0)
		{
			// The puffs' quad, with the flames' own instance layout.
			RenderDevice& device = Device();
			flameVao = device.CreateVertexArray();
			device.SetVertexAttribute(flameVao, 0, quad, 2, 4 * sizeof(float), 0);
			device.SetVertexAttribute(flameVao, 1, quad, 2, 4 * sizeof(float), 2 * sizeof(float));
			flameFallback = device.CreateBuffer(MAX_LICKS * sizeof(LickInstance), nullptr, BufferUsage::Dynamic);
		}
		return flameVao.id != 0 && flameShader != nullptr && flameShader->GetID() != 0;
	}

	// How long and how wide a lick is at its age, and the way it points.
	void LickNow(const Lick& l, glm::vec3& root, glm::vec3& dir, float& length, float& width)
	{
		const FlameLick& b = l.birth;
		const float x = std::min(1.0f, l.age / std::max(0.05f, b.life));
		const float speed = glm::length(l.vel);
		dir = (speed > 0.01f) ? l.vel / speed : glm::vec3(0.0f, -1.0f, 0.0f);
		if (b.spark > 0.5f)
		{
			// A spark is a streak behind its head, as long as it is fast.
			length = b.length * std::max(0.25f, std::min(1.0f, speed / 5.0f));
			width = b.width;
			root = l.pos - dir * length;
			return;
		}
		// A flame stands up off its root, leaning with the air, grows and
		// shrinks again as it burns out.
		dir = glm::normalize(dir + glm::vec3(0.0f, -1.5f, 0.0f));
		length = b.length * (0.35f + 0.65f * std::sin(3.14159265f * std::min(1.0f, x * 1.15f)));
		width = b.width * (1.0f - 0.45f * x);
		root = l.pos;
	}

	// How big and how thick a puff is at its age.
	void Now(const Puff& p, float& radius, float& opacity)
	{
		const SmokePuff& b = p.birth;
		const float x = std::min(1.0f, p.age / std::max(0.05f, b.life));
		// It spreads fastest at first, as a jet mixes into the air.
		radius = b.radius + b.spread * (1.0f - (1.0f - x) * (1.0f - x));
		// Thickest as it leaves the stack: only the first moment is faded in,
		// so a puff is not born as a hard-edged card.
		const float fadeIn = std::min(1.0f, p.age / 0.04f);
		// Steam evaporates: it thins faster than smoke and is gone at the end.
		opacity = b.opacity * fadeIn * std::pow(1.0f - x, 1.2f + 1.5f * b.steam);
	}
}

void EmitSmoke(const SmokePuff& puff)
{
	if ((int)puffs.size() >= MAX_PUFFS || puff.life <= 0.0f)
		return;
	Puff p;
	p.birth = puff;
	p.pos = puff.position;
	p.vel = puff.velocity;
	p.rotation = Rand01() * 6.2831853f;
	p.spin = (Rand01() - 0.5f) * 0.8f;
	p.seed = Rand01();
	puffs.push_back(p);
}

void ClearSmoke()
{
	puffs.clear();
	licks.clear();
}

void EmitFlame(const FlameLick& lick)
{
	if ((int)licks.size() >= MAX_LICKS || lick.life <= 0.0f)
		return;
	Lick l;
	l.birth = lick;
	l.pos = lick.position;
	l.vel = lick.velocity;
	l.seed = Rand01();
	licks.push_back(l);
}

int FlameCount()
{
	return (int)licks.size();
}

int SmokeCount()
{
	return (int)puffs.size();
}

void SetSmokeWind(const glm::vec3& windPerSecond)
{
	wind = windPerSecond;
}

void UpdateSmoke(float dt)
{
	if (dt <= 0.0f)
		return;
	dt = std::min(dt, 0.25f);
	flameClock += dt;

	// The flames: risen by their heat, carried by the air, flickering about.
	size_t keepLicks = 0;
	for (size_t i = 0; i < licks.size(); i++)
	{
		Lick& l = licks[i];
		l.age += dt;
		if (l.age >= l.birth.life)
			continue;
		const FlameLick& b = l.birth;
		const float k = 1.0f - std::exp(-b.drag * dt);
		l.vel += (wind - l.vel) * k;
		l.vel.y -= b.rise * dt;                            // up is -Y
		const float s = l.seed * 6.2831853f;
		const glm::vec3 eddy(std::sin(s * 3.0f + l.age * 9.0f), 0.4f * std::cos(s * 5.0f + l.age * 7.0f),
			std::sin(s * 7.0f + l.age * 8.0f));
		l.vel += eddy * (b.turbulence * 3.0f * dt);
		l.pos += l.vel * dt;
		licks[keepLicks++] = l;
	}
	licks.resize(keepLicks);

	if (puffs.empty())
		return;
	size_t keep = 0;
	for (size_t i = 0; i < puffs.size(); i++)
	{
		Puff& p = puffs[i];
		p.age += dt;
		if (p.age >= p.birth.life)
			continue;
		const SmokePuff& b = p.birth;

		// Into the moving air: the jet slows to the wind's speed.
		const float k = 1.0f - std::exp(-b.drag * dt);
		p.vel += (wind - p.vel) * k;
		// Warm, it rises - less as it mixes with the air round it.
		const float x = p.age / b.life;
		p.vel.y -= b.rise * std::exp(-2.5f * x) * dt;   // up is -Y
		// ...and wanders: slow eddies, different for every puff.
		const float s = p.seed * 6.2831853f;
		const glm::vec3 eddy(std::sin(s * 3.0f + p.age * 1.7f), 0.6f * std::cos(s * 5.0f + p.age * 1.3f),
			std::sin(s * 7.0f + p.age * 1.1f));
		p.vel += eddy * (b.turbulence * 1.5f * dt);
		p.pos += p.vel * dt;
		p.rotation += p.spin * dt;
		p.spin *= std::exp(-0.3f * dt);

		puffs[keep++] = p;
	}
	puffs.resize(keep);
}

void LoadSmokeSettings()
{
	auto config = ReadRendererConfig();
	drawOn = !(config.count("smoke") > 0 && config["smoke"] == "0");
	if (const char* e = std::getenv("KINJO_SMOKE"))
		drawOn = (e[0] != '0');
}

bool SmokeToDraw()
{
	return drawOn && (!puffs.empty() || !licks.empty());
}

void RenderSmoke(const Renderer& renderer, unsigned int sceneDepth, int width, int height)
{
	(void)width;
	(void)height;
	if (!SmokeToDraw() || renderer.camera.useOrthoCamera)
		return;
	if (!EnsureResources())
		return;
	RenderPuffs(renderer, sceneDepth);
	RenderFlames(renderer, sceneDepth);
}

namespace
{
void RenderPuffs(const Renderer& renderer, unsigned int sceneDepth)
{
	if (puffs.empty())
		return;
	// Back to front from this camera: each puff is blended over what is behind it.
	const glm::vec3 eye = renderer.camera.position;
	order.clear();
	for (int i = 0; i < (int)puffs.size(); i++)
	{
		const glm::vec3 d = puffs[i].pos - eye;
		order.push_back({ glm::dot(d, d), i });
	}
	std::sort(order.begin(), order.end(),
		[](const std::pair<float, int>& a, const std::pair<float, int>& b) { return a.first > b.first; });
	instances.clear();
	for (const auto& o : order)
	{
		const Puff& p = puffs[o.second];
		float radius = 0.0f, opacity = 0.0f;
		Now(p, radius, opacity);
		if (opacity < 0.003f)
			continue;
		Instance in;
		in.posRadius = glm::vec4(p.pos, radius);
		in.params = glm::vec4(p.rotation, opacity, p.birth.steam, p.seed);
		in.color = glm::vec4(p.birth.color, p.birth.glow);
		instances.push_back(in);
	}
	if (instances.empty())
		return;

	RenderDevice& device = Device();
	const size_t bytes = instances.size() * sizeof(Instance);
	const TransientAlloc a = TransientUpload(instances.data(), bytes);
	BufferHandle buffer = fallback;
	size_t offset = 0;
	if (a.Valid())
	{
		buffer = BufferHandle(a.buffer);
		offset = a.offset;
	}
	else
		device.UpdateBuffer(fallback, 0, bytes, instances.data());
	for (unsigned int k = 0; k < 3; k++)
		device.SetVertexAttribute(vao, 3 + k, buffer, 4, sizeof(Instance), offset + k * sizeof(glm::vec4), 1);

	shader->UseShader();
	const unsigned int id = shader->GetID();
	renderer.BindWorldCameraBlock();
	Scene3D::Get().ApplyLighting(id, renderer);   // sun, sky, lamps, shadows, fog
	const bool soft = sceneDepth != 0;
	device.SetUniform(ShaderProgram::DrawUniformLocation(id, "uDepth"),
		glm::vec4(renderer.camera.nearPlane, renderer.camera.farPlane, soft ? 1.0f : 0.0f, 0.5f));
	device.SetUniform(ShaderProgram::DrawUniformLocation(id, "uFade"), glm::vec4(0.2f, 1.0f, 0.0f, 0.0f));
	device.BindTexture(0, puffTexture);
	device.SetUniform(device.UniformLocation(ProgramHandle(id), "puffTex"), 0);
	if (soft)
	{
		device.BindTexture(1, TextureHandle(sceneDepth));
		device.SetUniform(device.UniformLocation(ProgramHandle(id), "sceneDepth"), 1);
	}

	RenderState s = CurrentRenderState();
	s.blend = BlendMode::Premultiplied;
	s.depthWrite = false;
	s.depthTest = !soft;          // with the scene's depth, the shader does the test
	s.depthCompare = CompareOp::LessEqual;
	s.cull = CullMode::None;
	{
		ScopedRenderState scope(s);
		device.Draw(vao, Primitive::Triangles, 0, 6, (int)instances.size());
	}
	if (soft)
		device.BindTexture(1, TextureHandle());
	renderer.drawCallsPerFrame++;
}

// The flames and sparks: over the smoke, adding their light (written with no
// coverage, so the premultiplied blend adds them), in no particular order.
void RenderFlames(const Renderer& renderer, unsigned int sceneDepth)
{
	if (licks.empty() || !EnsureFlameResources())
		return;
	lickInstances.clear();
	for (const Lick& l : licks)
	{
		glm::vec3 root, dir;
		float length = 0.0f, width = 0.0f;
		LickNow(l, root, dir, length, width);
		LickInstance in;
		in.rootLength = glm::vec4(root, length);
		in.dirWidth = glm::vec4(dir, width);
		in.params = glm::vec4(l.age / std::max(0.05f, l.birth.life), l.birth.brightness, l.birth.temperature, l.seed);
		in.params2 = glm::vec4(l.birth.spark, 0.0f, 0.0f, 0.0f);
		lickInstances.push_back(in);
	}

	RenderDevice& device = Device();
	const size_t bytes = lickInstances.size() * sizeof(LickInstance);
	const TransientAlloc a = TransientUpload(lickInstances.data(), bytes);
	BufferHandle buffer = flameFallback;
	size_t offset = 0;
	if (a.Valid())
	{
		buffer = BufferHandle(a.buffer);
		offset = a.offset;
	}
	else
		device.UpdateBuffer(flameFallback, 0, bytes, lickInstances.data());
	for (unsigned int k = 0; k < 4; k++)
		device.SetVertexAttribute(flameVao, 3 + k, buffer, 4, sizeof(LickInstance), offset + k * sizeof(glm::vec4), 1);

	flameShader->UseShader();
	const unsigned int id = flameShader->GetID();
	renderer.BindWorldCameraBlock();
	Scene3D::Get().ApplyLighting(id, renderer);   // the scene block: fog
	const bool soft = sceneDepth != 0;
	device.SetUniform(ShaderProgram::DrawUniformLocation(id, "uDepth"),
		glm::vec4(renderer.camera.nearPlane, renderer.camera.farPlane, soft ? 1.0f : 0.0f, 0.25f));
	device.SetUniform(ShaderProgram::DrawUniformLocation(id, "uFade"), glm::vec4(0.2f, 1.0f, flameClock, 0.0f));
	if (soft)
	{
		device.BindTexture(1, TextureHandle(sceneDepth));
		device.SetUniform(device.UniformLocation(ProgramHandle(id), "sceneDepth"), 1);
	}

	RenderState s = CurrentRenderState();
	s.blend = BlendMode::Premultiplied;
	s.depthWrite = false;
	s.depthTest = !soft;
	s.depthCompare = CompareOp::LessEqual;
	s.cull = CullMode::None;
	{
		ScopedRenderState scope(s);
		device.Draw(flameVao, Primitive::Triangles, 0, 6, (int)lickInstances.size());
	}
	if (soft)
		device.BindTexture(1, TextureHandle());
	renderer.drawCallsPerFrame++;
}
}
