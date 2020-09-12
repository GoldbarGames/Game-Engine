#ifndef TREE_H
#define TREE_H
#pragma once

#include "Entity.h"

class Sprite;

class Tree : public Entity
{
public:
	int hiddenEntityID = -1;
	Entity* hiddenEntity = nullptr;
	Sprite* bottomSprite = nullptr;

	Tree(Vector2 pos);
	~Tree();

	void Init(const std::string& n);

	void Update(Game& game);
	void Render(const Renderer& renderer);

	void GetProperties(std::vector<Property*>& properties);
	void SetProperty(const std::string& key, const std::string& newValue);

	void OnTriggerStay(Entity& other, Game& game);
	void OnTriggerEnter(Entity& other, Game& game);
	void OnTriggerExit(Entity& other, Game& game);

	void Save(std::ostringstream& level);

	static Entity* __stdcall Create(const Vector2& pos) { return new Tree(pos); };
};

#endif