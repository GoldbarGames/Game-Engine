#include "PhysicsEntity.h"
#include "Game.h"
#include "debug_state.h"
#include "Physics.h"

PhysicsEntity::PhysicsEntity(Entity* entity)
{
	our = entity;
}

PhysicsEntity::~PhysicsEntity()
{

}

void PhysicsEntity::SetVelocity(Vector2 newVelocity)
{
	velocity = newVelocity;
}

float PhysicsEntity::CalcCollisionVelocity(PhysicsEntity* their, bool x)
{
	if (x)
	{
		if (mass > their->mass)
			return velocity.x;
		else if (mass == their->mass)
			return 0;
		else
			return their->velocity.x;
	}
	else
	{
		if (mass > their->mass)
			return velocity.y;
		else if (mass == their->mass)
			return 0;
		else
			return their->velocity.y;
	}
	
}

bool PhysicsEntity::IsEntityPushingOther(Entity* their, bool x)
{
	if (x)
	{
		float diffPosX = their->GetPosition().x - our->GetPosition().x;
		return (velocity.x > 0 && diffPosX > 0) || (velocity.x < 0 && diffPosX < 0);
	}
	else
	{
		return 0; //TODO: Do we even need this here at all?
	}	
}

Entity* PhysicsEntity::CheckPrevParent()
{
	our->GetAnimator()->SetBool("hasParent", false);

	Entity* prevParent = parent;
	parent = nullptr;

	if (prevParent != nullptr && prevParent->physics != nullptr)
	{
		// If parent is moving and we are not, then make us move with parent
		// Otherwise, just move according to our own speed
		if (prevParent->physics->velocity.x != 0 && velocity.x == 0)
		{
			velocity.x = prevParent->physics->velocity.x;
		}

		if (prevParent->physics->velocity.y != 0)
		{
			velocity.y = prevParent->physics->velocity.y;
		}
	}

	if (prevParent != nullptr && prevParent->physics->velocity.y != 0)
	{
		our->GetAnimator()->SetBool("isGrounded", true);

		if (prevParent->physics->velocity.y > 0)
		{
			velocity.y = prevParent->physics->velocity.y;
		}
	}

	return prevParent;
}

bool PhysicsEntity::CheckCollisionHorizontal(Entity* their, Game& game)
{
	// if one entity is moving in the direction of the other entity...
	if (their->IsEntityPushingOther(this, true))
	{
		velocity.x = their->CalcCollisionVelocity(this, true);
		return (velocity.x == 0);
	}
	else
	{
		velocity.x = 0;
		return true;
	}

	return false;
}

bool PhysicsEntity::CheckVerticalJumpThru(Entity* their, Game& game)
{
	// TODO: If we don't have NPCs using this, put it in the player script
	if (our->etype == "player" && their->jumpThru)
	{
		bool holdingDown = our->GetAnimator()->GetBool("holdingDown");

		if (holdingDown)
		{
			if ((!hadPressedJump && pressingJumpButton))
			{
				jumpsRemaining--;
				our->position.y -= JUMP_SPEED * game.dt;
				PreviousFrameCollisions(game);
				return true;
			}
		}
	}
	
	return false;
}

bool PhysicsEntity::MoveVerticallyWithParent(Entity* their, Game& game)
{
	// Move vertically with the parent if there was one (keep this outside of the if)
	// WARNING: Do not place a vertically moving platform next to a ceiling
	// or Kaneko will go through the ceiling and get stuck there!
	// TODO: Can we think of a good way around this?
	if (useGravity && prevParent == their && prevParent->physics != nullptr
		&& prevParent->physics->velocity.y != 0)
	{
		velocity.y = 0;

		if (canJump)
		{			
			jumpsRemaining--;
			our->position.y += (JUMP_SPEED * game.dt);
			return true;
		}
		else
		{
			our->position.y = (float)their->GetBounds()->y;
			our->position.y += their->CalcCollisionVelocity(this, false) * game.dt;
		}
	}

	return false;
}

