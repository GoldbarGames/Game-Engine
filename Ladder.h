#pragma once
#include "Entity.h"
#include "Collider.h"

class Ladder : public Entity
{
public:

	int spriteIndex = 0;
	
	Collider collider;

	void Render(Renderer * renderer, Vector2 cameraOffset);

	Ladder(Vector2 pos);
	~Ladder();

	const SDL_Rect* GetBounds();

	void OnTriggerStay(Entity* other, Game& game);
	void OnTriggerEnter(Entity* other, Game& game);
	void OnTriggerExit(Entity* other, Game& game);

	void Save(std::ostringstream& level);
};


