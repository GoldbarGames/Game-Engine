#ifndef EDITOR_H
#define EDITOR_H
#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include "globals.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <deque>
#include "leak_check.h"
#include "EditorButton.h"
#include "Property.h"
#include "Texture.h"
#include "filesystem_types.h"
#include "EditorHelper.h"

class Door;
class Entity;
class Ladder;
class Renderer;
class NPC;
class Path;
class Text;
class Dialog;
class Platform;

class KINJO_API Editor
{
private:
	Vector2 tilesheetPosition = Vector2(0, 0);
	
	std::vector<Sprite*> tilesheetSprites;	
	std::vector<std::string> tilesheetFilenames;

	Vector2 spriteSheetTileFrame = Vector2(0,0);
	Vector2 selectedTilePosition = Vector2(0, 0);
	Vector2 objPreviewPosition = Vector2(0,0);

	SDL_Rect objectPropertiesRect;

	Uint32 previousMouseState = 0;

	std::vector<EditorButton*> buttons;
	std::vector<EditorButton*> layerButtons;
	std::vector<EditorButton*> layerVisibleButtons;

	std::vector<std::string> previewMapObjectNames;

	// Variables for the Properties Inspector
	Entity* selectedEntity = nullptr;
	std::vector<Property*> properties;	
	int propertyOptionIndex = 0;

	// Variables for the Grab Button
	Entity* grabbedEntity = nullptr;
	Vector2 oldGrabbedPosition = Vector2(0, 0);

	const unsigned int BUTTONS_PER_PAGE = 16;

	bool playOpeningDemoCutscene = true;

	Sprite* rectSprite = nullptr;
	Sprite* outlineSprite = nullptr;
public:
	static FontInfo* fontInfo;
	int cameraTargetID = -1;
	bool switchTargetBackToPlayer = false;

	int entitySubtype = 0;
	Game* game = nullptr;
	EditorHelper* helper = nullptr;
	std::unordered_map<std::string, Entity*> previewMap;
	
	int tilesheetIndex = 0;
	unsigned int currentButtonPage = 0;
	int propertyIndex = -1;
	int GRID_SIZE = 24;
	int SPAWN_TILE_SIZE = 24;

	std::unordered_map<std::string, std::vector<std::string>> loadDataMap;

	std::string startEditorLevel = "";

	std::string objectMode = "tile";

	Entity* objectPreview = nullptr;
	Sprite* grid = nullptr;

	Dialog* dialog;
	EditorButton* clickedButton = nullptr;
	Text* currentLevelText = nullptr;

	std::vector<std::string> levelNames;
	void GetLevelList();

	// Settings
	int replaceSettingIndex = 0;
	int deleteSettingIndex = 0;
	int colorSettingIndex = 0;

	std::deque<string> levelStrings;
	int levelStringIndex = -1;

	unsigned int hoveredEntityID = 0;

	Editor(Game &g);
	~Editor();
	void StartEdit();
	void StopEdit();
	void HandleEdit();
	void NewLevel();
	std::string SaveLevelAsString();
	void SaveLevel(std::string levelName = "");
	void InitLevelFromFile(std::string levelName);
	void Render(const Renderer& renderer);
	void RenderDebug(const Renderer& renderer);
	DrawingLayer drawingLayer = DrawingLayer::BACK;

	std::string ReadLoadingData(const std::string& data, std::unordered_map < std::string, std::vector<std::string>>& map);

	void ClickedButton();
	void ClickedLayerButton(std::string buttonText);
	void ToggleGridSize();
	void ToggleTileset();
	void ToggleObjectMode(std::string mode);
	void ToggleSpriteMap(int num);
	void ToggleInspectionMode();

	void UpdateLevelFiles();
	
	void CreateEditorButtons();

	//TODO: Make these return bools, and if true, DoAction()?
	void LeftClick(Vector2 clickedScreenPosition, int mouseX, int mouseY, Vector2 clickedWorldPosition);
	void RightClick(Vector2 clickedPosition, int mouseX, int mouseY, Vector2 clickedWorldPosition);
	void MiddleClick(Vector2 clickedPosition, int mouseX, int mouseY, Vector2 clickedWorldPosition);

	Entity* GetClickedEntity(const Vector2& clickedWorldPosition, bool includeTiles=false);

	void SetLayer(DrawingLayer layer);
	void DestroyLadder(std::string startingState, Vector2 lastPosition);

	void DestroyDialog();
	void CreateDialog(const std::string& txt);

	void PlaceTile(Vector2 clickedPosition, int mouseX, int mouseY);
	void PlaceObject(Vector2 clickedPosition, int mouseX, int mouseY);
	void InspectObject(const Vector2& clickedWorldPosition, const Vector2& clickedScreenPosition);
	void SetPropertyPositions();

	void SetPropertyText(const std::string& newText);

	std::string GetCurrentPropertyOptionString(int diff);

	void UndoAction();
	void RedoAction();
	void DoAction();

	void ClearLevelEntities();

	std::string ReadLevelFromFile(std::string levelName);
	void CreateLevelFromString(std::string level);
	const std::string& GetTileSheetFileName(const int index) const;
};

#endif