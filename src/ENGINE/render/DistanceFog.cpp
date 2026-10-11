// Distance fog - see DistanceFog.h. Settings only: the shaders read the values
// from the Scene block (Scene3D::ApplyLighting), the outline's from the
// Outline block (Game.cpp) and the particles' from their per-draw data.

#include "DistanceFog.h"
#include "ColorPipeline.h"
#include "../globals.h"
#include <glm/glm.hpp>
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <sstream>

namespace
{
	DistanceFogSettings project;     // renderer.dat distanceFog*
	bool sceneSet = false;           // the scene's own (.scene `distfog`, script, game)
	DistanceFogSettings scene;
	bool forced = false;             // KINJO_DISTFOG
	DistanceFogSettings forcedFog;

	// A fade (scene3d distfog ... <seconds>), from what showed when it began.
	DistanceFogFrame fadeFrom;
	float fadeSeconds = 0.0f;
	std::chrono::steady_clock::time_point fadeStart;

	// No negative distances, and far no nearer than near (equal = a hard edge).
	DistanceFogSettings Sanitized(DistanceFogSettings fog)
	{
		fog.color = glm::clamp(fog.color, glm::vec3(0.0f), glm::vec3(1.0f));
		fog.nearDistance = std::max(fog.nearDistance, 0.0f);
		fog.farDistance = std::max(fog.farDistance, fog.nearDistance);
		return fog;
	}

	DistanceFogSettings Target()
	{
		if (forced)
			return forcedFog;
		return sceneSet ? scene : project;
	}

	DistanceFogFrame FrameOf(const DistanceFogSettings& fog)
	{
		DistanceFogFrame f;
		f.color = fog.color;
		f.nearDistance = fog.nearDistance;
		f.farDistance = fog.farDistance;
		f.amount = fog.on ? 1.0f : 0.0f;
		return f;
	}

	DistanceFogFrame Current()
	{
		DistanceFogFrame to = FrameOf(Target());
		if (forced || fadeSeconds <= 0.0f)
			return to;
		const float t = std::chrono::duration<float>(std::chrono::steady_clock::now() - fadeStart).count() / fadeSeconds;
		if (t >= 1.0f)
		{
			fadeSeconds = 0.0f;
			return to;
		}
		// Fading in from none, or out to none, changes only the amount: the
		// colour and distances are the ones that show.
		DistanceFogFrame from = fadeFrom;
		if (from.amount <= 0.0f)
		{
			from.color = to.color;
			from.nearDistance = to.nearDistance;
			from.farDistance = to.farDistance;
		}
		else if (to.amount <= 0.0f)
		{
			to.color = from.color;
			to.nearDistance = from.nearDistance;
			to.farDistance = from.farDistance;
		}
		DistanceFogFrame f;
		f.color = glm::mix(from.color, to.color, t);
		f.nearDistance = glm::mix(from.nearDistance, to.nearDistance, t);
		f.farDistance = glm::mix(from.farDistance, to.farDistance, t);
		f.amount = glm::mix(from.amount, to.amount, t);
		return f;
	}

	bool ParseFloat(const std::string& s, float& out)
	{
		try { out = std::stof(s); return true; }
		catch (...) { return false; }
	}
}

void LoadDistanceFogSettings()
{
	auto config = ReadRendererConfig();
	project = DistanceFogSettings();
	project.on = config.count("distanceFog") > 0 && config["distanceFog"] == "1";
	float v = 0.0f;
	if (config.count("distanceFogNear") > 0 && ParseFloat(config["distanceFogNear"], v)) project.nearDistance = v;
	if (config.count("distanceFogFar") > 0 && ParseFloat(config["distanceFogFar"], v)) project.farDistance = v;
	if (config.count("distanceFogColor") > 0)
	{
		std::istringstream ss(config["distanceFogColor"]);
		glm::vec3 c;
		if (ss >> c.r >> c.g >> c.b)
			project.color = c;
	}
	project = Sanitized(project);

	forced = false;
	if (const char* e = std::getenv("KINJO_DISTFOG"))
	{
		DistanceFogSettings f;
		const int n = std::sscanf(e, "%f %f %f %f %f", &f.color.r, &f.color.g, &f.color.b, &f.nearDistance, &f.farDistance);
		if (n == 5)
		{
			f.on = true;
			forced = true;
			forcedFog = Sanitized(f);
			std::cout << "Distance fog: KINJO_DISTFOG colour " << forcedFog.color.r << " " << forcedFog.color.g << " "
				<< forcedFog.color.b << ", " << forcedFog.nearDistance << " to " << forcedFog.farDistance
				<< " (overrides scenes)" << std::endl;
		}
		else if (n == 1 && f.color.r == 0.0f)
		{
			forced = true;
			forcedFog = DistanceFogSettings();   // on = false
			std::cout << "Distance fog: KINJO_DISTFOG=0, off everywhere" << std::endl;
		}
		else
			std::cout << "Distance fog: KINJO_DISTFOG wants \"r g b near far\" or 0; ignored" << std::endl;
	}
}

void SetSceneDistanceFog(bool set, const DistanceFogSettings& fog, float seconds)
{
	fadeFrom = Current();
	fadeSeconds = std::max(seconds, 0.0f);
	fadeStart = std::chrono::steady_clock::now();
	sceneSet = set;
	scene = set ? Sanitized(fog) : DistanceFogSettings();
}

bool GetSceneDistanceFog(DistanceFogSettings& fog)
{
	fog = scene;
	return sceneSet;
}

DistanceFogSettings DistanceFogInForce()
{
	return Target();
}

DistanceFogFrame CurrentDistanceFog()
{
	return Current();
}
