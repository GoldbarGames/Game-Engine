#pragma once
#include "Entity.h"
class Enemy : public Entity
{
public:
	Enemy(Vector2 pos);
	~Enemy();

	//TODO: Make different member variables for each type of enemy
	bool playerIsToTheRight = false;

	Timer actionTimer;
	Collider* bottomLeftGround = nullptr;
	Collider* bottomRightGround = nullptr;

	void Update(Game& game);
	void Render(const Renderer& renderer);

	void Init(const std::string& n);

	void OnTriggerStay(Entity& other, Game& game);
	void OnTriggerEnter(Entity& other, Game& game);
	void OnTriggerExit(Entity& other, Game& game);

	void Save(std::ostringstream& level);
	void Load(std::unordered_map<std::string, std::string>& map, Game& game);

	static Entity* __stdcall Create(const Vector2& pos) { return new Enemy(pos); };
};

