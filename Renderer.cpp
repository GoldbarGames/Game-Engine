#include "Renderer.h"

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

bool Renderer::IsVisible(DrawingLayer layer)
{
	return layersVisible[GetDrawingLayerName(layer)];
}

void Renderer::ToggleVisibility(std::string layer)
{
	layersVisible[layer] = !layersVisible[layer];
}