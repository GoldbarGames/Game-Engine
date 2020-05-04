#include "tile.h"
#include "debug_state.h"
#include "globals.h"
#include "Game.h"

using std::string;

Tile::Tile(Vector2 pos, Vector2 frame, Texture * image, Renderer * renderer) : Entity(pos)
{	
	ChangeSprite(frame, image, renderer);
	etype = "tile";
}

Tile::~Tile()
{

}


void Tile::ChangeSprite(Vector2 frame, Texture * image, Renderer * renderer)
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
	return collisionBounds;
	//return currentSprite->GetRect();
}

void Tile::Destroy()
{

}

bool Tile::CanSpawnHere(Vector2 spawnPosition, Game& game, bool useCamera)
{
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