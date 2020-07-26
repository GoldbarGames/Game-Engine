#include "Enemy.h"
#include "Player.h"
#include "Game.h"
#include "PhysicsInfo.h"

Enemy::Enemy(Vector2 pos) : Entity(pos)
{
	layer = DrawingLayer::COLLISION;
	drawOrder = 100;
	etype = "enemy";
	trigger = true;

	CreateCollider(0, -4, 54, 32);

	physics = new PhysicsInfo(this);
	physics->useGravity = true;
	physics->startPosition = pos;
}

Enemy::~Enemy()
{

}

void Enemy::OnTriggerStay(Entity* other, Game& game)
{

}

void Enemy::OnTriggerEnter(Entity* other, Game& game)
{
	if (other->etype == "debug_missile")
	{
		Missile* missile = dynamic_cast<Missile*>(other);

		if (!missile->destroyed)
		{
			shouldDelete = true;
			missile->Destroy();
		}
	}
}

void Enemy::OnTriggerExit(Entity* other, Game& game)
{

}

void Enemy::Save(std::ostringstream& level)
{
	level << std::to_string(id) << " " << etype << " " << physics->startPosition.x <<
		" " << physics->startPosition.y << " " << spriteIndex << std::endl;
}