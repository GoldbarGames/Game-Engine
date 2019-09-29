#include "PhysicsEntity.h"
#include "Game.h"
#include "debug_state.h"
#include "Physics.h"

PhysicsEntity::PhysicsEntity(Vector2 pos) : Entity(pos)
{
	CreateCollider(0, 0, 0, 0, 1, 1);
}

PhysicsEntity::~PhysicsEntity()
{

}

const SDL_Rect* PhysicsEntity::GetColliderBounds()
{
	return collisionBounds;
}

void PhysicsEntity::SetVelocity(Vector2 newVelocity)
{
	velocity = newVelocity;
}

Vector2 PhysicsEntity::GetCenter()
{
	float x = position.x + (collisionBounds->w / 2.0f);
	float y = position.y + (collisionBounds->h / 2.0f);
	return Vector2(x, y);
}

void PhysicsEntity::CreateCollider(float startX, float startY, float x, float y, float w, float h)
{
	if (collider != nullptr)
		delete collider;

	collider = new SDL_Rect();
	collider->x = x;
	collider->y = y;
	collider->w = 1;
	collider->h = 1;

	colliderWidth = w;
	colliderHeight = h;

	if (collisionBounds != nullptr)
		delete collisionBounds;

	collisionBounds = new SDL_Rect();
	collisionBounds->x = x;
	collisionBounds->y = y;
	collisionBounds->w = 1;
	collisionBounds->h = 1;

	startSpriteSize.x = startX * Renderer::GetScale();
	startSpriteSize.y = startY * Renderer::GetScale();
}

void PhysicsEntity::Pause(Uint32 ticks)
{
	if (animator != nullptr)
		animator->animationTimer.Pause(ticks);
}

void PhysicsEntity::Unpause(Uint32 ticks)
{
	if (animator != nullptr)
		animator->animationTimer.Unpause(ticks);
}

