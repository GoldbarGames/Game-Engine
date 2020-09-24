#include "Missile.h"
#include "Game.h"
#include "PhysicsComponent.h"

Missile::Missile(const Vector2& pos) : Entity(pos)
{
	CreateCollider(0, -3, 10, 10);

	timeToLive.Start(2000);

	// This is important so that it has a quadtree for collisions
	// otherwise it will not collide with anything
	trigger = true;

	etype = "missile";
	physics = new PhysicsComponent(this);
}

Missile::~Missile()
{

}

void Missile::Init(const std::string& n)
{
	name = n;

	if (name == "pop")
	{
		CreateCollider(0, 0, 4, 4);
		physics->useGravity = true;
		physics->canBePushed = true;
		destroyAfterTime = false;
		animator->SetState("pop_moving");
		animator->Update(*this);
		animator->DoState(*this);
		layer = DrawingLayer::OBJECT;
	}
	else if (name == "float")
	{
		CreateCollider(0, 0, 50, 50);
		physics->useGravity = false;
		physics->canBePushed = false;
		physics->applyFriction = false;
		destroyAfterTime = true;
		timeToLive.Start(3000);
		animator->SetState("float_bubble_moving");
		animator->Update(*this);
		animator->DoState(*this);
		layer = DrawingLayer::OBJECT;
		scale = Vector2(0.1f, 0.1f);
	}
	else if (name == "freeze")
	{
		CreateCollider(0, 0, 16, 16);
		physics->useGravity = false;
		physics->canBePushed = false;
		physics->applyFriction = false;
		destroyAfterTime = true;
		timeToLive.Start(3000);
		animator->SetState("freeze_moving");
		animator->Update(*this);
		animator->DoState(*this);
		layer = DrawingLayer::OBJECT;
	}
}

void Missile::Update(Game& game)
{
	Entity::Update(game);

	if (destroyAfterTime && timeToLive.HasElapsed())
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
		if (name == "debug")
		{ 
			// check for collisions, and destroy if it hits a wall or an enemy'
			if (physics->hadCollisionsThisFrame)
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
		else if (name == "pop")
		{
			if (animator->GetBool("hitGround"))
			{
				position = landedPosition;

				if (animator->GetBool("animationElapsed"))
				{
					if (animator->GetBool("actionTimerElapsed"))
					{
						// pop_fire_end: After the last animation plays, destroy the object
						if (currentSprite->HasAnimationElapsed())
						{
							shouldDelete = true;
						}
					}
					else
					{
						// pop_fire_loop: Check that the loop has ended (X amount of time has passed)
						if (actionTimer.HasElapsed())
						{
							animator->SetBool("actionTimerElapsed", true);
						}
					}
				}
				else
				{
					// pop_fire_init: Check that the fire starts up
					animator->SetBool("animationElapsed", currentSprite->HasAnimationElapsed());
					if (animator->GetBool("animationElapsed"))
					{
						actionTimer.Start(3000);
					}
				}
			}
			else
			{
				// check for collisions, and destroy if it hits a wall or an enemy'
				if (physics->hadCollisionsThisFrame)
				{
					animator->SetBool("hitGround", true);
					physics->velocity = Vector2(0, 0);
					landedPosition = Vector2(position.x, position.y = 16);
					
					// TODO: What if it's on a moving object?
					
					// When we detonate the bomb
					physics->useGravity = false;
					CreateCollider(0, 0, 19, 27);
				}
			}
		}
		else if (name == "float")
		{
			// Grow the bubble from small to large
			if (scale.x < 1.0f)
			{
				LerpVector2(scale, Vector2(1.0f, 1.0f), 0.05f, 0.025f);
			}

			// Burst the bubble if it hits anything
			if (physics->thisFrameCollisions.size() > 0)
			{
				for (int i = 0; i < physics->thisFrameCollisions.size(); i++)
				{
					// TODO: Refactor this so that it is more obvious
					// what can and cannot burst a bubble
					if (physics->thisFrameCollisions[i]->etype == "tile")
					{
						animator->SetBool("destroyed", true);
						physics->velocity = Vector2(0, 0);
					}
					else if (physics->thisFrameCollisions[i]->physics != nullptr
						&& physics->thisFrameCollisions[i]->physics->canBePickedUp)
					{
						// Pick up the object and carry it in the bubble
						pickedUpEntity = physics->thisFrameCollisions[i];
						//physics->thisFrameCollisions[i]->physics->parent = this;


						//TODO: Before we can finish this, we need to do collision layers
						// because the bubble needs to pass through objects it can pick up
						// even if the player cannot walk through them.
						// Also, it is not checking collisions against the player
						// because the player is neither impassable nor a trigger
					}
				}
			}

			// TODO: What happens if the picked up entity is deleted?
			
			if (pickedUpEntity != nullptr)
			{
				pickedUpEntity->position = position;
			}
			
		}
		else if (name == "freeze")
		{
			// TODO: Freeze anything it comes into contact with
			// such as enemies, water tiles, etc.
		}
	}

	// TODO: If the missile somehow goes out of bounds, destroy it
}

void Missile::Destroy()
{
	animator->SetBool("destroyed", true);
	physics->velocity = Vector2(0, 0);
}

void Missile::SetVelocity(const Vector2& newVelocity)
{
	physics->velocity = newVelocity;

	if (name == "debug")
	{
		if (physics->velocity.x > 0)
			rotation = glm::vec3(0, 0, 0);
		else if (physics->velocity.x < 0)
			rotation = glm::vec3(0, 180, 0);
		else if (physics->velocity.y > 0)
			rotation = glm::vec3(0, 0, 270);
		else if (physics->velocity.y < 0)
			rotation = glm::vec3(0, 0, 90);
	}
}