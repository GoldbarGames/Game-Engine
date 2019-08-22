#pragma once
#include "SDL.h"
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <SDL_mixer.h>
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

using std::string;

class Game
{
private:
	SDL_Surface * screenSurface = nullptr;
	
	SDL_GLContext mainContext = nullptr;

	Uint64 timeNow = SDL_GetPerformanceCounter();
	Uint64 timePrev = 0;

	void Update();
	void Render();
	bool SetOpenGLAttributes();
	
	void CalcDt();
	
	Mix_Music* currentBGM = nullptr;

	std::vector<Background*> backgrounds;

	Timer timer;
	Timer fpsLimit;

	bool limitFPS = false;

	std::unordered_map<std::string, MenuScreen*> allMenus;

	Uint32 lastPressedKeyTicks = 0;

	void MainLoop();
	bool HandleEvent(SDL_Event& event);
	bool HandleMenuEvent(SDL_Event& event);
	void HandleEditMode();

	
	void DeleteEntity(Entity* entity);
	void DeleteEntity(int index);
public:
	Renderer * renderer = nullptr;
	SpriteManager spriteManager;
	TTF_Font* theFont = nullptr;
	
	Text* jumpsRemainingText = nullptr;
	Text* fpsText = nullptr;
	Text* timerText = nullptr;
	std::vector<MenuScreen*> openedMenus;

	bool quit = false;

	Editor* editor = nullptr;
	SDL_Window* window = nullptr;
	Player* player = nullptr;
	
	//TODO: Make an input class maybe
	bool pressedJumpButton;
	bool pressedDebugButton;

	std::vector<Entity*> entities;
	void ShouldDeleteEntity(int index);

	Game();
	~Game();
	double dt = 0;
	Vector2 camera = Vector2(0, 0);
	void InitSDL();
	void EndSDL();
	void Play(string gameName);
	void SortEntities(std::vector<Entity*>& entityVector);
	
	// Spawn functions
	Tile* SpawnTile(Vector2 frame, string tilesheet, Vector2 position, DrawingLayer drawingLayer);
	Player* SpawnPlayer(Vector2 position);
	void SpawnPerson(Vector2 position);
	Background* SpawnBackground(Vector2 pos);
	bool SpawnMissile(Vector2 position, Vector2 velocity, float angle);

	Door* CreateDoor(Vector2 position); // returns the Door entity with default parameters
	Door* SpawnDoor(Vector2 position); // only returns Door if it can be spawned succesfully in-game, else null

	Ladder* CreateLadder(Vector2 position);
	Ladder* SpawnLadder(Vector2 position);
	
	void PlayLevel(string gameName, string levelName);



	Vector2 SnapToGrid(Vector2 position);

	//bool clickedMouse = false;
};

