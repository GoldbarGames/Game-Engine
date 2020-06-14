#include "Background.h"
#include "BackgroundLayer.h"
#include "Game.h"
#include "SpriteManager.h"
#include "Entity.h"
#include "Vector2.h"

//TODO: Probably remove this entire class, refactor it away

Background::Background(std::string n, Vector2 pos)
{
	name = n;
	position = pos;
}

void Background::CreateBackground(std::string n, Vector2 pos, SpriteManager* spriteManager, Renderer* renderer)
{
	name = n;

	//TODO: Should this stuff go inside the Background class constructor?
	if (name == "forest")
	{
		Entity* blueBG = AddLayer(pos + Vector2(0, 0), spriteManager, renderer, "assets/gui/white.png", -99, 0.0f);
		blueBG->GetSprite()->color = { 0, 0, 83, 255 };
		blueBG->GetSprite()->SetScale(Vector2(19.875f, 11.2f * 4));
		blueBG->position.y -= (358 * 4);

		AddLayer(pos + Vector2(0, 0), spriteManager, renderer, "assets/bg/forest/forest_sky1.png", -98, 0.0f);
		//background->layers[0]->GetSprite()->renderRelativeToCamera = true;
		//background->layers[0]->GetSprite()->keepScaleRelativeToCamera = true;

		AddLayer(pos + Vector2(0, 0), spriteManager, renderer, "assets/bg/forest/forest_ground.png", -90, 1.0f);
		AddLayer(pos + Vector2(0, 0), spriteManager, renderer, "assets/bg/forest/forest_trees_back_curved.png", -21, 0.7f);
		AddLayer(pos + Vector2(0, 0), spriteManager, renderer, "assets/bg/forest/forest_trees_back.png", -20, 0.6f);
		AddLayer(pos + Vector2(0, 0), spriteManager, renderer, "assets/bg/forest/forest_trees_front_curved.png", -11, 0.5f);
		AddLayer(pos + Vector2(0, 100), spriteManager, renderer, "assets/bg/forest/forest_trees_front.png", -10, 0.4f);
	}
	else if (name == "title")
	{
		AddLayer(pos + Vector2(0, 0), spriteManager, renderer, "assets/bg/title/title.png", -98, 0.0f);
	}
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
	for (unsigned int i = 0; i < layers.size(); i++)
	{
		layers[i]->Render(renderer);
	}
}

Entity* Background::AddLayer(Vector2 offset, SpriteManager* spriteManager, Renderer* renderer, std::string filepath, int drawOrder, float parallax)
{
	Sprite* layer = new Sprite(1, spriteManager, filepath, renderer->shaders[ShaderName::Default], Vector2(0, 0));
	Entity* bg = new BackgroundLayer(offset, parallax);
	bg->drawOrder = drawOrder;
	bg->SetSprite(layer);
	layers.emplace_back(bg);
	return bg;
}

void Background::DeleteLayers(Game& game)
{
	for (unsigned int i = 0; i < layers.size(); i++)
	{
		game.ShouldDeleteEntity(layers[i]);
	}
}

void Background::ResetBackground()
{
	for (unsigned int i = 0; i < layers.size(); i++)
		delete_it(layers[i]);
	
	layers.clear();
}

void Background::Save(std::ostringstream& level)
{
	level << "0 bg " << position.x << " " << position.y << " " << name << std::endl;
}