#ifndef MYENTITY_H
#define MYENTITY_H
#pragma once

#include "../ENGINE/Entity.h"

class PhysicsComponent;
class HealthComponent;

class MyEntity : public Entity
{
public:
	PhysicsComponent* physics = nullptr;
	HealthComponent* health = nullptr;

	MyEntity(const Vector2& pos);
	MyEntity(const Vector2& pos, Sprite* sprite);
	~MyEntity();
	void Update(Game& game);

	virtual void OnTriggerStay(MyEntity& other, Game& game);
	virtual void OnTriggerEnter(MyEntity& other, Game& game);
	virtual void OnTriggerExit(MyEntity& other, Game& game);
};

#endif