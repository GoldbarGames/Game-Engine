#ifndef PATHNODE_H
#define PATHNODE_H
#pragma once

#include "Vector2.h"
#include <SDL.h>
class PathNode
{
	SDL_Rect rect;
	SDL_Rect renderRect;
public:
	Vector2 point = Vector2(0,0);
	
	PathNode(Vector2 pos);
	~PathNode();
	const SDL_Rect* GetRect();
	const SDL_Rect* GetRenderRect();
	const SDL_Rect* CalcRenderRect(Vector2 cameraOffset);
};

#endif