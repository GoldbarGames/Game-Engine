#ifndef HEALTHCOMPONENT_H
#define HEALTHCOMPONENT_H
#pragma once

#include "globals.h"
#include "Renderable.h"
#include "Timer.h"

class Sprite;
class Renderer;

class HealthComponent : public Renderable
{
private:
	int currentHP = 1;
	int maxHP = 1;
	Sprite* healthbarFront = nullptr;
	Sprite* healthbarBack = nullptr;
	std::vector<Sprite*> healthIcons;
	Timer timer;
public:
	bool showRelativeToCamera = false;
	int invincibleDuration = 1000;
	Vector2 position = Vector2(0, 0);
	std::string iconPath = "";
	Vector2 initialHealthBarScale = Vector2(0, 0);
	bool showHealthBar = false;
	bool showHealthIcons = false;
	bool invincible = false;
	HealthComponent(int max, int current=-1);
	~HealthComponent();
	int GetMaxHP();
	int GetCurrentHP();
	void SetMaxHP(int newValue);
	void SetCurrentHP(int newValue);
	void AddCurrentHP(int value);
	bool IsAlive();
	float GetPercentHP();
	void CreateHealthBar(const Renderer& renderer, Vector2 scale, Color colorFront, Color colorBack, bool relativeToCamera);
	void Render(const Renderer& renderer);
};

#endif

