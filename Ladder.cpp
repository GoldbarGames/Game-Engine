#include "Ladder.h"
#include "Player.h"
#include "debug_state.h"

Ladder::Ladder(Vector2 pos) : Entity(pos)
{
	etype = "ladder";
	layer = DrawingLayer::OBJECT;
	drawOrder = 90;
	trigger = true;
	collider.CreateCollider(TILE_SIZE, TILE_SIZE,
		(TILE_SIZE / 2) * SCALE, 0, 0.35f, 1);
}

Ladder::~Ladder()
{
	
}

void Ladder::OnTriggerStay(Entity* other)
{

}

void Ladder::OnTriggerEnter(Entity* other)
{
	if (other->etype == "player")
	{
		Player* player = static_cast<Player*>(other);
		player->currentLadder = this;
	}
}

void Ladder::OnTriggerExit(Entity* other)
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

void Ladder::Render(SDL_Renderer * renderer, Vector2 cameraOffset)
{
	Entity::Render(renderer, cameraOffset);
	collider.CalculateCollider(position, cameraOffset); // calculate here for next update frame

	if (GetModeDebug())
	{
		SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
		SDL_RenderDrawRect(renderer, currentSprite->GetRect());

		SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
		collider.CalculateCollider(position, cameraOffset); //TODO: better way than calculating this twice?

		SDL_RenderDrawRect(renderer, collider.collisionBounds);
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	}
}

const SDL_Rect* Ladder::GetBounds()
{
	return collider.collisionBounds;
}

