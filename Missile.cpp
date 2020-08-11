#include "Missile.h"
#include "Game.h"
#include "PhysicsComponent.h"

Missile::Missile(const Vector2& pos) : Entity(pos)
{
	CreateCollider(0, -3, 10, 10);

	timeToLive.Start(2000);

	etype = "missile";
	physics = new PhysicsComponent(this);
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
		// check for collisions, and destroy if it hits a wall or an enemy'
		if (physics->CheckCollisions(game))
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