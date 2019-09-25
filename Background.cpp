#include "Background.h"
#include "BackgroundLayer.h"
#include "Game.h"

Background::Background(Vector2 pos)
{
	position = pos;
}


Background::~Background()
{
	for (unsigned int i = 0; i < layers.size(); i++)
	{
		delete layers[i];
	}
}

void Background::Render(Renderer * renderer, Vector2 cameraOffset)
{
	//TODO: For parallax scrolling, manipulate the position
	for (unsigned int i = 0; i < layers.size(); i++)
	{
		layers[i]->Render(renderer, cameraOffset);
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

void Background::DeleteLayers(Game& game)
{
	for (unsigned int i = 0; i < layers.size(); i++)
	{
		game.ShouldDeleteEntity(layers[i]);
	}
}