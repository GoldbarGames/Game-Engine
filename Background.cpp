#include "Background.h"

Background::Background(Vector2 pos)
{
	position = pos;
}


Background::~Background()
{

}

void Background::Render(SDL_Renderer * renderer, Vector2 cameraOffset)
{
	for (int i = 0; i < layers.size(); i++)
	{
		layers[i]->Render(renderer, cameraOffset + position);
	}
}