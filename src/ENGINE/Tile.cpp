#include "Tile.h"
#include "globals.h"
#include "Game.h"
#include "Texture.h"
#include "Shader.h"
#include "Sprite.h"
#include "Renderer.h"
#include "Editor.h"
#include "Entity.h"

Tile::Tile(const Vector2& pos, const Vector2& frame, Texture* image, const Renderer& renderer) : Entity(pos)
{	
	shouldSave = true;
	ChangeSprite(frame, image, renderer);
	etype = "tile";
}

Tile::~Tile()
{

}

// TODO: Move this inside the Game class: ChangeTileSprite?
void Tile::ChangeSprite(const Vector2& frame, Texture* image, const Renderer& renderer)
{
	tileCoordinates = frame;
	currentSprite = Sprite(frame, image, renderer.shaders[ShaderName::Default]);
}

void Tile::Animate()
{

}

void Tile::Destroy()
{

}

bool Tile::CanSpawnHere(const Vector2& spawnPosition, Game& game, bool useCamera)
{
	return Entity::CanSpawnHere(spawnPosition, game, useCamera);

	for (unsigned int i = 0; i < game.entities.size(); i++)
	{
		if (game.entities[i]->GetPosition() == spawnPosition &&
			game.entities[i]->layer == layer &&
			game.entities[i]->etype == "tile")
		{
			return false;
		}
	}

	return true;
}

void Tile::Save(std::unordered_map<std::string, std::string>& map)
{
	Entity::Save(map);

	int passableState = 0;
	if (impassable)
		passableState = 1;
	if (jumpThru)
		passableState = 2;

	static const std::string STR_FRAMEX("frameX");
	static const std::string STR_FRAMEY("frameY");
	static const std::string STR_TILESHEET("tilesheet");
	static const std::string STR_DRAWORDER("drawOrder");
	static const std::string STR_LAYER("layer");
	static const std::string STR_SUBTYPE("subtype");
	static const std::string STR_PASSABLESTATE("passableState");

	map[STR_DRAWORDER] = std::to_string(drawOrder);
	map[STR_LAYER] = std::to_string((int)layer);
	map[STR_PASSABLESTATE] = std::to_string(passableState);
	map[STR_TILESHEET] = std::to_string(tilesheetIndex);
	map[STR_SUBTYPE] = std::to_string(subtype);
	map[STR_FRAMEX] = std::to_string((int)tileCoordinates.x);
	map[STR_FRAMEY] = std::to_string((int)tileCoordinates.y);
}

void Tile::Load(std::unordered_map<std::string, std::string>& map, Game& game)
{
	Entity::Load(map, game);

	// TODO: The below code breaks things, but we should ultimately use something like it
	// rather than calling the spawn tile function when loading tiles

	/*
	Tile* newTile = game.SpawnTile(Vector2(std::stoi(map["frameX"]), std::stoi(map["frameY"])),
		"assets/tiles/" + game.editor->GetTileSheetFileName(std::stoi(map["tilesheet"])) + ".png",
		Vector2(std::stoi(map["positionX"]), std::stoi(map["positionY"])), (DrawingLayer)layer);

	if (std::stoi(map["passableState"]) == 2)
		newTile->jumpThru = true;

	newTile->tilesheetIndex = std::stoi(map["tilesheet"]);
	*/
}