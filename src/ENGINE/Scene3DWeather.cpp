// Scene3D weather: rain/snow particles, the fountain jet and the lightning flash.
// Split out of Scene3D.cpp (render-pass refactor, Phase 1 step 6).

#include "Scene3D.h"
#include "render/RenderDevice.h"
#include "Game.h"
#include "Renderer.h"
#include "Camera.h"
#include "Sprite.h"
#include "SpriteManager.h"
#include "Texture.h"
#include "Shader.h"
#include "Mesh.h"
#include "Skybox.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <filesystem>
#include <map>
#include <tuple>
#include <vector>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <cstddef>
#include "UniformBlocks.h"
#include "UniformBufferCache.h"
#include "RenderState.h"
#include "TransientBuffer.h"

#include "Scene3DInternal.h"
#include "render/DistanceFog.h"
#include "render/RenderViews.h"

using Scene3DInternal::ProgramHasBlock;

namespace
{
	// Where the weather volume was last centred (the main camera, Scene3D::Update).
	// Each split-screen view draws it moved to its own camera: the box wraps, so
	// the moved copy is just as full.
	glm::vec3 weatherCentre(0.0f);

	// Particles fade out into the distance fog (weather.frag; amount 0 = off).
	void SetParticleDistanceFog(unsigned int program)
	{
		const DistanceFogFrame fog = CurrentDistanceFog();
		const glm::vec4 range = (fog.amount > 0.0f)
			? glm::vec4(fog.nearDistance, fog.farDistance, fog.amount, 0.0f) : glm::vec4(0.0f);
		Device().SetUniform(ShaderProgram::DrawUniformLocation(program, "uDistFog"), range);
	}

	// Upload one float-vector instance attribute (location `location`, `components`
	// floats per instance) for this frame and point `vao` at it: via the transient
	// ring when available, else into the feature's own `fallbackVBO` as before.
	void StreamInstanceAttrib(unsigned int vao, unsigned int fallbackVBO, unsigned int location, int components,
		const void* data, size_t bytes)
	{
		RenderDevice& device = Device();
		const size_t stride = components * sizeof(float);
		const TransientAlloc a = TransientUpload(data, bytes);
		if (a.Valid())
		{
			device.SetVertexAttribute(VertexArrayHandle(vao), location, BufferHandle(a.buffer),
				components, stride, a.offset, 1);
		}
		else
		{
			device.UpdateBuffer(BufferHandle(fallbackVBO), 0, bytes, data);
			device.SetVertexAttribute(VertexArrayHandle(vao), location, BufferHandle(fallbackVBO),
				components, stride, 0, 1);
		}
	}

	// A per-instance float-vector attribute with its own dynamic buffer of
	// `bytes`, re-pointed each frame by StreamInstanceAttrib.
	unsigned int CreateInstanceAttrib(VertexArrayHandle vao, unsigned int location, int components, size_t bytes)
	{
		RenderDevice& device = Device();
		const BufferHandle vbo = device.CreateBuffer(bytes, nullptr, BufferUsage::Dynamic);
		device.SetVertexAttribute(vao, location, vbo, components, components * sizeof(float), 0, 1);
		return vbo.id;
	}

	// The shared particle quad's corner (location 0) and uv (1) on `vao`.
	void PointAtParticleQuad(VertexArrayHandle vao, unsigned int quadVBO)
	{
		RenderDevice& device = Device();
		device.SetVertexAttribute(vao, 0, BufferHandle(quadVBO), 2, 4 * sizeof(float), 0);
		device.SetVertexAttribute(vao, 1, BufferHandle(quadVBO), 2, 4 * sizeof(float), 2 * sizeof(float));
	}

	// Weather and fountain particles: depth TEST on (geometry occludes them) but
	// depth WRITE off (so they don't z-fight each other and are ignored by the
	// toon outline's depth read), alpha blended.
	RenderState ParticleState()
	{
		RenderState s = CurrentRenderState();
		s.depthTest = true;
		s.depthWrite = false;
		s.blend = BlendMode::Alpha;
		return s;
	}

}

// ------------------------------------------------------- weather particles

void Scene3D::SetWeather(WeatherType type, float intensity)
{
	weatherType = type;
	weatherIntensity = glm::clamp(intensity, 0.0f, 1.0f);
	weatherInit = false;   // reseed the particle volume on the next Update
	// Rearm the lightning system for a fresh storm (idle for non-storm weather).
	stormSeeded = false;
	flashIntensity = 0.0f;
	reflashTimer = thunderTimer = -1.0f;
}

