#include "Renderer.h"
#include "Sprite.h"
#include "Game.h"
#include <algorithm>

ShaderProgram* Renderer::textShader;

ShaderProgram* Renderer::GetTextShader()
{
	return textShader;
}

Renderer::Renderer(Game* g)
{
	game = g;

	layersVisible[DrawingLayer::BACK] = true;
	layersVisible[DrawingLayer::MIDDLE] = true;
	layersVisible[DrawingLayer::OBJECT] = true;
	layersVisible[DrawingLayer::COLLISION] = true;
	layersVisible[DrawingLayer::COLLISION2] = true;
	layersVisible[DrawingLayer::FRONT] = true;

	timerOverlayColor.Start(1);
}

void Renderer::CreateShaders()
{
	CreateShader(ShaderName::Default, "data/shaders/default.vert", "data/shaders/default.frag");
	//CreateShader("special", "data/shaders/special.vert", "data/shaders/special.frag");
	CreateShader(ShaderName::Multiply, "data/shaders/default.vert", "data/shaders/multiply.frag");
	CreateShader(ShaderName::Add, "data/shaders/default.vert", "data/shaders/add.frag");
	//CreateShader("hue-shift", "data/shaders/hue-shift.vert", "data/shaders/hue-shift.frag");
	CreateShader(ShaderName::FadeInOut, "data/shaders/default.vert", "data/shaders/fade-in-out.frag");
	CreateShader(ShaderName::Glow, "data/shaders/default.vert", "data/shaders/glow.frag");
	CreateShader(ShaderName::GUI, "data/shaders/gui.vert", "data/shaders/gui.frag");
	CreateShader(ShaderName::NoAlpha, "data/shaders/default.vert", "data/shaders/noalpha.frag");
	CreateShader(ShaderName::SolidColor, "data/shaders/default.vert", "data/shaders/solidcolor.frag");
	CreateShader(ShaderName::Grid, "data/shaders/default.vert", "data/shaders/grid.frag");

	textShader = shaders[ShaderName::GUI];
}

Renderer::~Renderer()
{

}

void Renderer::RenderDebugRect(const SDL_Rect& targetRect, const Vector2& targetScale) const
{
	debugSprite->SetScale(CalculateScale(*debugSprite, targetRect.w, targetRect.h, targetScale));
	debugSprite->Render(Vector2(targetRect.x, targetRect.y), *this);
}

// This function takes two sprites, the source and the target
// It returns a vector2 representing the amount that the source 
// should be scaled by in order to equal the scaled size of the target
Vector2 Renderer::CalculateScale(const Sprite& sourceSprite, const Sprite& targetSprite) const
{
	float sourceWidth = sourceSprite.texture->GetWidth();
	float sourceHeight = sourceSprite.texture->GetHeight();

	float targetWidth = targetSprite.frameWidth;
	float targetHeight = targetSprite.frameHeight;

	return Vector2(targetWidth * targetSprite.scale.x / sourceWidth, 
		targetHeight * targetSprite.scale.y / sourceHeight);
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

void Renderer::Update()
{
	if (changingOverlayColor) // && timerOverlayColor.HasElapsed())
	{		
		changingOverlayColor = false;

		float difference = 0;
		float currentTime = game->timer.GetTicks() - overlayStartTime;
		float t = 1.0f;
		if (overlayEndTime != 0)
		{
			difference = overlayEndTime - overlayStartTime;
			t = currentTime / difference;
		}

		//std::cout << currentTime << " / " << difference << " = " << t << std::endl;

		UpdateOverlayColor(overlayColor.r, startColor.r, targetColor.r, t);
		UpdateOverlayColor(overlayColor.g, startColor.g, targetColor.g, t);
		UpdateOverlayColor(overlayColor.b, startColor.b, targetColor.b, t);
		UpdateOverlayColor(overlayColor.a, startColor.a, targetColor.a, t);
		//std::cout << overlayColor.a << std::endl;
		//std::cout << timerOverlayColor.GetTicks() << std::endl;
		//timerOverlayColor.Start(1);
	}
}

void Renderer::UpdateOverlayColor(uint8_t& color, const int& start, const int& target, const float& t)
{
	if (color != target)
	{
		changingOverlayColor = true;
		color = start + (t * (target - start));

		if ((color - target) * (color - target) < 3)
			color = target;
	}
}

void Renderer::FadeOverlay(const int screenWidth, const int screenHeight) const
{
	// Draw the screen overlay above everything else
	float rWidth = overlaySprite->texture->GetWidth();
	float rHeight = overlaySprite->texture->GetHeight();
	overlaySprite->color = overlayColor;
	overlaySprite->pivot = Vector2(0, 0);
	overlaySprite->SetScale(Vector2(screenWidth / rWidth, screenHeight / rHeight));
}

void Renderer::CreateShader(const ShaderName shaderName, const char* vertexFilePath, const char* fragmentFilePath)
{
	if (shaders[shaderName] != nullptr)
		delete shaders[shaderName];

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