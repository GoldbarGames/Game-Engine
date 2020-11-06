#ifndef SHROOM_H
#define SHROOM_H
#pragma once

#include "MyEntity.h"

class Shroom : public MyEntity
{
public:
	Shroom(const Vector2& pos);
	~Shroom();

	void Init(const Game& g, const std::string& n);
	void Update(Game& game);

	void OnTriggerEnter(MyEntity& other, Game& game);

	void Save(std::unordered_map<std::string, std::string>& map);

	static Entity* __stdcall Create(const Vector2& pos) { return neww Shroom(pos); };
};

#endif