void Scene3D::EnsureWeatherResources()
{
	// The shader is built here rather than only in Load(), so that weather can
	// be used WITHOUT a scene: a game that draws its own world (TrainRails does)
	// can set a weather type, tick Update, and call RenderWeather, and every
	// resource it needs is made on demand. Before this, weatherShader stayed
	// null for such a game and RenderWeather silently drew nothing.
	if (weatherShader == nullptr)
	{
		weatherShader = new ShaderProgram(-1, weatherShaderVert.c_str(),
			weatherShaderFrag.c_str());
	}

	if (weatherVAO != 0)
		return;

	// A unit quad, corners in [-0.5, 0.5] with 0..1 UVs (two triangles, no EBO).
	const float quad[] = {
		// corner.xy      uv
		-0.5f, -0.5f,   0.0f, 0.0f,
		 0.5f, -0.5f,   1.0f, 0.0f,
		 0.5f,  0.5f,   1.0f, 1.0f,
		-0.5f, -0.5f,   0.0f, 0.0f,
		 0.5f,  0.5f,   1.0f, 1.0f,
		-0.5f,  0.5f,   0.0f, 1.0f,
	};

	RenderDevice& device = Device();
	const VertexArrayHandle vao = device.CreateVertexArray();
	weatherVAO = vao.id;
	weatherQuadVBO = device.CreateBuffer(sizeof(quad), quad, BufferUsage::Static).id;
	PointAtParticleQuad(vao, weatherQuadVBO);

	// Per-instance buffer: vec4 (world pos xyz + random seed w), streamed each frame.
	weatherInstVBO = CreateInstanceAttrib(vao, 3, 4, kMaxWeatherParticles * sizeof(glm::vec4));

	// Procedural particle sprites (white RGBA with a soft alpha falloff), so
	// weather needs no art asset. Snow = soft round dot; rain = a tapered vertical
	// streak (bright core, fading toward the ends and side edges).
	auto uploadTex = [](unsigned int& id, int w, int h, const std::vector<unsigned char>& px)
	{
		TextureDesc desc;
		desc.width = w;
		desc.height = h;
		desc.filter = TextureFilter::Linear;
		desc.wrap = TextureWrap::ClampToEdge;
		id = Device().CreateTexture(desc, px.data()).id;
	};

	{
		const int S = 32;
		std::vector<unsigned char> px(S * S * 4);
		for (int y = 0; y < S; y++)
			for (int x = 0; x < S; x++)
			{
				float dx = (x + 0.5f) / S - 0.5f;
				float dy = (y + 0.5f) / S - 0.5f;
				float d = std::sqrt(dx * dx + dy * dy) / 0.5f;   // 0 center .. 1 edge
				float a = std::exp(-d * d * 4.0f);               // gaussian falloff
				if (a < 0.0f) a = 0.0f;
				int i = (y * S + x) * 4;
				px[i] = px[i + 1] = px[i + 2] = 255;
				px[i + 3] = (unsigned char)(a * 255.0f);
			}
		uploadTex(snowTex, S, S, px);
	}
	{
		const int W = 8, H = 64;
		std::vector<unsigned char> px(W * H * 4);
		const float PI = 3.14159265f;
		for (int y = 0; y < H; y++)
			for (int x = 0; x < W; x++)
			{
				float u = (x + 0.5f) / W;   // 0..1 across width
				float v = (y + 0.5f) / H;   // 0..1 along length
				float core = 1.0f - std::fabs(u - 0.5f) * 2.0f;   // bright center column
				core = std::pow(core < 0.0f ? 0.0f : core, 1.5f);
				float taper = std::sin(v * PI);                    // fade the two ends
				float a = core * (0.35f + 0.65f * taper);
				int i = (y * W + x) * 4;
				px[i] = px[i + 1] = px[i + 2] = 255;
				px[i + 3] = (unsigned char)((a < 0.0f ? 0.0f : (a > 1.0f ? 1.0f : a)) * 255.0f);
			}
		uploadTex(rainTex, W, H, px);
	}
}

