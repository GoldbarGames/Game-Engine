#ifndef MISSILE_H
#define MISSILE_H

#include "Entity.h"
#include "Timer.h"

class Missile : public Entity
{
	Timer timeToLive;
public:
	bool destroyed = false;
	Missile(Vector2 pos);
	~Missile();
	void Update(Game& game);
	void UpdatePhysics(Game& game);
	bool CheckCollisions(Game& game);
	void SetVelocity(Vector2 newVelocity);
	void Destroy();

	static Entity* __stdcall Create(const Vector2& pos) { return new Missile(pos); };
};

#endif