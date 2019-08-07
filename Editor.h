#pragma once
#include "SDL.h"
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <GL/glew.h>
#include "editor_state.h"
#include "globals.h"
#include <string>
#include <vector>

#include "Text.h"
#include "EditorButton.h"

using std::string;

class Editor
{
private:
	//SDL_Window* toolbox = nullptr;

	SDL_Texture * toolboxTexture = nullptr;
	SDL_Rect toolboxTextureRect;
	SDL_Rect toolboxWindowRect;

	SDL_Rect selectedRect;
	SDL_Rect hoveredTileRect;

	int editorTileX = 0;
	int editorTileY = 0;

	Uint32 previousMouseState = 0;

	TTF_Font* theFont = nullptr;

	std::vector<EditorButton*> buttons;

public:
	Text* currentEditModeLayer = nullptr;
	Text* cursorPosition = nullptr;

	Editor(SDL_Renderer* renderer);
	~Editor();
	void StartEdit(Game &game);
	void StopEdit();
	void HandleEdit(Game& game);
	void SaveLevel(Game& game);
	void LoadLevel(Game& game, std::string levelName);
	void Render(SDL_Renderer* renderer);
	void SetText(string newText, SDL_Renderer* renderer);
	DrawingLayer drawingLayer = BACKGROUND;
	int tilesheetIndex = 0;
	const string tilesheets[2] = { "housetiles5", "foresttiles" };
	void ClickedButton(std::string buttonName, Game& game);
	void ToggleLayer();
	void ToggleTileset(Game& game);
};

