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

void Background::AddLayer(SpriteManager& spriteManager, SDL_Renderer* renderer, std::string filepath, int drawOrder)
{
	Sprite* layer = new Sprite(1, spriteManager, filepath, renderer, Vector2(0, 0));
	Entity* bg = new Entity();
	bg->drawOrder = drawOrder;
	bg->SetSprite(layer);
	layers.emplace_back(bg);
}