void Scene3D::UpdateWeather(const glm::vec3& camPos, float dtSec)
{
	// The volume is a box centered on the camera; particles fall and wrap so the
	// density stays constant around the player regardless of where they move.
	weatherCentre = camPos;
	const float scale = (weatherScale > 0.001f) ? weatherScale : 1.0f;
	const float R = 1600.0f * scale;     // half extent in X/Z
	const float topH = 1800.0f * scale;  // how far ABOVE the camera (up = -Y)
	const float botM = 600.0f * scale;   // how far below before a particle recycles

	auto frand = []() { return (float)std::rand() / (float)RAND_MAX; };
	auto rr = [&](float a, float b) { return a + (b - a) * frand(); };

	const bool snow = (weatherType == WeatherType::Snow);
	int want = (int)(kMaxWeatherParticles * (snow ? 0.45f : 1.0f) * weatherIntensity);
	if (want < 0) want = 0;
	if (want > kMaxWeatherParticles) want = kMaxWeatherParticles;

	if (!weatherInit || (int)weatherParticles.size() != want)
	{
		weatherParticles.resize(want);
		for (int i = 0; i < want; i++)
			weatherParticles[i] = glm::vec4(camPos.x + rr(-R, R),
			                                camPos.y + rr(-topH, botM),   // fill the column
			                                camPos.z + rr(-R, R), frand());
		weatherInit = true;
	}

	// Falling and drifting scale with the volume, so weather looks the same
	// however big the world's units are - it just covers less ground.
	const float fall  = (snow ? 260.0f : 2600.0f) * scale;
	const float windX = (snow ? 55.0f  : 190.0f) * scale;
	const float windZ = (snow ? 35.0f  : 70.0f) * scale;

	for (glm::vec4& p : weatherParticles)
	{
		p.y += fall * dtSec;
		p.x += windX * dtSec;
		p.z += windZ * dtSec;

		if (p.y > camPos.y + botM)   // fell below -> respawn at the top with fresh X/Z
		{
			p.y = camPos.y - topH;
			p.x = camPos.x + rr(-R, R);
			p.z = camPos.z + rr(-R, R);
			p.w = frand();
		}
		// Keep the volume centered on the (possibly moving) camera by wrapping X/Z.
		if (p.x < camPos.x - R) p.x += 2.0f * R; else if (p.x > camPos.x + R) p.x -= 2.0f * R;
		if (p.z < camPos.z - R) p.z += 2.0f * R; else if (p.z > camPos.z + R) p.z -= 2.0f * R;
	}
}

void Scene3D::RenderWeather(Game& game, const Renderer& renderer)
{
	if (!active || weatherType == WeatherType::None || renderer.camera.useOrthoCamera)
		return;
	if (weatherParticles.empty())
		return;

	EnsureWeatherResources();
	if (weatherShader == nullptr || weatherVAO == 0)
		return;

	// This frame's particle positions (instance attribute 3): streamed through
	// the transient ring, or into weatherInstVBO if it is unavailable. A
	// split-screen view draws them around its own camera.
	const glm::vec4* particles = weatherParticles.data();
	static std::vector<glm::vec4> moved;
	if (RenderingViews())
	{
		const glm::vec4 d(renderer.camera.position - weatherCentre, 0.0f);
		moved.resize(weatherParticles.size());
		for (size_t i = 0; i < weatherParticles.size(); i++)
			moved[i] = weatherParticles[i] + d;
		particles = moved.data();
	}
	StreamInstanceAttrib(weatherVAO, weatherInstVBO, 3, 4, particles, weatherParticles.size() * sizeof(glm::vec4));

	weatherShader->UseShader();
	renderer.BindWorldCameraBlock();
	unsigned int id = weatherShader->GetID();
	// A storm renders the same fast rain streaks as plain rain (the lightning /
	// thunder ride on top); only snow takes the flake path.
	const bool rain = (weatherType != WeatherType::Snow);

	Device().SetUniform((int)(ShaderProgram::DrawUniformLocation(id, "uMode")), (int)(rain ? 1 : 0));
	Device().SetUniform((int)(ShaderProgram::DrawUniformLocation(id, "uTime")), (float)(renderer.now * 0.001f));
	glm::vec3 cp = renderer.camera.position;
	Device().SetUniform((int)(ShaderProgram::DrawUniformLocation(id, "uCamPos")), glm::vec3(cp.x, cp.y, cp.z));

	// Fall direction (down = +Y) with a little wind lean; used to orient rain streaks.
	glm::vec3 fdir = rain ? glm::normalize(glm::vec3(0.07f, 1.0f, 0.025f)) : glm::vec3(0, 1, 0);
	Device().SetUniform((int)(ShaderProgram::DrawUniformLocation(id, "uFallDir")), glm::vec3(fdir.x, fdir.y, fdir.z));
	SetParticleDistanceFog(id);

	const float pscale = (weatherScale > 0.001f) ? weatherScale : 1.0f;

	if (rain)
	{
		Device().SetUniform((int)(ShaderProgram::DrawUniformLocation(id, "uSize")), (float)(3.0f * pscale));      // streak width
		Device().SetUniform((int)(ShaderProgram::DrawUniformLocation(id, "uLength")), (float)(95.0f * pscale));   // streak length
		Device().SetUniform((int)(ShaderProgram::DrawUniformLocation(id, "uSway")), (float)(0.0f));
		Device().SetUniform((int)(ShaderProgram::DrawUniformLocation(id, "uColor")), glm::vec4(0.62f, 0.70f, 0.82f, 0.5f));
		Device().BindTexture(0, TextureHandle(rainTex));
	}
	else
	{
		Device().SetUniform((int)(ShaderProgram::DrawUniformLocation(id, "uSize")), (float)(13.0f * pscale));   // flake size
		Device().SetUniform((int)(ShaderProgram::DrawUniformLocation(id, "uLength")), (float)(13.0f * pscale));
		Device().SetUniform((int)(ShaderProgram::DrawUniformLocation(id, "uSway")), (float)(55.0f * pscale));   // sideways drift
		Device().SetUniform((int)(ShaderProgram::DrawUniformLocation(id, "uColor")), glm::vec4(1.0f, 1.0f, 1.0f, 0.85f));
		Device().BindTexture(0, TextureHandle(snowTex));
	}
	Device().SetUniform((int)(Device().UniformLocation(ProgramHandle(id), "theTexture")), (int)(0));

	{
		ScopedRenderState scope(ParticleState());
		Device().Draw(VertexArrayHandle(weatherVAO), Primitive::Triangles, 0, 6, (int)weatherParticles.size());
	}
	renderer.drawCallsPerFrame++;

	// Lightning flash goes on last, over the rain and everything else.
	if (weatherType == WeatherType::Storm && flashIntensity > 0.001f)
		RenderLightningFlash(renderer);
}

