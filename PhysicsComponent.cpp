#include "PhysicsComponent.h"
#include "Game.h"
#include "globals.h"
#include "Physics.h"
#include "SoundManager.h"
#include "Entity.h"
#include "Game.h"
#include "QuadTree.h"
#include <algorithm>

PhysicsComponent::PhysicsComponent(Entity* entity)
{
	our = entity;
}

PhysicsComponent::~PhysicsComponent()
{

}

void PhysicsComponent::SetVelocity(const Vector2& newVelocity)
{
	velocity = newVelocity;
}

float PhysicsComponent::CalcTerminalVelocity()
{
	//float density = 1.225f; // for air
	//float terminalVelocity = (2 * mass * Physics::GRAVITY) / (density * 0.294);
	return 1.5f;
}

float PhysicsComponent::CalcCollisionVelocity(PhysicsComponent* their, bool x)
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

bool PhysicsComponent::IsEntityPushingOther(const Entity& their)
{
	float diffPosX = their.GetPosition().x - our->GetPosition().x;
	return (their.physics->velocity.x > 0 && diffPosX > 0) 
		|| (their.physics->velocity.x < 0 && diffPosX < 0);
}

Entity* PhysicsComponent::CheckPrevParent()
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
		//our->GetAnimator()->SetBool("isGrounded", true);
		isGrounded = true;

		if (prevParent->physics->velocity.y > 0)
		{
			velocity.y = prevParent->physics->velocity.y;
		}
	}

	return prevParent;
}

bool PhysicsComponent::CheckCollisionHorizontal(Entity* their, Game& game)
{
	// If a big object collides against a small object,
	// the big should keep moving, and the small should move along with it.

	// If two objects of the same size collide with each other 
	// then both should stop moving immediately

	// If we are colliding with something that has no physics (such as a wall)
	// then we should not move into it
	if (their->physics == nullptr)
	{
		velocity.x = 0;
		return true;
	}
	// if one entity is moving in the direction of the other entity...
	else // if (their->physics->IsEntityPushingOther(*our))
	{
		velocity.x = their->physics->CalcCollisionVelocity(this, true);
		return (velocity.x == 0); 
	}

	return false;
}

bool PhysicsComponent::CheckVerticalJumpThru(Entity* their, Game& game)
{
	// TODO: If we don't have NPCs using this, put it in the player script
	if (our->etype == "player" && their->jumpThru)
	{
		bool holdingDown = our->GetAnimator()->GetBool("holdingDown");

		if (holdingDown)
		{
			if (pressingJumpButton)
			{
				Jump(game);
				PreviousFrameCollisions(game);
				return true;
			}

		}
	}
	
	return false;
}

bool PhysicsComponent::MoveVerticallyWithParent(Entity* their, Game& game)
{
	// Move vertically with the parent if there was one (keep this outside of the if)
	// WARNING: Do not place a vertically moving platform next to a ceiling
	// or Kaneko will go through the ceiling and get stuck there!
	// TODO: Can we think of a good way around this?

	bool b1 = useGravity;
	bool b2 = prevParent == their;
	bool b3 = prevParent != nullptr && prevParent->physics != nullptr;
	bool b4 = prevParent != nullptr && prevParent->physics->velocity.y != 0;

	if (our->etype == "player" && their->etype == "platform")
	{
		if (their->physics->velocity.y != 0)
		{
			int test = 0;
		}
	}

	if (useGravity && prevParent == their && prevParent->physics != nullptr
		&& prevParent->physics->velocity.y != 0)
	{
		velocity.y = 0;

		if (canJump) // Jumping off the parent object
		{			
			Jump(game);
			return true;
		}
		else
		{
			our->position.y = (float)their->GetBounds()->y;
			our->position.y += their->physics->CalcCollisionVelocity(this, false) * game.dt;
		}
	}

	return false;
}

bool PhysicsComponent::CheckCollisionCeiling(Game& game)
{
	//CheckCollisionTrigger(other, game);

	if (our->etype == "player")
	{
		velocity.y = 20 * Physics::GRAVITY;
		our->position.y += (velocity.y * game.dt);
	}

	// The reason we add to the position here is because we will not move
	// at all due to verticalCollision being true, so we have to move her down here

	return true;
}


