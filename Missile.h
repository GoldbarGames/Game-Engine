#ifndef MISSILE_H
#define MISSILE_H

#include "Entity.h"
#include "Timer.h"

class Missile : public Entity
{
	Timer timeToLive;
	Timer actionTimer;
public:
	bool destroyAfterTime = true;
	void Init(const std::string& n);
	Missile(const Vector2& pos);
	~Missile();
	void Update(Game& game);
	void SetVelocity(const Vector2& newVelocity);
	void Destroy();

	static Entity* __stdcall Create(const Vector2& pos) { return new Missile(pos); };
};

#endif