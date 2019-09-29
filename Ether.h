#pragma once
#include "Entity.h"

class Ether : public Entity
{
public:
	int spriteIndex = 0;


	Ether(Vector2 pos);
	~Ether();

	void OnTriggerStay(Entity* other, Game& game);
	void OnTriggerEnter(Entity* other, Game& game);
	void OnTriggerExit(Entity* other, Game& game);
};

