#include "Switch.h"
#include "Game.h"

Switch::Switch(Vector2 pos) : Entity(pos)
{
	layer = DrawingLayer::OBJECT;
	drawOrder = 92;
	etype = "switch";
	trigger = true;
	CreateCollider(0, 0, 24, 24);
	int test = id;
}

void Switch::OnTriggerStay(Entity& other, Game& game)
{
	if (animator != nullptr)
	{
		animator->SetBool("isPressed", true);
	}
}

void Switch::OnTriggerEnter(Entity& other, Game& game)
{
	if (animator != nullptr)
	{
		animator->SetBool("isPressed", true);
	}
}

void Switch::OnTriggerExit(Entity& other, Game& game)
{
	if (animator != nullptr)
	{
		animator->SetBool("isPressed", false);
	}
}

//TODO: Make sure we save WHAT this switch is actually doing!
void Switch::Save(std::ostringstream& level)
{
	level << std::to_string(id) << " " << etype << " " << position.x << " " <<
		position.y << " " << spriteIndex << "" << std::endl;
}