bool PhysicsComponent::CheckCollisions(Game& game)
{	
	bool hadCollision = false;
	shouldStickToGround = false;

	if (our->etype == "player")
		int test = 0;

	// copy this frame into previous frame list (could be done at beginning or end)
	prevFrameCollisions = thisFrameCollisions;
	thisFrameCollisions.clear();

	our->CalculateCollider();

	Entity* prevParent = CheckPrevParent();	

	// if we were on the ground last frame, and the player wants to jump, then jump
	wasGrounded = isGrounded; // our->GetAnimator()->GetBool("isGrounded");
	//our->GetAnimator()->SetBool("isGrounded", false);
	isGrounded = false;
	
	horizontalCollision = false;
	verticalCollision = false;

	// Get bounds assuming the move is valid
	SDL_Rect myBounds = *(our->GetBounds());
	myBounds.x -= (myBounds.w / 2);
	//myBounds.y += (myBounds.h / 2);

	newBoundsHorizontal = myBounds;
	newBoundsHorizontal.x = (int)(myBounds.x + (velocity.x * game.dt));

	newBoundsVertical = myBounds;
	newBoundsVertical.y = (int)(newBoundsVertical.y + (velocity.y * game.dt)); 

	//SDL_Rect ceilingBounds = myBounds;
	//ceilingBounds.y = (int)(ceilingBounds.y + (velocity.y * game.dt));

	// THIS NEEDS TO BE HERE BECAUSE OTHERWISE THE INTERSECTION CODE WILL NOT WORK
	// SDL's intersection code returns false if our y + h = their y, but we want it to return true!
	newBoundsVertical.y += 1;

	// 2.5D look
	floorBounds = newBoundsVertical;
	floorBounds.y += 20 + (myBounds.h / 2);

	const int FLOOR_SIZE = standAboveGround ? 16 : 0;		
	floorBounds.h += FLOOR_SIZE;

	//TODO: Re-implement this
	bool fallThru = false;

	if (our->quadrant == nullptr)
		return false;

	Entity* entity = nullptr;
	std::vector<Entity*> entities;

	// We want to retrieve all entities testing against all these different bounds,
	// then remove any duplicates (keep the unique ones) to iterate over.
	game.quadTree.Retrieve(&newBoundsHorizontal, entities, &game.quadTree);
	game.quadTree.Retrieve(&newBoundsVertical, entities, &game.quadTree);
	game.quadTree.Retrieve(&floorBounds, entities, &game.quadTree);
	entities.erase(std::unique(entities.begin(), entities.end()), entities.end());

	// This is just for getting the entities so we can render the quadtree for debugging
	if (our->etype == "player")
	{
		game.quadrantEntities = std::vector<Entity*>(entities);
		//std::cout << "" << std::endl;
	}

	SDL_Rect theirBounds;
	SDL_Rect hColBounds;

	for (unsigned int i = 0; i < entities.size(); i++)
	{
		game.collisionChecks++;
		entity = entities[i];
		if (entity == our)
			continue;

		theirBounds = *(entity->GetBounds());
		theirBounds.x -= theirBounds.w;
		theirBounds.w *= 2;

		if (shouldCheckCollisions && (entity->impassable || entity->jumpThru))
		{	
			if (entity->physics != nullptr && entity->physics->isPickedUp)
				continue;

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

			if (!horizontalCollision && HasIntersection(newBoundsHorizontal, theirBounds))
			{		
				if (our->etype == "player")
					int test = 0;

				horizontalCollision = CheckCollisionHorizontal(entity, game);
				hadCollision = hadCollision || horizontalCollision;		
				hColBounds = theirBounds;
			}

			// Separate checking the ceiling vs. the floor 
			// (because we can't go up and down at the same time)
			if (velocity.y < 0)
			{
				// Moving up
				if (!entity->jumpThru && HasIntersection(newBoundsVertical, theirBounds))
				{
					verticalCollision = CheckCollisionCeiling(game);
					hadCollision = hadCollision || verticalCollision;
					our->position.y = (float)(theirBounds.y + (theirBounds.h) + myBounds.h + our->collider->offset.y);
				}
			}
			else
			{
				bool shouldIgnoreJumpThru = (entity->jumpThru &&
					our->GetAnimator()->GetBool("onLadder"));

				// We need to change this here so that when we
				// check for an intersection it is using coords
				// using a top-left origin rather than centered
				if (entity->etype == "block")
				{
					theirBounds.y -= theirBounds.h;
					//theirBounds.h *= 2;
				}

				// Moving down
				// checks the ground (using a rect that is a little bit larger
				if (!verticalCollision && !shouldIgnoreJumpThru && 
					HasIntersection(floorBounds, theirBounds))
				{
					// We want to change this back to the previous value
					// so that when we reset the player's position
					// they go to the correct spot
					// TODO: Maybe make a different function
					// that takes in centered-origin coordinates?
					if (entity->etype == "block")
					{
						theirBounds.y += theirBounds.h;
						//theirBounds.h *= 2;
					}

					hadCollision = true;
					verticalCollision = true;
					CheckCollisionTrigger(entity, game);

					//our->GetAnimator()->SetBool("isGrounded", true);
					isGrounded = true;

					if (CheckVerticalJumpThru(entity, game))
					{
						if (isGrounded != wasGrounded)
							our->GetAnimator()->SetBool("isGrounded", isGrounded);
						return hadCollision;
					}

					bool jumped = MoveVerticallyWithParent(entity, game);

					// if colliding with ground, set velocity.y to zero (we need this if statement!)				
					if (velocity.y >= 0)
					{
						//our->GetAnimator()->SetBool("isGrounded", true);
						isGrounded = true;

						//TODO: Can we do this without casting?
						// Sets the parent object that the player is standing on, if there is one, if we have not jumped
						if (entity->physics != nullptr && !jumped)
						{
							//std::cout << "set parent!" << std::endl;
							parent = entity;
							our->GetAnimator()->SetBool("hasParent", true);
						}

						// Apply friction horizontally
						ApplyFriction(0.005f);

						jumpsRemaining = 1;

						if (useGravity)
						{
							velocity.y = 0;
							// theirBounds.y - theirBounds.h gets the top of the other's bounds (center.y - half height)
							// then we subtract half our height (myBounds.h) to get our neww position on top of the other object
							// then we subtract any collider offsets (usually zero)
							// then finally we subtract the floor to get a 2.5D look
							our->position.y = (float)(theirBounds.y - theirBounds.h - myBounds.h - our->collider->offset.y - FLOOR_SIZE);

							shouldStickToGround = true;
						}
					}
					else
					{

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



			
		}
		else if (entity->trigger)
		{
			if (our->etype == "missile")
				int test = 0;

			if (HasIntersection(newBoundsHorizontal, theirBounds))
			{
				CheckCollisionTrigger(entity, game);
			}
			else if (HasIntersection(newBoundsVertical, theirBounds))
			{
				CheckCollisionTrigger(entity, game);
			}
		}
	}


	if (our->etype == "player")
	{
		//TODO: Get this from an Input Manager or something
		const Uint8* input = SDL_GetKeyboardState(NULL);
		pressingJumpButton = input[SDL_SCANCODE_X];
		//TODO: In release mode, if you have two monitors with different refresh rates,
		// it will get synced to the refresh rate of one monitor. If the game window
		// is on the wrong monitor, then it will not count the button press
	}

	jumped = false;

	if (pressingJumpButton)
	{
		Jump(game);
	}

	if (prevParent != nullptr && parent == nullptr)
	{
		//std::cout << "lost parent!" << std::endl;
	}

	// When a collision is only horizontal and not vertical,
	// keep our position set next to the horizontal collider
	if (horizontalCollision && !verticalCollision)
	{
		if (our->position.x < hColBounds.x)
		{
			our->position.x = (float)(hColBounds.x - (myBounds.w) - our->collider->offset.x);
		}
		else
		{
			our->position.x = (float)(hColBounds.x + hColBounds.w + myBounds.w + our->collider->offset.x);
		}
	}

	if (!horizontalCollision)
	{
		our->position.x += (velocity.x * game.dt);
	}

	// make sure we don't jump into a ceiling
	if (!verticalCollision || our->GetAnimator()->GetBool("onLadder"))
	{
		if ((!hadPressedJump && pressingJumpButton) && jumpsRemaining > 0)
		{

		}
		else if (!shouldStickToGround)
		{
			our->position.y += (velocity.y * game.dt);
		}
	}
	else if (jumped)
	{
		our->position.y += (velocity.y * game.dt);
	}

	if (isGrounded != wasGrounded)
		our->GetAnimator()->SetBool("isGrounded", isGrounded);

	PreviousFrameCollisions(game);

	return hadCollision;
}

void PhysicsComponent::Jump(Game& game)
{
	if (!hadPressedJump)
	{
		if (jumpsRemaining > 0 && wasGrounded)
		{
			currentJumpSpeed = 0.25f;
			game.soundManager.PlaySound("se/Jump.wav", 0);
			velocity.y = (jumpSpeed);
			jumpsRemaining--;
		}
	}
	else
	{
		if (currentJumpSpeed > 0)
		{
			//velocity.y -= currentJumpSpeed * 0.1f;
			//std::cout << currentJumpSpeed << " , " << velocity.y << std::endl;
		}
	}

	currentJumpSpeed -= 0.05f;
	jumped = true;
}

void PhysicsComponent::ApplyFriction(float friction)
{
	//TODO: Apply friction based on the tile we are colliding with
	// and maybe the object's resistance to friction

	// NOTE: Be careful, because if friction exceeds acceleration,
	// then the object won't be able to move anywhere

	if (applyFriction)
	{
		if (velocity.x > 0)
		{
			velocity.x = std::max(velocity.x - friction, 0.0f);
		}
		else if (velocity.x < 0)
		{
			velocity.x = std::min(velocity.x + friction, 0.0f);
		}
	}

}

void PhysicsComponent::PreviousFrameCollisions(Game& game)
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
			prevFrameCollisions[i]->OnTriggerExit(*our, game);
	}
}

//TODO: Move this function/logic to the Collider class?
bool PhysicsComponent::CheckCollisionTrigger(Entity* collidedEntity, Game& game)
{
	// Each frame, when we are in this function, we check to see if the collided entity is in a list.
	// If it is not, then we do OnTriggerEnter and add it to the list
	// If it is in the list, then we do OnTriggerStay
	bool hadCollision = false;

	if (collidedEntity->trigger)
	{
		hadCollision = true;
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
			collidedEntity->OnTriggerStay(*our, game);
		}
		else
		{
			collidedEntity->OnTriggerEnter(*our, game);
			thisFrameCollisions.emplace_back(collidedEntity);
		}		
	}

	return hadCollision;
}

