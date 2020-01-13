#include "Collider.h"
#include "globals.h"
#include "Renderer.h"

Collider::Collider()
{
}


Collider::~Collider()
{
}

// Set up the collider
void Collider::CreateCollider(float startX, float startY, float x, float y, float w, float h)
{
	if (collider != nullptr)
		delete collider;

	collider = new SDL_Rect();
	collider->x = (int)x;
	collider->y = (int)y;
	collider->w = 1;
	collider->h = 1;

	colliderWidth = w;
	colliderHeight = h;

	if (collisionBounds != nullptr)
		delete collisionBounds;

	collisionBounds = new SDL_Rect();
	collisionBounds->x = (int)x;
	collisionBounds->y = (int)y;
	collisionBounds->w = 1;
	collisionBounds->h = 1;

	startSpriteSize.x = startX;
	startSpriteSize.y = startY;
}

// Actually calculate the location of the collider in world space
void Collider::CalculateCollider(Vector2 position, Vector2 cameraOffset)
{
	// scale the bounds of the sprite by a number to set the collider's width and height
	collisionBounds->w = (int)(startSpriteSize.x * colliderWidth);
	collisionBounds->h = (int)(startSpriteSize.y * colliderHeight);

	// set the collision bounds position to where the player actually is
	collisionBounds->x = (int)(position.x + collider->x - cameraOffset.x - (collisionBounds->w / 2));
	collisionBounds->y = (int)(position.y + collider->y - cameraOffset.y); //TODO: ???? -(collisionBounds->h / 2);
}