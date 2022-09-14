#ifndef PARTICLESYSTEM_H
#define PARTICLESYSTEM_H
#pragma once

#include <vector>
#include "Entity.h"

struct ParticleInfo
{
	bool active = false;
	glm::vec3 velocity = glm::vec3(0, 0, 0);
	Timer lifeTimer;
};

class ParticleSystem : public Entity
{
public:
	int nextActiveIndex = 0;
	int nextParticleColliderWidth = 0;
	int nextParticleColliderHeight = 0;
	glm::vec3 nextParticleVelocity = glm::vec3(0, 0, 0);
	glm::vec2 nextParticleScale = glm::vec2(1, 1);
	float nextParticleTimeToLive = 1.0f;
	Timer spawnTimer;
	int maxNumberofParticles = 1;
	std::vector<std::string> nextParticleSpriteFilename;
	std::vector<Entity> particles;
	std::vector<ParticleInfo> infos;
	void Update(Game& game);
	void Render(const Renderer& renderer);
	void Resize(int newSize);
	ParticleSystem(const glm::vec3& pos);
	~ParticleSystem();

	void Save(std::unordered_map<std::string, std::string>& map);
	void Load(std::unordered_map<std::string, std::string>& map, Game& game);

	static Entity* __stdcall Create(const glm::vec3& pos) { return new ParticleSystem(pos); };
};

#endif