#pragma once
#include "PhysicsEntity.h"

class Block : public PhysicsEntity
{
public:
	int spriteIndex = 0;
	Block(Vector2 pos);
	~Block();

	//void Push(Vector2 direction, Game &game);
	void GetProperties(Renderer * renderer, TTF_Font * font, std::vector<Property*>& properties);
	void SetProperty(std::string prop, std::string newValue);
	void Render(Renderer * renderer, GLuint uniformModel);

	void Save(std::ostringstream& level);
};
