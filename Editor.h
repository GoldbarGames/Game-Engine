#pragma once
#include "SDL.h"
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <GL/glew.h>
#include "editor_state.h"
#include "globals.h"
#include <string>
#include <vector>
#include <unordered_map>

#include "Text.h"
#include "EditorButton.h"

using std::string;

class Door;
class Entity;

class Editor
{
private:
	//SDL_Window* toolbox = nullptr;

	Game* game = nullptr;

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

	Door* currentDoor = nullptr;

	std::unordered_map<std::string, Entity*> previewMap;

	bool placingDoor = false;

public:
	Text* currentEditModeLayer = nullptr;
	Text* cursorPosition = nullptr;

	std::string objectMode = "tile";

	Entity * objectPreview = nullptr;

	Editor(Game &g);
	~Editor();
	void StartEdit();
	void StopEdit();
	void HandleEdit();
	void SaveLevel();
	void LoadLevel(std::string levelName);
	void Render(SDL_Renderer* renderer);
	void SetText(string newText, SDL_Renderer* renderer);
	DrawingLayer drawingLayer = BACKGROUND;
	int tilesheetIndex = 0;
	const string tilesheets[2] = { "housetiles5", "foresttiles" };
	void ClickedButton(std::string buttonName);
	void ToggleLayer();
	void ToggleTileset();
	void LeftClick(Vector2 clickedPosition, int mouseX, int mouseY);
	void RightClick(Vector2 clickedPosition);
	void SetLayer(DrawingLayer layer);
};

