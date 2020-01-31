#include "NPC.h"
#include "Game.h"


NPC::NPC(std::string n, Vector2 pos) : Entity(pos)
{
	name = n;
	etype = "npc";
	CreateCollider(0, 0, 0, 0, 0, 0);
	layer = DrawingLayer::COLLISION;
	drawOrder = 20;
	

	physics = new PhysicsEntity(this);
	physics->standAboveGround = true;
}


NPC::~NPC()
{

}

void NPC::ChangeCollider(float x, float y, float w, float h)
{
	CreateCollider(0, 0, x, y, w, h);
}


bool NPC::CanSpawnHere(Vector2 spawnPosition, Game& game, bool useCamera)
{
	return true; //TODO: Deal with this later. NPCs could have different sizes!
}

void NPC::OnTriggerStay(Entity* other, Game& game)
{

}

void NPC::OnTriggerEnter(Entity* other, Game& game)
{
	if (other->etype == "player")
	{
		Player* player = static_cast<Player*>(other);
		player->currentNPC = this;
	}
}

void NPC::OnTriggerExit(Entity* other, Game& game)
{
	if (other->etype == "player")
	{
		Player* player = static_cast<Player*>(other);
		player->currentNPC = nullptr;
	}
}

void NPC::GetProperties(Renderer * renderer, TTF_Font * font, std::vector<Property*>& properties)
{
	Entity::GetProperties(renderer, font, properties);

	properties.emplace_back(new Property(new Text(renderer, font, "Name: " + name)));
	properties.emplace_back(new Property(new Text(renderer, font, "Label: " + cutsceneLabel)));
}

void NPC::SetProperty(std::string prop, std::string newValue)
{
	// 1. Split the string into two (key and value)
	std::string key = "";

	int index = 0;
	while (prop[index] != ':')
	{
		key += prop[index];
		index++;
	}

	// 2. Based on the key, change its value
	if (key == "Label")
	{
		cutsceneLabel = newValue;
	}
	else if (key == "Name")
	{
		name = newValue;
	}
}

void NPC::Save(std::ostringstream& level)
{
	std::string npcLabel = cutsceneLabel;
	if (npcLabel == "")
		npcLabel = "null";

	level << std::to_string(id) << " " << etype << " " << physics->startPosition.x << " " << physics->startPosition.y << " " << name <<
		" " << cutsceneLabel << " " << spriteIndex << " " << drawOrder <<
		" " << layer << " " << impassable << std::endl;
}