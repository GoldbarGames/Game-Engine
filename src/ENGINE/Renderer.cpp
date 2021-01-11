#include "leak_check.h"
#include "Renderer.h"
#include "Sprite.h"
#include "Game.h"
#include <algorithm>

ShaderProgram* Renderer::textShader;

ShaderProgram* Renderer::GetTextShader()
{
	return textShader;
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
	CreateShader(ShaderName::Default, "data/shaders/default.vert", "data/shaders/default.frag");
	//CreateShader("special", "data/shaders/special.vert", "data/shaders/special.frag");
	//CreateShader(ShaderName::Multiply, "data/shaders/default.vert", "data/shaders/multiply.frag");
	CreateShader(ShaderName::Add, "data/shaders/default.vert", "data/shaders/add.frag");
	//CreateShader("hue-shift", "data/shaders/hue-shift.vert", "data/shaders/hue-shift.frag");
	CreateShader(ShaderName::FadeInOut, "data/shaders/default.vert", "data/shaders/fade-in-out.frag");
	CreateShader(ShaderName::Glow, "data/shaders/default.vert", "data/shaders/glow.frag");
	CreateShader(ShaderName::GUI, "data/shaders/gui.vert", "data/shaders/gui.frag");
	CreateShader(ShaderName::NoAlpha, "data/shaders/default.vert", "data/shaders/noalpha.frag");
	CreateShader(ShaderName::SolidColor, "data/shaders/default.vert", "data/shaders/solidcolor.frag");
	//CreateShader(ShaderName::Grid, "data/shaders/default.vert", "data/shaders/grid.frag");
	CreateShader(ShaderName::Grayscale, "data/shaders/default.vert", "data/shaders/grayscale.frag");
	CreateShader(ShaderName::Sharpen, "data/shaders/default.vert", "data/shaders/sharpen.frag");
	CreateShader(ShaderName::Blur, "data/shaders/default.vert", "data/shaders/blur.frag");
	CreateShader(ShaderName::Edge, "data/shaders/default.vert", "data/shaders/edge.frag");
	CreateShader(ShaderName::Test, "data/shaders/default.vert", "data/shaders/test.frag");
	CreateShader(ShaderName::Diffuse, "data/shaders/default.vert", "data/shaders/diffuse.frag");

	textShader = shaders[ShaderName::GUI];
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

void Renderer::RenderDebugRect(const SDL_Rect& targetRect, const Vector2& targetScale, Color color) const
{
	RenderDebugRect(targetRect, targetScale, Vector2(0, 0), color);
}

void Renderer::RenderDebugRect(const SDL_Rect& targetRect, const Vector2& targetScale, const Vector2& targetPivot, Color color) const
{
	debugSprite->color = color;
	debugSprite->pivot = targetPivot;
	debugScale = Vector2(CalculateScale(*debugSprite, targetRect.w, targetRect.h, targetScale));
	debugSprite->Render(glm::vec3(targetRect.x, targetRect.y, 0), *this, debugScale);
}

Vector2 Renderer::CalculateScale(const Sprite& sourceSprite, int targetWidth, int targetHeight, const Vector2& targetScale) const
{
	if (sourceSprite.texture == nullptr)
		return Vector2(targetWidth * targetScale.x, targetHeight * targetScale.y);

	float sourceWidth = sourceSprite.texture->GetWidth();
	float sourceHeight = sourceSprite.texture->GetHeight();

	return Vector2(targetWidth * targetScale.x / sourceWidth,
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
	overlaySprite->pivot = Vector2(0, 0);
	overlayScale = Vector2(screenWidth / rWidth, screenHeight / rHeight);
}

void Renderer::CreateShader(const ShaderName shaderName, const char* vertexFilePath, const char* fragmentFilePath)
{
	if (shaders[shaderName] != nullptr)
		delete_it(shaders[shaderName]);

	shaders[shaderName] = neww ShaderProgram(shaderName, vertexFilePath, fragmentFilePath);
}

bool Renderer::IsVisible(DrawingLayer layer) const
{
	return layersVisible[layer];
}

void Renderer::ToggleVisibility(DrawingLayer layer)
{
	layersVisible[layer] = !layersVisible[layer];
}