// ----------------------------------------------------- fountain particle jet

void Scene3D::SetFountain(const glm::vec3& pos, float jetSpeed, float fallDist, float spread)
{
	fountainPos = pos;
	fountainJetSpeed = jetSpeed;
	fountainFallDist = fallDist;
	fountainSpread = spread;
	hasFountain = true;
	fountainInit = false;   // reseed the droplets
}

void Scene3D::FountainRespawn(int i, bool stagger)
{
	auto frand = []() { return (float)std::rand() / (float)RAND_MAX; };
	auto rr = [&](float a, float b) { return a + (b - a) * frand(); };

	// Launch from a small nozzle disk, upward (-Y) with a horizontal spread so the
	// jets fan into a dome that falls back into the basin.
	const float nozzleR = 6.0f;
	float ang = rr(0.0f, 6.2831853f);
	glm::vec3 pos = fountainPos
		+ glm::vec3(std::cos(ang) * rr(0.0f, nozzleR), 0.0f, std::sin(ang) * rr(0.0f, nozzleR));
	glm::vec3 vel(rr(-fountainSpread, fountainSpread),
	              -fountainJetSpeed * rr(0.80f, 1.05f),
	              rr(-fountainSpread, fountainSpread));

	if (stagger)
	{
		// Advance to a random point along the ballistic arc so the whole jet is
		// full on the very first frame (no all-at-the-nozzle pop-in).
		float tmax = (2.0f * fountainJetSpeed) / fountainGravity;   // ~up+down flight time
		float t = rr(0.0f, tmax);
		pos += vel * t;
		pos.y += 0.5f * fountainGravity * t * t;
		vel.y += fountainGravity * t;
	}

	fountainParticles[i] = glm::vec4(pos, frand());
	fountainVel[i] = vel;
}

