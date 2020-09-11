#ifndef PLATFORM_H
#define PLATFORM_H
#pragma once

#include "Path.h"

class Platform : public Entity
{
public:
	std::string platformType = "Idle";
	int tilesToMove = 3;
	bool shouldLoop = true;
	Vector2 startVelocity = Vector2(0, 0);

	// Path variables
	Path* currentPath = nullptr;
	int pathID = 0;
	int pathNodeID = 0;
	float pathSpeed = 0;
	bool traversePathForward = true;
	std::string endPathBehavior = "";
	std::string directionX = "none";
	std::string directionY = "none";

	int switchID = -1;
	Vector2 switchPressedPosition = Vector2(0, 0);
	Vector2 switchUnpressedPosition = Vector2(0, 0);
	Vector2 switchDistanceMoved = Vector2(0, 0);

	Platform(const Vector2& pos);
	~Platform();
	void GetProperties(std::vector<Property*>& properties);
	void SetProperty(const std::string& key, const std::string& newValue);
	void Render(const Renderer& renderer);
	void Update(Game& game);

	void Save(std::ostringstream& level);

	std::string CalcDirection(bool x);

	static Entity* __stdcall Create(const Vector2& pos) { return new Platform(pos); };
};

#endif