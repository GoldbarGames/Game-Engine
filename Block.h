#ifndef BLOCK_H
#define BLOCK_H
#pragma once

#include "Entity.h"

class Block : public Entity
{
public:
	Block(const Vector2& pos);
	~Block();

	void Init(const std::string& n);

	//void Push(Vector2 direction, Game &game);
	void GetProperties(std::vector<Property*>& properties);
	void SetProperty(const std::string& key, const std::string& newValue);
	void Render(const Renderer& renderer);

	void Save(std::unordered_map<std::string, std::string>& map);

	static Entity* __stdcall Create(const Vector2& pos) { return new Block(pos); };
};

#endif