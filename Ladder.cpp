#include "Ladder.h"
#include "Player.h"
#include "globals.h"
#include "Renderer.h"
#include "Game.h"

Ladder::Ladder(Vector2 pos) : Entity(pos)
{
	etype = "ladder";
	layer = DrawingLayer::OBJECT;
	drawOrder = 90;
	trigger = true;
	CreateCollider(0, 0, TILE_SIZE, TILE_SIZE);
}

Ladder::~Ladder()
{
	
}

void Ladder::OnTriggerStay(Entity* other, Game& game)
{

}

void Ladder::OnTriggerEnter(Entity* other, Game& game)
{
	if (other->etype == "player")
	{
		Player* player = static_cast<Player*>(other);
		player->currentLadder = this;
	}
}

void Ladder::OnTriggerExit(Entity* other, Game& game)
{
	if (other->etype == "player")
	{
		Player* player = static_cast<Player*>(other);
		if (player->currentLadder == this)
		{			
			player->GetAnimator()->SetBool("onLadder", false);
			player->currentLadder = nullptr;
		}		
	}
}

void Ladder::Render(Renderer * renderer, unsigned int uniformModel)
{
	Entity::Render(renderer);
}

const SDL_Rect* Ladder::GetBounds()
{
	return collisionBounds;
}

void Ladder::Save(std::ostringstream& level)
{
	Vector2 pos = GetPosition();

	level << std::to_string(id) << " " << etype << " " << pos.x << " " <<
		pos.y << " " << GetAnimator()->currentState->name
		<< " " << spriteIndex << "" << std::endl;
}