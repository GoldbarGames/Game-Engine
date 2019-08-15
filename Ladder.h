#pragma once
#include "Entity.h"

class Ladder : public Entity
{
public:
	
	Ladder(Vector2 pos);
	~Ladder();

	void OnTriggerStay(Entity* other);
	void OnTriggerEnter(Entity* other);
	void OnTriggerExit(Entity* other);
};


