#include "NPC.h"
#include "Game.h"
#include "PhysicsComponent.h"
#include "HealthComponent.h"
#include "NPC.h"
#include "Property.h"
#include "Player.h"
#include "Entity.h"

NPC::NPC(const Vector2& pos) : Entity(pos)
{
	etype = "npc";
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


NPC::~NPC()
{

}

void NPC::Init(const std::string& n)
{
	name = n;

	if (name == "gramps")
	{
		CreateCollider(0, 0, 22, 39);
	}
	else if (name == "the_man")
	{
		CreateCollider(0, 0, 32, 60);
	}
	else if (name == "signpost")
	{
		CreateCollider(0, 24, 24, 32);
	}
	else if (name == "daisy")
	{
		CreateCollider(0, 8, 42, 44);
	}
}

void NPC::ChangeCollider(float x, float y, float w, float h)
{
	CreateCollider(x, y, w, h);
}


bool NPC::CanSpawnHere(const Vector2& spawnPosition, Game& game, bool useCamera)
{
	return true; //TODO: Deal with this later. NPCs could have different sizes!
}

void NPC::OnTriggerStay(Entity& other, Game& game)
{

}

void NPC::OnTriggerEnter(Entity& other, Game& game)
{
	if (other.etype == "player")
	{
		Player* player = static_cast<Player*>(&other);
		player->currentNPC = this;
	}
}

void NPC::OnTriggerExit(Entity& other, Game& game)
{
	if (other.etype == "player")
	{
		Player* player = static_cast<Player*>(&other);
		player->currentNPC = nullptr;
	}
}

void NPC::GetProperties(std::vector<Property*>& properties)
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

void NPC::SetProperty(const std::string& key, const std::string& newValue)
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

void NPC::Save(std::unordered_map<std::string, std::string>& map)
{
	Entity::Save(map);

	map["subtype"] = std::to_string(subtype);
	map["name"] = name;
	map["cutsceneLabel"] = cutsceneLabel == "" ? "null" : cutsceneLabel;
	map["drawOrder"] = std::to_string(drawOrder);
	map["layer"] = std::to_string((int)layer);
	map["impassable"] = std::to_string(impassable);
}

void NPC::Load(std::unordered_map<std::string, std::string>& map, Game& game)
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