#include "Door.h"
#include "Game.h"
#include "globals.h"

Door::Door(Vector2 pos, Vector2 dest) : Entity(pos)
{
	destination = dest;
	layer = DrawingLayer::OBJECT;
	drawOrder = 90;
	etype = "door";
	trigger = true;
	CreateCollider(0, 0, 96, 96);
}

Door::~Door()
{

}

void Door::Update(Game& game)
{
	if (name == "goal")
	{
		isLocked = game.bugsRemaining > 0;
	}
	
	animator->SetBool("opened", !isLocked);
	Entity::Update(game);
}

void Door::OnTriggerStay(Entity& other, Game& game)
{

}

void Door::OnTriggerEnter(Entity& other, Game& game)
{
	if (other.etype == "player")
	{
		Player* player = static_cast<Player*>(&other);
		player->currentDoor = this;
	}
}

void Door::OnTriggerExit(Entity& other, Game& game)
{
	if (other.etype == "player")
	{
		Player* player = static_cast<Player*>(&other);
		player->currentDoor = nullptr;
	}
}

Vector2 Door::GetDestination()
{
	return destination;
}

void Door::SetDestination(Vector2 dest)
{
	destination = dest;
}

bool Door::CanSpawnHere(Vector2 spawnPosition, Game& game, bool useCamera)
{
	return Entity::CanSpawnHere(spawnPosition, game, useCamera);

	bool shouldSpawn = true;

	if (currentSprite == nullptr)
		return false;

	//TODO: Set this to false. It fails right now because the other entities sprites
	// have not had their window rects set, so they are at 0,0 which would always fail
	// to cause a collision and therefore shouldSpawn always becomes false

	return shouldSpawn;
}

void Door::GetProperties(FontInfo* font, std::vector<Property*>& properties)
{
	Entity::GetProperties(font, properties);

	properties.emplace_back(new Property("Name", name));
	properties.emplace_back(new Property("Is Locked", isLocked));
	properties.emplace_back(new Property("Next Level", nextLevelName));
}

void Door::SetProperty(const std::string& key, const std::string& newValue)
{
	// Based on the key, change its value
	if (key == "Next Level")
	{
		nextLevelName = newValue;
	}
	else if (key == "Is Locked")
	{
		if (newValue != "")
			isLocked = std::stoi(newValue);
	}
	else if (key == "Name")
	{
		name = newValue;
	}
}

void Door::Save(std::ostringstream& level)
{
	level << std::to_string(id) << " " << etype << " " << position.x << " " <<
		position.y << " " << GetDestination().x << " " << GetDestination().y
		<< " " << spriteIndex << "" << std::endl;
}