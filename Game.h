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
#include "Editor.h"

using std::string;

class Game
{
private:
	SDL_Surface* screenSurface = nullptr;

	SDL_Renderer * renderer = nullptr;
	
	SDL_GLContext mainContext = nullptr;

	SpriteManager spriteManager;

	Uint64 timeNow = SDL_GetPerformanceCounter();
	Uint64 timePrev = 0;

	Editor editor;
	
	void Update();
	void Render();
	bool SetOpenGLAttributes();

	
	void CalcDt();

	TTF_Font* theFont = nullptr;
	SDL_Texture* textTexture = nullptr;
	SDL_Surface* textSurface = nullptr;
	SDL_Rect textWindowRect;
	SDL_Rect textTextureRect;

	Mix_Music* currentBGM = nullptr;

	
	
	
	void MainLoop();
public:
	SDL_Window* window = nullptr;
	Player* player = nullptr;
	bool pressedJumpButton;
	std::vector<Entity*> entities;
	Game();
	~Game();
	double dt = 0;
	Vector2 camera = Vector2(0, 0);
	void InitSDL();
	void EndSDL();
	void Play(string gameName);
	void SortEntities();
	void SpawnTile(Vector2 frame, string tilesheet, Vector2 position, bool impassable, DrawingLayer drawingLayer);
	Player* SpawnPlayer(Vector2 position);
	void SpawnPerson(Vector2 position);
	void SetText(string newText);
	void PlayLevel(string gameName, string levelName);

};

