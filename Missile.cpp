#include "Missile.h"
#include "Game.h"
#include "debug_state.h"

Missile::Missile(Vector2 pos) : PhysicsEntity(pos)
{
	CreateCollider(23, 16, 0, 0, 0.75f, 0.9f);

	timeToLive.Start(2000);

	etype = "missile";

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

void Missile::Destroy()
{
	destroyed = true;
	animator->SetBool("destroyed", true);
	velocity = Vector2(0, 0);
}

void Missile::UpdatePhysics(Game& game)
{
	if (velocity.x > 0)
		flip = SDL_FLIP_NONE;
	else
		flip = SDL_FLIP_HORIZONTAL;

	// check for collisions, and destroy if it hits a wall or an enemy'
	if (CheckCollisions(game))
	{
		if (animator->GetBool("destroyed") && animator->animationTimer.HasElapsed())
		{
			shouldDelete = true;
		}
		else
		{
			Destroy();
		}
	}
	else
	{
		// move the missile
		SetPosition(Vector2(position.x + (velocity.x * (float)game.dt), position.y + (velocity.y * (float)game.dt)));
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

		if (game.entities[i]->impassable && game.entities[i] != this)
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
		else if (game.entities[i]->trigger && game.entities[i] != this)
		{
			if (SDL_HasIntersection(&newBoundsHorizontal, theirBounds))
			{
				CheckCollisionTrigger(game.entities[i], game);
			}
			else if (SDL_HasIntersection(&newBoundsVertical, theirBounds))
			{
				CheckCollisionTrigger(game.entities[i], game);
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