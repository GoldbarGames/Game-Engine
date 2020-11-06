#ifndef TREE_H
#define TREE_H
#pragma once

#include "../ENGINE/filesystem_types.h"
#include "MyEntity.h"

class Tree : public MyEntity
{
public:
	int hiddenEntityID = -1;
	MyEntity* hiddenEntity = nullptr;
	Sprite* bottomSprite = nullptr;

	Tree(Vector2 pos);
	~Tree();

	void Init(const Game& g, const std::string& n);

	void Update(Game& game);
	void Render(const Renderer& renderer);

	void GetProperties(std::vector<Property*>& properties);
	void SetProperty(const std::string& key, const std::string& newValue);

	void OnTriggerStay(MyEntity& other, Game& game);
	void OnTriggerEnter(MyEntity& other, Game& game);
	void OnTriggerExit(MyEntity& other, Game& game);

	void Save(std::unordered_map<std::string, std::string>& map);
	void Load(std::unordered_map<std::string, std::string>& map, Game& game);

	static Entity* __stdcall Create(const Vector2& pos) { return neww Tree(pos); };
};

#endif