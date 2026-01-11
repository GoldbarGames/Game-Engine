#include "BackgroundLayer.h"

BackgroundLayer::BackgroundLayer(const glm::vec3& pos, float p) : Entity(pos)
{
	parallaxAmount = p;
	drawDebugRect = false;
	layer = DrawingLayer::BG;
}

BackgroundLayer::~BackgroundLayer()
{

}

void BackgroundLayer::Render(const Renderer& renderer)
{
	Entity::Render(renderer, parallaxAmount);
}
