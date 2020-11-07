#ifndef CUTSCENETRIGGER_H
#define CUTSCENETRIGGER_H
#pragma once

#include "Entity.h"
#include "leak_check.h"
class KINJO_API CutsceneTrigger : public Entity
{
public:
	SDL_Rect triggerRect;
	std::string cutsceneLabel = "null";

	CutsceneTrigger(Vector2 pos);
	~CutsceneTrigger();

	void Init(const std::string& n);

	void OnTriggerStay(Entity& other, Game& game);
	void OnTriggerEnter(Entity& other, Game& game);
	void OnTriggerExit(Entity& other, Game& game);

	void Save(std::unordered_map<std::string, std::string>& map);
	void Load(std::unordered_map<std::string, std::string>& map, Game& game);

	const SDL_Rect* GetBounds();
};

#endif