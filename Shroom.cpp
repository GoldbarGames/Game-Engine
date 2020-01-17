#include "Shroom.h"
#include "Game.h"

Shroom::Shroom(Vector2 pos) : PhysicsEntity(pos)
{
	etype = "shroom";
	CreateCollider(0, 0, 0, -15, 24, 32);
	layer = DrawingLayer::COLLISION;
	drawOrder = 10;
	
	canBePushed = false;
	standAboveGround = true;

	impassable = false;
	trigger = true;
	mass = 5;
}

Shroom::~Shroom()
{

}


void Shroom::OnTriggerEnter(Entity* other, Game& game)
{
	if (other->etype == "player")
	{
		Player* player = static_cast<Player*>(other);
		player->velocity.y = -1;
	}
}

void Shroom::Save(std::ostringstream& level)
{
	level << std::to_string(id) << " " << etype << " " << startPosition.x <<
		" " << startPosition.y << " " << spriteIndex << std::endl;
}