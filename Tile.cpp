#include "tile.h"
#include "globals.h"
#include "Game.h"
#include "Texture.h"

using std::string;

Tile::Tile(const Vector2& pos, const Vector2& frame, Texture* image, Renderer* renderer) : Entity(pos)
{	
	ChangeSprite(frame, image, renderer);
	etype = "tile";
}

Tile::~Tile()
{

}


void Tile::ChangeSprite(const Vector2& frame, Texture* image, Renderer* renderer)
{
	if (currentSprite != nullptr)
		delete currentSprite;
	
	tileCoordinates = frame;
	currentSprite = new Sprite(frame, image, renderer->shaders[ShaderName::Default]);
}

void Tile::Animate()
{

}

const SDL_Rect* Tile::GetBounds()
{
	if (collider == nullptr)
		return currentSprite->GetRect();
	else
		return collider->bounds;
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

void Tile::Save(std::ostringstream& level)
{
	float x = GetPosition().x;
	float y = GetPosition().y;

	int passableState = 0;
	if (impassable)
		passableState = 1;
	if (jumpThru)
		passableState = 2;

	level << std::to_string(id) << " " << etype << " " << x << " " << y << " " << drawOrder << " " << (int)layer << " " 
		<< passableState << " " << tilesheetIndex << " " << tileCoordinates.x <<
		" " << tileCoordinates.y << "" << std::endl;
}