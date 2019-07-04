#pragma once
#include "SDL.h"
#include <SDL_image.h>
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

using std::string;

class Game
{
private:
	SDL_Window* window = nullptr;
	SDL_Window* toolbox = nullptr;

	SDL_Surface* screenSurface = nullptr;

	SDL_Renderer * renderer = nullptr;
	SDL_Renderer * rendererToolbox = nullptr;

	SDL_GLContext mainContext = nullptr;

	SpriteManager spriteManager;

	// EDITOR TILE STUFF

	SDL_Texture * toolboxTexture = nullptr;
	SDL_Rect toolboxTextureRect;
	SDL_Rect toolboxWindowRect;

	int editorTileX = 0;
	int editorTileY = 0;

	// EDITOR TILE STUFF


	int screenWidth = 640;
	int screenHeight = 480;

	Uint64 timeNow = SDL_GetPerformanceCounter();
	Uint64 timePrev = 0;
	
	void Update();
	void Render();
	bool SetOpenGLAttributes();
	Player* SpawnPlayer(Vector2 position);
	void SpawnPerson(Vector2 position);
	void SpawnTile(Vector2 frame, string tilesheet, Vector2 position, bool impassable);
	void CalcDt();
	void SortEntities();
public:
	bool pressedJumpButton;
	std::vector<Entity*> entities;
	Game();
	~Game();
	double dt = 0;
	void InitSDL();
	void EndSDL();
	void Play(string gameName);
};

