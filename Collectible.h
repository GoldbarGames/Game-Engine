#ifndef COLLECTIBLE_H
#define COLLECTIBLE_H
#pragma once

#include "Entity.h"

class Collectible : public Entity
{
public:
	Collectible(Vector2 pos);
	~Collectible();

	void Update(Game& game);
	void Render(const Renderer& renderer);

	void Init(const std::string& n);

	void OnTriggerStay(Entity& other, Game& game);
	void OnTriggerEnter(Entity& other, Game& game);
	void OnTriggerExit(Entity& other, Game& game);

	void Save(std::ostringstream& level);

	static Entity* __stdcall Create(const Vector2& pos) { return new Collectible(pos); };
};

#endif