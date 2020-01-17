#include "Bug.h"
#include "Player.h"
#include "Game.h"

Bug::Bug(Vector2 pos) : PhysicsEntity(pos)
{
	layer = DrawingLayer::OBJECT;
	drawOrder = 90;
	etype = "bug";
	trigger = true;
	useGravity = false;
	CreateCollider(24, 24, 0, -4, 15, 18);
}

Bug::~Bug()
{

}

void Bug::OnTriggerStay(Entity* other, Game& game)
{

}

void Bug::OnTriggerEnter(Entity* other, Game& game)
{
	if (other->etype == "debug_missile")
	{		
		Missile* missile = dynamic_cast<Missile*>(other);

		if (!missile->destroyed)
		{
			shouldDelete = true;
			missile->Destroy();
			game.bugsRemaining--;
			game.bugText->SetText("Bugs Remaining: " + std::to_string(game.bugsRemaining));
		}
	}
}

void Bug::OnTriggerExit(Entity* other, Game& game)
{

}

void Bug::Save(std::ostringstream& level)
{
	level << std::to_string(id) << " " << etype << " " << startPosition.x <<
		" " << startPosition.y << " " << spriteIndex << std::endl;
}