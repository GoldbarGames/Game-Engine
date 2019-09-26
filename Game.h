#pragma once
#include "SDL.h"
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <GL/glew.h>
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
#include "CutsceneManager.h"
#include "SoundManager.h"

using std::string;

class Game
{
private:
	SDL_Surface * screenSurface = nullptr;
	
	SDL_GLContext mainContext = nullptr;

	Uint64 timeNow = SDL_GetPerformanceCounter();
	Uint64 timePrev = 0;

	

	int test = 0;

	
	std::unordered_map<int, std::string> spriteMapDoor;
	std::unordered_map<int, std::string> spriteMapLadder;
	std::unordered_map<int, std::string> spriteMapNPCs;

	void DeleteEntity(Entity* entity);
	void DeleteEntity(int index);
public:
	std::unordered_map<std::string, MenuScreen*> allMenus;

	void Update();
	void Render();
	bool SetOpenGLAttributes();

	bool HandleEvent(SDL_Event& event);
	bool HandleMenuEvent(SDL_Event& event);
	void HandleEditMode();

	void CalcDt();
	bool CheckInputs();
	void CheckDeleteEntities();

	Timer timer;
	Timer fpsLimit;
	bool limitFPS = false;

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
	
	Text* jumpsRemainingText = nullptr;
	Text* fpsText = nullptr;
	Text* timerText = nullptr;
	std::vector<MenuScreen*> openedMenus;

	bool watchingCutscene = false;
	bool getKeyboardInput = false;

	std::string inputText = "";
	std::string inputType = "";

	std::string currentLevel = "";

	Editor* editor = nullptr;
	SDL_Window* window = nullptr;
	Player* player = nullptr;

	bool isFullscreen = false;
	bool showFPS = false;
	bool showTimer = false;
	
	//TODO: Make an input class maybe
	bool pressedJumpButton;
	bool pressedDebugButton;

	std::vector<Entity*> entities;
	void ShouldDeleteEntity(int index);
	void ShouldDeleteEntity(Entity* entity);

	Game();
	~Game();
	double dt = 0;
	Vector2 camera = Vector2(0, 0);
	void InitSDL();
	void EndSDL();
	void SortEntities(std::vector<Entity*>& entityVector);
	
	// Spawn functions
	Tile* SpawnTile(Vector2 frame, string tilesheet, Vector2 position, DrawingLayer drawingLayer);
	Player* SpawnPlayer(Vector2 position);
	Background* SpawnBackground(Vector2 pos);
	bool SpawnMissile(Vector2 position, Vector2 velocity, float angle);

	Door* CreateDoor(Vector2 position, int spriteIndex); // returns the Door entity with default parameters
	Door* SpawnDoor(Vector2 position, int spriteIndex=0); // only returns Door if it can be spawned succesfully in-game, else null

	Ladder* CreateLadder(Vector2 position, int spriteIndex);
	Ladder* SpawnLadder(Vector2 position, int spriteIndex=0);

	NPC* CreateNPC(std::string name, Vector2 position, int spriteIndex);
	NPC* SpawnNPC(std::string name, Vector2 position, int spriteIndex = 0);
	
	void LoadTitleScreen();
	void PlayLevel(string levelName);
	Vector2 CalcObjPos(Vector2 pos);

	Vector2 SnapToGrid(Vector2 position);

	void UpdateTextInput();
	void StartTextInput(std::string reason);
	void StopTextInput();

	void UpdateOverlayColor(int& color, const int& target);

	void EscapeMenu();

	void SaveSettings();
	void LoadSettings();
};

