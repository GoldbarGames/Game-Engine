#ifndef MISSILE_H
#define MISSILE_H

#include "MyEntity.h"
#include "../ENGINE/Timer.h"

class Missile : public MyEntity
{
	Timer timeToLive;
	Timer actionTimer;
	Vector2 landedPosition = Vector2(0, 0);
public:
	// TODO: Figure out a better way than this
	Missile** selfPointer = nullptr;

	MyEntity* pickedUpEntity = nullptr;
	bool destroyAfterTime = true;
	void Init(const Game& g, const std::string& n);
	Missile(const Vector2& pos);
	~Missile();
	void Update(Game& game);
	void SetVelocity(const Vector2& newVelocity);
	void Destroy();

	static Entity* __stdcall Create(const Vector2& pos) { return neww Missile(pos); };
};

#endif