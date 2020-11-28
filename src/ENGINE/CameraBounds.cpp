#include "CameraBounds.h"
#include "Property.h"
#include "Renderer.h"
#include "Game.h"

CameraBounds::CameraBounds(Vector2 pos) : Entity(pos)
{
	etype = "cameraBounds";
	shouldSave = true;
	CreateCollider(0, 0, Globals::TILE_SIZE, Globals::TILE_SIZE);
}

CameraBounds::~CameraBounds()
{

}

void CameraBounds::Render(const Renderer& renderer)
{
	if (renderer.game->editMode || renderer.game->debugMode)
	{
		renderer.debugSprite->color = { 0, 255, 255, 255 };
		Vector2 adjustedScale = Vector2(collider.scale.x / Globals::TILE_SIZE, collider.scale.y / Globals::TILE_SIZE);
		renderer.debugSprite->Render(position, renderer, adjustedScale);
	}
}

void CameraBounds::GetProperties(std::vector<Property*>& properties)
{
	Entity::GetProperties(properties);

	properties.emplace_back(new Property("Collider Pos X", (int)collider.offset.x));
	properties.emplace_back(new Property("Collider Pos Y", (int)collider.offset.y));
	properties.emplace_back(new Property("Collider Width", collider.scale.x));
	properties.emplace_back(new Property("Collider Height", collider.scale.y));
}

void CameraBounds::SetProperty(const std::string& key, const std::string& newValue)
{
	try
	{
		// Based on the key, change its value
		//TODO: Make this more robust
		if (key == "Collider Pos X")
		{
			if (newValue != "")
				collider.offset.x = std::stoi(newValue);
		}
		else if (key == "Collider Pos Y")
		{
			if (newValue != "")
				collider.offset.y = std::stoi(newValue);
		}
		else if (key == "Collider Width")
		{
			if (newValue != "")
				collider.scale.x = std::stof(newValue);
		}
		else if (key == "Collider Height")
		{
			if (newValue != "")
				collider.scale.y = std::stof(newValue);
		}
	}
	catch (std::exception ex)
	{
		std::cout << ex.what() << std::endl;
	}
}

void CameraBounds::Save(std::unordered_map<std::string, std::string>& map)
{
	Entity::Save(map);

	map["colliderOffsetX"] = std::to_string((int)collider.offset.x);
	map["colliderOffsetY"] = std::to_string((int)collider.offset.y);
	map["colliderScaleX"] = std::to_string((int)collider.scale.x);
	map["colliderScaleY"] = std::to_string((int)collider.scale.y);
}

void CameraBounds::Load(std::unordered_map<std::string, std::string>& map, Game& game)
{
	Entity::Load(map, game);

	int colliderOffsetX = std::stoi(map["colliderOffsetX"]);
	int colliderOffsetY = std::stoi(map["colliderOffsetY"]);
	int colliderScaleX = std::stoi(map["colliderScaleX"]);
	int colliderScaleY = std::stoi(map["colliderScaleY"]);

	CreateCollider(colliderOffsetX, colliderOffsetY, colliderScaleX, colliderScaleY);
}
