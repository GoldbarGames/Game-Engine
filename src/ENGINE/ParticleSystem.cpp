#include "ParticleSystem.h"
#include "Game.h"

ParticleSystem::ParticleSystem(const Vector2& pos) : Entity(pos)
{
	etype = "particlesystem";
	spawnTimer.Start(0);
	Resize(20);
}

ParticleSystem::~ParticleSystem()
{

}

void ParticleSystem::Update(Game& game)
{
	if (spawnTimer.HasElapsed() && nextActiveIndex < infos.size())
	{
		particles[nextActiveIndex].position = position;

		std::string nextFilename = "";

		if (nextParticleSpriteFilename.size() > 0)
		{
			int spriteIndex = game.randomManager.RandomInt(nextParticleSpriteFilename.size());
			if (spriteIndex >= 0)
			{
				nextFilename = nextParticleSpriteFilename[spriteIndex];
			}
		}

		particles[nextActiveIndex].GetSprite()->SetTexture(game.spriteManager.GetImage(nextFilename));

		// TODO: Add a way to set the shader to something else
		particles[nextActiveIndex].GetSprite()->SetShader(game.renderer.shaders[ShaderName::Default]);

		int colliderWidth = particles[nextActiveIndex].GetSprite()->frameWidth;
		if (nextParticleColliderWidth > 0)
			colliderWidth = nextParticleColliderWidth;

		int colliderHeight = particles[nextActiveIndex].GetSprite()->frameHeight;
		if (nextParticleColliderHeight > 0)
			colliderHeight = nextParticleColliderHeight;

		particles[nextActiveIndex].CreateCollider(0, 0, colliderWidth, colliderHeight);

		infos[nextActiveIndex].velocity = nextParticleVelocity;
		infos[nextActiveIndex].lifeTimer.Start(nextParticleTimeToLive);
		infos[nextActiveIndex].active = true;

		spawnTimer.Reset();

		nextActiveIndex++;
	}

	// Update all particles
	for (int i = 0; i < particles.size(); i++)
	{
		if (infos[i].active)
		{
			particles[i].Update(game);

			// TODO: Should we include physics in the engine?
			// Because the physics component is not included in the core engine,
			// we can just update the physics here based on velocity

			particles[i].position.x += infos[i].velocity.x * game.dt;
			particles[i].position.y += infos[i].velocity.y * game.dt;

			if (infos[i].lifeTimer.HasElapsed())
			{
				infos[i].active = false;
				nextActiveIndex = std::min(i, nextActiveIndex);
			}
		}
	}
}

// TODO: Should probably change this so that particles
// can be renderered in a different order even if they
// are spawned from the same particle system
void ParticleSystem::Render(const Renderer& renderer)
{
	for (auto& particle : particles)
	{
		particle.Render(renderer);
	}
}

void ParticleSystem::Resize(int newSize)
{
	maxNumberofParticles = newSize;

	if (newSize > particles.size())
	{
		particles.reserve(newSize);
		while (particles.size() < newSize)
		{			
			Entity newParticle(position);
			ParticleInfo newParticleInfo;

			particles.emplace_back(newParticle);
			infos.emplace_back(newParticleInfo);
		}
	}
	else if (newSize < particles.size())
	{
		// TODO
	}
}