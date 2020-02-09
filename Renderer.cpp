#include "Renderer.h"
#include "Sprite.h"

Renderer::Renderer()
{
	layersVisible["BACK"] = true;
	layersVisible["MIDDLE"] = true;
	layersVisible["OBJECT"] = true;
	layersVisible["COLLISION"] = true;
	layersVisible["FRONT"] = true;
}

Renderer::~Renderer()
{

}

// This function takes two sprites, the source and the target
// It returns a vector2 representing the amount that the source 
//should be scaled by in order to equal the scaled size of the target
Vector2 Renderer::CalculateScale(Sprite* sourceSprite, Sprite* targetSprite)
{
	float sourceWidth = sourceSprite->texture->GetWidth();
	float sourceHeight = sourceSprite->texture->GetHeight();

	float targetWidth = targetSprite->frameWidth;
	float targetHeight = targetSprite->frameHeight;

	return Vector2(targetWidth * targetSprite->scale.x / sourceWidth, 
		targetHeight * targetSprite->scale.y / sourceHeight);
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