#ifndef ENEMY_H
#define ENEMY_H
#pragma once

#include "../ENGINE/filesystem_types.h"
#include "MyEntity.h"

class Enemy : public MyEntity
{
public:
	Enemy(Vector2 pos);
	~Enemy();

	Timer actionTimer;
	Collider* bottomLeftGround = nullptr;
	Collider* bottomRightGround = nullptr;

	void Update(Game& game);
	void Render(const Renderer& renderer);
	void RenderDebug(const Renderer& renderer);

	void Init(const Game& g, const std::string& n);

	void OnTriggerStay(MyEntity& other, Game& game);
	void OnTriggerEnter(MyEntity& other, Game& game);
	void OnTriggerExit(MyEntity& other, Game& game);

	void Save(std::unordered_map<std::string, std::string>& map);
	void Load(std::unordered_map<std::string, std::string>& map, Game& game);

	static Entity* __stdcall Create(const Vector2& pos) { return neww Enemy(pos); };
};

#endif