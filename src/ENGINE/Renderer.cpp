#include "leak_check.h"
#include "Renderer.h"
#include "Sprite.h"
#include "Game.h"
#include <algorithm>

ShaderProgram* Renderer::textShader;
ShaderProgram* Renderer::tileShader;

ShaderProgram* Renderer::GetTextShader()
{
	return textShader;
}

ShaderProgram* Renderer::GetShader(int key) const
{
	if (shaders.count(key) != 0)
	{
		return shaders[key];
	}

	game->logger.Log("ERROR: Shader not in renderer list: " + std::to_string(key));

	return shaders[1];
}

Renderer::Renderer()
{
	layersVisible[DrawingLayer::BACK] = true;
	layersVisible[DrawingLayer::MIDDLE] = true;
	layersVisible[DrawingLayer::OBJECT] = true;
	layersVisible[DrawingLayer::COLLISION] = true;
	layersVisible[DrawingLayer::COLLISION2] = true;
	layersVisible[DrawingLayer::FRONT] = true;
	layersVisible[DrawingLayer::INVISIBLE] = false;

	timerOverlayColor.Start(1);
}

void Renderer::Init(Game* g)
{
	game = g;
}

void Renderer::CreateShaders()
{
	std::vector<std::string> shaderList = ReadStringsFromFile("data/shaders.dat");
	std::string vertexFile = "";
	std::string fragmentFile = "";
	std::string shaderFolder = "data/shaders/";

#ifdef EMSCRIPTEN
	shaderFolder += "webgl/";
#endif

	for (int i = 0; i < shaderList.size(); i++)
	{
		int index = 0;
		ParseWord(shaderList[i], ' ', index);
		vertexFile = shaderFolder + ParseWord(shaderList[i], ' ', index);
		fragmentFile = shaderFolder + ParseWord(shaderList[i], ' ', index);
		CreateShader(i + 1, vertexFile.c_str(), fragmentFile.c_str());
	}

	// shaders[0] = custom shader
	tileShader = shaders[1]; // default
	textShader = shaders[2]; // gui
}

Renderer::~Renderer()
{
	for (auto& [key, val] : shaders)
	{
		if (val != nullptr)
			delete_it(val);
	}

	if (debugSprite != nullptr)
		delete_it(debugSprite);

	if (overlaySprite != nullptr)
		delete_it(overlaySprite);
}

void Renderer::RenderDebugRect(const SDL_Rect& targetRect, const glm::vec2& targetScale, Color color) const
{
	RenderDebugRect(targetRect, targetScale, glm::vec2(0, 0), color);
}

void Renderer::RenderDebugRect(const SDL_Rect& targetRect, const glm::vec2& targetScale, const glm::vec2& targetPivot, Color color) const
{
	debugSprite->color = color;
	debugSprite->pivot = targetPivot;
	debugScale = glm::vec2(CalculateScale(*debugSprite, targetRect.w, targetRect.h, targetScale));
	debugSprite->Render(glm::vec3(targetRect.x, targetRect.y, 0), *this, debugScale);
}

glm::vec2 Renderer::CalculateScale(const Sprite& sourceSprite, int targetWidth, int targetHeight, const glm::vec2& targetScale) const
{
	if (sourceSprite.texture == nullptr)
		return glm::vec2(targetWidth * targetScale.x, targetHeight * targetScale.y);

	float sourceWidth = sourceSprite.texture->GetWidth();
	float sourceHeight = sourceSprite.texture->GetHeight();

	return glm::vec2(targetWidth * targetScale.x / sourceWidth,
		targetHeight * targetScale.y / sourceHeight);
}

void Renderer::LerpColor(float& color, float target, const float& speed)
{
	if (color > target)
	{
		color -= speed * game->dt;
		if (color < target)
			color = target;
	}
	else
	{
		color += speed * game->dt;
		if (color > target)
			color = target;
	}
}

