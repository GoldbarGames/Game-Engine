#include "Missile.h"
#include "Game.h"
#include "PhysicsInfo.h"

Missile::Missile(Vector2 pos) : Entity(pos)
{
	//TODO: Check to see whether this collider exists or if it gets replaced with the base class
	CreateCollider(0, -3, 10, 10);

	timeToLive.Start(2000);

	etype = "missile";
	physics = new PhysicsInfo(this);

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
		if (animator->GetBool("destroyed") && 
			currentSprite->HasAnimationElapsed())
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
	physics->velocity = Vector2(0, 0);
}

void Missile::SetVelocity(Vector2 newVelocity)
{
	physics->velocity = newVelocity;
	if (physics->velocity.x > 0)
		rotation = glm::vec3(0, 0, 0);
	else if (physics->velocity.x < 0)
		rotation = glm::vec3(0, 180, 0);
	else if (physics->velocity.y > 0)
		rotation = glm::vec3(0, 0, 270);
	else if (physics->velocity.y < 0)
		rotation = glm::vec3(0, 0, 90);
}

void Missile::UpdatePhysics(Game& game)
{
	// check for collisions, and destroy if it hits a wall or an enemy'
	if (CheckCollisions(game))
	{
		if (animator->GetBool("destroyed") && currentSprite->HasAnimationElapsed())
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
		SetPosition(Vector2(position.x + (physics->velocity.x * (float)game.dt), 
			position.y + (physics->velocity.y * (float)game.dt)));
	}
}

bool Missile::CheckCollisions(Game& game)
{
	if (currentSprite == nullptr && animator != nullptr)
		animator->DoState(this);

	entityPivot = currentSprite->pivot;
	CalculateCollider();

	bool horizontalCollision = false;
	bool verticalCollision = false;

	// Get bounds assuming the move is valid
	SDL_Rect myBounds = *GetBounds();

	SDL_Rect newBoundsHorizontal = myBounds;
	newBoundsHorizontal.x = myBounds.x + (physics->velocity.x * game.dt);

	SDL_Rect newBoundsVertical = myBounds;
	newBoundsVertical.y = myBounds.y + (physics->velocity.y * game.dt);

	// this needs to be here so that it does not check for horizontal collision when moving vertically
	if (physics->velocity.x > 0)
	{
		newBoundsVertical.x -= 1;
	}
	else if (physics->velocity.x < 0)
	{
		newBoundsVertical.x += 1;
		newBoundsHorizontal.x -= 1;
	}
	else
	{
		newBoundsVertical.x -= 1;
	}

	// this needs to be here so that it does not check for vertical collision when moving horizontally
	if (physics->velocity.y > 0)
	{
		newBoundsHorizontal.y -= 1;
	}
	else if (physics->velocity.y < 0)
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
				physics->CheckCollisionTrigger(game.entities[i], game);
			}
			else if (SDL_HasIntersection(&newBoundsVertical, theirBounds))
			{
				physics->CheckCollisionTrigger(game.entities[i], game);
			}
		}
	}

	return false;
}