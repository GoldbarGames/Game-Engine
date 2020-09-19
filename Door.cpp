#include "Door.h"
#include "Game.h"
#include "globals.h"

Door::Door(Vector2 pos) : Entity(pos)
{
	layer = DrawingLayer::OBJECT;
	drawOrder = 90;
	name = "door";
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

	//TODO: Make this a template function?
	// Only check for the attached door if we think it can be found in the current level
	// Otherwise, we can assume that it can be found in the next level we go to
	if (attachedDoor == nullptr && destinationID > -1 && nextLevelName == "")
	{
		bool found = false;
		for (int i = 0; i < game.entities.size(); i++)
		{
			if (game.entities[i]->id == destinationID && game.entities[i]->etype == "door")
			{
				attachedDoor = static_cast<Door*>(game.entities[i]);
				found = true;
				break;
			}
		}
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
	if (attachedDoor != nullptr)
	{
		return attachedDoor->position;
	}

	return position;
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

void Door::GetProperties(std::vector<Property*>& properties)
{
	Entity::GetProperties(properties);

	properties.emplace_back(new Property("Name", name));
	properties.emplace_back(new Property("Is Locked", isLocked));
	properties.emplace_back(new Property("Next Level", nextLevelName));
	properties.emplace_back(new Property("Destination ID", destinationID));
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
	else if (key == "Destination ID")
	{
		if (newValue != "")
			destinationID = std::stoi(newValue);
	}
}

void Door::Save(std::unordered_map<std::string, std::string>& map)
{
	shouldSave = true;
	Entity::Save(map);

	map["subtype"] = std::to_string(subtype);
	map["name"] = name;
	map["isLocked"] = std::to_string(isLocked);
	map["nextLevelName"] = nextLevelName;
	map["destinationID"] = std::to_string(destinationID);
}

void Door::Load(std::unordered_map<std::string, std::string>& map, Game& game)
{
	Entity::Load(map, game);

	subtype = std::stoi(map["subtype"]);
	name = map["name"];
	isLocked = std::stoi(map["isLocked"]);
	nextLevelName = map["nextLevelName"];
	destinationID = std::stoi(map["destinationID"]);

	game.editor->loadListDoors.push_back(this);
}