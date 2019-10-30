#pragma once
#include "Path.h"
#include "PhysicsEntity.h"
class Platform : public PhysicsEntity
{
public:
	int spriteIndex = 0;
	std::string platformType = "Idle";
	int tilesToMove = 3;
	bool shouldLoop = true;
	Vector2 startVelocity = Vector2(0, 0);
	Vector2 startPosition = Vector2(0, 0);

	// Path variables
	Path* currentPath = nullptr;
	int pathID = 0;
	float pathSpeed = 0;
	std::string endPathBehavior = "";

	Platform(Vector2 pos);
	~Platform();
	void GetProperties(Renderer * renderer, TTF_Font * font, std::vector<Property*>& properties);
	void SetProperty(std::string prop, std::string newValue);
	void Render(Renderer * renderer, Vector2 cameraOffset);
	void Update(Game& game);

	void Save(std::ostringstream& level);
};

