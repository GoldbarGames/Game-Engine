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
#include <deque>

#include "Text.h"
#include "EditorButton.h"

#include "Property.h"

#include "Texture.h"

using std::string;

class Door;
class Entity;
class Ladder;
class Renderer;
class NPC;
class Path;


class Editor
{
private:
	//SDL_Window* toolbox = nullptr;

	Game* game = nullptr;

	Vector2 tilesheetPosition = Vector2(0, 0);
	
	std::vector<Sprite*> tilesheetSprites;	
	std::vector<string> tilesheetFilenames = { "housetiles5", "foresttiles" };

	Vector2 spriteSheetTileFrame = Vector2(0,0);
	Vector2 selectedTilePosition = Vector2(0, 0);
	Vector2 objPreviewPosition = Vector2(0,0);

	SDL_Rect objectPropertiesRect;
	SDL_Rect dialogRect;

	Text* dialogText = nullptr;

	

	Uint32 previousMouseState = 0;

	TTF_Font* theFont = nullptr;

	std::vector<EditorButton*> buttons;
	std::vector<EditorButton*> layerButtons;
	std::vector<EditorButton*> layerVisibleButtons;

	std::vector<std::string> npcNames;

	Door* currentDoor = nullptr;
	Ladder* currentLadder = nullptr;
	NPC* currentNPC = nullptr;
	Path* currentPath = nullptr;

	int spriteMapIndex = 0;

	std::unordered_map<std::string, Entity*> previewMap;

	bool placingDoor = false;
	bool placingLadder = false;

	// Variables for the Properties Inspector
	Entity* selectedEntity = nullptr;
	std::vector<Property*> properties;	
	int propertyOptionIndex = 0;

	// Variables for the Grab Button
	Entity* grabbedEntity = nullptr;
	Vector2 oldGrabbedPosition = Vector2(0, 0);

	const unsigned int BUTTONS_PER_PAGE = 18;

public:
	int tilesheetIndex = 0;
	unsigned int currentButtonPage = 0;
	int propertyIndex = -1;
	int GRID_SIZE = 24;
	std::unordered_map<std::string, Text*> editorText;

	std::string objectMode = "tile";

	Entity * objectPreview = nullptr;

	Text* dialogInput = nullptr;
	bool showDialogPopup = false;

	// Settings
	int replaceSettingIndex = 0;
	int deleteSettingIndex = 0;
	int colorSettingIndex = 0;

	std::deque<string> levelStrings;
	int levelStringIndex = -1;

	Editor(Game &g);
	~Editor();
	void StartEdit();
	void StopEdit();
	void HandleEdit();
	void NewLevel();
	std::string SaveLevelAsString();
	void SaveLevel(std::string levelName = "");
	void InitLevelFromFile(std::string levelName);
	void Render(Renderer* renderer);
	DrawingLayer drawingLayer = BACK;


	EditorButton* clickedButton = nullptr;
	void ClickedButton();
	void ClickedLayerButton(std::string buttonText);
	void ToggleGridSize();
	void ToggleTileset();
	void ToggleObjectMode(std::string mode);
	void ToggleSpriteMap();
	void ToggleInspectionMode();

	void CreateEditorButtons();

	//TODO: Make these return bools, and if true, DoAction()?
	void LeftClick(Vector2 clickedPosition, int mouseX, int mouseY);
	void RightClick(Vector2 clickedPosition);
	void MiddleClick(Vector2 clickedPosition);

	void SetLayer(DrawingLayer layer);
	void DestroyLadder(std::string startingState, Vector2 lastPosition);
	void DrawGrid();

	void DestroyDialog();
	void CreateDialog(std::string txt);

	void PlaceTile(Vector2 clickedPosition, int mouseX, int mouseY);
	void PlaceObject(Vector2 clickedPosition, int mouseX, int mouseY);
	void InspectObject(int mouseX, int mouseY);
	void SetPropertyPositions();

	void SetPropertyText();

	void SetLayerButtonColor(Color color);

	std::string GetCurrentPropertyOptionString(int diff);

	void UndoAction();
	void RedoAction();
	void DoAction();

	void ClearLevelEntities();

	std::string ReadLevelFromFile(std::string levelName);
	void CreateLevelFromString(std::string level);
};