void Renderer::Update()
{
	if (changingOverlayColor) // && timerOverlayColor.HasElapsed())
	{		
		//changingOverlayColor = false;

		static float colorSpeed = -1.0f;
		static float r, g, b, a = 0.0f;

		if (colorSpeed < 0.0f)
		{
			// Calculate the longest amount of time it should take to change all colors
			int maxDiff = std::abs(overlayColor.r - targetColor.r);
			maxDiff = std::max(maxDiff, std::abs(overlayColor.g - targetColor.g));
			maxDiff = std::max(maxDiff, std::abs(overlayColor.b - targetColor.b));
			maxDiff = std::max(maxDiff, std::abs(overlayColor.a - targetColor.a));

			// Calculate the speed to change colors based on the desired time
			colorSpeed = maxDiff / (float)(overlayEndTime - overlayStartTime);

			std::cout << colorSpeed << std::endl;

			r = overlayColor.r;
			g = overlayColor.g;
			b = overlayColor.b;
			a = overlayColor.a;
		}

		//std::cout << currentTime << " / " << difference << " = " << t << std::endl;

		// Only update every millisecond
		LerpColor(r, targetColor.r, colorSpeed);
		LerpColor(g, targetColor.g, colorSpeed);
		LerpColor(b, targetColor.b, colorSpeed);
		LerpColor(a, targetColor.a, colorSpeed);

		// std::cout << a << " / " << (int)targetColor.a << std::endl;

		overlayColor = { (uint8_t)r, (uint8_t)g, (uint8_t)b, (uint8_t)a };

		if (overlayColor.r == targetColor.r
			&& overlayColor.g == targetColor.g
			&& overlayColor.b == targetColor.b
			&& overlayColor.a == targetColor.a)
		{
			changingOverlayColor = false;
			colorSpeed = -1.0f;
			std::cout << colorSpeed << std::endl;
		}
	}
}

void Renderer::FadeOverlay(const int screenWidth, const int screenHeight) const
{
	// Draw the screen overlay above everything else
	float rWidth = overlaySprite->texture->GetWidth();
	float rHeight = overlaySprite->texture->GetHeight();
	overlaySprite->color = overlayColor;
	overlaySprite->pivot = glm::vec2(0, 0);
	overlayScale = glm::vec2(screenWidth / rWidth, screenHeight / rHeight);
}

void Renderer::CreateShader(const int shaderName, const char* vertexFilePath, const char* fragmentFilePath)
{
	if (shaders[shaderName] != nullptr)
		delete_it(shaders[shaderName]);

	shaders[shaderName] = new ShaderProgram(shaderName, vertexFilePath, fragmentFilePath);
}

bool Renderer::IsVisible(DrawingLayer layer) const
{
	return layersVisible[layer];
}

void Renderer::ToggleVisibility(DrawingLayer layer)
{
	layersVisible[layer] = !layersVisible[layer];
}

void Renderer::UseLight(ShaderProgram& shader) const
{
	if (light != nullptr)
	{
		light->UseLight(shader);
	}

	// For point lights
	if (pointLights != nullptr)
	{
		if (pointLightCount > MAX_POINT_LIGHTS)
			pointLightCount = MAX_POINT_LIGHTS;

		glUniform1i(shader.GetUniformVariable(ShaderVariable::pointLightCount), pointLightCount);

		for (size_t i = 0; i < pointLightCount; i++)
		{
			pointLights[i]->UseLight(shader);
		}
	}

	// For spot lights
	if (spotLights != nullptr)
	{
		if (pointLightCount > MAX_SPOT_LIGHTS)
			pointLightCount = MAX_SPOT_LIGHTS;

		glUniform1i(shader.GetUniformVariable(ShaderVariable::spotLightCount), spotLightCount);

		for (size_t i = 0; i < spotLightCount; i++)
		{
			spotLights[i]->UseLight(shader);
		}
	}
}