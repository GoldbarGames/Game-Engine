#include "Tree.h"
#include "Entity.h"
#include "Sprite.h"
#include "Property.h"
#include "PhysicsComponent.h"
#include "Game.h"
#include "Vector2.h"

Tree::Tree(Vector2 pos) : Entity(pos)
{
	etype = "tree";

	layer = DrawingLayer::COLLISION;
	drawOrder = 6;
	trigger = true;

	CreateCollider(0, 0, 115, 166);
}

Tree::~Tree()
{
	if (bottomSprite != nullptr)
		delete_it(bottomSprite);
}

void Tree::Init(const std::string& n)
{
	name = n;
	//TODO: Maybe we don't need the local variable?
	//bottomSprite = animator->GetState("trunk")->sprite;
}

void Tree::Update(Game& game)
{
	if (hiddenEntity != nullptr)
	{
		if (animator->GetBool("isPushed"))
		{
			animator->SetBool("isPushed", false);

			// Drop the object that is hidden in the tree
			if (hiddenEntity->physics != nullptr)
			{
				hiddenEntity->physics->useGravity = true;
				//hiddenEntity->physics->SetVelocity(Vector2(0.0f, 0.5f));
			}
		}
	}
	else if (hiddenEntityID > -1)
	{
		bool found = false;
		for (int i = 0; i < game.entities.size(); i++)
		{
			if (game.entities[i]->id == hiddenEntityID)
			{
				hiddenEntity = game.entities[i];
				hiddenEntity->position = position;
				found = true;
				break;
			}
		}

		if (!found)
		{
			hiddenEntityID = -1;
		}
	}
}

void Tree::Render(const Renderer& renderer)
{
	if (bottomSprite != nullptr)
	{
		bottomSprite->Render(position, renderer);
	}

	if (hiddenEntity != nullptr)
	{
		hiddenEntity->layer = DrawingLayer::COLLISION;
		hiddenEntity->Render(renderer);
		hiddenEntity->layer = DrawingLayer::INVISIBLE;
	}

	Entity::Render(renderer);
}

void Tree::GetProperties(std::vector<Property*>& properties)
{
	Entity::GetProperties(properties);

	properties.emplace_back(new Property("Hidden Entity ID", hiddenEntityID));
}

void Tree::SetProperty(const std::string& key, const std::string& newValue)
{
	if (key == "Hidden Entity ID")
	{
		if (newValue != "")
			hiddenEntityID = std::stoi(newValue);
	}
}

void Tree::OnTriggerStay(Entity& other, Game& game)
{

}

void Tree::OnTriggerEnter(Entity& other, Game& game)
{

}

void Tree::OnTriggerExit(Entity& other, Game& game)
{

}

void Tree::Save(std::unordered_map<std::string, std::string>& map)
{
	shouldSave = true;
	Entity::Save(map);

	map["subtype"] = std::to_string(subtype);
	map["hiddenEntityID"] = std::to_string(hiddenEntityID);
}

void Tree::Load(std::unordered_map<std::string, std::string>& map, Game& game)
{
	Entity::Load(map, game);	
	subtype = std::stoi(map["subtype"]);
	hiddenEntityID = std::stoi(map["hiddenEntityID"]);
}