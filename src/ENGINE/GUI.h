#ifndef GUI_H
#define GUI_H
#pragma once

#include <vector>
#include <unordered_map>
#include "Text.h"
#include "leak_check.h"
class Renderer;
class HealthComponent;
class Game;

class KINJO_API GUI
{
public:
	Game* game = nullptr;

	Timer timer;
	bool showTimer = false;
	float timerSpeed = 1.0f;
	uint32_t lastTimerValue = 0;

	bool renderOnCutscene = false;

	std::vector<Text*> texts;
	std::unordered_map<std::string, unsigned int> textNames;
	std::unordered_map<std::string, std::string> originalTexts;

	std::vector<Entity*> images;
	std::unordered_map<std::string, unsigned int> imageNames;

	std::vector<std::string> guiNames;
	std::unordered_map<std::string, std::vector<Entity*>> guiImages;
	std::unordered_map<std::string, std::vector<Text*>> guiTexts;

	std::string currentGUI = "";

	Text* GetText(const std::string& name);
	void UpdateText(const std::string& key);

	std::string GetTimerString(uint32_t ticks);

	virtual void Init(Game* g);
	virtual void RenderStart();
	virtual void Update();
	virtual void Render(const Renderer& renderer);
	virtual void ResetText();
	virtual void LoadData(Game* g);

	virtual void SetShaderVariables(const Sprite& sprite, const ShaderProgram* shader);

	void SaveData();

	~GUI();
};

#endif

