#include "BackgroundLayer.h"

BackgroundLayer::BackgroundLayer(const glm::vec3& pos, float p) : Entity(pos)
{
	parallaxAmount = p;
	drawDebugRect = false;
}

BackgroundLayer::~BackgroundLayer()
{

}

void BackgroundLayer::Render(const Renderer& renderer)
{
	Entity::RenderParallax(renderer, parallaxAmount);
}
