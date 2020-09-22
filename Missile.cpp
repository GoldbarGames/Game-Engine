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

					// When we detonate the bomb
					CreateCollider(0, 0, 19, 27);
				}
			}
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