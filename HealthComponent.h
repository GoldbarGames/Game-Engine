#ifndef HEALTHCOMPONENT_H
#define HEALTHCOMPONENT_H
#pragma once

class HealthComponent
{
private:
	int currentHP = 1;
	int maxHP = 1;
public:
	bool invincible = false;
	HealthComponent(int max, int current=-1);
	int GetMaxHP();
	int GetCurrentHP();
	void SetMaxHP(int newValue);
	void SetCurrentHP(int newValue);
	void AddCurrentHP(int value);
	bool IsAlive();
	float GetPercentHP();
};

#endif

