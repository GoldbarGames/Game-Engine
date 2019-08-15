#include "Ladder.h"

Ladder::Ladder(Vector2 pos) : Entity(pos)
{
	etype = "ladder";
	layer = DrawingLayer::OBJECT;
	drawOrder = 90;
	trigger = true;
}

Ladder::~Ladder()
{

}

void Ladder::OnTriggerStay(Entity* other)
{

}

void Ladder::OnTriggerEnter(Entity* other)
{

}

void Ladder::OnTriggerExit(Entity* other)
{

}