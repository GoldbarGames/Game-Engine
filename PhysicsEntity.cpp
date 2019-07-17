#include "PhysicsEntity.h"

PhysicsEntity::PhysicsEntity()
{
	CreateCollider(0, 0, 1, 1);
}

PhysicsEntity::~PhysicsEntity()
{

}

const SDL_Rect* PhysicsEntity::GetColliderBounds()
{
	return collisionBounds;
}

void PhysicsEntity::CreateCollider(float x, float y, float w, float h)
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

	startSpriteSize.x = 27 * SCALE;
	startSpriteSize.y = 46 * SCALE;
}