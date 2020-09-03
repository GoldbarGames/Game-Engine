#include "HealthComponent.h"
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
	SetCurrentHP(currentHP + value);
}

bool HealthComponent::IsAlive()
{
	return currentHP > 0;
}

float HealthComponent::GetPercentHP()
{
	return currentHP / ((float)maxHP);
}