#include "leak_check.h"
#include "HealthComponent.h"
#include "Sprite.h"
#include "Renderer.h"
#include "Game.h"
#include <algorithm>

// For this health system,
// we set things up such that
// the smallest HP value is zero,
// and the largest HP value is the max value.
// Also, the max HP cannot be below the current HP
// (and therefore also cannot be below zero)

HealthComponent::HealthComponent(int max, int current)
{
	maxHP = max;

	if (current < 0)
		currentHP = maxHP;
	else
		currentHP = current;
}

HealthComponent::~HealthComponent()
{
	if (healthbarFront != nullptr)
	{
		delete_it(healthbarFront);
	}

	if (healthbarBack != nullptr)
	{
		delete_it(healthbarBack);
	}

	for (int i = 0; i < healthIcons.size(); i++)
	{
		delete_it(healthIcons[i]);
	}
}

int HealthComponent::GetMaxHP()
{
	return maxHP;
}

int HealthComponent::GetCurrentHP()
{
	return currentHP;
}

void HealthComponent::SetMaxHP(int newValue)
{
	maxHP = std::max(currentHP, newValue);
}

void HealthComponent::SetCurrentHP(int newValue)
{
	if (!invincible)
	{
		currentHP = std::max(std::min(newValue, maxHP), 0);
	}
}

void HealthComponent::AddCurrentHP(int value)
{
	if (timer.HasElapsed())
	{
		SetCurrentHP(currentHP + value);

		if (value < 0) // if taking damage
		{
			timer.Start(invincibleDuration); // invincible frames
		}
	}
}

bool HealthComponent::IsAlive()
{
	return currentHP > 0;
}

float HealthComponent::GetPercentHP()
{
	return currentHP / ((float)maxHP);
}

void HealthComponent::CreateHealthBar(const Renderer& renderer, const Vector2& scale, 
	Color colorFront, Color colorBack, bool relativeToCamera)
{
	if (healthbarFront != nullptr)
		delete healthbarFront;

	healthbarFront = neww Sprite(renderer.shaders[ShaderName::SolidColor]);
	healthbarFront->color = colorFront;
	healthbarFront->keepPositionRelativeToCamera = relativeToCamera;
	healthbarFront->keepScaleRelativeToCamera = relativeToCamera;

	if (healthbarBack != nullptr)
		delete healthbarBack;

	healthbarBack = neww Sprite(renderer.shaders[ShaderName::SolidColor]);
	healthbarBack->color = colorBack;
	healthbarBack->keepPositionRelativeToCamera = relativeToCamera;
	healthbarBack->keepScaleRelativeToCamera = relativeToCamera;
}

void HealthComponent::Render(const Renderer& renderer)
{
	if (showHealthBar)
	{
		if (healthbarBack == nullptr || healthbarFront == nullptr)
		{
			CreateHealthBar(renderer, initialHealthBarScale, 
				{ 0, 255, 0, 255 }, { 255, 0, 0, 255 }, showRelativeToCamera);
		}

		if (healthbarBack != nullptr)
		{			
			Vector2 positionBack = position;
			healthbarBack->Render(positionBack, renderer, initialHealthBarScale);
		}

		if (healthbarFront != nullptr)
		{
			Vector2 positionFront = position;
			const float width = initialHealthBarScale.x * GetPercentHP();
			float offset = (initialHealthBarScale.x - (width));
			positionFront.x -= offset;

			healthbarFront->Render(positionFront, renderer, Vector2(width, initialHealthBarScale.y));
		}
	}

	if (showHealthIcons)
	{
		if (healthIcons.size() != maxHP)
		{
			for (int i = 0; i < healthIcons.size(); i++)
			{
				delete healthIcons[i];
			}

			healthIcons.clear();

			for (int i = 0; i < maxHP; i++)
			{
				healthIcons.push_back(renderer.game->CreateSprite(iconPath));
				healthIcons[i]->keepPositionRelativeToCamera = true;
				healthIcons[i]->keepScaleRelativeToCamera = true;
			}
		}

		Vector2 currentPosition = position;
		for (int i = 0; i < healthIcons.size(); i++)
		{
			if (i >= currentHP)
			{
				healthIcons[i]->color = { 64, 64, 64, 255 };
			}
			else
			{
				healthIcons[i]->color = { 255, 255, 255, 255 };
			}

			healthIcons[i]->Render(currentPosition, renderer, Vector2(1,1));
			currentPosition += Vector2(100, 0);
		}
	}


}