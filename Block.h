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
	void GetProperties(FontInfo* font, std::vector<Property*>& properties);
	void SetProperty(const std::string& prop, const std::string& newValue);
	void Render(const Renderer& renderer);

	void Save(std::ostringstream& level);

	static Entity* __stdcall Create(const Vector2& pos) { return new Block(pos); };
};

#endif