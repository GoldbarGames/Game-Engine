// Renderer settings from game code (since 2026-10-09; Golf Galaxy's E13): a
// game's own values over renderer.dat's - a player's graphics level - which
// every module reads through ReadRendererConfig, and the live reload the 3D
// editor's PROJECT tab uses too.

#include "../Renderer.h"
#include "../globals.h"
#include "../Scene3DInternal.h"
#include "ColorPipeline.h"
#include "AmbientOcclusion.h"
#include "ClusteredLights.h"
#include "ColorGrading.h"
#include "DepthOfField.h"
#include "Environment.h"
#include "VolumetricFog.h"
#include "DistanceFog.h"
#include "Multisample.h"
#include "Reflections.h"
#include "TemporalAA.h"
#include "TextureFiles.h"

#include <iostream>
#include <map>

namespace
{
	// Kept here, not in Renderer, so its layout doesn't change
	std::map<std::string, std::string> overrides;
}

std::unordered_map<std::string, std::string> ReadRendererConfig()
{
	auto config = GetMapStringsFromFile(RendererConfigPath());
	for (const auto& [key, value] : overrides)
		config[key] = value;
	return config;
}

const std::map<std::string, std::string>& RendererSettingOverrides()
{
	return overrides;
}

void ReloadRenderSettingsLive()
{
	ReloadColorSettings();
	LoadEnvironmentSettings();
	LoadAmbientOcclusionSettings();
	LoadTemporalAASettings();
	LoadClusteredLightSettings();
	LoadColorGradeSettings();
	LoadDepthOfFieldSettings();
	LoadFogSettings();
	LoadDistanceFogSettings();
	LoadMultisampleSettings();
	LoadReflectionSettings();
	LoadTextureSettings();
	Scene3DInternal::ReloadShadowSettings();
}

bool Renderer::SetRenderSetting(const std::string& key, const std::string& value)
{
	if (key.empty())
		return false;
	if (key == "linearLighting" || key == "gpuDriven")
	{
		std::cout << "Renderer: " << key << " needs a restart; it can only be set in renderer.dat" << std::endl;
		return false;
	}
	overrides[key] = value;
	return true;
}

void Renderer::ClearRenderSettings()
{
	overrides.clear();
}

void Renderer::ApplyRenderSettings()
{
	ReloadRenderSettingsLive();
}
