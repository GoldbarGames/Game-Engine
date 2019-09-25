#include "Missile.h"
#include "Game.h"
#include "debug_state.h"

Missile::Missile(Vector2 pos) : PhysicsEntity(pos)
{
	CreateCollider(23, 16, 0, 0, 0.75f, 0.9f);

	timeToLive.Start(2000);

	//TODO: Is there a good way to do this from within the constructor?
	//animator = new Animator("missile", "moving"); //TODO: Make these parameters?
	//animator->SetBool("destroyed", false);
}

Missile::~Missile()
{

}

void Missile::Update(Game& game)
{
	if (timeToLive.HasElapsed())
	{
		if (animator->GetBool("destroyed") && animator->animationTimer.HasElapsed())
		{
			shouldDelete = true;
		}
		else
		{
			animator->SetBool("destroyed", true);
		}
	}
	else
	{
		UpdatePhysics(game);
	}

	if (animator != nullptr)
		animator->Update(this);
}

void Missile::UpdatePhysics(Game& game)
{
	// check for collisions, and destroy if it hits a wall or an enemy'
	if (CheckCollisions(game))
	{
		if (animator->GetBool("destroyed") && animator->animationTimer.HasElapsed())
		{
			shouldDelete = true;
		}
		else
		{
			animator->SetBool("destroyed", true);
		}
	}
	else
	{
		// move the missile
		SetPosition(Vector2(position.x + velocity.x, position.y + velocity.y));
	}
}

void Missile::Render(Renderer * renderer, Vector2 cameraOffset)
{
	if (currentSprite != nullptr)
	{
		float collisionCenterX = (collisionBounds->x + (collisionBounds->w / 2));
		float collisionCenterY = (collisionBounds->y + (collisionBounds->h / 2));
		Vector2 collisionCenter = Vector2(collisionCenterX, collisionCenterY);
		Vector2 scaledPivot = Vector2(currentSprite->pivot.x * Renderer::GetScale(), currentSprite->pivot.y * Renderer::GetScale());
		Vector2 pivotOffset = collisionCenter - scaledPivot;

		Vector2 offset = pivotOffset;

		if (GetModeEdit())
			offset -= cameraOffset;

		if (animator != nullptr)
			currentSprite->Render(offset, animator->speed, animator->animationTimer.GetAnimationTime(), flip, renderer, angle);
		else
			currentSprite->Render(offset, 0, -1, flip, renderer, angle);

		if (GetModeDebug())
		{
			if (impassable)
				SDL_SetRenderDrawColor(renderer->renderer, 255, 0, 0, 255);
			else
				SDL_SetRenderDrawColor(renderer->renderer, 0, 255, 0, 255);

			SDL_RenderDrawRect(renderer->renderer, currentSprite->GetRect());

			SDL_SetRenderDrawColor(renderer->renderer, 255, 255, 255, 255);
			CalculateCollider(cameraOffset); //TODO: better way than calculating this twice?

			SDL_RenderDrawRect(renderer->renderer, collisionBounds);
			SDL_SetRenderDrawColor(renderer->renderer, 0, 0, 0, 255);
		}
	}
}

void Missile::CalculateCollider(Vector2 cameraOffset)
{
	// set the collision bounds position to where the entity actually is
	collisionBounds->x = position.x + collider->x - cameraOffset.x;
	collisionBounds->y = position.y + collider->y - cameraOffset.y;

	// scale the bounds of the sprite by a number to set the collider's width and height
	collisionBounds->w = startSpriteSize.x * colliderWidth;
	collisionBounds->h = startSpriteSize.y * colliderHeight;
}

bool Missile::CheckCollisions(Game& game)
{
	if (currentSprite == nullptr && animator != nullptr)
		animator->DoState(this);

	entityPivot = currentSprite->pivot;
	CalculateCollider(game.camera);

	bool horizontalCollision = false;
	bool verticalCollision = false;

	// Get bounds assuming the move is valid
	SDL_Rect myBounds = *GetColliderBounds();

	SDL_Rect newBoundsHorizontal = myBounds;
	newBoundsHorizontal.x = myBounds.x + (velocity.x * game.dt);

	SDL_Rect newBoundsVertical = myBounds;
	newBoundsVertical.y = myBounds.y + (velocity.y * game.dt);

	// this needs to be here so that it does not check for horizontal collision when moving vertically
	if (velocity.x > 0)
	{
		newBoundsVertical.x -= 1;
	}
	else if (velocity.x < 0)
	{
		newBoundsVertical.x += 1;
		newBoundsHorizontal.x -= 1;
	}
	else
	{
		newBoundsVertical.x -= 1;
	}

	// this needs to be here so that it does not check for vertical collision when moving horizontally
	if (velocity.y > 0)
	{
		newBoundsHorizontal.y -= 1;
	}
	else if (velocity.y < 0)
	{
		newBoundsHorizontal.y += 1;
	}

	for (unsigned int i = 0; i < game.entities.size(); i++)
	{
		const SDL_Rect * theirBounds = game.entities[i]->GetBounds();

		if (game.entities[i] != this && game.entities[i]->impassable)
		{
			if (SDL_HasIntersection(&newBoundsHorizontal, theirBounds))
			{
				return true;
			}

			if (SDL_HasIntersection(&newBoundsVertical, theirBounds))
			{
				return true;
			}
		}
	}

	return false;
}

void Missile::Pause(Uint32 ticks)
{
	//std::cout << "----" << std::endl;
	//std::cout << string(50, '\n');
	//std::cout << "pause missile" << std::endl;
	timeToLive.Pause(ticks);
	if (animator != nullptr)
		animator->animationTimer.Pause(ticks);
	//std::cout << "----" << std::endl;
}

void Missile::Unpause(Uint32 ticks)
{
	//std::cout << "----" << std::endl;
	//std::cout << "unpause missile" << std::endl;
	timeToLive.Unpause(ticks);
	if (animator != nullptr)
		animator->animationTimer.Unpause(ticks);
	//std::cout << "----" << std::endl;
}