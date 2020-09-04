#ifndef RENDERABLE_H
#define RENDERABLE_H
#pragma once

class Renderer;

class Renderable
{
public:
	virtual void Render(const Renderer& renderer);
};

#endif

