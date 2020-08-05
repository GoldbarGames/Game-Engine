#include "PhysicsComponent.h"
#include "Game.h"
#include "globals.h"
#include "Physics.h"

PhysicsComponent::PhysicsComponent(Entity* entity)
{
	our = entity;
	startPosition = entity->position;
}

PhysicsComponent::~PhysicsComponent()
{

}

void PhysicsComponent::SetVelocity(Vector2 newVelocity)
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

bool PhysicsComponent::IsEntityPushingOther(Entity* their, bool x)
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
	if (their->physics == nullptr)
	{
		velocity.x = 0;
		return true;
	}
	// if one entity is moving in the direction of the other entity...
	else if (their->physics->IsEntityPushingOther(their, true))
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
			if ((!hadPressedJump && pressingJumpButton))
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

bool PhysicsComponent::CheckCollisionCeiling(Entity* other, Game& game)
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


bool PhysicsComponent::CheckCollisions(Game& game)
{	
	bool hadCollision = false;
	shouldStickToGround = false;

	// copy this frame into previous frame list (could be done at beginning or end)
	prevFrameCollisions = thisFrameCollisions;
	thisFrameCollisions.clear();

	our->CalculateCollider();

	Entity* prevParent = CheckPrevParent();	

	// if we were on the ground last frame, and the player wants to jump, then jump
	bool wasGrounded = isGrounded; // our->GetAnimator()->GetBool("isGrounded");
	//our->GetAnimator()->SetBool("isGrounded", false);
	isGrounded = false;
	
	bool horizontalCollision = false;
	bool verticalCollision = false;

	//const int TARGET_FPS = 30;

	// Get bounds assuming the move is valid
	SDL_Rect myBounds = *(our->GetBounds());
	myBounds.x -= (myBounds.w / 2);
	//myBounds.y += (myBounds.h / 2);

	SDL_Rect newBoundsHorizontal = myBounds;
	newBoundsHorizontal.x = (int)(myBounds.x + (velocity.x * game.dt));

	SDL_Rect newBoundsVertical = myBounds;
	newBoundsVertical.y += (myBounds.h / 2);
	newBoundsVertical.y = (int)(newBoundsVertical.y + (velocity.y * game.dt)); 

	// THIS NEEDS TO BE HERE BECAUSE OTHERWISE THE INTERSECTION CODE WILL NOT WORK
	// SDL's intersection code returns false if our y + h = their y, but we want it to return true!
	newBoundsVertical.y += 1;

	// 2.5D look
	SDL_Rect floorBounds = newBoundsVertical;
	floorBounds.y += 20;

	const int FLOOR_SIZE = 16;
	if (standAboveGround)
		floorBounds.h += FLOOR_SIZE; // (int)(newBoundsVertical.h * 0.25f);

	//TODO: Re-implement this
	bool fallThru = false;

	Entity* entity = nullptr;

	//TODO: Use a quadtree instead of looping over everything
	for (unsigned int i = 0; i < game.entities.size(); i++)
	{
		game.collisionChecks++;

		entity = game.entities[i];

		if (entity == our)
			continue;

		SDL_Rect theirBounds = *(entity->GetBounds());
		theirBounds.w *= 2;
		theirBounds.x -= (theirBounds.w/2);

		if (our->etype == "player" && entity->id == 2799)
			int test = 0;

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

			if (!horizontalCollision && HasIntersection(newBoundsHorizontal, theirBounds))
			{		
				if (our->etype == "player")
					int test = 0;

				horizontalCollision = CheckCollisionHorizontal(entity, game);
				hadCollision = hadCollision || horizontalCollision;

				if (velocity.x > 0)
					our->position.x = (float)(theirBounds.x - myBounds.w - our->collider->offset.x);
				else if (velocity.x < 0)
					our->position.x = (float)(theirBounds.x + theirBounds.w + our->collider->offset.x);
			}

			// Separate checking the ceiling vs. the floor 
			// (because we can't go up and down at the same time)
			if (velocity.y < 0)
			{
				// Moving up
				if (!entity->jumpThru && !verticalCollision && 
					HasIntersection(newBoundsVertical, theirBounds))
				{
					verticalCollision = CheckCollisionCeiling(entity, game);
					hadCollision = hadCollision || verticalCollision;
					our->position.y = (float)(theirBounds.y + (theirBounds.h) + myBounds.h + our->collider->offset.y);
				}
			}
			else
			{
				bool shouldIgnoreJumpThru = (entity->jumpThru &&
					our->GetAnimator()->GetBool("onLadder"));

				// Moving down
				// checks the ground (using a rect that is a little bit larger
				if (!verticalCollision && !shouldIgnoreJumpThru && 
					HasIntersection(floorBounds, theirBounds))
				{
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

						jumpsRemaining = 1;

						if (useGravity)
						{
							velocity.y = 0;
							if (standAboveGround)
								our->position.y = (float)(theirBounds.y - (theirBounds.h) - myBounds.h - FLOOR_SIZE - our->collider->offset.y);
							else
								our->position.y = (float)(theirBounds.y - (theirBounds.h) - myBounds.h - our->collider->offset.y);
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
			if (our->etype == "player")
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

	bool jumped = false;

	if ((!hadPressedJump && pressingJumpButton) && jumpsRemaining > 0 && wasGrounded)
	{
		game.soundManager->PlaySound("se/Jump.wav", 0);
		jumpsRemaining--;
		velocity.y = jumpSpeed;
		shouldStickToGround = false;
		//std::cout << "jump!" << std::endl;
		jumped = true;
	}

	if (prevParent != nullptr && parent == nullptr)
	{
		//std::cout << "lost parent!" << std::endl;
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
			//std::cout << "j2" << std::endl;
			//Jump(game);
		}
		else if (!shouldStickToGround)
		{
			//if (our->etype == "player")
			//	std::cout << "position.y += " << velocity.y << " * " << game.dt << std::endl;

			our->position.y += (velocity.y * game.dt);
			//if (jumped)
				//std::cout << "j3" << std::endl;
		}

		//if (etype == "player")
		//	std::cout << "v: " << velocity.y << std::endl;

	}
	else if (jumped)
	{
		our->position.y += (velocity.y * game.dt);
		//std::cout << "j4" << std::endl;
	}

	if (isGrounded != wasGrounded)
		our->GetAnimator()->SetBool("isGrounded", isGrounded);

	PreviousFrameCollisions(game);

	return hadCollision;
}

void PhysicsComponent::Jump(Game& game)
{
	std::cout << "jump 1" << std::endl;
	game.soundManager->PlaySound("se/Jump.wav", 0);
	jumpsRemaining--;
	our->position.y -= jumpSpeed * game.dt;
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
			prevFrameCollisions[i]->OnTriggerExit(our, game);
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
			collidedEntity->OnTriggerStay(our, game);
		}
		else
		{
			collidedEntity->OnTriggerEnter(our, game);
		}

		thisFrameCollisions.emplace_back(collidedEntity);
	}

	return hadCollision;
}

void PhysicsComponent::Push(Vector2 pushVelocity)
{
	velocity = pushVelocity;
	hitByPushSpell = true;
	totalDistancePushed = 0;
}

void PhysicsComponent::Update(Game& game)
{
	if (useGravity)
	{
		if (velocity.y < CalcTerminalVelocity())
			velocity.y += Physics::GRAVITY * game.dt;
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