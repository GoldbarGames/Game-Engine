#include "Enemy.h"
#include "Player.h"
#include "Game.h"
#include "PhysicsInfo.h"

Enemy::Enemy(Vector2 pos) : Entity(pos)
{
	layer = DrawingLayer::COLLISION;
	drawOrder = 90;
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

void Enemy::Update(Game& game)
{
	if (name == "crawler")
	{
		if (physics->isGrounded)
		{
			//TODO: Only move if the player is nearby
			//TODO: Don't move off ledges

			if (game.player->position.x > position.x)
				physics->velocity.x = 0.1f;
			else
				physics->velocity.x = -0.1f;
		}		
	}

	physics->Update(game);
}

void Enemy::OnTriggerStay(Entity* other, Game& game)
{
	if (other->etype == "debug_missile")
	{
		shouldDelete = true;
	}
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