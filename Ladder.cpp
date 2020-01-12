#include "Ladder.h"
#include "Player.h"
#include "debug_state.h"
#include "Renderer.h"
#include "Game.h"

Ladder::Ladder(Vector2 pos) : Entity(pos)
{
	etype = "ladder";
	layer = DrawingLayer::OBJECT;
	drawOrder = 90;
	trigger = true;
	collider.CreateCollider(TILE_SIZE, TILE_SIZE,
		(TILE_SIZE / 2) * Renderer::GetScale(), 0, 0.35f, 1);
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

void Ladder::Render(Renderer * renderer, GLuint uniformModel)
{
	Entity::Render(renderer);
	collider.CalculateCollider(position, Vector2(0,0)); // calculate here for next update frame

	if (GetModeDebug())
	{
		SDL_SetRenderDrawColor(renderer->renderer, 0, 255, 0, 255);
		SDL_RenderDrawRect(renderer->renderer, currentSprite->GetRect());

		SDL_SetRenderDrawColor(renderer->renderer, 255, 255, 255, 255);
		collider.CalculateCollider(position, Vector2(0,0)); //TODO: better way than calculating this twice?

		SDL_RenderDrawRect(renderer->renderer, collider.collisionBounds);
		SDL_SetRenderDrawColor(renderer->renderer, 0, 0, 0, 255);
	}
}

const SDL_Rect* Ladder::GetBounds()
{
	return collider.collisionBounds;
}

void Ladder::Save(std::ostringstream& level)
{
	int SCALE = Renderer::GetScale();
	Vector2 pos = GetPosition();

	level << std::to_string(id) << " " << etype << " " << (pos.x / SCALE) << " " <<
		(pos.y / SCALE) << " " << GetAnimator()->currentState->name
		<< " " << spriteIndex << "" << std::endl;
}