#include "ParticleSystem.h"
#include "Game.h"

ParticleSystem::ParticleSystem(const glm::vec3& pos) : Entity(pos)
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
		particles[nextActiveIndex].GetSprite()->SetShader(game.renderer.shaders[1]);

		int colliderWidth = particles[nextActiveIndex].GetSprite()->frameWidth;
		if (nextParticleColliderWidth > 0)
			colliderWidth = nextParticleColliderWidth;

		int colliderHeight = particles[nextActiveIndex].GetSprite()->frameHeight;
		if (nextParticleColliderHeight > 0)
			colliderHeight = nextParticleColliderHeight;

		particles[nextActiveIndex].CreateCollider(0, 0, colliderWidth, colliderHeight);
		particles[nextActiveIndex].scale = nextParticleScale;

		infos[nextActiveIndex].velocity = nextParticleVelocity;
		infos[nextActiveIndex].lifeTimer.Start(nextParticleTimeToLive);
		infos[nextActiveIndex].active = true;

		spawnTimer.Reset();

		nextActiveIndex++;
	}

	// Update all particles
	for (size_t i = 0; i < particles.size(); i++)
	{
		if (infos[i].active) // TODO: Only update if within screen
		{
			particles[i].Update(game);

			// Because the physics component is not included in the core engine,
			// we can just update the physics here based on velocity

			particles[i].position.x += infos[i].velocity.x * game.dt;
			particles[i].position.y += infos[i].velocity.y * game.dt;
			particles[i].position.z += infos[i].velocity.z * game.dt;

			if (infos[i].lifeTimer.HasElapsed())
			{
				infos[i].active = false;
				nextActiveIndex = std::min(static_cast<int>(i), nextActiveIndex);
			}
		} 
	}
}

// TODO: Should probably change this so that particles
// can be renderered in a different order even if they
// are spawned from the same particle system
void ParticleSystem::Render(const Renderer& renderer)
{
	for (auto& particle : particles) // TODO: Only if within screen
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
			newParticle.etype = "particle";
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

void ParticleSystem::Save(std::unordered_map<std::string, std::string>& map)
{
	Entity::Save(map);
}

void ParticleSystem::Load(std::unordered_map<std::string, std::string>& map, Game& game)
{
	Entity::Load(map, game);

	nextParticleSpriteFilename.emplace_back(game.cutsceneManager.commands.pathPrefix + map["sprite"]);
	nextParticleVelocity = glm::vec3(std::stof(map["vx"]), std::stof(map["vy"]), 0);
	nextParticleScale = glm::vec2(std::stof(map["sx"]), std::stof(map["sy"]));

	if (map["timeToSpawn"][0] == 'r') // randomize spawn time
	{
		bool second = false;
		int num1 = 0;
		int num2 = 0;
		const std::string& spawnString = map["timeToSpawn"];
		std::string temp = "";

		// Read in bounds
		for (int i = 1; i < spawnString.size(); i++)
		{
			if (second)
			{
				if (spawnString[i] == '\n')
				{
					num2 = std::stoi(temp);
					temp = "";
				}
				else
				{
					temp += spawnString[i];
				}
			}
			else
			{
				if (spawnString[i] == ',')
				{
					second = true;
					num1 = std::stoi(temp);
					temp = "";
				}
				else
				{
					temp += spawnString[i];
				}
			}
		}

		// Get random number
		int r = game.randomManager.RandomRange(num1, num2);
		spawnTimer.Start(r);
	}
	else
	{
		spawnTimer.Start(std::stoi(map["timeToSpawn"]));
	}

	
	Resize(std::stoi(map["maxNumber"]));
	nextParticleTimeToLive = std::stoi(map["timeToLive"]);
}