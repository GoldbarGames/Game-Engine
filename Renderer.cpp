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