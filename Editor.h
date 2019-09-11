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
class Ladder;
class Renderer;
class NPC;

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

	SDL_Rect objectPropertiesRect;

	int editorTileX = 0;
	int editorTileY = 0;

	Uint32 previousMouseState = 0;

	TTF_Font* theFont = nullptr;

	std::vector<EditorButton*> buttons;
	std::vector<EditorButton*> layerButtons;
	std::vector<EditorButton*> layerVisibleButtons;

	std::vector<std::string> npcNames;

	Door* currentDoor = nullptr;
	Ladder* currentLadder = nullptr;
	NPC* currentNPC = nullptr;

	int spriteMapIndex = 0;

	std::unordered_map<std::string, Entity*> previewMap;

	bool placingDoor = false;
	bool placingLadder = false;

	bool inspectionMode = false;

	Entity* selectedEntity = nullptr;
	std::vector<Text*> properties;	

public:
	int GRID_SIZE = 24;
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
	void Render(Renderer* renderer);
	void SetText(string newText);
	DrawingLayer drawingLayer = BACK;
	int tilesheetIndex = 0;
	const string tilesheets[2] = { "housetiles5", "foresttiles" };
	void ClickedButton(std::string buttonName);
	void ClickedLayerButton(std::string buttonText);
	void ToggleGridSize();
	void ToggleTileset();
	void ToggleObjectMode(std::string mode);
	void ToggleSpriteMap();
	void ToggleInspectionMode();
	void LeftClick(Vector2 clickedPosition, int mouseX, int mouseY);
	void RightClick(Vector2 clickedPosition);
	void SetLayer(DrawingLayer layer);
	void DestroyLadder(std::string startingState, Vector2 lastPosition);
	void DrawGrid();

	void PlaceTile(Vector2 clickedPosition, int mouseX, int mouseY);
	void PlaceObject(Vector2 clickedPosition, int mouseX, int mouseY);
	void InspectObject(int mouseX, int mouseY);
	void SetPropertyPositions();
};