bool PhysicsEntity::CheckCollisionCeiling(Entity* other, Game& game)
{
	CheckCollisionTrigger(our, game);

	if (our->etype == "player")
	{
		velocity.y = 20 * Physics::GRAVITY;
		our->position.y += (velocity.y * game.dt);
	}

	// The reason we add to the position here is because we will not move
	// at all due to verticalCollision being true, so we have to move her down here

	return true;
}


void PhysicsEntity::CheckCollisions(Game& game)
{
	if (our->etype == "player")
		int test = 0;

	shouldStickToGround = false;

	// copy this frame into previous frame list (could be done at beginning or end)
	prevFrameCollisions = thisFrameCollisions;
	thisFrameCollisions.clear();

	our->CalculateCollider();

	Entity* prevParent = CheckPrevParent();	

	// if we were on the ground last frame, and the player wants to jump, then jump
	bool wasGrounded = our->GetAnimator()->GetBool("isGrounded");

	our->GetAnimator()->SetBool("isGrounded", false);
	
	bool horizontalCollision = false;
	bool verticalCollision = false;

	//const int TARGET_FPS = 30;

	// Get bounds assuming the move is valid
	SDL_Rect myBounds = *(our->GetBounds());
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

		if (entity == our)
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

				if (our->etype == "player")
					int test = 0;

				horizontalCollision = CheckCollisionHorizontal(entity, game);

				if (velocity.x > 0)
					our->position.x = (float)(theirBounds.x - myBounds.w - our->colliderOffset.x);
				else if (velocity.x < 0)
					our->position.x = (float)(theirBounds.x + theirBounds.w + our->colliderOffset.x);
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

				our->GetAnimator()->SetBool("isGrounded", true);

				if (CheckVerticalJumpThru(entity, game))
					return;

				bool jumped = MoveVerticallyWithParent(entity, game);

				// if colliding with ground, set velocity.y to zero (we need this if statement!)				
				if (velocity.y >= 0)
				{
					our->GetAnimator()->SetBool("isGrounded", true);

					//TODO: Can we do this without casting?
					// Sets the parent object that the player is standing on, if there is one, if we have not jumped
					if (entity->physics != nullptr && !jumped)
					{
						//std::cout << "set parent!" << std::endl;
						parent = entity;
						our->GetAnimator()->SetBool("hasParent", true);
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
				if (our->etype == "player")
					int test = 0;
			}
			else
			{
				if (our->etype == "player")
					int test = 0;
			}
		}
		else if (entity->trigger)
		{

			if (our->etype == "player")
				int test = 0;

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
		our->position.x += (velocity.x * game.dt);
	}

	if (!verticalCollision || our->GetAnimator()->GetBool("onLadder"))
	{
		if ((!hadPressedJump && pressingJumpButton) && jumpsRemaining > 0)
		{
			jumpsRemaining--;
			our->position.y += JUMP_SPEED * game.dt;
			
		}
		else if (!shouldStickToGround)
		{
			//if (etype == "player")
			//	std::cout << "position.y += " << velocity.y << " * " << game.dt << std::endl;

			our->position.y += (velocity.y * game.dt);
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
			prevFrameCollisions[i]->OnTriggerExit(our, game);
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
			collidedEntity->OnTriggerStay(our, game);
		}
		else
		{
			collidedEntity->OnTriggerEnter(our, game);
		}

		thisFrameCollisions.emplace_back(collidedEntity);
	}
}

Vector2 PhysicsEntity::CalcScaledPivot()
{
	if (our->flip == SDL_FLIP_HORIZONTAL)
	{
		//entityPivot.x = (currentSprite->windowRect.w) - currentSprite->pivot.x;
	}

	// scale the pivot and subtract it from the collision center
	return Vector2(our->entityPivot.x, our->GetSprite()->pivot.y);
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

	if (our->GetAnimator() != nullptr)
		our->GetAnimator()->Update(our);
}

void PhysicsEntity::RenderDebug(Renderer* renderer)
{

}

void PhysicsEntity::Render(Renderer* renderer)
{

}
