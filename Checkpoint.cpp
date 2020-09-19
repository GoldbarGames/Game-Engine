#include "Checkpoint.h"
#include "Game.h"
#include "globals.h"

Checkpoint::Checkpoint(Vector2 pos) : Entity(pos)
{
	layer = DrawingLayer::OBJECT;
	drawOrder = 90;
	etype = "checkpoint";
	trigger = true;
	CreateCollider(0, 0, 32, 32);
}

Checkpoint::~Checkpoint()
{

}

void Checkpoint::Update(Game& game)
{
	Entity::Update(game);
}

void Checkpoint::OnTriggerStay(Entity& other, Game& game)
{

}

void Checkpoint::OnTriggerEnter(Entity& other, Game& game)
{
	if (other.etype == "player")
	{
		Player* player = static_cast<Player*>(&other);
		player->currentCheckpoint = this;
	}
}

void Checkpoint::OnTriggerExit(Entity& other, Game& game)
{
	if (other.etype == "player")
	{
		Player* player = static_cast<Player*>(&other);
		player->currentCheckpoint = nullptr;
	}
}

bool Checkpoint::CanSpawnHere(const Vector2& spawnPosition, Game& game, bool useCamera)
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

void Checkpoint::GetProperties(std::vector<Property*>& properties)
{
	Entity::GetProperties(properties);

	properties.emplace_back(new Property("Cutscene Label", name));
}

void Checkpoint::SetProperty(const std::string& key, const std::string& newValue)
{
	// Based on the key, change its value
	if (key == "Cutscene Label")
	{
		name = newValue;
	}
}

void Checkpoint::Save(std::unordered_map<std::string, std::string>& map)
{
	shouldSave = true;
	Entity::Save(map);

	map["subtype"] = std::to_string(subtype);
	map["name"] = name;
}

void Checkpoint::Load(std::unordered_map<std::string, std::string>& map, Game& game)
{
	Entity::Load(map, game);

	subtype = std::stoi(map["subtype"]);
	name = map["name"];
}