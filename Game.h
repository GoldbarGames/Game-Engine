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
#include "Window.h"
#include "Camera.h"
#include "Texture.h"


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

	
public:

	std::unordered_map<std::string, std::vector<std::string>> spriteMap;

	void DeleteEntity(Entity* entity);
	void DeleteEntity(int index);

	// Player / Level Info
	int startingEther = 40;
	int currentEther = 40;
	int bugsDefeated = 0;
	int bugsRemaining = 0;


	using clock = std::chrono::steady_clock;
	using seconds = std::chrono::seconds;
	using milliseconds = std::chrono::milliseconds;

	clock::time_point startOfGame;
	clock::time_point start_time;

	std::unordered_map<std::string, MenuScreen*> allMenus;


	std::vector<SDL_Rect*> debugRectangles;

	void InitOpenGL();
	void CreateShaders();
	void CreateTextures();
	void CreateMeshes();
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

	Timer timer;
	Timer fpsLimit;
	bool limitFPS = false;

	bool goToNextLevel = false;
	std::string nextLevel = "";

	Uint32 lastPressedKeyTicks = 0;
	std::vector<Background*> backgrounds;

	CutsceneManager * cutscene;

	SDL_Rect overlayRect;
	Color overlayColor { 0, 0, 0, 0 };
	Color targetColor{ 0, 0, 0, 0 };
	bool changingOverlayColor = false;
	Timer timerOverlayColor;

	Renderer * renderer = nullptr;
	SpriteManager* spriteManager = nullptr;
	SoundManager* soundManager = nullptr;
	TTF_Font* theFont = nullptr;
	TTF_Font* headerFont = nullptr;
	
	Text* fpsText = nullptr;
	Text* timerText = nullptr;
	std::vector<MenuScreen*> openedMenus;

	Text* bugText = nullptr;
	Text* etherText = nullptr;

	bool watchingCutscene = false;
	bool getKeyboardInput = false;

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
	
	//TODO: Make an input class maybe
	bool pressedDebugButton;
	bool pressedSpellButton;
	bool pressedLeftTrigger;
	bool pressedRightTrigger;



	std::vector<Entity*> entities;
	void ShouldDeleteEntity(int index);
	void ShouldDeleteEntity(Entity* entity);

	Game();
	~Game();
	float dt = 0;


	void InitSDL();
	void EndSDL();
	void SortEntities(std::vector<Entity*>& entityVector);
	
	// Spawn functions
	Tile* SpawnTile(Vector2 frame, string tilesheet, Vector2 position, DrawingLayer drawingLayer);
	Player* SpawnPlayer(Vector2 position);
	Background* SpawnBackground(Vector2 pos);
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

	Vector2 CalcTileSpawnPos(Vector2 pos);

	Vector2 SnapToGrid(Vector2 position);

	void UpdateTextInput();
	void StartTextInput(std::string reason);
	void StopTextInput();

	void UpdateOverlayColor(int& color, const int& target);

	void EscapeMenu();

	void SaveSettings();
	void LoadSettings();

	void SaveEditorSettings();
	void LoadEditorSettings();

	void ReadAnimData(std::string dataFilePath, std::vector<AnimState*> & animStates);
	void SaveScreenshot();

	void RenderEntities(glm::mat4 projection, std::vector<Entity*> renderedEntities);
};

