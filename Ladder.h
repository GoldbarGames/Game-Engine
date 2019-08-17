#pragma once
#include "Entity.h"
#include "Collider.h"

class Ladder : public Entity
{
public:
	
	Collider collider;

	void Render(SDL_Renderer * renderer, Vector2 cameraOffset);

	Ladder(Vector2 pos);
	~Ladder();

	
	const SDL_Rect* GetBounds();

	void OnTriggerStay(Entity* other);
	void OnTriggerEnter(Entity* other);
	void OnTriggerExit(Entity* other);
};


