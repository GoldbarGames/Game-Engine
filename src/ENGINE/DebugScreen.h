#ifndef DEBUG_SCREEN_H
#define DEBUG_SCREEN_H
#pragma once

#include <unordered_map>
#include "Vector2.h"
#include "leak_check.h"
#include <SDL2/SDL.h>
#include <vector>

class Text;
class Game;
class Renderer;
class Sprite;
class Camera;
class EditorButton;

enum class DebugText {
	cursorPositionInScreen, cursorPositionInWorld, currentEditModeLayer,
	drawCalls, updateCalls, collisionChecks, hoveredEntityID,
	cameraPosition, cameraAngle, cameraYaw, cameraPitch, cameraRoll
};

class KINJO_API DebugScreen
{
public:
	int mouseX = 0;
	int mouseY = 0;
	Game* game = nullptr;
	Sprite* onePixelSprite = nullptr;
	Camera* camera = nullptr;
	glm::vec3 worldPosition = glm::vec3(0, 0, 0);
	Vector2 scale = Vector2(1, 1);
	std::unordered_map<DebugText, Text*> debugText;

	std::vector<std::string> cutsceneVariableNames;
	std::vector<Text*> variableTextLeft;
	std::vector<Text*> variableTextCenter;
	std::vector<Text*> variableTextRight;

	EditorButton* insertVariableButton = nullptr;
	EditorButton* removeVariableButton = nullptr;

	void InsertVariable(const std::string& variableName);
	void RemoveVariable(const std::string& variableName);

	Uint32 previousMouseState = 0;
	bool updatedLine = false;

	DebugScreen(Game& g);
	~DebugScreen();
	bool Update();
	void Render(const Renderer& renderer);
	void CreateDebugText(const DebugText textName, const int x, const int y);
};

#endif
