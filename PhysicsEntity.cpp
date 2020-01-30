#include "PhysicsEntity.h"
#include "Game.h"
#include "debug_state.h"
#include "Physics.h"

PhysicsEntity::PhysicsEntity(Vector2 pos) : Entity(pos)
{
	startPosition = pos;
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
	return Vector2(position.x, position.y);
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

float PhysicsEntity::CalcCollisionVelocity(PhysicsEntity* other, bool x)
{
	if (x)
	{
		if (mass > other->mass)
			return velocity.x;
		else if (mass == other->mass)
			return 0;
		else
			return other->velocity.x;
	}
	else
	{
		if (mass > other->mass)
			return velocity.y;
		else if (mass == other->mass)
			return 0;
		else
			return other->velocity.y;
	}
	
}

bool PhysicsEntity::IsEntityPushingOther(PhysicsEntity* other, bool x)
{
	if (x)
	{
		float diffPosX = other->GetPosition().x - position.x;
		return (velocity.x > 0 && diffPosX > 0) || (velocity.x < 0 && diffPosX < 0);
	}
	else
	{
		return 0; //TODO: Do we even need this here at all?
	}	
}

PhysicsEntity* PhysicsEntity::CheckPrevParent()
{
	animator->SetBool("hasParent", false);

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

	if (prevParent != nullptr && prevParent->velocity.y != 0)
	{
		animator->SetBool("isGrounded", true);

		if (prevParent->velocity.y > 0)
		{
			velocity.y = prevParent->velocity.y;
		}
	}

	return prevParent;
}

bool PhysicsEntity::CheckCollisionHorizontal(Entity* entity, Game& game)
{
	// if one entity is moving in the direction of the other entity...
	if (entity->IsEntityPushingOther(this, true))
	{
		velocity.x = entity->CalcCollisionVelocity(this, true);
		return (velocity.x == 0);
	}
	else
	{
		velocity.x = 0;
		return true;
	}

	return false;
}

bool PhysicsEntity::CheckVerticalJumpThru(Entity* entity, Game& game)
{
	// TODO: If we don't have NPCs using this, put it in the player script
	if (etype == "player" && entity->jumpThru)
	{
		bool holdingDown = animator->GetBool("holdingDown");

		if (holdingDown)
		{
			if ((!hadPressedJump && pressingJumpButton))
			{
				jumpsRemaining--;
				position.y -= JUMP_SPEED * game.dt;
				PreviousFrameCollisions(game);
				return true;
			}
		}
	}
	
	return false;
}

bool PhysicsEntity::MoveVerticallyWithParent(Entity* entity, Game& game)
{
	// Move vertically with the parent if there was one (keep this outside of the if)
	// WARNING: Do not place a vertically moving platform next to a ceiling
	// or Kaneko will go through the ceiling and get stuck there!
	// TODO: Can we think of a good way around this?
	if (useGravity && prevParent == entity && prevParent->velocity.y != 0)
	{
		velocity.y = 0;

		if (canJump)
		{			
			jumpsRemaining--;
			position.y += (JUMP_SPEED * game.dt);
			return true;
		}
		else
		{
			position.y = (float)GetColliderBounds()->y;
			position.y += entity->CalcCollisionVelocity(this, false) * game.dt;
		}
	}

	return false;
}

bool PhysicsEntity::CheckCollisionCeiling(Entity* entity, Game& game)
{
	CheckCollisionTrigger(entity, game);

	if (etype == "player")
	{
		velocity.y = 20 * Physics::GRAVITY;
		position.y += (velocity.y * game.dt);
	}

	// The reason we add to the position here is because we will not move
	// at all due to verticalCollision being true, so we have to move her down here

	return true;
}


void PhysicsEntity::CheckCollisions(Game& game)
{
	if (etype == "player")
		int test = 0;

	shouldStickToGround = false;

	// copy this frame into previous frame list (could be done at beginning or end)
	prevFrameCollisions = thisFrameCollisions;
	thisFrameCollisions.clear();

	CalculateCollider();

	PhysicsEntity* prevParent = CheckPrevParent();	

	// if we were on the ground last frame, and the player wants to jump, then jump
	bool wasGrounded = animator->GetBool("isGrounded");

	animator->SetBool("isGrounded", false);
	
	bool horizontalCollision = false;
	bool verticalCollision = false;

	//const int TARGET_FPS = 30;

	// Get bounds assuming the move is valid
	SDL_Rect myBounds = *GetColliderBounds();
	myBounds.x -= (myBounds.w / 2);
	myBounds.y += (myBounds.h / 2);

	SDL_Rect newBoundsHorizontal = myBounds;
	newBoundsHorizontal.x = (int)(myBounds.x + (velocity.x * game.dt));

	SDL_Rect newBoundsVertical = myBounds;
	newBoundsVertical.y = (int)(myBounds.y + (velocity.y * game.dt));

	// THIS NEEDS TO BE HERE BECAUSE OTHERWISE THE INTERSECTION CODE WILL NOT WORK
	// SDL's intersection code returns false if our y + h = their y, but we want it to return true!
	newBoundsVertical.y += 1;

	SDL_Rect floorBounds = newBoundsVertical;
	//floorBounds.y += 1;

	// 2.5D look
	const int FLOOR_SIZE = 0;

	if (standAboveGround)
		floorBounds.h += FLOOR_SIZE; // (int)(newBoundsVertical.h * 0.25f);

	bool fallThru = false;

	for (unsigned int i = 0; i < game.entities.size(); i++)
	{
		Entity* entity = game.entities[i];

		SDL_Rect theirBounds = *(entity->GetBounds());
		theirBounds.w *= 2;
		theirBounds.x -= (theirBounds.w/2);

		if (entity == this)
			continue;

		if (entity->impassable || entity->jumpThru)
		{	
			if (entity->jumpThru)
			{
				// Check to see if we are on top of the platform
				// (our bottom edge is above the other's top edge)
				// If so, we deal with collision as normal
				// Else, we ignore the collider

				bool onTopOfPlatform = myBounds.y + myBounds.h < theirBounds.y;
				if (!onTopOfPlatform)
				{
					continue;
				}
			}


			if (!horizontalCollision && SDL_HasIntersection(&newBoundsHorizontal, &theirBounds))
			{		

				if (etype == "player")
					int test = 0;

				horizontalCollision = CheckCollisionHorizontal(entity, game);

				if (velocity.x > 0)
					position.x = (float)(theirBounds.x - myBounds.w - colliderOffset.x);
				else if (velocity.x < 0)
					position.x = (float)(theirBounds.x + theirBounds.w + colliderOffset.x);
			}

			// checks the ceiling
			//TODO: To check for ceiling collisions, use a collider that is closer to the top rather than the bottom
			
			/*
			newBoundsVertical.h -= 4;
			if (!verticalCollision && SDL_HasIntersection(&newBoundsVertical, theirBounds))
			{
				verticalCollision = CheckCollisionCeiling(entity, game);
				if (verticalCollision && etype == "player")
					std::cout << "ceiling collision!" << std::endl;
			}
			newBoundsVertical.h += 4;
			*/

			// checks the ground (using a rect that is a little bit larger
			if (!verticalCollision && SDL_HasIntersection(&floorBounds, &theirBounds))
			{
				verticalCollision = true;
				CheckCollisionTrigger(entity, game);

				animator->SetBool("isGrounded", true);

				if (CheckVerticalJumpThru(entity, game))
					return;

				bool jumped = MoveVerticallyWithParent(entity, game);

				// if colliding with ground, set velocity.y to zero (we need this if statement!)				
				if (velocity.y >= 0)
				{
					animator->SetBool("isGrounded", true);

					//TODO: Can we do this without casting?
					// Sets the parent object that the player is standing on, if there is one, if we have not jumped
					if (entity->isPhysicsEntity && !jumped)
					{
						//std::cout << "set parent!" << std::endl;
						parent = static_cast<PhysicsEntity*>(entity);
						animator->SetBool("hasParent", true);
					}

					jumpsRemaining = 1;

					if (useGravity)
					{
						velocity.y = 0;
						//if (standAboveGround)
						//	position.y = (float)(theirBounds->y - myBounds.h - FLOOR_SIZE - colliderOffset.y);
						//else
						//	position.y = (float)(theirBounds->y - myBounds.h - colliderOffset.y);
						shouldStickToGround = true;
					}						
				}
				else
				{
					// push Kaneko out of a ceiling just in case she gets stuck there
					// TODO: Make it so that we don't need to do this!
					/*
					if (SDL_HasIntersection(&newBoundsVertical, theirBounds))
					{
						velocity.y = 60 * Physics::GRAVITY;
						CheckCollisionTrigger(entity, game);
						position.y += (velocity.y * game.dt);
					}
					*/
				}		
			}
			else if (!verticalCollision)
			{
				if (etype == "player")
					int test = 0;
			}
			else
			{
				if (etype == "player")
					int test = 0;
			}
		}
		else if (entity->trigger)
		{
			if (SDL_HasIntersection(&newBoundsHorizontal, &theirBounds))
			{
				CheckCollisionTrigger(entity, game);
			}
			else if (SDL_HasIntersection(&newBoundsVertical, &theirBounds))
			{
				CheckCollisionTrigger(entity, game);
			}
		}
	}

	if ((!hadPressedJump && pressingJumpButton) && jumpsRemaining > 0 && wasGrounded)
	{
		jumpsRemaining--;
		velocity.y = JUMP_SPEED;
	}

	if (prevParent != nullptr && parent == nullptr)
	{
		//std::cout << "lost parent!" << std::endl;
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
			position.y += JUMP_SPEED * game.dt;
			
		}
		else if (!shouldStickToGround)
		{
			//if (etype == "player")
			//	std::cout << "position.y += " << velocity.y << " * " << game.dt << std::endl;

			position.y += (velocity.y * game.dt);
		}

		//if (etype == "player")
		//	std::cout << "v: " << velocity.y << std::endl;

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

Vector2 PhysicsEntity::CalcScaledPivot()
{
	if (flip == SDL_FLIP_HORIZONTAL)
	{
		//entityPivot.x = (currentSprite->windowRect.w) - currentSprite->pivot.x;
	}

	// scale the pivot and subtract it from the collision center
	return Vector2(entityPivot.x, currentSprite->pivot.y);
}

void PhysicsEntity::Push(Vector2 pushVelocity)
{
	velocity = pushVelocity;
	hitByPushSpell = true;
	totalDistancePushed = 0;
}

void PhysicsEntity::Update(Game& game)
{
	if (useGravity)
	{
		velocity.y += Physics::GRAVITY;
	}

	CheckCollisions(game);

	if (hitByPushSpell)
	{
		const int NUM_TILES = 2;
		totalDistancePushed += abs(velocity.x * game.dt);
		if (totalDistancePushed > NUM_TILES * TILE_SIZE)
		{
			hitByPushSpell = false;
			velocity.x = 0;
		}
	}

	if (animator != nullptr)
		animator->Update(this);
}

void PhysicsEntity::RenderDebug(Renderer* renderer)
{
	if (GetModeDebug())
	{
		if (renderer->debugSprite != nullptr && renderer->IsVisible(layer))
		{
			float rWidth = renderer->debugSprite->texture->GetWidth();
			float rHeight = renderer->debugSprite->texture->GetHeight();

			float targetWidth = GetSprite()->frameWidth;
			float targetHeight = GetSprite()->frameHeight;

			if (impassable)
				renderer->debugSprite->color = { 255, 0, 0, 255 };
			else
				renderer->debugSprite->color = { 0, 255, 0, 255 };

			renderer->debugSprite->pivot = GetSprite()->pivot;
			renderer->debugSprite->SetScale(Vector2(targetWidth / rWidth, targetHeight / rHeight));
			renderer->debugSprite->Render(position, 0, -1, flip, renderer, 0);

			if (etype == "player")
				int test = 0;

			// draw collider
			targetWidth = collisionBounds->w;
			targetHeight = collisionBounds->h;

			renderer->debugSprite->color = { 255, 255, 255, 255 };
			renderer->debugSprite->pivot = GetSprite()->pivot;
			renderer->debugSprite->SetScale(Vector2(targetWidth / rWidth, targetHeight / rHeight));

			Vector2 colliderPosition = Vector2(position.x + colliderOffset.x, position.y + colliderOffset.y);
			renderer->debugSprite->Render(colliderPosition, 0, -1, flip, renderer, 0);
		}
	}

	/*
	SDL_SetRenderDrawColor(renderer->renderer, 255, 255, 255, 255);

	SDL_RenderDrawRect(renderer->renderer, collisionBounds);
	SDL_SetRenderDrawColor(renderer->renderer, 0, 0, 0, 255);
	*/
}

void PhysicsEntity::Render(Renderer* renderer)
{
	if (currentSprite != nullptr)
	{
		//TODO: What is all of this code for? Why do we need this offset?
		// Is it so that when you turn around, the collision box always stays centered?
		entityPivot = currentSprite->pivot;

		// Get center of the white collision box, and use it as a vector2
		float collisionCenterX = (collisionBounds->x + (collisionBounds->w / 2.0f));
		float collisionCenterY = (collisionBounds->y + (collisionBounds->h / 2.0f));
		Vector2 collisionCenter = Vector2(collisionCenterX + colliderOffset.x, collisionCenterY + colliderOffset.y);

		Vector2 scaledPivot = CalcScaledPivot();
		Vector2 offset = collisionCenter - scaledPivot;

		if (GetModeEdit())
		{
			if (animator != nullptr)
				currentSprite->Render(position, animator->GetSpeed(), animator->animationTimer.GetTicks(), flip, renderer, 0);
			else
				currentSprite->Render(position, 0, -1, flip, renderer, 0);
		}
		else // use offset here?
		{
			if (animator != nullptr)
				currentSprite->Render(position, animator->GetSpeed(), animator->animationTimer.GetTicks(), flip, renderer, 0);
			else
				currentSprite->Render(position, 0, -1, flip, renderer, 0);
		}

		if (GetModeDebug())
		{
			RenderDebug(renderer);
		}
	}

}
