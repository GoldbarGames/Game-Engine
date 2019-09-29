#pragma once
#include "Entity.h"

class Goal : public Entity
{
public:
	int spriteIndex = 0;
	bool isOpen = false;


	Goal(Vector2 pos);
	~Goal();

	void Update(Game& game);

	void OnTriggerStay(Entity* other, Game& game);
	void OnTriggerEnter(Entity* other, Game& game);
	void OnTriggerExit(Entity* other, Game& game);
};

