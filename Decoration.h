#ifndef DECORATION_H
#define DECORATION_H
#pragma once
#include "Entity.h"

class Decoration : public Entity
{
public:
	std::string cutsceneLabel = "";
	Decoration(const Vector2& pos);
	~Decoration();

	void Init(const std::string& n);
	bool CanSpawnHere(const Vector2& spawnPosition, Game& game, bool useCamera = true);
	void OnTriggerStay(Entity& other, Game& game);
	void OnTriggerEnter(Entity& other, Game& game);
	void OnTriggerExit(Entity& other, Game& game);
	void ChangeCollider(float x, float y, float w, float h);
	void GetProperties(std::vector<Property*>& properties);
	void SetProperty(const std::string& key, const std::string& newValue);
	void Save(std::unordered_map<std::string, std::string>& map);
	void Load(std::unordered_map<std::string, std::string>& map, Game& game);

	static Entity* __stdcall Create(const Vector2& pos) { return neww Decoration(pos); };
};

#endif

