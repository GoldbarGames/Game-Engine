// Colour grading - see ColorGrading.h. Backend-agnostic: GPU work goes
// through RenderDevice.

#include "ColorGrading.h"
#include "ColorPipeline.h"
#include "../globals.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <vector>

namespace
{
	struct Lut
	{
		TextureHandle texture;
		int size = 0;
		bool failed = false;
	};
	std::map<std::string, Lut> luts;   // by path; failures are remembered so they're reported once

	std::string projectPath;        // renderer.dat `colorGrade`
	float projectStrength = 1.0f;   // renderer.dat `colorGradeStrength`
	std::string forcedPath;         // KINJO_GRADE
	std::string scenePath;          // "" = the project's, "none" = off
	float sceneStrength = 1.0f;

	// A cross-fade in progress: the look that was on screen when it began.
	struct Look
	{
		const Lut* lut = nullptr;
		float strength = 0.0f;
	};
	Look fadeFrom;
	float fadeSeconds = 0.0f;
	bool fading = false;
	std::chrono::steady_clock::time_point fadeStart;

	const Lut* LoadLut(const std::string& path)
	{
		auto it = luts.find(path);
		if (it != luts.end())
			return it->second.failed ? nullptr : &it->second;
		Lut& lut = luts[path];
		lut.failed = true;

		SDL_Surface* loaded = IMG_Load(path.c_str());
		if (loaded == nullptr)
		{
			std::cout << "ERROR: colour grade LUT " << path << " could not be read: " << IMG_GetError() << std::endl;
			return nullptr;
		}
		SDL_Surface* rgba = SDL_ConvertSurfaceFormat(loaded, SDL_PIXELFORMAT_RGBA32, 0);
		SDL_FreeSurface(loaded);
		if (rgba == nullptr)
			return nullptr;
		const int n = rgba->h;
		if (n < 2 || rgba->w != n * n)
		{
			std::cout << "ERROR: colour grade LUT " << path << " is " << rgba->w << "x" << rgba->h
				<< "; a strip LUT is N*N wide and N tall (e.g. 1024x32)" << std::endl;
			SDL_FreeSurface(rgba);
			return nullptr;
		}
		// Tightly packed rows, top row first (row 0 = green 0).
		std::vector<unsigned char> pixels((size_t)rgba->w * rgba->h * 4);
		for (int y = 0; y < rgba->h; y++)
			std::memcpy(&pixels[(size_t)y * rgba->w * 4], (const unsigned char*)rgba->pixels + (size_t)y * rgba->pitch,
				(size_t)rgba->w * 4);
		TextureDesc desc;
		desc.format = TextureFormat::RGBA8;   // display colours as they are, not sRGB-decoded
		desc.width = rgba->w;
		desc.height = rgba->h;
		desc.filter = TextureFilter::Linear;  // bilinear in red and green; the shader blends blue slices
		desc.wrap = TextureWrap::ClampToEdge;
		lut.texture = Device().CreateTexture(desc, pixels.data());
		lut.size = n;
		lut.failed = false;
		SDL_FreeSurface(rgba);
		std::cout << "Colour grade: loaded " << path << " (" << n << "^3)" << std::endl;
		return &lut;
	}

	// The look the settings ask for right now.
	Look Wanted()
	{
		Look look;
		std::string path;
		float strength = projectStrength;
		if (!forcedPath.empty())
			path = forcedPath;
		else if (!scenePath.empty())
		{
			path = (scenePath == "none") ? std::string() : scenePath;
			strength = sceneStrength;
		}
		else
			path = projectPath;
		if (!path.empty() && strength > 0.0f)
		{
			look.lut = LoadLut(path);
			look.strength = (look.lut != nullptr) ? std::min(strength, 1.0f) : 0.0f;
		}
		return look;
	}
}

void LoadColorGradeSettings()
{
	auto config = ReadRendererConfig();
	projectPath = (config.count("colorGrade") > 0 && config["colorGrade"] != "none") ? config["colorGrade"] : std::string();
	projectStrength = 1.0f;
	if (config.count("colorGradeStrength") > 0)
	{
		try { projectStrength = std::max(std::stof(config["colorGradeStrength"]), 0.0f); }
		catch (...) {}
	}
	if (const char* g = std::getenv("KINJO_GRADE"))
		forcedPath = g;
	if (LinearWorkflow() && (!projectPath.empty() || !forcedPath.empty()))
		std::cout << "Colour grade: " << (forcedPath.empty() ? projectPath : forcedPath + " (KINJO_GRADE)") << std::endl;
}

void SetSceneColorGrade(const std::string& path, float strength, float seconds)
{
	if (seconds > 0.0f)
	{
		fadeFrom = Wanted();
		fadeSeconds = seconds;
		fadeStart = std::chrono::steady_clock::now();
		fading = true;
	}
	else
	{
		fading = false;
	}
	scenePath = (path == "default") ? std::string() : path;
	sceneStrength = (strength < 0.0f) ? 1.0f : strength;
}

void GetProjectColorGrade(std::string& path, float& strength)
{
	path = projectPath;
	strength = projectStrength;
}

void GetSceneColorGrade(std::string& path, float& strength)
{
	path = scenePath;
	strength = sceneStrength;
}

bool CurrentColorGrade(ColorGradeState& out)
{
	out = ColorGradeState();
	if (!LinearWorkflow())
		return false;
	const Look now = Wanted();
	float blend = 1.0f;
	if (fading)
	{
		blend = std::chrono::duration<float>(std::chrono::steady_clock::now() - fadeStart).count() / fadeSeconds;
		if (blend >= 1.0f)
		{
			fading = false;
			blend = 1.0f;
		}
	}
	const Look from = fading ? fadeFrom : Look();
	if (now.lut == nullptr && from.lut == nullptr)
		return false;
	if (now.lut != nullptr)
	{
		out.lut = now.lut->texture;
		out.lutSize = (float)now.lut->size;
		out.strength = now.strength;
	}
	if (from.lut != nullptr)
	{
		out.previous = from.lut->texture;
		out.previousSize = (float)from.lut->size;
		out.previousStrength = from.strength;
	}
	out.blend = fading ? blend : 1.0f;
	return true;
}

void ReleaseColorGrading()
{
	for (auto& kv : luts)
		if (kv.second.texture)
			Device().DestroyTexture(kv.second.texture);
	luts.clear();
	fading = false;
	fadeFrom = Look();
}
