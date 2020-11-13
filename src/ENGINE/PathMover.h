#ifndef PATHMOVER_H
#define PATHMOVER_H
#pragma once

#include "leak_check.h"
#include "Path.h"

class KINJO_API PathMover
{
public:
	Path* currentPath = nullptr;
	int pathID = 0;
	int pathNodeID = 0;
	float pathSpeed = 0;

	int delayCounter = 0;
	int delayMax = 120;
	bool wasMovingForward = true;
	bool movingForwardOnPath = true;

	bool MoveAlongPath(Entity& entity, Game& game);
	PathNode* GetCurrentNode();
};

#endif