void PhysicsComponent::Push(const Vector2& pushVelocity)
{
	// TODO: Should wind resitance be based on mass, rather than a totally neww number?
	// Also, should there be a discrete number of levels of mass (Light, Medium, Heavy) for consistency?
	if (our->GetAnimator() != nullptr)
	{
		isPushed = true;
		//our->GetAnimator()->SetBool("isPushed", true);
	}

	velocity.x += pushVelocity.x / windResistance;
	velocity.y += pushVelocity.y / windResistance;
}

void PhysicsComponent::Update(Game& game)
{
	hadCollisionsThisFrame = false;
	previousVelocity = velocity;

	if (our->etype != "player" && our->position.y > game.deathBarrierY)
	{
		if (respawnOnDeath)
		{
			our->position = our->startPosition;
			velocity = Vector2(0, 0);
			acceleration = Vector2(0, 0);
		}
		else
		{
			our->shouldDelete = true;
		}

		return;
	}

	if (useGravity)
	{
		if (velocity.y < CalcTerminalVelocity())
			velocity.y += Physics::GRAVITY * game.dt;
	}

	hadCollisionsThisFrame = CheckCollisions(game);

	// TODO: Do we want to do things this way?
	// This pushes the object X tiles forward and then abruptly stops
	// rather than letting friction cause the object to slow down and stop
	/*
	if (hitByPushSpell)
	{
		const int NUM_TILES = 8;
		totalDistancePushed += abs(velocity.x * game.dt);
		if (totalDistancePushed > NUM_TILES * TILE_SIZE)
		{
			hitByPushSpell = false;
			velocity.x = 0;
		}
	}
	*/

	if (our->GetAnimator() != nullptr)
		our->GetAnimator()->Update(*our);
}