#include "NPC.h"
#include "Game.h"
#include "PhysicsComponent.h"

NPC::NPC(const std::string& n, const Vector2& pos) : Entity(pos)
{
	name = n;
	etype = "npc";
	CreateCollider(0, 0, 0, 0);
	CreateCollider(0, 0, TILE_SIZE, TILE_SIZE);
	layer = DrawingLayer::COLLISION;
	drawOrder = 20;
	trigger = true;

	physics = new PhysicsComponent(this);
	physics->standAboveGround = true;
	physics->useGravity = true;
	physics->startPosition = pos;
}


NPC::~NPC()
{

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

void NPC::GetProperties(FontInfo* font, std::vector<Property*>& properties)
{
	Entity::GetProperties(font, properties);

	properties.emplace_back(new Property("Name", name));
	properties.emplace_back(new Property("Label", cutsceneLabel));
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

void NPC::Save(std::ostringstream& level)
{
	std::string npcLabel = cutsceneLabel;
	if (npcLabel == "")
		npcLabel = "null";

	level << std::to_string(id) << " " << etype << " " << physics->startPosition.x << " " << physics->startPosition.y << " " << name <<
		" " << cutsceneLabel << " " << spriteIndex << " " << drawOrder <<
		" " << (int)layer << " " << impassable << std::endl;
}