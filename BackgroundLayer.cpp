#include "BackgroundLayer.h"

BackgroundLayer::BackgroundLayer(Vector2 pos, float p) : Entity(pos)
{
	parallaxAmount = p;
	drawDebugRect = false;
}

BackgroundLayer::~BackgroundLayer()
{

}

void BackgroundLayer::Render(Renderer* renderer)
{
	Entity::RenderParallax(renderer, parallaxAmount);
}
