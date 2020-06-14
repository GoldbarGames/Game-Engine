#ifndef BLOCK_H
#define BLOCK_H
#pragma once

#include "Entity.h"

class Block : public Entity
{
public:
	Block(Vector2 pos);
	~Block();

	//void Push(Vector2 direction, Game &game);
	void GetProperties(Renderer * renderer, FontInfo* font, std::vector<Property*>& properties);
	void SetProperty(std::string prop, std::string newValue);
	void Render(Renderer * renderer);

	void Save(std::ostringstream& level);
};

#endif