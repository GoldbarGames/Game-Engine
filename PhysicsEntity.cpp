#include "PhysicsEntity.h"

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

	startSpriteSize.x = startX * SCALE;
	startSpriteSize.y = startY * SCALE;
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