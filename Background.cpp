#include "Background.h"
#include "BackgroundLayer.h"

Background::Background(Vector2 pos)
{
	position = pos;
}


Background::~Background()
{

}

void Background::Render(Renderer * renderer, Vector2 cameraOffset)
{
	cameraOffset = Vector2(0.1f, 0);
	for (unsigned int i = 0; i < layers.size(); i++)
	{
		layers[i]->Render(renderer, position);
	}
}

void Background::AddLayer(SpriteManager* spriteManager, Renderer* renderer, std::string filepath, int drawOrder)
{
	Sprite* layer = new Sprite(1, spriteManager, filepath, renderer, Vector2(0, 0));
	Entity* bg = new BackgroundLayer(position);
	bg->drawOrder = drawOrder;
	bg->SetSprite(layer);
	layers.emplace_back(bg);
}