void PhysicsEntity::CheckCollisions(Game& game)
{
	// copy this frame into previous frame list (could be done at beginning or end)
	prevFrameCollisions = thisFrameCollisions;
	thisFrameCollisions.clear();

	CalculateCollider(game.camera);

	bool horizontalCollision = false;
	bool verticalCollision = false;

	// Get bounds assuming the move is valid
	SDL_Rect myBounds = *GetColliderBounds();

	SDL_Rect newBoundsHorizontal = myBounds;
	newBoundsHorizontal.x = myBounds.x + (velocity.x * game.dt);

	SDL_Rect newBoundsVertical = myBounds;
	newBoundsVertical.y = myBounds.y + (velocity.y * game.dt);

	SDL_Rect floorBounds = newBoundsVertical;
	floorBounds.h += 20; // (int)(newBoundsVertical.h * 0.25f);

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

	animator->SetBool("isGrounded", false);

	for (unsigned int i = 0; i < game.entities.size(); i++)
	{
		//if (horizontalCollision && verticalCollision)
		//	break;

		const SDL_Rect * theirBounds = game.entities[i]->GetBounds();

		if (game.entities[i] != this)
		{
			if (game.entities[i]->impassable)
			{
				if (!horizontalCollision && SDL_HasIntersection(&newBoundsHorizontal, theirBounds))
				{
					horizontalCollision = true;
					velocity.x = 0;
				}

				// checks the ground (using a rect that is a little bit larger
				if (!verticalCollision && SDL_HasIntersection(&floorBounds, theirBounds))
				{
					verticalCollision = true;
					CheckCollisionTrigger(game.entities[i], game);

					// if colliding with ground, set velocity.y to zero
					if (velocity.y > 0)
					{
						animator->SetBool("isGrounded", true);
						jumpsRemaining = 2;

						// this needs to be here to fix the collision with the ground
						if (position.y + myBounds.h > theirBounds->y + theirBounds->h + 1)
							position.y -= floorBounds.y + floorBounds.h - theirBounds->y - 1;

						velocity.y = 0;
					}
				}

				// checks the ceiling
				if (!verticalCollision && SDL_HasIntersection(&newBoundsVertical, theirBounds))
				{
					verticalCollision = true;
					CheckCollisionTrigger(game.entities[i], game);
				}
			}
			else if (game.entities[i]->trigger)
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
	}

	if (!horizontalCollision)
	{
		position.x += (velocity.x * game.dt);
	}

	if (!verticalCollision || animator->GetBool("onLadder"))
	{
		if (game.pressedJumpButton && jumpsRemaining > 0)
		{
			jumpsRemaining--;
		}

		position.y += (velocity.y * game.dt);
	}

	// Remove deleted objects from prevFrameCollisions
	unsigned int k = 0;
	while (k < prevFrameCollisions.size())
	{
		if (prevFrameCollisions[k]->shouldDelete)
		{
			prevFrameCollisions.erase(prevFrameCollisions.begin() + k);
		}
		else
		{
			k++;
		}			
	}

	// Now we go over the list
	// If there was an entity in the collision list that we did not collide with this frame, then call OnTriggerExit
	// (maybe have two lists? keep track of this frame and the previous one?)
	for (unsigned int i = 0; i < prevFrameCollisions.size(); i++)
	{
		bool triggerExit = true;
		for (unsigned int k = 0; k < thisFrameCollisions.size(); k++)
		{
			if (thisFrameCollisions[k] == prevFrameCollisions[i])
			{
				triggerExit = false;
				break;
			}
		}

		if (triggerExit && prevFrameCollisions[i] != nullptr)
			prevFrameCollisions[i]->OnTriggerExit(this, game);
	}
}

void PhysicsEntity::CheckCollisionTrigger(Entity* collidedEntity, Game& game)
{
	// Each frame, when we are in this function, we check to see if the collided entity is in a list.
	// If it is not, then we do OnTriggerEnter and add it to the list
	// If it is in the list, then we do OnTriggerStay

	if (collidedEntity->trigger)
	{
		bool collisionStay = false;
		for (unsigned int i = 0; i < thisFrameCollisions.size(); i++)
		{
			if (thisFrameCollisions[i] == collidedEntity)
			{
				collisionStay = true;
				break;
			}
		}

		if (collisionStay)
		{
			collidedEntity->OnTriggerStay(this, game);
		}
		else
		{
			collidedEntity->OnTriggerEnter(this, game);
		}
		thisFrameCollisions.emplace_back(collidedEntity);
	}
}

void PhysicsEntity::CalculateCollider(Vector2 cameraOffset)
{
	// set the collision bounds position to where the player actually is
	collisionBounds->x = position.x + collider->x - cameraOffset.x;
	collisionBounds->y = position.y + collider->y - cameraOffset.y;

	// scale the bounds of the sprite by a number to set the collider's width and height
	collisionBounds->w = startSpriteSize.x * colliderWidth;
	collisionBounds->h = startSpriteSize.y * colliderHeight;
}

Vector2 PhysicsEntity::CalcScaledPivot()
{
	if (flip == SDL_FLIP_HORIZONTAL)
	{
		entityPivot.x = (currentSprite->windowRect.w / Renderer::GetScale()) - currentSprite->pivot.x;
	}

	// scale the pivot and subtract it from the collision center
	return Vector2(entityPivot.x * Renderer::GetScale(), currentSprite->pivot.y * Renderer::GetScale());
}

void PhysicsEntity::Update(Game& game)
{
	if (usePhysics)
	{
		if (velocity.y < 1)
			velocity.y += Physics::GRAVITY;
	}	

	CheckCollisions(game);

	if (animator != nullptr)
		animator->Update(this);
}

void PhysicsEntity::Render(Renderer * renderer, Vector2 cameraOffset)
{
	if (currentSprite != nullptr)
	{
		entityPivot = currentSprite->pivot;

		// Get center of the white collision box, and use it as a vector2
		float collisionCenterX = (collisionBounds->x + (collisionBounds->w / 2));
		float collisionCenterY = (collisionBounds->y + (collisionBounds->h / 2));
		Vector2 collisionCenter = Vector2(collisionCenterX, collisionCenterY);

		Vector2 scaledPivot = CalcScaledPivot();
		Vector2 offset = collisionCenter - scaledPivot;

		if (GetModeEdit())
		{
			if (animator != nullptr)
				currentSprite->Render(position - cameraOffset, animator->speed, animator->animationTimer.GetTicks(), flip, renderer);
			else
				currentSprite->Render(position - cameraOffset, 0, -1, flip, renderer);
		}
		else
		{
			if (animator != nullptr)
				currentSprite->Render(offset, animator->speed, animator->animationTimer.GetTicks(), flip, renderer);
			else
				currentSprite->Render(offset, 0, -1, flip, renderer);
		}

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
