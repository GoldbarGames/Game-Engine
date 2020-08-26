#ifndef GOAL_H
#define GOAL_H
#pragma once

#include "Entity.h"

class Goal : public Entity
{
public:
	int spriteIndex = 0;
	bool isOpen = false;
	std::string nextLevelName = "";

	Goal(Vector2 pos);
	~Goal();

	void Update(Game& game);

	void OnTriggerStay(Entity* other, Game& game);
	void OnTriggerEnter(Entity* other, Game& game);
	void OnTriggerExit(Entity* other, Game& game);
	void GetProperties(Renderer * renderer, FontInfo* font, std::vector<Property*>& properties);
	void SetProperty(const std::string& key, const std::string& newValue);

	void Save(std::ostringstream& level);

	static Entity* __stdcall Create(const Vector2& pos) { return new Goal(pos); };
};

#endif