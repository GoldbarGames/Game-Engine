#pragma once
#include "Entity.h"

class CutsceneTrigger : public Entity
{
public:
	SDL_Rect triggerRect;
	std::string cutsceneLabel = "null";

	CutsceneTrigger(std::string label, Vector2 pos, float w, float h);
	~CutsceneTrigger();

	void OnTriggerStay(Entity* other);
	void OnTriggerEnter(Entity* other);
	void OnTriggerExit(Entity* other);

	const SDL_Rect* GetBounds();
};

