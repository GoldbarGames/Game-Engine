#include "Block.h"

Block::Block(Vector2 pos) : PhysicsEntity(pos)
{
	etype = "block";
	CreateCollider(48, 64, 0, 0, 1.0f, 0.9f);
	layer = DrawingLayer::COLLISION;
	drawOrder = 10;
	canBePushed = true;
}

Block::~Block()
{

}