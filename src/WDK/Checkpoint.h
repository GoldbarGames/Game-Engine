#ifndef CHECKPOINT_H
#define CHECKPOINT_H
#pragma once
#include "MyEntity.h"
class Checkpoint : public MyEntity
{
public:
	Checkpoint(Vector2 pos);
	~Checkpoint();

	void Update(Game& game);

	void GetProperties(std::vector<Property*>& properties);
	void SetProperty(const std::string& key, const std::string& newValue);

	bool CanSpawnHere(const Vector2& spawnPosition, Game& game, bool useCamera = true);

	void OnTriggerStay(MyEntity& other, Game& game);
	void OnTriggerEnter(MyEntity& other, Game& game);
	void OnTriggerExit(MyEntity& other, Game& game);

	void Save(std::unordered_map<std::string, std::string>& map);
	void Load(std::unordered_map<std::string, std::string>& map, Game& game);

	static Entity* __stdcall Create(const Vector2& pos) { return neww Checkpoint(pos); };
};

#endif