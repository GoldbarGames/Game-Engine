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
public:
	glm::vec3 tilesheetPosition = glm::vec3(0, 0, 0);
	
	std::vector<Sprite*> tilesheetSprites;	
	std::vector<std::string> tilesheetFilenames;

	glm::vec2 spriteSheetTileFrame = glm::vec2(0, 0);
	glm::vec3 selectedTilePosition = glm::vec3(0, 0, 0);
	glm::vec3 objPreviewPosition = glm::vec3(0, 0, 0);

	SDL_Rect objectPropertiesRect;

	Uint32 previousMouseState = 0;

	std::vector<EditorButton*> buttons;
	std::vector<EditorButton*> layerButtons;
	std::vector<EditorButton*> layerVisibleButtons;

	std::vector<std::string> previewMapObjectNames;

	const std::string MODE_TILE = "tile";
	const std::string MODE_REPLACE = "replace";
	const std::string MODE_COPY = "copy";
	const std::string MODE_FILL = "fill";

	// Variables for the Properties Inspector
	Entity* selectedEntity = nullptr;
	std::vector<Property*> properties;	
	int propertyOptionIndex = 0;

	// Variables for the Grab Button
	//Entity* grabbedEntity = nullptr;
	//glm::vec3 oldGrabbedPosition = glm::vec3(0, 0, 0);

	Entity* grabbedMenuEntity = nullptr;

	std::vector<Entity*> grabbedEntities;
	std::vector<glm::vec3> oldGrabbedPositions;

	const unsigned int BUTTONS_PER_PAGE = 16;

	bool playOpeningDemoCutscene = true;

	Sprite* rectSprite = nullptr;
	Sprite* outlineSprite = nullptr;


	static FontInfo* fontInfo;

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
	std::unordered_map<std::string, std::vector<std::string>> levelFilesMap;

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
	void HandleGUIMode();
	void NewLevel();
	std::string SaveLevelAsString();
	void SaveLevel(const std::string& levelName = "");
	void InitLevelFromFile(const std::string& levelName);
	void Render(const Renderer& renderer);
	void RenderDebug(const Renderer& renderer);
	DrawingLayer drawingLayer = DrawingLayer::BACK;

	std::string ReadLoadingData(const std::string& data, std::unordered_map <std::string, std::vector<std::string>>& map);

	void ClickedButton();
	void ClickedLayerButton(const std::string& buttonText);
	void ToggleGridSize();
	void ToggleTileset();
	void ToggleObjectMode(const std::string& mode);
	void ToggleSpriteMap(int num);
	void ToggleInspectionMode();

	void UpdateLevelFiles();
	
	void CreateEditorButtons();

	void RefreshTilePreview();

	void LeftClick(glm::vec2 clickedScreenPosition, int mouseX, int mouseY, glm::vec3 clickedWorldPosition);
	void RightClick(glm::vec2 clickedPosition, int mouseX, int mouseY, glm::vec3 clickedWorldPosition);
	void MiddleClick(glm::vec2 clickedPosition, int mouseX, int mouseY, glm::vec3 clickedWorldPosition);

	Entity* GetEntityAtWorldPosition(const glm::vec3& clickedWorldPosition, bool includeTiles=false);

	void SetLayer(DrawingLayer layer);

	void CreateDialog(const std::string& txt);

	void FillTiles(const glm::vec3& spawnPosition, int depth=0);
	void PlaceTile(const glm::vec2& clickedPosition);
	void PlaceObject(const glm::vec2& mousePos);
	void InspectObject(const glm::vec3& clickedWorldPosition, const glm::vec2& clickedScreenPosition);
	void SetPropertyPositions();

	void SetPropertyText(const std::string& newText);

	std::string GetCurrentPropertyOptionString(int diff);

	void UndoAction();
	void RedoAction();
	void DoAction();

	void ClearLevelEntities();

	std::string ReadLevelFromFile(std::string levelName);
	
	void CreateLevelFromString(const std::string& level, const std::string& levelName);
	void CreateLevelFromVector(const std::vector<std::string>& lines);

	const std::string& GetTileSheetFileName(const int index) const;
};

#endif