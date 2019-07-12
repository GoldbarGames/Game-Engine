#pragma once
#include "SDL.h"
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <GL/glew.h>
#include "editor_state.h"
#include "globals.h"
#include <string>

#include "Text.h"

using std::string;

class Game;

class Editor
{
private:
	SDL_Window* toolbox = nullptr;
	//SDL_Renderer * rendererToolbox = nullptr;

	SDL_Texture * toolboxTexture = nullptr;
	SDL_Rect toolboxTextureRect;
	SDL_Rect toolboxWindowRect;
	SDL_Rect selectedRect;

	int editorTileX = 0;
	int editorTileY = 0;

	TTF_Font* theFont = nullptr;

public:
	Text* currentEditModeLayer = nullptr;

	Editor(SDL_Renderer* renderer);
	~Editor();
	void StartEdit(SDL_Renderer* renderer, SDL_Surface* tilesheet);
	void StopEdit();
	void HandleEdit(Game& game);
	void SaveLevel(Game& game);
	void LoadLevel(Game& game, std::string levelName);
	void Render(SDL_Renderer* renderer);
	void SetText(string newText, SDL_Renderer* renderer);
	DrawingLayer drawingLayer = BACKGROUND;
	int tilesheetIndex = 0;
	const string tilesheets[2] = { "housetiles5", "foresttiles" };
};

