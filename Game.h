#ifndef GAME_H
#define GAME_H
#pragma once

#include "SDL.h"
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <GL/glew.h>

#include <stdio.h>
#include <GL/glew.h>

#include <cmath>
#include <vector>
#include <unordered_map>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Mesh.h"
#include "Shader.h"
#include "Camera.h"
#include "Texture.h"

#include "Logger.h"

#include <string>
#include <iostream>
#include <vector>
#include "Sprite.h"
#include <memory>
#include <cmath>
#include "Entity.h"
#include "SpriteManager.h"
#include "Player.h"
#include "Background.h"
#include "Editor.h"
#include "Tile.h"
#include "Timer.h"
#include "MenuScreen.h"
#include "Missile.h"
#include "Door.h"
#include "Ladder.h"
#include "Renderer.h"
#include "NPC.h"
#include "Block.h"
#include "Platform.h"
#include "Shroom.h"

#include "DebugScreen.h"
#include "EntityFactory.h"
#include "QuadTree.h"
#include "GUI.h"

#include "CutsceneManager.h"
#include "SoundManager.h"
#include <chrono>

using std::string; 

enum class GameState { NORMAL, EDIT_MODE, ON_MENU, RESET_LEVEL, LOAD_NEXT_LEVEL };

class Game
{
private:
	SDL_Surface* screenSurface = nullptr;
	
	SDL_GLContext mainContext = nullptr;

	Uint64 timeNow = SDL_GetPerformanceCounter();
	Uint64 timePrev = 0;

	float currentAngle = 0.0f;
	const float toRadians = 3.14159265f / 180.0f;

	float now = 0;
	
	std::unordered_map<std::string, Mesh*> meshes;

	Mesh* CreateSpriteMesh();
	Mesh* CreateCubeMesh();
public:
	std::vector<std::string> npcNames;
	std::vector<std::string> enemyNames;
	std::vector<std::string> collectibleNames;

	std::string windowIconFilepath = "";
	std::string windowTitle = "";

	Logger* logger;
	SDL_GameController* controller;

	EntityFactory* entityFactory;

	SDL_Rect mouseRect;
	Uint32 mouseState;
	Uint32 previousMouseState;

	int screenWidth = 1280;
	int screenHeight = 720;

	bool debugMode = false;
	bool editMode = false;

	// TODO: Make this more like a bunch of colliders than a solid line
	// TODO: Also have a way to create these barriers via the editor
	int deathBarrierY = 500;

	Mesh* cubeMesh;
	QuadTree* quadTree;
	GUI gui;

	unsigned int collisionChecks = 0;
	unsigned int updateCalls = 0;

	std::vector<Entity*> quadrantEntities;
	
	std::unordered_map<std::string, std::vector<std::string>> spriteMap;

	void DeleteEntity(Entity* entity);
	void DeleteEntity(int index);

	// Player / Level Info
	int startingEther = 40;
	int currentEther = 40;
	int bugsDefeated = 0;
	int bugsRemaining = 0;

	// play this cutscene on level start
	std::string levelStartCutscene = "";

	using clock = std::chrono::steady_clock;
	using seconds = std::chrono::seconds;
	using milliseconds = std::chrono::milliseconds;

	clock::time_point startOfGame;
	clock::time_point start_time;

	std::unordered_map<std::string, MenuScreen*> allMenus;


	std::vector<SDL_Rect*> debugRectangles;

	void InitOpenGL();

	void Update();
	void Render();
	bool SetOpenGLAttributes();

	bool HandleEvent(SDL_Event& event);
	bool HandleMenuEvent(SDL_Event& event);
	void HandleEditMode();

	void GetMenuInput();

	void CalcDt();
	bool CheckInputs();
	void CheckDeleteEntities();

	void ResetText();

	void SetScreenResolution(const unsigned int width, const unsigned int height);
	Entity* CreateEntity(const std::string& entityName, const Vector2& position, int spriteIndex);
	Entity* SpawnEntity(const std::string& entityName, const Vector2& position, const int spriteIndex);

	Timer timer;
	Timer fpsLimit;
	bool limitFPS = false;
	
	GameState state;
	//GameState previousState;

	std::string nextLevel = "";
	std::string nextBGM = "";

	Uint32 lastPressedKeyTicks = 0;
	//std::vector<Background*> backgrounds;
	Background* background = nullptr;

	CutsceneManager* cutscene = nullptr;

	Renderer* renderer = nullptr;
	SpriteManager* spriteManager = nullptr;
	SoundManager* soundManager = nullptr;
	FontInfo* theFont = nullptr;
	FontInfo* headerFont = nullptr;
	
	Text* fpsText = nullptr;
	Text* timerText = nullptr;
	std::vector<MenuScreen*> openedMenus;

	Text* bugText = nullptr;
	Text* etherText = nullptr;

	bool getKeyboardInput = false;
	bool shouldQuit = false;

	std::string inputText = "";
	std::string inputReason = "";
	std::string inputType = "";

	std::string currentLevel = "";
	int levelNumber = 1;
	float dt = 0;
	int transitionExit = -1;
	int transitionEnter = -1;
	int transitionState = 0;

	DebugScreen* debugScreen = nullptr;
	Editor* editor = nullptr;
	SDL_Window* window = nullptr;
	Player* player = nullptr;

	bool isFullscreen = false;
	bool showFPS = false;
	bool showTimer = false;
	int indexScreenResolution = 0;
	
	//TODO: Make an input class maybe
	bool pressedDebugButton = false;
	bool pressedSpellButton = false;
	bool pressedLeftTrigger = false;
	bool pressedRightTrigger = false;

	std::vector<Entity*> entities;
	std::vector<Entity*> bgEntities;

	void ShouldDeleteEntity(int index);
	void ShouldDeleteEntity(Entity* entity);

	Game();
	~Game();

	void InitSDL();
	void EndSDL();
	void SortEntities(std::vector<Entity*>& entityVector);
	
	// Spawn functions
	Tile* CreateTile(Vector2 frame, string tilesheet, Vector2 position, DrawingLayer drawingLayer);
	Tile* SpawnTile(Vector2 frame, string tilesheet, Vector2 position, DrawingLayer drawingLayer);

	Player* SpawnPlayer(Vector2 position);
	Missile* SpawnMissile(Vector2 position);

	void TransitionLevel();

	void LoadTitleScreen();
	void LoadLevel(const std::string& level, int onExit=0, int onEnter=0);

	Vector2 CalculateObjectSpawnPosition(Vector2 mousePos, const int GRID_SIZE);

	Vector2 SnapToGrid(Vector2 position);

	void UpdateTextInput();
	void StartTextInput(const std::string& reason);
	void StopTextInput();

	void EscapeMenu();

	void SaveSettings();
	void LoadSettings();

	void SaveEditorSettings();
	void LoadEditorSettings();

	void SaveScreenshot(std::string filepath="");
};

#endif