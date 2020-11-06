#ifndef LADDER_H
#define LADDER_H
#pragma once

#include "MyEntity.h"

class Ladder : public MyEntity
{
public:
	void Render(const Renderer& renderer);

	Ladder(const Vector2& pos);
	~Ladder();

	Ladder* top = nullptr;

	void OnTriggerStay(MyEntity& other, Game& game);
	void OnTriggerEnter(MyEntity& other, Game& game);
	void OnTriggerExit(MyEntity& other, Game& game);

	void Save(std::unordered_map<std::string, std::string>& map);
	void Load(std::unordered_map<std::string, std::string>& map, Game& game);

	static Entity* __stdcall Create(const Vector2& pos) { return neww Ladder(pos); };
};

#endif