void Scene3D::UpdateFountain(float dtSec)
{
	if (!hasFountain)
		return;
	if (fountainCount < 1) fountainCount = 1;

	if (!fountainInit || (int)fountainParticles.size() != fountainCount)
	{
		fountainParticles.resize(fountainCount);
		fountainVel.resize(fountainCount);
		for (int i = 0; i < fountainCount; i++)
			FountainRespawn(i, true);
		fountainInit = true;
	}

	const float respawnY = fountainPos.y + fountainFallDist;   // basin/pool level
	for (int i = 0; i < fountainCount; i++)
	{
		fountainVel[i].y += fountainGravity * dtSec;            // gravity (down = +Y)
		glm::vec3 p(fountainParticles[i]);
		p += fountainVel[i] * dtSec;
		if (p.y > respawnY)                                    // fell back down -> relaunch
			FountainRespawn(i, false);
		else
			fountainParticles[i] = glm::vec4(p, fountainParticles[i].w);
	}
}

void Scene3D::EnsureFountainResources()
{
	if (fountainVAO != 0)
		return;
	EnsureWeatherResources();   // builds the shared unit quad (weatherQuadVBO) + snow/rain textures
	if (weatherQuadVBO == 0)
		return;

	const int kMaxFountain = 1024;
	const VertexArrayHandle vao = Device().CreateVertexArray();
	fountainVAO = vao.id;

	// Shared unit quad (corner.xy @0, uv @1) from the weather resources.
	PointAtParticleQuad(vao, weatherQuadVBO);

	// Per-instance position (vec4 = xyz + seed) @3.
	fountainInstVBO = CreateInstanceAttrib(vao, 3, 4, kMaxFountain * sizeof(glm::vec4));

	// Per-instance velocity (vec3) @4 - streaks orient along it (uMode 2).
	fountainVelVBO = CreateInstanceAttrib(vao, 4, 3, kMaxFountain * sizeof(glm::vec3));
}

void Scene3D::RenderFountain(Game& game, const Renderer& renderer)
{
	if (!active || !hasFountain || renderer.camera.useOrthoCamera)
		return;
	if (fountainParticles.empty())
		return;

	EnsureFountainResources();   // own VAO (has per-particle velocity); reuses weather shader/textures
	if (weatherShader == nullptr || fountainVAO == 0)
		return;

	size_t cnt = fountainParticles.size() < fountainVel.size()
		? fountainParticles.size() : fountainVel.size();
	if (cnt > 1024) cnt = 1024;
	const int n = (int)cnt;

	// Positions (attribute 3) and velocities (4), streamed like the weather's.
	StreamInstanceAttrib(fountainVAO, fountainInstVBO, 3, 4, fountainParticles.data(), n * sizeof(glm::vec4));
	StreamInstanceAttrib(fountainVAO, fountainVelVBO, 4, 3, fountainVel.data(), n * sizeof(glm::vec3));

	weatherShader->UseShader();
	renderer.BindWorldCameraBlock();
	unsigned int id = weatherShader->GetID();
	const bool streak = (fountainStretch > 0.01f);
	Device().SetUniform((int)(ShaderProgram::DrawUniformLocation(id, "uMode")), (int)(streak ? 2 : 0));   // 2 = per-velocity streak, 0 = dot
	Device().SetUniform((int)(ShaderProgram::DrawUniformLocation(id, "uTime")), (float)(renderer.now * 0.001f));
	glm::vec3 cp = renderer.camera.position;
	Device().SetUniform((int)(ShaderProgram::DrawUniformLocation(id, "uCamPos")), glm::vec3(cp.x, cp.y, cp.z));
	Device().SetUniform((int)(ShaderProgram::DrawUniformLocation(id, "uFallDir")), glm::vec3(0.0f, 1.0f, 0.0f));
	SetParticleDistanceFog(id);
	Device().SetUniform((int)(ShaderProgram::DrawUniformLocation(id, "uSize")), (float)(streak ? fountainDropSize * 0.6f : fountainDropSize));
	Device().SetUniform((int)(ShaderProgram::DrawUniformLocation(id, "uLength")), (float)(fountainDropSize * (streak ? fountainStretch : 1.0f)));
	Device().SetUniform((int)(ShaderProgram::DrawUniformLocation(id, "uSway")), (float)(0.0f));
	Device().SetUniform((int)(ShaderProgram::DrawUniformLocation(id, "uColor")), glm::vec4(0.78f, 0.88f, 1.0f, 0.9f));
	Device().BindTexture(0, TextureHandle(streak ? rainTex : snowTex));   // tapered streak vs round dot
	Device().SetUniform((int)(Device().UniformLocation(ProgramHandle(id), "theTexture")), (int)(0));

	{
		ScopedRenderState scope(ParticleState());
		Device().Draw(VertexArrayHandle(fountainVAO), Primitive::Triangles, 0, 6, n);
	}
	renderer.drawCallsPerFrame++;
}

