#include "NPC.h"
#include "Game.h"


NPC::NPC(std::string n, Vector2 pos) : PhysicsEntity(pos)
{
	name = n;
	etype = "NPC";
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