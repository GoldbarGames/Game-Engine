#include "Decoration.h"
#include "../ENGINE/Game.h"
#include "../ENGINE/PhysicsComponent.h"
#include "../ENGINE/HealthComponent.h"
#include "NPC.h"
#include "../ENGINE/Property.h"
#include "Player.h"

Decoration::Decoration(const Vector2& pos) : MyEntity(pos)
{
	etype = "decoration";
	CreateCollider(0, 0, 0, 0);
	CreateCollider(0, 0, TILE_SIZE, TILE_SIZE);
	layer = DrawingLayer::COLLISION;
	drawOrder = 20;
	trigger = true;

	physics = neww PhysicsComponent(this);
	physics->standAboveGround = true;
	physics->useGravity = true;
	startPosition = pos;

	//health = neww HealthComponent(1);
	//health->invincible = true;
}


Decoration::~Decoration()
{

}

void Decoration::Init(const Game& g, const std::string& n)
{
	name = n;
	animator->SetState(name.c_str());
	animator->Update(*this);
	CreateCollider(0, 0, currentSprite.frameWidth, currentSprite.frameHeight);
}

void Decoration::ChangeCollider(float x, float y, float w, float h)
{
	CreateCollider(x, y, w, h);
}

bool Decoration::CanSpawnHere(const Vector2& spawnPosition, Game& game, bool useCamera)
{
	return true; //TODO: Deal with this later. NPCs could have different sizes!
}

void Decoration::OnTriggerStay(MyEntity& other, Game& game)
{

}

void Decoration::OnTriggerEnter(MyEntity& other, Game& game)
{
	if (other.etype == "player")
	{
		Player* player = static_cast<Player*>(&other);
		player->currentDecoration = this;
	}
}

void Decoration::OnTriggerExit(MyEntity& other, Game& game)
{
	if (other.etype == "player")
	{
		Player* player = static_cast<Player*>(&other);
		player->currentDecoration = nullptr;
	}
}

void Decoration::GetProperties(std::vector<Property*>& properties)
{
	Entity::GetProperties(properties);

	properties.emplace_back(new Property("Name", name));
	properties.emplace_back(new Property("Label", cutsceneLabel));
	//properties.emplace_back(new Property("Shader", ));
	properties.emplace_back(new Property("Collider Pos X", (int)collider->offset.x));
	properties.emplace_back(new Property("Collider Pos Y", (int)collider->offset.y));
	properties.emplace_back(new Property("Collider Width", collider->scale.x));
	properties.emplace_back(new Property("Collider Height", collider->scale.y));
}

void Decoration::SetProperty(const std::string& key, const std::string& newValue)
{
	// Based on the key, change its value
	if (key == "Label")
	{
		cutsceneLabel = newValue;
	}
	else if (key == "Name")
	{
		name = newValue;
	}
	else if (key == "Collider Pos X")
	{
		if (newValue != "")
			collider->offset.x = std::stoi(newValue);
	}
	else if (key == "Collider Pos Y")
	{
		if (newValue != "")
			collider->offset.y = std::stoi(newValue);
	}
	else if (key == "Collider Width")
	{
		if (newValue != "")
			collider->scale.x = std::stof(newValue);
	}
	else if (key == "Collider Height")
	{
		if (newValue != "")
			collider->scale.y = std::stof(newValue);
	}
}

void Decoration::Save(std::unordered_map<std::string, std::string>& map)
{
	Entity::Save(map);

	map["subtype"] = std::to_string(subtype);
	map["name"] = name;
	map["cutsceneLabel"] = cutsceneLabel == "" ? "null" : cutsceneLabel;
	map["drawOrder"] = std::to_string(drawOrder);
	map["layer"] = std::to_string((int)layer);
	map["impassable"] = std::to_string(impassable);
}

void Decoration::Load(std::unordered_map<std::string, std::string>& map, Game& game)
{
	shouldSave = true;
	Entity::Load(map, game);

	subtype = std::stoi(map["subtype"]);
	name = map["name"];
	cutsceneLabel = map["cutsceneLabel"];

	drawOrder = std::stoi(map["drawOrder"]);
	layer = (DrawingLayer)std::stoi(map["layer"]);
	impassable = std::stoi(map["impassable"]);
}