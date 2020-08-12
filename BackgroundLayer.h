#ifndef BACKGROUNDLAYER_H
#define BACKGROUNDLAYER_H
#pragma once

#include "Entity.h"

class BackgroundLayer : public Entity
{
public:
	BackgroundLayer(Vector2 pos, float p);
	~BackgroundLayer();
	float parallaxAmount = 0.5f;
	void Render(const Renderer& renderer);
};

#endif