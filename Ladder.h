#ifndef LADDER_H
#define LADDER_H
#pragma once

#include "Entity.h"

class Ladder : public Entity
{
public:

	int spriteIndex = 0;

	void Render(Renderer * renderer, unsigned int uniformModel);

	Ladder(Vector2 pos);
	~Ladder();

	const SDL_Rect* GetBounds();

	void OnTriggerStay(Entity* other, Game& game);
	void OnTriggerEnter(Entity* other, Game& game);
	void OnTriggerExit(Entity* other, Game& game);

	void Save(std::ostringstream& level);
};

#endif