#ifndef MENUSCREEN_H
#define MENUSCREEN_H
#pragma once

#include <vector>
#include "glm/vec3.hpp"
#include "MenuButton.h"
#include "SpriteManager.h"
#include "leak_check.h"

class Entity;

struct MenuAnimKeyframe
{
	// We need the previous frame to get the start values.
	// For the first keyframe, we need to make a "zero" keyframe
	// using the starting values of the entity, and make sure
	// to not actually try and update the "zero" keyframe
	// So the "zero" keyframe will keep this as nullptr
	// so that other functions can know if they reached it
	MenuAnimKeyframe* previousFrame = nullptr;

	Entity* entity = nullptr;
	uint32_t time = 0; // duration + current time
	uint32_t duration = 0; // duration only

	// Properties to change during keyframe
	glm::vec3 targetPosition = glm::vec3(0, 0, 0);
	glm::vec4 targetColor = { 255, 255, 255, 255 };

	// Whether or not we are changing this property
	bool setPosition = false;
	bool setColor = false;

	// The actual method to change the properties
	void Update(uint32_t currentTime);
};


class KINJO_API MenuScreen
{	
public:	
	std::string name = "";
	BaseButton* selectedButton = nullptr;
	std::vector<BaseButton*> buttons;
	std::vector<Text*> texts;
	std::vector<Entity*> images;

	std::vector<MenuAnimKeyframe*> enterAnimation;
	std::vector<MenuAnimKeyframe*> exitAnimation;

	bool isPlayingEnterAnimation = false;
	bool isPlayingExitAnimation = false;

	// Can press Escape to pop the menu from the stack?
	// Usually true, but not for things like Title Screens, etc.
	bool canEscapeFrom = true;

	int selectedButtonIndex = 0;
	MenuScreen(const std::string& n, Game& game);
	~MenuScreen();
	
	void Render(const Renderer& renderer);
	bool Update(Game& game);
	
	virtual void CreateMenu(const std::string& n, Game& game);
	virtual bool PressSelectedButton(Game& game);

	void ResetMenu();
	
	BaseButton* GetButtonByName(const std::string& buttonName);
	void AssignButtons(bool useLeftRight);
	bool FileExists(const std::string& filepath);
};

#endif