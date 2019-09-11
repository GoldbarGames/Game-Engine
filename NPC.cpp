#include "NPC.h"
#include "Game.h"


NPC::NPC(std::string n, Vector2 pos) : PhysicsEntity(pos)
{
	name = n;
	etype = "npc";
	CreateCollider(27, 46, 0, 0, 0.75f, 0.9f);
	layer = DrawingLayer::COLLISION;
	drawOrder = 20;
}


NPC::~NPC()
{

}

void NPC::ChangeCollider(float x, float y, float w, float h)
{
	CreateCollider(x, y, 0, 0, w, h);
}


bool NPC::CanSpawnHere(Vector2 spawnPosition, Game& game, bool useCamera)
{
	return true; //TODO: Deal with this later. NPCs could have different sizes!
}

void NPC::OnTriggerStay(Entity* other)
{

}

void NPC::OnTriggerEnter(Entity* other)
{
	if (other->etype == "player")
	{
		Player* player = static_cast<Player*>(other);
		player->currentNPC = this;
	}
}

void NPC::OnTriggerExit(Entity* other)
{
	if (other->etype == "player")
	{
		Player* player = static_cast<Player*>(other);
		player->currentNPC = nullptr;
	}
}

void NPC::GetProperties(Renderer * renderer, TTF_Font * font, std::vector<Text*>& properties)
{
	Entity::DeleteProperties(properties);

	properties.emplace_back(new Text(renderer, font, "Name: " + name));
	properties.emplace_back(new Text(renderer, font, "Label: " + cutsceneLabel));
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