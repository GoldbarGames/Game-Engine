#ifndef BACKGROUNDLAYER_H
#define BACKGROUNDLAYER_H
#pragma once
#include "leak_check.h"
#include "Entity.h"

class KINJO_API BackgroundLayer : public Entity
{
public:
	BackgroundLayer(const Vector2& pos, float p);
	~BackgroundLayer();
	float parallaxAmount = 0.5f;
	void Render(const Renderer& renderer);
};

#endif