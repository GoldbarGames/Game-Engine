#include "Background.h"
#include "BackgroundLayer.h"
#include "Game.h"

Background::Background(std::string n, Vector2 pos)
{
	name = n;
	position = pos;
}


Background::~Background()
{
	for (unsigned int i = 0; i < layers.size(); i++)
	{
		delete_it(layers[i]);
	}
}

void Background::Render(Renderer * renderer)
{
	Vector2 offset = Vector2(10, 0);

	//TODO: For parallax scrolling, manipulate the position
	for (unsigned int i = 0; i < layers.size(); i++)
	{
		layers[i]->Render(renderer);
	}
}

void Background::AddLayer(SpriteManager* spriteManager, Renderer* renderer, std::string filepath, int drawOrder, float parallax)
{
	Sprite* layer = new Sprite(1, spriteManager, filepath, renderer->shaders["default"], Vector2(0, 0));
	Entity* bg = new BackgroundLayer(position, parallax);
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

void Background::Save(std::ostringstream& level)
{
	level << "0 bg " << position.x << " " << position.y << " " << name << std::endl;
}