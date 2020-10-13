#pragma once

#include "filesystem_types.h"
#include "Entity.h"

class Enemy : public Entity
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

	void Init(const std::string& n);

	void OnTriggerStay(Entity& other, Game& game);
	void OnTriggerEnter(Entity& other, Game& game);
	void OnTriggerExit(Entity& other, Game& game);

	void Save(std::unordered_map<std::string, std::string>& map);
	void Load(std::unordered_map<std::string, std::string>& map, Game& game);

	static Entity* __stdcall Create(const Vector2& pos) { return neww Enemy(pos); };
};

