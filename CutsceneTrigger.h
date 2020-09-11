#ifndef CUTSCENETRIGGER_H
#define CUTSCENETRIGGER_H
#pragma once

#include "Entity.h"

class CutsceneTrigger : public Entity
{
public:
	SDL_Rect triggerRect;
	std::string cutsceneLabel = "null";

	CutsceneTrigger(std::string label, Vector2 pos, float w, float h);
	~CutsceneTrigger();

	void OnTriggerStay(Entity& other, Game& game);
	void OnTriggerEnter(Entity& other, Game& game);
	void OnTriggerExit(Entity& other, Game& game);

	const SDL_Rect* GetBounds();
};

#endif