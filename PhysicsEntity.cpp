#include "PhysicsEntity.h"
#include "Game.h"
#include "debug_state.h"
#include "Physics.h"

PhysicsEntity::PhysicsEntity(Vector2 pos) : Entity(pos)
{
	CreateCollider(0, 0, 0, 0, 1, 1);
	isPhysicsEntity = true;
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

	colliderWidth = w * Renderer::GetScale();
	colliderHeight = h * Renderer::GetScale();

	if (collisionBounds != nullptr)
		delete collisionBounds;

	collisionBounds = new SDL_Rect();
	collisionBounds->x = x;
	collisionBounds->y = y;
	collisionBounds->w = 1;
	collisionBounds->h = 1;

	//startSpriteSize.x = startX * Renderer::GetScale();
	//startSpriteSize.y = startY * Renderer::GetScale();
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

	PhysicsEntity* prevParent = parent;
	parent = nullptr;

	if (prevParent != nullptr)
	{
		// If parent is moving and we are not, then make us move with parent
		// Otherwise, just move according to our own speed
		if (prevParent->velocity.x != 0 && velocity.x == 0)
		{
			velocity.x = prevParent->velocity.x;
		}

		if (prevParent->velocity.y != 0)
		{
			velocity.y = prevParent->velocity.y;
		}
	}

	animator->SetBool("isGrounded", false);

	if (prevParent != nullptr && prevParent->velocity.y != 0)
	{
		animator->SetBool("isGrounded", true);

		if (prevParent->velocity.y > 0)
		{
			velocity.y = prevParent->velocity.y;
		}
	}

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
		newBoundsHorizontal.x += 1;
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

	SDL_Rect floorBounds = newBoundsVertical;

	newBoundsVertical.y -= 1;

	if (etype == "player" || etype == "npc")
		floorBounds.h += 20; // (int)(newBoundsVertical.h * 0.25f);



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

					animator->SetBool("isGrounded", true);

					// Move vertically with the parent if there was one (keep this outside of the if)
					// WARNING: Do not place a vertically moving platform next to a ceiling
					// or Kaneko will go through the ceiling and get stuck there!
					// TODO: Can we think of a good way around this?
					if (prevParent == game.entities[i] && prevParent->velocity.y != 0)
					{
						position.y -= floorBounds.y + floorBounds.h - theirBounds->y + 1;
					}						

					// if colliding with ground, set velocity.y to zero (we need this if statement!)				
					if (velocity.y > -0.01f)
					{
						animator->SetBool("isGrounded", true);
						
						jumpsRemaining = 2;

						// this needs to be here to fix the collision with the ground
						if (position.y + myBounds.h > theirBounds->y + theirBounds->h + 1)
						{
							position.y -= floorBounds.y + floorBounds.h - theirBounds->y - 1;
						}

						//TODO: Can we do this without casting?
						if (game.entities[i]->isPhysicsEntity)
							parent = static_cast<PhysicsEntity*>(game.entities[i]);

						velocity.y = 0;
					}

					// push Kaneko out of a ceiling just in case she gets stuck there
					// TODO: Make it so that we don't need to do this!
					if (SDL_HasIntersection(&newBoundsVertical, theirBounds))
					{
						velocity.y = 60 * Physics::GRAVITY;
						CheckCollisionTrigger(game.entities[i], game);
						position.y += (velocity.y * game.dt);
					}
				}

				// checks the ceiling (don't know how necessary this realy is)			
				if (!verticalCollision && SDL_HasIntersection(&newBoundsVertical, theirBounds))
				{
					verticalCollision = true;
					velocity.y = 20 * Physics::GRAVITY;
					CheckCollisionTrigger(game.entities[i], game);
					position.y += (velocity.y * game.dt);
					// The reason we add to the position here is because we will not move
					// at all due to verticalCollision being true, so we have to move her down here
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
		if ((!hadPressedJump && pressingJumpButton) && jumpsRemaining > 0)
		{
			jumpsRemaining--;
		}

		position.y += (velocity.y * game.dt);
	}

	PreviousFrameCollisions(game);
}

void PhysicsEntity::PreviousFrameCollisions(Game& game)
{
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
	collisionBounds->x = position.x - cameraOffset.x + collider->x;
	collisionBounds->y = position.y - cameraOffset.y + collider->y;

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

void PhysicsEntity::Push(Vector2 pushVelocity)
{
	velocity = pushVelocity;
}

void PhysicsEntity::Update(Game& game)
{
	if (useGravity)
	{
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
		Vector2 collisionCenter = Vector2(collisionCenterX + collider->x, collisionCenterY + collider->y);

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
