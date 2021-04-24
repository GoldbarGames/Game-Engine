#ifndef MENUSCREEN_H
#define MENUSCREEN_H
#pragma once

#include <vector>
#include "glm/vec3.hpp"
#include "MenuButton.h"
#include "SpriteManager.h"
#include "leak_check.h"

class Entity;

struct KINJO_API MenuAnimKeyframe
{
	// We need the previous frame to get the start values.
	// For the first keyframe, we need to make a "zero" keyframe
	// using the starting values of the entity, and make sure
	// to not actually try and update the "zero" keyframe
	// So the "zero" keyframe will keep this as nullptr
	// so that other functions can know if they reached it
	MenuAnimKeyframe* previousFrame = nullptr;

	uint32_t time = 0; // duration + current time
	uint32_t duration = 0; // duration only

	// Properties to change during keyframe
	glm::vec3 targetPosition = glm::vec3(0, 0, 0);
	glm::vec4 targetColor = { 255, 255, 255, 255 };

	MenuAnimKeyframe();
	MenuAnimKeyframe(MenuAnimKeyframe* p, uint32_t d);

	// The actual method to change the properties
	void Update(Entity* entity, uint32_t currentTime);

	void CalculateTime();
};

struct KINJO_API MenuAnimation
{
	Entity* entity = nullptr;
	std::vector<MenuAnimKeyframe*> keyframes;

	MenuAnimation(Entity* e);
	MenuAnimKeyframe* CreateKeyframe(uint32_t duration);
};

class KINJO_API MenuScreen
{	
public:	
	std::string name = "";
	BaseButton* selectedButton = nullptr;
	std::vector<BaseButton*> buttons;
	std::vector<Text*> texts;
	std::vector<Entity*> images;

	BaseButton* lastButton = nullptr;

	std::vector<MenuAnimation*> enterAnimation;
	std::vector<MenuAnimation*> exitAnimation;

	bool isPlayingEnterAnimation = false;
	bool isPlayingExitAnimation = false;

	// Should this menu be recreated each time it is opened?
	// Set to true if this menu uses any variables
	bool isDynamic = false;

	// If true, then when we create the menu,
	// it should auto select the last button
	// that was previously selected rather than the first
	bool rememberLastButton = false;
	int lastButtonIndex = 0;

	// Can press Escape to pop the menu from the stack?
	// Usually true, but not for things like Title Screens, etc.
	bool canEscapeFrom = true;

	int selectedButtonIndex = 0;
	MenuScreen(const std::string& n, Game& game);
	~MenuScreen();
	
	void Render(const Renderer& renderer);
	virtual bool Update(Game& game);

	MenuAnimation* CreateEnterAnimation(Entity* entity);
	MenuAnimation* CreateExitAnimation(Entity* entity);

	virtual void CreateMenu(const std::string& n, Game& game);
	virtual bool PressSelectedButton(Game& game);

	virtual void ResetMenu();

	void HighlightSelectedButton(Game& game);
	void UnhighlightSelectedButton(Game& game);
	
	BaseButton* GetButtonByName(const std::string& buttonName);
	void AssignButtons(bool useLeftRight, bool useUpDown=true);
	bool FileExists(const std::string& filepath);
};

#endif