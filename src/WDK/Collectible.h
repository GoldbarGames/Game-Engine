#ifndef COLLECTIBLE_H
#define COLLECTIBLE_H
#pragma once

#include "../ENGINE/Entity.h"

class Collectible : public Entity
{
public:
	Collectible(Vector2 pos);
	~Collectible();

	void Update(Game& game);
	void Render(const Renderer& renderer);

	void Init(const Game& g, const std::string& n);

	void OnTriggerStay(Entity& other, Game& game);
	void OnTriggerEnter(Entity& other, Game& game);
	void OnTriggerExit(Entity& other, Game& game);

	static Entity* __stdcall Create(const Vector2& pos) { return neww Collectible(pos); };
};

#endif