// --------------------------------------------------- storm lightning

void Scene3D::UpdateLightning(Game& game, float dtSec)
{
	auto frand = []() { return (float)std::rand() / (float)RAND_MAX; };
	auto rr = [&](float a, float b) { return a + (b - a) * frand(); };

	// Fade any active flash quickly (a bright pop that dies in ~0.15s).
	if (flashIntensity > 0.0f)
	{
		flashIntensity -= dtSec / 0.15f;
		if (flashIntensity < 0.0f) flashIntensity = 0.0f;
	}

	// A secondary flicker a beat after the main strike (real lightning rarely
	// flashes just once).
	if (reflashTimer >= 0.0f)
	{
		reflashTimer -= dtSec;
		if (reflashTimer <= 0.0f)
		{
			flashIntensity = glm::max(flashIntensity, reflashMag);
			reflashTimer = -1.0f;
		}
	}

	// Thunder trails the flash by its travel time (sound is slow), so a distant
	// bolt cracks seconds later while a near one is almost instant.
	if (thunderTimer >= 0.0f)
	{
		thunderTimer -= dtSec;
		if (thunderTimer <= 0.0f)
		{
			thunderTimer = -1.0f;
			// PlaySound no-ops on channel < 0, so pass an explicit channel.
			if (!thunderSound.empty())
				game.soundManager.PlaySound(thunderSound, thunderChannel);
		}
	}

	// Arm the first strike a moment into the storm, then keep counting down.
	const float intens = glm::clamp(weatherIntensity, 0.05f, 1.0f);
	if (!stormSeeded)
	{
		lightningTimer = rr(0.8f, 2.5f);
		stormSeeded = true;
	}

	lightningTimer -= dtSec;
	if (lightningTimer > 0.0f)
		return;

	// --- strike! ---
	// Brighter, more frequent flashes at higher intensity.
	flashIntensity = rr(0.75f, 1.0f);
	// Maybe a quick second flicker.
	if (frand() < 0.6f)
	{
		reflashTimer = rr(0.06f, 0.16f);
		reflashMag = rr(0.4f, 0.75f);
	}
	// Thunder delay = "distance": near strikes (short delay) are the loud ones.
	thunderTimer = rr(0.25f, 2.75f);

	// Schedule the next strike; denser as intensity rises.
	float gap = rr(lightningMinGap, lightningMaxGap) * glm::mix(1.6f, 0.6f, intens);
	if (gap < 1.0f) gap = 1.0f;
	lightningTimer = gap;
}

void Scene3D::RenderLightningFlash(const Renderer& renderer)
{
	if (flashShader == nullptr)
	{
		flashShader = new ShaderProgram(-1, flashShaderVert.c_str(), flashShaderFrag.c_str());
	}
	if (flashVAO == 0)
		flashVAO = Device().CreateVertexArray().id;   // attribute-less full-screen triangle
	if (flashShader == nullptr || flashVAO == 0)
		return;

	flashShader->UseShader();
	unsigned int id = flashShader->GetID();
	// A cool-white flash; alpha carries the intensity. Gamma the curve a touch so
	// the pop reads punchy rather than a flat wash.
	float a = flashIntensity;
	a = a * a * (3.0f - 2.0f * a);   // smoothstep
	Device().SetUniform((int)(ShaderProgram::DrawUniformLocation(id, "uFlashColor")), glm::vec3(0.80f, 0.85f, 1.0f));
	// Lighter than before: the scene surfaces now brighten on their own (see
	// ApplyLighting's lightningFlash), so this overlay just lifts the sky/haze.
	Device().SetUniform((int)(ShaderProgram::DrawUniformLocation(id, "uFlashIntensity")), (float)(a * 0.35f));

	RenderState flash = CurrentRenderState();
	flash.depthTest = false;
	// ...and do not WRITE depth either. A full-screen triangle with the depth
	// test off but the write mask on stamps its own depth over the entire
	// buffer, which costs nothing here (this is the last thing drawn) and
	// wrecks anything that reads depth afterwards - including, in a game that
	// draws its own world and calls this itself, the next frame's sky.
	flash.depthWrite = false;
	flash.blend = BlendMode::Additive;   // brighten whatever is on screen
	{
		ScopedRenderState scope(flash);
		Device().Draw(VertexArrayHandle(flashVAO), Primitive::Triangles, 0, 3);
	}
	renderer.drawCallsPerFrame++;
}
