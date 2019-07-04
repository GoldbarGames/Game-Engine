#pragma once
#include "SDL.h"
#include <SDL_image.h>
#include <GL/glew.h>
#include "editor_state.h"

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
public:
	Editor();
	~Editor();
	void StartEdit(SDL_Renderer* renderer, SDL_Surface* tilesheet);
	void StopEdit();
	void HandleEdit(Game& game);
	void Render(SDL_Renderer* renderer);
};

