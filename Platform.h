#pragma once
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

	Platform(Vector2 pos);
	~Platform();
	void GetProperties(Renderer * renderer, TTF_Font * font, std::vector<Text*>& properties);
	void SetProperty(std::string prop, std::string newValue);
	void Render(Renderer * renderer, Vector2 cameraOffset);
	void Update(Game& game);

	void Save(std::ostringstream& level);
};

