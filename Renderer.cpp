#include "Renderer.h"
#include "Sprite.h"

Renderer::Renderer()
{
	layersVisible["BACK"] = true;
	layersVisible["MIDDLE"] = true;
	layersVisible["OBJECT"] = true;
	layersVisible["COLLISION"] = true;
	layersVisible["FRONT"] = true;

	timerOverlayColor.Start(1);
}

Renderer::~Renderer()
{

}

// This function takes two sprites, the source and the target
// It returns a vector2 representing the amount that the source 
// should be scaled by in order to equal the scaled size of the target
Vector2 Renderer::CalculateScale(Sprite* sourceSprite, Sprite* targetSprite)
{
	float sourceWidth = sourceSprite->texture->GetWidth();
	float sourceHeight = sourceSprite->texture->GetHeight();

	float targetWidth = targetSprite->frameWidth;
	float targetHeight = targetSprite->frameHeight;

	return Vector2(targetWidth * targetSprite->scale.x / sourceWidth, 
		targetHeight * targetSprite->scale.y / sourceHeight);
}

void Renderer::Update()
{
	if (changingOverlayColor && timerOverlayColor.HasElapsed())
	{
		timerOverlayColor.Start(1);
		changingOverlayColor = false;
		UpdateOverlayColor(overlayColor.r, targetColor.r);
		UpdateOverlayColor(overlayColor.g, targetColor.g);
		UpdateOverlayColor(overlayColor.b, targetColor.b);
		UpdateOverlayColor(overlayColor.a, targetColor.a);
		//std::cout << overlayColor.a << std::endl;
		//std::cout << timerOverlayColor.GetTicks() << std::endl;
	}
}

void Renderer::UpdateOverlayColor(int& color, const int& target)
{
	if (color != target)
	{
		changingOverlayColor = true;
		if (target > color)
			color++;
		else
			color--;
	}
}

void Renderer::FadeOverlay(const int screenWidth, const int screenHeight)
{
	// Draw the screen overlay above everything else
	float rWidth = overlaySprite->texture->GetWidth();
	float rHeight = overlaySprite->texture->GetHeight();
	overlaySprite->color = overlayColor;
	overlaySprite->pivot = Vector2(0, 0);
	overlaySprite->SetScale(Vector2(screenWidth / rWidth, screenHeight / rHeight));
	overlaySprite->Render(Vector2(0, 0), this);
}

void Renderer::CreateShader(const char* shaderName, const char* vertexFilePath, const char* fragmentFilePath)
{
	if (shaders[shaderName] != nullptr)
		delete shaders[shaderName];

	shaders[shaderName] = new ShaderProgram(shaderName, vertexFilePath, fragmentFilePath);
}

bool Renderer::IsVisible(DrawingLayer layer)
{
	return layersVisible[GetDrawingLayerName(layer)];
}

void Renderer::ToggleVisibility(std::string layer)
{
	layersVisible[layer] = !layersVisible[layer];
}