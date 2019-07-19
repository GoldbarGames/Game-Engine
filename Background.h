#pragma once

#include <vector>
#include "Entity.h"

class Background
{
public:
	Vector2 position = Vector2(0, 0);
	std::vector<Entity*> layers;
	Background(Vector2 pos);
	~Background();
	void Render(SDL_Renderer * renderer, Vector2 cameraOffset);
};

