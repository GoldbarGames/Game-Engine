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
};

#endif