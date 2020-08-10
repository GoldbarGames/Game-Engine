#ifndef DEBUG_SCREEN_H
#define DEBUG_SCREEN_H
#pragma once

#include <unordered_map>
#include "Vector2.h"

class Text;
class Game;
class Renderer;
class Sprite;
class Camera;

enum class DebugText {
	cursorPositionInScreen, cursorPositionInWorld, currentEditModeLayer,
	drawCalls, updateCalls, collisionChecks, hoveredEntityID,
	cameraPosition, cameraAngle, cameraYaw, cameraPitch, cameraRoll
};

class DebugScreen
{
public:
	int mouseX = 0;
	int mouseY = 0;
	Game* game = nullptr;
	Sprite* sprite = nullptr;
	Camera* camera = nullptr;
	Vector2 worldPosition = Vector2(0, 0);
	std::unordered_map<DebugText, Text*> debugText;
	DebugScreen(Game& g);
	void Update();
	void Render(const Renderer& renderer);
	void CreateDebugText(const DebugText textName, const int x, const int y);
};

#endif
