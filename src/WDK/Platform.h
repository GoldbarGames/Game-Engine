#ifndef PLATFORM_H
#define PLATFORM_H
#pragma once

#include "../ENGINE/Path.h"

class Tile;

class Platform : public Entity
{
public:
	std::string platformType = "Idle";
	int tilesToMove = 3;
	bool shouldLoop = true;
	Vector2 startVelocity = Vector2(0, 0);
	
	std::vector<Tile*> tiles;

	// Path variables
	Path* currentPath = nullptr;
	int pathID = 0;
	int pathNodeID = 0;
	float pathSpeed = 0;

	int delayCounter = 0;
	int delayMax = 100;
	bool wasMovingForward = true;
	bool movingForwardOnPath = true;

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

	void Init(const Game& g, const std::string& n);

	void GetProperties(std::vector<Property*>& properties);
	void SetProperty(const std::string& key, const std::string& newValue);
	void Render(const Renderer& renderer);
	void Update(Game& game);

	void Save(std::unordered_map<std::string, std::string>& map);
	void Load(std::unordered_map<std::string, std::string>& map, Game& game);

	std::string CalcDirection(bool x);

	static Entity* __stdcall Create(const Vector2& pos) { return neww Platform(pos); };
};

#endif