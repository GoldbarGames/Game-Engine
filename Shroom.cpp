#include "Shroom.h"
#include "Game.h"
#include "PhysicsComponent.h"

Shroom::Shroom(const Vector2& pos) : Entity(pos)
{
	etype = "shroom";
	CreateCollider(0, -15, 24, 32);
	layer = DrawingLayer::COLLISION;
	drawOrder = 10;

	impassable = false;
	trigger = true;

	physics = new PhysicsComponent(this);
	physics->canBePushed = true;
	physics->standAboveGround = true;
	physics->respawnOnDeath = true;
	physics->mass = 5;
}

Shroom::~Shroom()
{

}


void Shroom::OnTriggerEnter(Entity& other, Game& game)
{
	if (other.physics != nullptr)
	{
		other.physics->velocity.y = -1.5f;
		game.soundManager->PlaySound("se/Jump.wav", 0);
	}
}

void Shroom::Save(std::ostringstream& level)
{
	level << std::to_string(id) << " " << etype << " " << physics->startPosition.x <<
		" " << physics->startPosition.y << " " << spriteIndex << std::endl;
}