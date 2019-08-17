#include <SDL.h>
#include "Vector2.h"

#pragma once
class Collider
{
private:

	float colliderWidth = 1;
	float colliderHeight = 1;

public:
	Collider();
	~Collider();

	SDL_Rect* collider = nullptr;        // adjust the bounds this way
	SDL_Rect* collisionBounds = nullptr; // do not touch this until render time
	Vector2 startSpriteSize = Vector2(0, 0); // initialize to starting sprite rectangle

	void CreateCollider(float startX, float startY, float x, float y, float w, float h);
	void CalculateCollider(Vector2 position, Vector2 cameraOffset);
};

