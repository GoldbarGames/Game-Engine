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
#include "Bug.h"
#include "Ether.h"
#include "Goal.h"
#include "Block.h"
#include "Platform.h"
#include "Shroom.h"


#include "CutsceneManager.h"
#include "SoundManager.h"
#include <chrono>

using std::string; 

enum class GameState { NORMAL, EDIT_MODE, ON_MENU, RESET_LEVEL, LOAD_NEXT_LEVEL };

class Game
{
private:
	SDL_Surface * screenSurface = nullptr;
	
	SDL_GLContext mainContext = nullptr;

	Uint64 timeNow = SDL_GetPerformanceCounter();
	Uint64 timePrev = 0;

	float currentAngle = 0.0f;
	const float toRadians = 3.14159265f / 180.0f;

	float now = 0;
	
	std::unordered_map<std::string, Mesh*> meshes;

	Mesh* CreateSpriteMesh();
public:

	std::string windowIconFilepath = "";
	std::string windowTitle = "";

	Logger* logger;

	int screenWidth = 1280;
	int screenHeight = 720;

	unsigned int collisionChecks = 0;
	unsigned int updateCalls = 0;
	
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
	void CreateShaders();
	void CreateObjects();

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
	 

	Timer timer;
	Timer fpsLimit;
	bool limitFPS = false;
	
	GameState state;
	//GameState previousState;

	std::string nextLevel = "";

	Uint32 lastPressedKeyTicks = 0;
	//std::vector<Background*> backgrounds;
	Background* background;

	CutsceneManager* cutscene;

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
	std::string inputType = "";

	std::string currentLevel = "";
	int levelNumber = 1;

	Editor* editor = nullptr;
	SDL_Window* window = nullptr;
	Player* player = nullptr;

	bool isFullscreen = false;
	bool showFPS = false;
	bool showTimer = false;
	int indexScreenResolution = 0;
	
	//TODO: Make an input class maybe
	bool pressedDebugButton;
	bool pressedSpellButton;
	bool pressedLeftTrigger;
	bool pressedRightTrigger;



	std::vector<Entity*> entities;
	std::vector<Entity*> bgEntities;

	void ShouldDeleteEntity(int index);
	void ShouldDeleteEntity(Entity* entity);

	Game();
	~Game();
	float dt = 0;


	void InitSDL();
	void EndSDL();
	void SortEntities(std::vector<Entity*>& entityVector);
	
	// Spawn functions
	Tile* CreateTile(Vector2 frame, string tilesheet, Vector2 position, DrawingLayer drawingLayer);
	Tile* SpawnTile(Vector2 frame, string tilesheet, Vector2 position, DrawingLayer drawingLayer);
	Player* SpawnPlayer(Vector2 position);
	Missile* SpawnMissile(Vector2 position, Vector2 velocity, float angle);

	Door* CreateDoor(Vector2 position, int spriteIndex); // returns the Door entity with default parameters
	Door* SpawnDoor(Vector2 position, int spriteIndex=0); // only returns Door if it can be spawned successfully in-game, else null

	Ladder* CreateLadder(Vector2 position, int spriteIndex);
	Ladder* SpawnLadder(Vector2 position, int spriteIndex=0);

	NPC* CreateNPC(std::string name, Vector2 position, int spriteIndex);
	NPC* SpawnNPC(std::string name, Vector2 position, int spriteIndex = 0);

	Bug* CreateBug(Vector2 position, int spriteIndex);
	Bug* SpawnBug(Vector2 position, int spriteIndex = 0);

	Ether* CreateEther(Vector2 position, int spriteIndex);
	Ether* SpawnEther(Vector2 position, int spriteIndex = 0);

	Goal* CreateGoal(Vector2 position, int spriteIndex);
	Goal* SpawnGoal(Vector2 position, int spriteIndex = 0);

	Block* CreateBlock(Vector2 position, int spriteIndex);
	Block* SpawnBlock(Vector2 position, int spriteIndex = 0);

	Platform* CreatePlatform(Vector2 position, int spriteIndex);
	Platform* SpawnPlatform(Vector2 position, int spriteIndex = 0);

	Shroom* CreateShroom(Vector2 position, int spriteIndex);
	Shroom* SpawnShroom(Vector2 position, int spriteIndex = 0);

	void LoadTitleScreen();
	void PlayLevel(string levelName);
	void LoadNextLevel();

	Vector2 CalculateObjectSpawnPosition(Vector2 mousePos, const int GRID_SIZE);

	Vector2 SnapToGrid(Vector2 position);

	void UpdateTextInput();
	void StartTextInput(std::string reason);
	void StopTextInput();

	void EscapeMenu();

	void SaveSettings();
	void LoadSettings();

	void SaveEditorSettings();
	void LoadEditorSettings();

	void SaveScreenshot(std::string filepath="");
};

