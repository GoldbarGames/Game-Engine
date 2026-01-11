#include "Editor.h"
#include "Game.h"
#include "globals.h"
#include <fstream>
#include <sstream>
#include <iterator>
#include "Tile.h"
#include "CutsceneTrigger.h"
#include "Dialog.h"
#include "CutsceneCommands.h"
#include "Renderer.h"
#include "DebugScreen.h"
#include "QuadTree.h"
#include "Property.h"
#include "Logger.h"
#include "CutsceneManager.h"
#include "Background.h"
#include "EditorHelper.h"
#include "MenuScreen.h"
#include "CutsceneFunctions.h"

FontInfo* Editor::fontInfo;

Editor::Editor(Game& g)
{
	game = &g;

	GRID_SIZE = Globals::TILE_SIZE;
	SPAWN_TILE_SIZE = Globals::TILE_SIZE;

	tilesheetFilenames = ReadStringsFromFile("data/lists/tilesheet.list");
	for (size_t i = 0; i < tilesheetFilenames.size(); i++)
	{
		tilesheetFilenames[i] = "assets/tiles/" + tilesheetFilenames[i] + ".png";
	}

	GetLevelList();

	if (fontInfo == nullptr)
	{
		fontInfo = game->theFont;
	}

	currentLevelText = new Text(fontInfo, "");
	currentLevelText->SetPosition(100, 50);
	currentLevelText->GetSprite()->keepPositionRelativeToCamera = true;
	currentLevelText->GetSprite()->keepScaleRelativeToCamera = true;

	playOpeningDemoCutscene = false;
	dialog = new Dialog(glm::vec3(g.screenWidth, g.screenHeight, 0), &g.spriteManager);
	dialog->text = new Text(fontInfo, "");
	dialog->input = new Text(fontInfo, "");

	dialog->text->SetPosition(dialog->position.x, dialog->position.y + 20);
	dialog->input->SetPosition(dialog->position.x, dialog->position.y + 70);

	// shaders[4] = SolidColor
	dialog->sprite->SetShader(game->renderer.shaders[4]);
	dialog->sprite->color = { 255, 0, 0, 255 };
	dialog->sprite->keepPositionRelativeToCamera = true;
	dialog->sprite->keepScaleRelativeToCamera = true;
	dialog->scale = (game->renderer.CalculateScale(*dialog->sprite, 
		dialog->text->GetTextWidth(), dialog->text->GetTextHeight() * 4, dialog->text->scale));

	dialog->text->GetSprite()->keepPositionRelativeToCamera = true;
	dialog->input->GetSprite()->keepPositionRelativeToCamera = true;
	dialog->text->GetSprite()->keepScaleRelativeToCamera = true;
	dialog->input->GetSprite()->keepScaleRelativeToCamera = true;


	grid = new Sprite(game->renderer.shaders[1]);
	//grid->SetScale(Vector2(game->screenWidth, game->screenHeight));

	game->entities.clear();	

	SetLayer(DrawingLayer::BACK);

	objectPropertiesRect.w = 400;
	objectPropertiesRect.h = 600;
	objectPropertiesRect.y = 100;

	// Check to see if the object properties (saving/loading)
	// require the level to be re-calculated
	UpdateLevelFiles();
}

Editor::~Editor()
{
	for (auto& [key, val] : previewMap)
	{
		if (val != nullptr)
			delete_it(val);
	}

	for (size_t i = 0; i < buttons.size(); i++)
	{
		if (buttons[i] != nullptr)
			delete_it(buttons[i]);
	}

	for (size_t i = 0; i < layerButtons.size(); i++)
	{
		if (layerButtons[i] != nullptr)
			delete_it(layerButtons[i]);
	}

	for (size_t i = 0; i < layerVisibleButtons.size(); i++)
	{
		if (layerVisibleButtons[i] != nullptr)
			delete_it(layerVisibleButtons[i]);
	}

	for (size_t i = 0; i < tilesheetSprites.size(); i++)
	{
		if (tilesheetSprites[i] != nullptr)
			delete_it(tilesheetSprites[i]);
	}

	if (dialog != nullptr)
		delete_it(dialog);	

	if (grid != nullptr)
		delete_it(grid);

	if (currentLevelText != nullptr)
		delete_it(currentLevelText);

	if (outlineSprite != nullptr)
		delete_it(outlineSprite);

	if (rectSprite != nullptr)
		delete_it(rectSprite);
}

// Updates the level file based on changes in how entities are saved/loaded
void Editor::UpdateLevelFiles()
{
	// Fill in the new array
	loadDataMap.clear();
	const std::string LOAD_FILE = "data/config/load_key.dat";
	std::string newData = ReadLoadingData(LOAD_FILE, loadDataMap);

	const std::string OLD_FILEPATH = "data/config/load_old.dat";
	std::unordered_map<std::string, std::vector<std::string>> oldMap;
	std::string oldData = ReadLoadingData(OLD_FILEPATH, oldMap);

	// If no changes, return now
	if (newData == oldData)
	{
		return;
	}

#if _DEBUG
	const std::string STR_ENTITY("entity");

	std::string levelsFolder = "data\\levels\\";
	fs::path path = fs::current_path().append(levelsFolder);
	for (const auto& entry : fs::directory_iterator(path))
	{
		if (entry.path().extension().string() == ".lvl")
		{
			std::string levelName = entry.path().stem().string();
			std::cout << levelName << std::endl;

			std::unordered_map<std::string, std::vector<glm::vec3>> reorderMap;
			std::unordered_map<std::string, int> resizeMap;

			// Store the new size of the list in order to resize it (add/remove)
			int oldEntitySize = oldMap[STR_ENTITY].size();
			int newEntitySize = loadDataMap[STR_ENTITY].size();

			for (int i = 0; i < oldEntitySize; i++)
			{
				// Check for any elements that have been re-ordered or re-named
				std::vector<std::string>::iterator it = std::find(loadDataMap[STR_ENTITY].begin(),
					loadDataMap[STR_ENTITY].end(), oldMap[STR_ENTITY][i]);

				// If we can find this element in the new data, 
				// keep track of where to place it in the new array
				if (it != loadDataMap[STR_ENTITY].end())
				{
					int newIndex = std::distance(loadDataMap[STR_ENTITY].begin(), it);
					reorderMap[STR_ENTITY].push_back(glm::vec3(i, newIndex, 0));
					// index 2 old => index 2 new
					// index 3 old => index 4 new
					// etc...
				}

				// Ignore elements we can't find anymore
			}

			// TODO: Maybe figure out how to handle numbers vs. strings better
			for (auto const& [entityType, currentList] : oldMap)
			{
				for (int i = 0; i < currentList.size(); i++)
				{
					if (entityType != STR_ENTITY)
					{
						// Store the new size of the list in order to resize it (add/remove)
						resizeMap[entityType] = loadDataMap[entityType].size();

						// Check for any elements that have been re-ordered
						std::vector<std::string>::iterator it = std::find(loadDataMap[entityType].begin(),
							loadDataMap[entityType].end(), currentList[i]);

						// If we can find this element in the new data...
						if (it != loadDataMap[entityType].end())
						{
							// keep track of the indices here and swap them in the level file
							int newIndex = std::distance(loadDataMap[entityType].begin(), it);
							reorderMap[entityType].push_back(glm::vec3(i, newIndex, 0));
						}
					}
				}
			}

			// Update the level files
			std::ofstream fout;
			std::stringstream ss{ ReadLevelFromFile(levelName) };

			const int LINE_SIZE = 1024;
			char lineChar[LINE_SIZE];
			ss.getline(lineChar, LINE_SIZE);

			int line = 0;
			int index = 0;

			std::string newLevel = "";
			std::string etype = "";

			while (ss.good() && !ss.eof())
			{
				std::istringstream buf(lineChar);
				std::istream_iterator<std::string> beg(buf), end;
				std::vector<std::string> tokens(beg, end);

				// Get the type of entity we are dealing with
				if (tokens.size() > 0)
					etype = tokens[1];

				// Depending on the type of entity, figure out the rest

				// 1. Create a new vector of strings equal to the new size
				std::vector<std::string> newTokens;

				oldEntitySize = oldMap[STR_ENTITY].size();
				newEntitySize = loadDataMap[STR_ENTITY].size();

				int difference = newEntitySize - oldEntitySize;

				if (resizeMap.count(etype) != 0)
				{
					oldEntitySize += resizeMap[etype];
					newEntitySize += resizeMap[etype];
				}

				for (int i = 0; i < newEntitySize; i++)
				{
					newTokens.push_back("0");
				}

				// 2. For each reorder, take the value in the old index, place it in new index
				// 3. For anything that is not a reorder, just keep the values where they are
				int reorderEntitySize = reorderMap[STR_ENTITY].size();
				for (int i = 0; i < reorderEntitySize; i++)
				{
					int oldIndex = reorderMap[STR_ENTITY][i].x;
					int newIndex = reorderMap[STR_ENTITY][i].y;
					if (tokens.size() > oldIndex)
						newTokens[newIndex] = tokens[oldIndex];
				}

				int reorderTypeSize = reorderMap[etype].size();

				if (etype == "camera")
				{
					for (int i = 0; i < reorderTypeSize; i++)
					{
						int oldIndex = reorderMap[etype][i].x;
						int newIndex = reorderMap[etype][i].y + difference;
						newTokens[newIndex] = tokens[oldIndex];
					}
				}
				else
				{
					for (int i = 0; i < reorderTypeSize; i++)
					{
						int oldIndex = reorderMap[etype][i].x + reorderEntitySize;
						int newIndex = reorderMap[etype][i].y + reorderEntitySize + difference;
						newTokens[newIndex] = tokens[oldIndex];
					}
				}


				// After setting all the tokens, write them to the level string
				for (int i = 0; i < newTokens.size(); i++)
				{
					newLevel += newTokens[i];
					newLevel += " ";
				}

				newLevel += "\n";

				ss.getline(lineChar, LINE_SIZE);
				//std::cout << lineChar << std::endl;
			}

			// Output the new level to a file
			fout.open("data/levels/" + levelName + ".lvl");
			fout << newLevel;
			fout.close();
		}
	}

	// After updating the level files,
	// update the old loading file
	std::ofstream fout;
	fout.open(OLD_FILEPATH);
	fout << newData;
	fout.close();
#endif

	
}

std::string Editor::ReadLoadingData(const std::string& filepath, 
	std::unordered_map <std::string, std::vector<std::string>>& map)
{
	std::ifstream fin;
	fin.open(filepath);

	std::string loadingData = "";
	for (std::string line; std::getline(fin, line); )
	{
		loadingData += line + "\n";
	}

	fin.close();

	std::stringstream ss{ loadingData };
	std::string etype = "";

	const int LINE_SIZE = 1024;
	char lineChar[LINE_SIZE];
	ss.getline(lineChar, LINE_SIZE);

	// Read in data for loading from files
	map.clear();
	while (ss.good() && !ss.eof())
	{
		std::istringstream buf(lineChar);
		std::istream_iterator<std::string> beg(buf), end;
		std::vector<std::string> tokens(beg, end);

		etype = tokens[0].substr(0, tokens[0].size() - 1);

		for (size_t i = 1; i < tokens.size(); i++)
		{
			map[etype].push_back(tokens[i]);
		}

		ss.getline(lineChar, LINE_SIZE);
	}

	return loadingData;
}

void Editor::CreateEditorButtons()
{
	// Create all the buttons for the bottom of the editor
	for (unsigned int i = 0; i < buttons.size(); i++)
	{
		delete_it(buttons[i]);
	}

	buttons.clear();

	const int buttonStartX = 50;
	const int buttonWidth = 50;
	const int buttonHeight = 50 * (game->screenHeight/1280.0f);
	const int buttonSpacing = 20;

	int buttonX = buttonStartX + buttonWidth + buttonSpacing;

	std::vector<string> buttonNames = ReadStringsFromFile("data/lists/editorbuttons.list"); 

	auto objectIndex = std::find(buttonNames.begin(), buttonNames.end(), "objects");
	
	if (objectIndex != buttonNames.end())
	{
		buttonNames.insert(objectIndex, previewMapObjectNames.begin(), previewMapObjectNames.end());
		objectIndex = std::find(buttonNames.begin(), buttonNames.end(), "objects");
		buttonNames.erase(objectIndex);
	}
	else
	{
		buttonNames.insert(buttonNames.begin() + 7, previewMapObjectNames.begin(), previewMapObjectNames.end());
	}

	unsigned int BUTTON_LIST_START = currentButtonPage * BUTTONS_PER_PAGE;
	unsigned int BUTTON_LIST_END = BUTTON_LIST_START + BUTTONS_PER_PAGE;

	if (BUTTON_LIST_END > buttonNames.size())
		BUTTON_LIST_END = buttonNames.size();

	float bPadding = buttonHeight * Camera::MULTIPLIER;

	float buttonY = (game->screenHeight * Camera::MULTIPLIER) - (bPadding * 2);
	float buttonY2 = (game->screenHeight * Camera::MULTIPLIER) - (bPadding * 2 * Camera::MULTIPLIER);

	for (unsigned int i = BUTTON_LIST_START; i < BUTTON_LIST_END; i++)
	{
		if (i > buttonNames.size() - 1)
			break;

		EditorButton* editorButton = new EditorButton("", buttonNames[i], 
			glm::vec3(buttonX * Camera::MULTIPLIER, buttonY, 0), *game);
		
		buttons.emplace_back(editorButton);

		buttonX += buttonWidth + buttonSpacing; 
	}

	EditorButton* previousButton = new EditorButton("", "prevpage", 
		glm::vec3(buttonStartX * Camera::MULTIPLIER, buttonY, 0), *game);
	
	buttons.emplace_back(previousButton);

	EditorButton* nextButton = new EditorButton("", "nextpage", 
		glm::vec3(buttonX * Camera::MULTIPLIER, buttonY, 0), *game);
	
	buttons.emplace_back(nextButton);

	// Level navigation

	EditorButton* previousLevelButton = new EditorButton("", "prevlevel",
		glm::vec3((buttonStartX + (buttonWidth + buttonSpacing)) * Camera::MULTIPLIER,
			buttonY2, 0), *game);

	buttons.emplace_back(previousLevelButton);

	EditorButton* nextLevelButton = new EditorButton("", "nextlevel",
		glm::vec3((buttonStartX + ( 2 * (buttonWidth + buttonSpacing))) * Camera::MULTIPLIER, 
			buttonY2, 0), *game);

	buttons.emplace_back(nextLevelButton);

	for (auto& button : buttons)
	{
		button->image->keepScaleRelativeToCamera = true;
	}
}

void Editor::StartEdit()
{
	game->LoadEditorSettings();

	grabbedEntities.clear();
	oldGrabbedPositions.clear();

	helper->OnEditorStart();

	previewMap[MODE_TILE] = game->CreateTile(glm::vec2(0, 0), 0,
		glm::vec3(0, 0, 0), DrawingLayer::FRONT);

	previewMap[MODE_TILE]->GetSprite()->color = { 255, 255, 255, 64 };

	// Create a preview object of every entity type
	previewMapObjectNames = ReadStringsFromFile("data/lists/entityTypes.list");

	for (size_t i = 0; i < previewMapObjectNames.size(); i++)
	{
		previewMap[previewMapObjectNames[i]] = game->CreateEntity(previewMapObjectNames[i],
			glm::vec3(0, 0, 0), entitySubtype);
	}

	objectPreview = previewMap[MODE_TILE];

	//game->renderer.camera.ResetProjection();
	currentLevelText->SetText(game->currentLevel);

	// TILE SHEET FOR TOOLBOX
	if (tilesheetSprites.empty())
	{
		// shaders[3] = NoAlpha
		for (size_t i = 0; i < tilesheetFilenames.size(); i++)
		{
			tilesheetSprites.push_back(new Sprite(1, game->spriteManager,
				tilesheetFilenames[i], game->renderer.shaders[3], glm::vec2(0, 0)));

			tilesheetSprites[i]->keepPositionRelativeToCamera = true;
			tilesheetSprites[i]->keepScaleRelativeToCamera = true;
		}
	}
	
	tilesheetPosition.x = (game->screenWidth * 2) - tilesheetSprites[tilesheetIndex]->frameWidth;
	tilesheetPosition.y = tilesheetSprites[tilesheetIndex]->frameHeight;

	// this centers the yellow rectangle on the top left tile in the tilesheet
	// (we need to subtract the width/height to get to the top left corner,
	// and then add the tile size to center it within the actual tile)
	selectedTilePosition.x = tilesheetPosition.x - tilesheetSprites[tilesheetIndex]->frameWidth + SPAWN_TILE_SIZE;
	selectedTilePosition.y = tilesheetPosition.y - tilesheetSprites[tilesheetIndex]->frameHeight + SPAWN_TILE_SIZE;

	objectPropertiesRect.x = (game->screenWidth * 2) - objectPropertiesRect.w;

	CreateEditorButtons();

	// Create the layer buttons

	for (unsigned int i = 0; i < layerButtons.size(); i++)
		delete_it(layerButtons[i]);
	layerButtons.clear();

	for (unsigned int i = 0; i < layerVisibleButtons.size(); i++)
		delete_it(layerVisibleButtons[i]);
	layerVisibleButtons.clear();

	int buttonX = 200;
	int buttonY = 200;
	const int layerButtonWidth = 100;
	const int layerButtonHeight = 50;
	const int layerButtonSpacing = 80;

	// To add a new layer:
	// 1. Add the name here
	// 2. Add the enum
	// 3. In renderer, add the layersVisible for it to true
	// 4. In globals.cpp, add the switch/case for it

	std::vector<string> layerButtonNames = { "BACK", "MIDDLE", "OBJECT", "COLLISION", "COLLISION2", "FRONT", "BG"};

	for (unsigned int i = 0; i < layerButtonNames.size(); i++)
	{
		EditorButton* layerButton = new EditorButton(layerButtonNames[i], "Layer", 
			glm::vec3(buttonX, buttonY, 0), *game, glm::vec2(layerButtonWidth, 50), { 255, 255, 255, 255 });

		layerButton->image->keepScaleRelativeToCamera = true;
		layerButton->text->GetSprite()->keepScaleRelativeToCamera = true;
		layerButtons.emplace_back(layerButton);
		
		EditorButton* layerVisibleButton = new EditorButton("", "Visible", 
			glm::vec3(buttonX - 125, buttonY, 0), *game, glm::vec2(50, 50), { 255, 255, 255, 255 });

		layerVisibleButton->image->keepScaleRelativeToCamera = true;
		layerVisibleButton->text->GetSprite()->keepScaleRelativeToCamera = true;
		layerVisibleButtons.emplace_back(layerVisibleButton);

		buttonY += layerButtonHeight + layerButtonSpacing; // TODO: is there a way to not make this hard-coded? is it worth it?
	}

	ClickedLayerButton("BACK");
}

void Editor::StopEdit()
{
	//game->renderer.camera.Zoom(0.0f, game->screenWidth, game->screenHeight);	
	//inspectionMode = false;	
	game->SaveEditorSettings();
	selectedEntity = nullptr;
	propertyIndex = -1;

	game->CheckDeleteEntities();

	helper->OnEditorEnd();
}

void Editor::RefreshTilePreview()
{
	Entity*& prev = previewMap[objectMode];
	
	if (prev != nullptr)
		delete_it(prev);

	prev = game->CreateTile(spriteSheetTileFrame, tilesheetIndex,
		glm::vec3(0, 0, 0), DrawingLayer::FRONT);

	objectPreview = prev;
}

void Editor::LeftClick(glm::vec2 clickedScreenPosition, int mouseX, int mouseY, glm::vec3 clickedWorldPosition)
{
	bool clickedToolboxWindow = mouseX >= tilesheetPosition.x - tilesheetSprites[tilesheetIndex]->frameWidth
		&& mouseY <= tilesheetSprites[tilesheetIndex]->frameHeight * Camera::MULTIPLIER;

	bool clickedNewButton = false;

	bool onlyPressedButtonDown = !(previousMouseState & SDL_BUTTON(SDL_BUTTON_LEFT));

	// We are definitely holding the left mouse button at this point,
	// but this checks whether we are just now pressing it down.
	if (onlyPressedButtonDown)
	{
		// Get name of the button that was clicked, if any
		for (unsigned int i = 0; i < buttons.size(); i++)
		{
			if (buttons[i]->IsPointInsideButton(mouseX, mouseY))
			{
				clickedNewButton = true;
				if (buttons[i] == clickedButton)
				{
					clickedButton->isClicked = false;
				}
				else
				{
					if (clickedButton != nullptr)
						clickedButton->isClicked = false;

					clickedButton = buttons[i];
					clickedButton->isClicked = true;
				}
			}
		}
	}

	std::string clickedLayerButton = "";
	for (unsigned int i = 0; i < layerButtons.size(); i++)
	{
		if (layerButtons[i]->IsPointInsideButton(mouseX, mouseY))
			clickedLayerButton = layerButtons[i]->text->txt;
	}

	std::string clickedLayerVisibleButton = "";
	for (unsigned int i = 0; i < layerVisibleButtons.size(); i++)
	{
		if (layerVisibleButtons[i]->IsPointInsideButton(mouseX, mouseY))
		{
			clickedLayerVisibleButton = layerButtons[i]->text->txt;
		}
	}

	mouseX /= Camera::MULTIPLIER;
	mouseY /= Camera::MULTIPLIER;

	// Allow the tile sheet to be clicked when in certain modes
	if ( (objectMode == MODE_TILE || objectMode == MODE_REPLACE || objectMode == MODE_COPY) && clickedToolboxWindow)
	{
		mouseX *= Camera::MULTIPLIER;
		mouseY *= Camera::MULTIPLIER;

		const int topLeftX = tilesheetPosition.x - tilesheetSprites[tilesheetIndex]->frameWidth;
		const int topLeftY = tilesheetPosition.y - tilesheetSprites[tilesheetIndex]->frameHeight;

		int xOffset = (mouseX - topLeftX);
		int yOffset = (mouseY - topLeftY);

		// Calculate the position in the tilesheet texture to use for drawing the tile
		float x2 = (xOffset / (float)(SPAWN_TILE_SIZE));
		float y2 = (yOffset / (float)(SPAWN_TILE_SIZE));

		spriteSheetTileFrame.x = (int)(roundf(x2)/ Camera::MULTIPLIER) + 1;
		spriteSheetTileFrame.y = (int)(roundf(y2)/ Camera::MULTIPLIER) + 1;

		int moveRight = ( (spriteSheetTileFrame.x - 1) * SPAWN_TILE_SIZE * Camera::MULTIPLIER);
		int moveDown = ( (spriteSheetTileFrame.y - 1) * SPAWN_TILE_SIZE * Camera::MULTIPLIER);

		//std::cout << "(" << x2 << "," << y2 << ")" << std::endl;
		//std::cout << "(" << spriteSheetTileFrame.x << "," << spriteSheetTileFrame.y << ")" << std::endl;

		// Set the location of the yellow rectangle indicating which tile will be drawn
		selectedTilePosition.x = topLeftX + SPAWN_TILE_SIZE + moveRight;
		selectedTilePosition.y = topLeftY + SPAWN_TILE_SIZE + moveDown;

		//TODO: Make sure that when saving the level, each tile remembers its size

		RefreshTilePreview();
	}
	else if (clickedNewButton && clickedButton != nullptr)
	{
		if (!(previousMouseState & SDL_BUTTON(SDL_BUTTON_LEFT)))
		{
			ClickedButton();
		}
	}
	else if (clickedLayerButton != "")
	{
		if (!(previousMouseState & SDL_BUTTON(SDL_BUTTON_LEFT)))
		{
			ClickedLayerButton(clickedLayerButton);
		}
	}
	else if (clickedLayerVisibleButton != "")
	{
		if (!(previousMouseState & SDL_BUTTON(SDL_BUTTON_LEFT)))
		{		
			if (clickedLayerVisibleButton != "")
			{
				if (clickedLayerVisibleButton == "BACK")
				{
					game->renderer.ToggleVisibility(DrawingLayer::BACK);
				}
				else if (clickedLayerVisibleButton == "MIDDLE")
				{
					game->renderer.ToggleVisibility(DrawingLayer::MIDDLE);
				}
				else if (clickedLayerVisibleButton == "OBJECT")
				{
					game->renderer.ToggleVisibility(DrawingLayer::OBJECT);
				}
				else if (clickedLayerVisibleButton == "COLLISION")
				{
					game->renderer.ToggleVisibility(DrawingLayer::COLLISION);
				}
				else if (clickedLayerVisibleButton == "COLLISION2")
				{
					game->renderer.ToggleVisibility(DrawingLayer::COLLISION2);
				}
				else if (clickedLayerVisibleButton == "FRONT")
				{
					game->renderer.ToggleVisibility(DrawingLayer::FRONT);
				}
				else if (clickedLayerVisibleButton == "BG")
				{
					game->renderer.ToggleVisibility(DrawingLayer::BG);
				}
			}

			for (unsigned int i = 0; i < layerVisibleButtons.size(); i++)
			{
				if (layerButtons[i]->text->txt == clickedLayerVisibleButton)
				{
					// Toggle between white and black colors
					layerVisibleButtons[i]->isClicked = !layerVisibleButtons[i]->isClicked;
				}
			}
		}
	}
	else if (objectMode == "inspect")
	{
		glm::vec3 inspectPosition = glm::vec3(mouseX, mouseY, 0);
		inspectPosition.x += game->renderer.camera.position.x;
		inspectPosition.y += game->renderer.camera.position.y;
		InspectObject(inspectPosition, glm::vec2(mouseX, mouseY));
	}
	else if (objectMode == "rotate")
	{
		if (!(previousMouseState & SDL_BUTTON(SDL_BUTTON_LEFT)))
		{
			glm::vec3 rotatePosition = glm::vec3(mouseX, mouseY, 0);
			rotatePosition.x += game->renderer.camera.position.x;
			rotatePosition.y += game->renderer.camera.position.y;

			Entity* rotatedEntity = GetEntityAtWorldPosition(rotatePosition);

			if (rotatedEntity != nullptr)
			{
				rotatedEntity->rotation.z += 90;
				if (rotatedEntity->rotation.z >= 360)
					rotatedEntity->rotation.z -= 360;
			}
		}
	}
	else if (objectMode == "grab")
	{
		if (!(previousMouseState & SDL_BUTTON(SDL_BUTTON_LEFT)))
		{
			// Either grab a new entity, or place the currently grabbed one
			if (grabbedEntities.size() == 0)
			{
				helper->GrabEntity(mouseX, mouseY);
			}
			else
			{
				bool canSpawnAllEntities = true;
				for (size_t i = 0; i < grabbedEntities.size(); i++)
				{
					if (!grabbedEntities[i]->CanSpawnHere(glm::vec3(mouseX, mouseY, 0), *game, false))
					{
						canSpawnAllEntities = false;
					}
				}

				// If the entity is allowed to spawn here, then place it there
				if (canSpawnAllEntities)
				{
					for (size_t i = 0; i < grabbedEntities.size(); i++)
					{
						grabbedEntities[i]->startPosition = grabbedEntities[i]->position;
						grabbedEntities[i]->CalculateCollider();
					}

					grabbedEntities.clear();
					oldGrabbedPositions.clear();
					DoAction();
				}
			}
		}
		
	}
	else if (mouseY < 1290/Camera::MULTIPLIER) // we clicked somewhere in the game world, so place a tile/object
	{
		// if we are placing a tile...
		if (objectMode == MODE_TILE)
		{
			PlaceTile(clickedScreenPosition);
		}
		else if (objectMode == MODE_FILL)
		{
			FillTiles(game->CalculateObjectSpawnPosition(clickedScreenPosition, GRID_SIZE));
		}
		else if (objectMode == MODE_REPLACE)
		{
			bool foundTile = false;
			glm::vec2 coordsToReplace = glm::vec2(0, 0);

			std::vector<Tile*> tilesInLevel;
			
			for (size_t i = 0; i < game->entities.size(); i++)
			{
				if (game->entities[i]->etype == MODE_TILE)
				{
					Tile* tile = static_cast<Tile*>(game->entities[i]);
					tilesInLevel.push_back(tile);

					if (!foundTile)
					{
						if (IsVec3Equals(game->entities[i]->GetPosition(), clickedWorldPosition) &&
							game->entities[i]->layer == drawingLayer)
						{
							// Save the index of the tile
							coordsToReplace = tile->tileCoordinates;
							foundTile = true;
						}
					}
				}
			}

			if (foundTile)
			{
				// Replace the tile with the one selected in the sprite sheet
				for (unsigned int i = 0; i < tilesInLevel.size(); i++)
				{
					// If this tile is the one we need to replace, then replace it
					if (tilesInLevel[i]->tileCoordinates == coordsToReplace)
					{
						// Set the index of the tile
						tilesInLevel[i]->ChangeSprite(spriteSheetTileFrame,
							game->spriteManager.GetImage(tilesheetFilenames[tilesheetIndex]), game->renderer, SPAWN_TILE_SIZE);
					}
				}

				if (onlyPressedButtonDown)
				{
					DoAction();
				}

			}
		}
		else if (objectMode == MODE_COPY)
		{
			// We want to set the active tilesheet to the copied tile's
			// and we want to set the selected tile to the copied tile's
			glm::vec2 coordsToCopy = glm::vec2(0, 0);
			Tile* tile = nullptr;

			for (size_t i = 0; i < game->entities.size(); i++)
			{
				if (IsVec3Equals(game->entities[i]->GetPosition(), clickedWorldPosition) &&
					game->entities[i]->layer == drawingLayer &&
					game->entities[i]->etype == MODE_TILE)
				{
					tile = static_cast<Tile*>(game->entities[i]);

					// Save the index of the tile
					coordsToCopy = tile->tileCoordinates;
				}
			}

			if (tile != nullptr)
			{
				tilesheetIndex = tile->tilesheetIndex;

				StartEdit();
				objectMode = MODE_TILE;

				selectedTilePosition.x = (int)((tile->tileCoordinates.x - 1) * SPAWN_TILE_SIZE);
				selectedTilePosition.y = (int)((tile->tileCoordinates.y - 1) * SPAWN_TILE_SIZE);

				spriteSheetTileFrame.x = (int)tile->tileCoordinates.x;
				spriteSheetTileFrame.y = (int)tile->tileCoordinates.y;

				selectedTilePosition.x += tilesheetPosition.x;
			}

		}
		else // when placing an object
		{
			PlaceObject(glm::vec2(mouseX, mouseY));
		}
	}
}

// TODO: This algorithm doesn't seem to be working perfectly.
// It looks like it fills more vertically than horizontally,
// It looks like it fills more vertically than horizontally,
// but I don't know why...
void Editor::FillTiles(const glm::vec3& spawnPosition, int depth)
{
	static std::vector<glm::vec3> seenTiles;
	static std::unordered_map<std::string, Entity*> mapEntities;

	// Initialization
	if (depth == 0)
	{
		seenTiles.clear();
		mapEntities.clear();

		for (const auto& entity : game->entities)
		{
			if (entity->etype == MODE_TILE)
			{
				mapEntities[Vec3ToString(entity->position)] = entity;
			}
		}
	}

	// Don't bother with tiles we have already checked to save time
	for (const auto& tile : seenTiles)
	{
		if (tile == spawnPosition)
			return;
	}

	//std::cout << "Checking tile at " << spawnPosition.x << "," << spawnPosition.y << std::endl;

	seenTiles.emplace_back(spawnPosition);

	// 1. Place the tile where we clicked
	bool canPlaceTileHere = true;

	// TODO: Faster than a for loop, but technically still bad (allocating strings).
	// Need a single map that can take a vector2 as a key, entity* as value
	const std::string vkey = Vec3ToString(spawnPosition);
	if (mapEntities.count(vkey) != 0)
	{
		if (IsVec3Equals(mapEntities[vkey]->position, spawnPosition))
		{
			canPlaceTileHere = false;
		}
	}

	if (canPlaceTileHere)
	{
		Tile* tile = game->SpawnTile(spriteSheetTileFrame, tilesheetIndex, spawnPosition, drawingLayer);

		if (tile != nullptr)
		{
			//std::cout << "Spawned at (" << spawnPosition.x << "," << spawnPosition.y << ") !" << std::endl;

			if (helper != nullptr)
			{
				// NOTE: This is a major source of slowdown, 
				// so keep this commented unless we can improve its speed
				//helper->PlaceTile(*tile);
			}
		}
	}
	else
	{
		std::cout << "Cannot spawn tile here." << std::endl;
		return; // don't go beyond an existing tile
	}

	// don't fill more than MAX_DEPTH tiles at a time, just to be safe!
	const int MAX_DEPTH = 50;
	if (depth < MAX_DEPTH)
	{
		// 2. For each direction (up, down, left, right)
		// recursively call this function (with a limit)

		const int newTilePos = Globals::TILE_SIZE * Camera::MULTIPLIER;

		FillTiles(spawnPosition + glm::vec3(0, newTilePos, 0), depth + 1);
		FillTiles(spawnPosition + glm::vec3(0, -newTilePos, 0), depth + 1);
		FillTiles(spawnPosition + glm::vec3(newTilePos, 0, 0), depth + 1);
		FillTiles(spawnPosition + glm::vec3(-newTilePos, 0, 0), depth + 1);
	}

	if (canPlaceTileHere && depth == 0)
	{
		game->SortEntities(game->entities);
		DoAction();
	}
}

Entity* Editor::GetEntityAtWorldPosition(const glm::vec3& clickedWorldPosition, bool includeTiles)
{
	SDL_Rect point;
	point.x = clickedWorldPosition.x;
	point.y = clickedWorldPosition.y;
	point.w = 1;
	point.h = 1;

	bool isValidType = true;

	// Find the selected entity
	for (int i = game->entities.size() - 1; i >= 0; i--)
	{
		if (!includeTiles)
		{
			isValidType = (game->entities[i]->etype != MODE_TILE);
		}

		if (isValidType && HasIntersection(point, game->entities[i]->GetTopLeftBounds()))
		{
			return game->entities[i];
		}
	}

	return nullptr;
}

void Editor::InspectObject(const glm::vec3& clickedWorldPosition, const glm::vec2& clickedScreenPosition)
{
	SDL_Rect screenPoint;
	screenPoint.x = clickedScreenPosition.x * Camera::MULTIPLIER;
	screenPoint.y = clickedScreenPosition.y * Camera::MULTIPLIER;
	screenPoint.w = 1;
	screenPoint.h = 1;

	screenPoint = ConvertCoordsFromCenterToTopLeft(screenPoint);
	
	//std::cout << "SP: " << screenPoint.x << "," << screenPoint.y << std::endl;

	if (properties.size() > 1)
	{
		SDL_Rect textRect;
		textRect.w = properties[1]->text->GetTextWidth();
		textRect.h = properties[1]->text->GetTextHeight();
		textRect.x = properties[1]->text->position.x - (textRect.w);
		textRect.y = properties[1]->text->position.y - (textRect.h * 2);

		textRect = ConvertCoordsFromCenterToTopLeft(textRect);

		//std::cout << "TX: " << textRect.x << "," << textRect.y << "," 
		//	<< (textRect.x + textRect.w) << "," << (textRect.y + textRect.h) << std::endl;
	}

	bool clickedOnProperty = false;

	for (unsigned int i = 0; i < properties.size(); i++)
	{
		SDL_Rect textRect;
		textRect.x = properties[i]->text->position.x;
		textRect.y = properties[i]->text->position.y;
		textRect.w = properties[i]->text->GetTextWidth();
		textRect.h = properties[i]->text->GetTextHeight();

		textRect = ConvertCoordsFromCenterToTopLeft(textRect);

		if (HasIntersection(screenPoint, textRect))
		{
			if (selectedEntity != nullptr)
			{
				clickedOnProperty = true;
				if (properties[i]->pType != PropertyType::ReadOnly)
				{
					propertyIndex = i;
					CreateDialog("Edit property '" + properties[i]->key + "': ");
					game->StartTextInput(*dialog, "properties");

					//TODO: Why not just use the enum directly?
					switch (properties[i]->pType)
					{
					case PropertyType::String:
						game->inputType = "String";
						break;
					case PropertyType::Integer:
						game->inputType = "Integer";
						break;
					case PropertyType::Float:
						game->inputType = "Float";
						break;
					}

					SetPropertyText(game->inputText);
				}
			}
			break;
		}
	}

	if (!clickedOnProperty)
	{
		Entity* newSelectedEntity = GetEntityAtWorldPosition(clickedWorldPosition);

		// If selected entity was found, then generate text for all properties of it
		if (newSelectedEntity != nullptr)
		{
			selectedEntity = newSelectedEntity;
			selectedEntity->GetProperties(properties);
			SetPropertyPositions();
		}
	}
}

std::string Editor::GetCurrentPropertyOptionString(int diff)
{
	// Either increment or decrement the index
	propertyOptionIndex += diff;

	// Keep the index inside the bounds
	if (propertyOptionIndex < 0)
		propertyOptionIndex = properties[propertyIndex]->options.size() - 1;
	else if (propertyOptionIndex >= properties[propertyIndex]->options.size())
		propertyOptionIndex = 0;

	// Return the name of the property option
	if (properties[propertyIndex]->options.size() == 0)
		return "";
	else
		return properties[propertyIndex]->options[propertyOptionIndex];
}

void Editor::SetPropertyText(const std::string& newText)
{	
	selectedEntity->SetProperty(properties[propertyIndex]->key, newText, properties);
	selectedEntity->GetProperties(properties);
	SetPropertyPositions();
}

void Editor::SetPropertyPositions()
{
	// Set the text position of all properties
	int propertyX = objectPropertiesRect.x + 10;
	int propertyY = objectPropertiesRect.y + 10;

	objectPropertiesRect.h = 50;
	for (unsigned int i = 0; i < properties.size(); i++)
	{
		properties[i]->text->SetPosition(propertyX, propertyY);
		propertyY += 50;
		objectPropertiesRect.h += 50;
	}
}

void Editor::PlaceObject(const glm::vec2& mousePos)
{
	bool canPlaceObjectHere = true;

	glm::vec3 snappedPosition = game->CalculateObjectSpawnPosition(mousePos, GRID_SIZE);

	for (unsigned int i = 0; i < game->entities.size(); i++)
	{
		if (game->entities[i]->GetPosition() == snappedPosition &&
			game->entities[i]->etype == objectMode)
		{
			canPlaceObjectHere = false;
			break;
		}
	}

	if (canPlaceObjectHere)
	{
		if (helper != nullptr)
		{
			helper->PlaceObject(snappedPosition);
		}

		// Only save this action if the object was successfully placed in the level
		DoAction();
	}
}

void Editor::PlaceTile(const glm::vec2& clickedPosition)
{
	bool canPlaceTileHere = true;

	glm::vec3 spawnPos = game->CalculateObjectSpawnPosition(clickedPosition, GRID_SIZE);

	for (size_t i = 0; i < game->entities.size(); i++)
	{
		if (IsVec3Equals(game->entities[i]->GetPosition(), spawnPos) &&
			game->entities[i]->layer == drawingLayer &&
			game->entities[i]->etype == MODE_TILE)
		{

			if (replaceSettingIndex == 0)
			{
				canPlaceTileHere = false;
			}
			else // overwrite
			{
				// Delete this tile, then break
				game->DeleteEntity(i);
				std::cout << "Overwriting tile" << std::endl;
			}

			break;
		}
	}

	if (canPlaceTileHere)
	{
		Tile* tile = game->SpawnTile(spriteSheetTileFrame, tilesheetIndex, spawnPos, drawingLayer);

		if (tile != nullptr)
		{
			tile->rotation.z = previewMap[objectMode]->rotation.z;
			game->SortEntities(game->entities);

			std::cout << "Spawned at (" << spawnPos.x << "," << spawnPos.y << "!" << std::endl;

			if (helper != nullptr)
			{
				helper->PlaceTile(*tile);
			}

			// Only save this action if the tile was successfully placed in the level
			DoAction();
		}
	}
}

// Toggle special properties of the selected entity
void Editor::MiddleClick(glm::vec2 clickedPosition, int mouseX, int mouseY, glm::vec3 clickedWorldPosition)
{
	if (previousMouseState & SDL_BUTTON(SDL_BUTTON_MIDDLE))
		return;

	const Uint8* input = SDL_GetKeyboardState(NULL);

	// TODO: Maybe assign this to a different button or something?
	if (input[SDL_SCANCODE_LCTRL])
	{
		glm::vec3 worldPosition = glm::vec3(clickedPosition.x + game->renderer.camera.position.x,
			clickedPosition.y + game->renderer.camera.position.y, 0);

		SDL_Rect mouseRect;
		mouseRect.x = worldPosition.x;
		mouseRect.y = worldPosition.y;
		mouseRect.w = Globals::TILE_SIZE;
		mouseRect.h = Globals::TILE_SIZE;

		// NOTE: You will need to exit out of edit mode and then go back in
		// to change tiles that are being placed via the editor
		for (unsigned int i = 0; i < game->entities.size(); i++)
		{
			if (HasIntersection(mouseRect, *game->entities[i]->GetBounds()))
			{
				// Toggle the jump thru property of tiles
				if (game->entities[i]->etype == MODE_TILE)
				{
					game->entities[i]->jumpThru = !game->entities[i]->jumpThru;
					break;
				}
			}
		}
	}
	else if (previewMap[objectMode] != nullptr)
	{
		previewMap[objectMode]->rotation.z -= 90;
		if (previewMap[objectMode]->rotation.z < 0)
			previewMap[objectMode]->rotation.z += 360;
	}
}

//TODO: Figure out how to structure this so we can add deleting stuff as an Action
void Editor::RightClick(glm::vec2 clickedPosition, int mouseX, int mouseY, glm::vec3 clickedWorldPosition)
{
	// If we have grabbed an entity, return it to its old position and immediately exit
	if (grabbedEntities.size() > 0)
	{
		for (size_t i = 0; i < grabbedEntities.size(); i++)
		{
			grabbedEntities[i]->SetPosition(oldGrabbedPositions[i]);
			grabbedEntities[i] = nullptr;
		}

		grabbedEntities.clear();
		oldGrabbedPositions.clear();

		return;
	}

	mouseX /= Camera::MULTIPLIER;
	mouseY /= Camera::MULTIPLIER;
	clickedWorldPosition = game->ConvertFromScreenSpaceToWorldSpace(glm::vec2(mouseX, mouseY));

	if (objectMode == "rotate")
	{
		Entity* rotatedEntity = GetEntityAtWorldPosition(clickedWorldPosition);

		if (rotatedEntity != nullptr)
		{
			rotatedEntity->rotation.z -= 90;
			if (rotatedEntity->rotation.z < 0)
				rotatedEntity->rotation.z += 360;
		}
	}
	else // delete the object
	{
		SDL_Rect point;
		point.x = clickedWorldPosition.x;
		point.y = clickedWorldPosition.y;
		point.w = 1;
		point.h = 1;

		bool isValidType = true;

		// Find the selected entity
		for (int i = game->entities.size() - 1; i >= 0; i--)
		{
			// If we clicked on it...
			if (game->entities[i] != nullptr && HasIntersection(point, game->entities[i]->GetTopLeftBounds()))
			{
				bool onlyPressedButtonDown = !(previousMouseState & SDL_BUTTON(SDL_BUTTON_RIGHT));

				bool sameMode = game->entities[i]->etype == objectMode;
				bool sameLayer = game->entities[i]->layer == drawingLayer;
				bool shouldDeleteThis = false;

				switch (deleteSettingIndex)
				{
				case 0:
					// Same layer, same mode
					shouldDeleteThis = sameLayer && sameMode;
					break;
				case 1:
					// Only same layer
					shouldDeleteThis = sameLayer;
					break;
				case 2:
					// Only same mode
					shouldDeleteThis = sameMode;
					break;
				case 3:
				default:
					// Can delete if at same position
					shouldDeleteThis = true;
					break;
				}

				if (shouldDeleteThis && helper != nullptr)
				{
					helper->DeleteObject(shouldDeleteThis, game->entities[i]);
					if (onlyPressedButtonDown)
					{
						DoAction();
					}

					return;
				}
			}
		}		
	}	
}

void Editor::HandleGUIMode()
{
#if _DEBUG
	int mouseX = 0;
	int mouseY = 0;

	const uint32_t mouseState = SDL_GetMouseState(&mouseX, &mouseY);

	SDL_Rect clickedRect;
	clickedRect.x = mouseX - ((int)mouseX % (GRID_SIZE));
	clickedRect.y = mouseY - ((int)mouseY % (GRID_SIZE));
	clickedRect.w = 1;
	clickedRect.h = 1;

	if (mouseState & SDL_BUTTON(SDL_BUTTON_LEFT)) // 
	{
		// If not holding anything, grab it; else, let go.

		if (grabbedMenuEntity == nullptr)
		{
			// check to see if we grabbed something
			
			// // TODO: Do this for menu buttons/text/images as well
			//std::vector<Text*>& menuTexts = game->openedMenus[game->openedMenus.size() - 1]->texts;

			for (int i = 0; i < game->gui->texts.size(); i++)
			{
				// TODO: Fix this
				if (HasIntersection(*game->gui->texts[i]->GetBounds(), clickedRect))
				{
					grabbedMenuEntity = game->gui->texts[i];
					game->draggedEntity = game->gui->texts[i];
					break;
				}
			}
		}
		else
		{
			grabbedMenuEntity = nullptr;
			game->draggedEntity = nullptr;
		}

	}
	else if (mouseState & SDL_BUTTON(SDL_BUTTON_RIGHT)) // 
	{
		// save changes to file
		game->gui->SaveData();
	}
	else if (mouseState & SDL_BUTTON(SDL_BUTTON_MIDDLE)) // 
	{
		game->gui->LoadData(game);
	}
	else // no button was clicked, just check for hovering
	{
		// Highlight the hovered object

	}

#endif
}

void Editor::HandleEdit()
{
	int mouseX, mouseY = 0;

	const uint32_t mouseState = SDL_GetMouseState(&mouseX, &mouseY);
	game->worldPosition = game->ConvertFromScreenSpaceToWorldSpace(glm::vec2(mouseX, mouseY));

	int clickedX = mouseX - ((int)mouseX % (GRID_SIZE));
	int clickedY = mouseY - ((int)mouseY % (GRID_SIZE));

	// snapped position in screen space (0,0) to (1280,720)
	glm::vec2 snappedScreenPosition(clickedX, clickedY);

	objPreviewPosition = game->CalculateObjectSpawnPosition(snappedScreenPosition, GRID_SIZE);

	std::string clickedText = std::to_string(mouseX) + " " + std::to_string(mouseY);
	game->debugScreen->debugText[DebugText::cursorPositionInScreen]->SetText("Mouse Screen: " + clickedText);

	std::string clickedText2 = std::to_string((int)game->worldPosition.x) + " " + std::to_string((int)game->worldPosition.y);
	game->debugScreen->debugText[DebugText::cursorPositionInWorld]->SetText("Mouse World: " + clickedText2);

	std::string clickedText3 = std::to_string((int)(game->worldPosition.x / (Globals::TILE_SIZE * Camera::MULTIPLIER)))
		+ " " + std::to_string((int)(game->worldPosition.y / (Globals::TILE_SIZE * Camera::MULTIPLIER)));
	game->debugScreen->debugText[DebugText::cursorPositionInTiles]->SetText("Mouse Tiles: " + clickedText3);

	glm::mat4 invertedProjection = glm::inverse(game->renderer.camera.projection);
	glm::vec4 spawnPos = (invertedProjection * glm::vec4(mouseX, mouseY, 0, 1));

	if (mouseState & SDL_BUTTON(SDL_BUTTON_LEFT))
	{
		// We multiply X and Y by 2 because the guiProjection is multiplied by 2
		// TODO: Maybe remove the multiplier
		LeftClick(snappedScreenPosition, mouseX*Camera::MULTIPLIER, mouseY* Camera::MULTIPLIER, objPreviewPosition);
	}
	else if (mouseState & SDL_BUTTON(SDL_BUTTON_RIGHT)) // deletes tiles in order, nearest first
	{
		RightClick(snappedScreenPosition, mouseX * Camera::MULTIPLIER, mouseY * Camera::MULTIPLIER, objPreviewPosition);
	}
	else if (mouseState & SDL_BUTTON(SDL_BUTTON_MIDDLE)) // toggles special properties
	{
		MiddleClick(snappedScreenPosition, mouseX * Camera::MULTIPLIER, mouseY * Camera::MULTIPLIER, objPreviewPosition);
	}
	else // no button was clicked, just check for hovering
	{
		for (unsigned int i = 0; i < buttons.size(); i++)
		{
			buttons[i]->isHovered = false;
			if (buttons[i]->IsPointInsideButton(mouseX * Camera::MULTIPLIER, mouseY * Camera::MULTIPLIER))
				buttons[i]->isHovered = true;		
		}
	}

	if (grabbedEntities.size() > 0)
	{
		glm::vec3 worldPosition = game->ConvertFromScreenSpaceToWorldSpace(glm::vec2(mouseX, mouseY));
		glm::vec3 diff = oldGrabbedPositions[0] - worldPosition;

		// Apply the difference to every entity to get their new positions
		for (size_t i = 0; i < grabbedEntities.size(); i++)
		{
			glm::vec3 newPosition = oldGrabbedPositions[i] - diff;
			grabbedEntities[i]->SetPosition(game->SnapToGrid(newPosition, grabbedEntities[i]->GetGridSize()));
		}
	}

	previousMouseState = mouseState;
}

void Editor::ClickedButton()
{
	if (clickedButton == nullptr)
		return;

	// Check if the clicked button is setting an object mode
	if (clickedButton->name == "tileset")
	{
		ToggleTileset();
		clickedButton->isClicked = false;
	}
	else if (previewMap.find(clickedButton->name) != previewMap.end())
	{
		ToggleObjectMode(clickedButton->name);
	}
	else if (clickedButton->name == "map")
	{
		//TODO: When not in any object mode, allow scroll wheel to zoom the camera
		ToggleSpriteMap(1);
		return; // intentionally exit early to avoid sprite map issue
	}
	else if (clickedButton->name == "grid")
	{
		ToggleGridSize();
	}
	else if (clickedButton->name == "tilesize")
	{
		// TODO: Let us enter specific numbers instead of being limited to half the size
		if (SPAWN_TILE_SIZE == Globals::TILE_SIZE)
			SPAWN_TILE_SIZE = Globals::TILE_SIZE / 2;
		else
			SPAWN_TILE_SIZE = Globals::TILE_SIZE;
	}
	else if (clickedButton->name == "inspect")
	{
		ToggleInspectionMode();
	}
	else if (clickedButton->name == "newlevel")
	{
		NewLevel();
		clickedButton->isClicked = false;
	}
	else if (clickedButton->name == "load")
	{
		CreateDialog("Type in the name of the file to load:");
		game->StartTextInput(*dialog, "load_file_as");
		clickedButton->isClicked = false;
	}
	else if (clickedButton->name == "save")
	{
		SaveLevel();
		clickedButton->isClicked = false;
	}
	else if (clickedButton->name == "undo")
	{
		UndoAction();
		clickedButton->isClicked = false;
	}
	else if (clickedButton->name == "redo")
	{
		RedoAction();
		clickedButton->isClicked = false;
	}
	else if (clickedButton->name == "BG")
	{
		CreateDialog("Type in the name of the background to use:");
		game->StartTextInput(*dialog, "set_background");
		clickedButton->isClicked = false;
	}
	else if (clickedButton->name == MODE_FILL)
	{
		if (objectMode == MODE_FILL)
		{
			//SetLayer(DrawingLayer::BACK);
			objectMode = MODE_TILE;
		}
		else
		{
			objectMode = MODE_FILL;
		}
	}
	else if (clickedButton->name == MODE_REPLACE)
	{
		if (objectMode == MODE_REPLACE)
		{
			//SetLayer(DrawingLayer::BACK);
			objectMode = MODE_TILE;
		}
		else
		{
			objectMode = MODE_REPLACE;
		}
	}
	else if (clickedButton->name == MODE_COPY)
	{
		if (objectMode == MODE_COPY)
		{
			//SetLayer(DrawingLayer::BACK);
			objectMode = MODE_TILE;
		}
		else
		{
			objectMode = MODE_COPY;
		}
	}
	else if (clickedButton->name == "grab")
	{
		ToggleObjectMode("grab");
	}
	else if (clickedButton->name == "rotate")
	{
		ToggleObjectMode("rotate");
	}
	else if (clickedButton->name == "prevpage")
	{
		if (currentButtonPage > 0)
		{
			currentButtonPage--;
			CreateEditorButtons();
			clickedButton->isClicked = false;
			objectMode = "none";
		}
	}
	else if (clickedButton->name == "nextpage")
	{
		if (currentButtonPage <= (int)(buttons.size() / BUTTONS_PER_PAGE))
		{
			currentButtonPage++;
			CreateEditorButtons();
			clickedButton->isClicked = false;
			objectMode = "none";
		}
	}
	else if (clickedButton->name == "prevlevel")
	{
		auto it = std::find(levelNames.begin(), levelNames.end(), game->currentLevel);

		int index = 0;
		if (it != levelNames.end())
		{
			index = std::distance(levelNames.begin(),it);
		}

		// Update the index
		index = (index > 0) ? index - 1 : levelNames.size() - 1;

		// Load the level
		InitLevelFromFile(levelNames[index]);

		clickedButton->isClicked = false;
	}
	else if (clickedButton->name == "nextlevel")
	{
		auto it = std::find(levelNames.begin(), levelNames.end(), game->currentLevel);

		size_t index = 0;
		if (it != levelNames.end())
		{
			index = std::distance(levelNames.begin(), it);
		}

		// Update the index
		index = (index < levelNames.size() - 1) ? index + 1 : 0;

		// Load the level
		InitLevelFromFile(levelNames[index]);

		clickedButton->isClicked = false;
	}
	else if (clickedButton->name == "newEntity")
	{
		// 0. Open dialog to get name of entity
		CreateDialog("Type in the name of the new Entity Type:");
		game->StartTextInput(*dialog, "new_entity_type");
		clickedButton->isClicked = false;	
	}
	else // can't find a button with this name, so check helper
	{
		if (!helper->ClickedCustomButton(clickedButton->name))
		{
			game->logger.Log("Could not find editor button with name " + clickedButton->name);
		}
	}

	ToggleSpriteMap(9999); // reset the preview sprite
	objectPreview = previewMap[objectMode];

	if (objectPreview != nullptr)
	{
		//GRID_SIZE = objectPreview->GetGridSize();
	}
	
}

void Editor::ToggleSpriteMap(int num)
{
	// Because the sprite map is populated from a list,
	// it does not contain "tile" so this function
	// is never utilized when scrolling on the tileset object mode
	if (game->spriteMap.count(objectMode) != 1)
		return;

	entitySubtype += num;

	if (entitySubtype < 0)
		entitySubtype = game->spriteMap[objectMode].size() - 1;

	if (entitySubtype >= game->spriteMap[objectMode].size())
		entitySubtype = 0;

	Entity*& prev = previewMap[objectMode];

	// Delete the current preview object
	if (prev != nullptr)
		delete_it(prev);

	// Update the preview sprites accordingly
	prev = game->CreateEntity(objectMode, glm::vec3(0, 0, 0), entitySubtype);
	prev->Init(*game, game->entityTypes[objectMode][entitySubtype]);

	objectPreview = prev;
}


void Editor::ClickedLayerButton(const std::string& buttonText)
{
	// Highlight the current layer, return all others to normal
	for (unsigned int i = 0; i < layerButtons.size(); i++)
	{		
		layerButtons[i]->isClicked = false;
		if (layerButtons[i]->text->txt == buttonText)
		{
			layerButtons[i]->isClicked = true;
		}
	}

	DrawingLayer layer = DrawingLayer::BACK;

	if (buttonText == "BACK")
	{
		layer = DrawingLayer::BACK;
	}
	else if (buttonText == "MIDDLE")
	{
		layer = DrawingLayer::MIDDLE;
	}
	else if (buttonText == "OBJECT")
	{
		layer = DrawingLayer::OBJECT;
	}
	else if (buttonText == "COLLISION")
	{
		layer = DrawingLayer::COLLISION;
	}
	else if (buttonText == "COLLISION2")
	{
		layer = DrawingLayer::COLLISION2;
	}
	else if (buttonText == "FRONT")
	{
		layer = DrawingLayer::FRONT;
	}

	SetLayer(layer);
}

void Editor::ToggleObjectMode(const std::string& mode)
{
	if (objectMode == mode)
	{
		SetLayer(DrawingLayer::BACK);
		objectMode = MODE_TILE;
	}
	else
	{
		entitySubtype = 0;

		helper->ToggleObjectMode(mode);

		objectMode = mode;
	}

	game->debugScreen->debugText[DebugText::currentEditModeLayer]->SetText("Active Mode: " + objectMode);
}

void Editor::ToggleGridSize()
{
	static int sizeNum = 0;

	sizeNum++;

	if (sizeNum > 2)
		sizeNum = 0;

	std::cout << sizeNum << std::endl;


	switch (sizeNum)
	{
	case 0:
		GRID_SIZE = Globals::TILE_SIZE;
		break;
	case 1:
		GRID_SIZE = Globals::TILE_SIZE / 2;
		break;
	case 2:
		GRID_SIZE = 1;
		break;
	default:
		GRID_SIZE = Globals::TILE_SIZE;
		break;
	}

	std::cout << "Grid size is now " << GRID_SIZE << std::endl;
}

void Editor::ToggleInspectionMode()
{
	if (objectMode != "inspect")
		objectMode = "inspect";
	else
		objectMode = MODE_TILE;

	// If we already have an entity selected, deselect it
	if (selectedEntity != nullptr)
	{
		selectedEntity->DeleteProperties(properties);
		selectedEntity = nullptr;
	}

	game->debugScreen->debugText[DebugText::currentEditModeLayer]->SetText("Active Mode: " + objectMode);
}

void Editor::SetLayer(DrawingLayer layer)
{
	drawingLayer = layer;
}

void Editor::ToggleTileset()
{
	if (objectMode == "none")
	{
		objectMode = MODE_TILE;
		return;
	}
	else
	{
		objectMode = MODE_TILE;
		game->debugScreen->debugText[DebugText::currentEditModeLayer]->SetText("Active Mode: " + objectMode);

		tilesheetIndex = (tilesheetIndex + 1 > tilesheetSprites.size() - 1) ? 0 : tilesheetIndex + 1;

		// Calculate and set positions for the selected tilesheet
		// No need to make this its own function because the only time we care about
		// changing the position is when we also change the current tileset
		tilesheetPosition.x = (game->screenWidth * 2) - tilesheetSprites[tilesheetIndex]->frameWidth;
		tilesheetPosition.y = tilesheetSprites[tilesheetIndex]->frameHeight;
		selectedTilePosition.x = tilesheetPosition.x - tilesheetSprites[tilesheetIndex]->frameWidth + SPAWN_TILE_SIZE;
		selectedTilePosition.y = tilesheetPosition.y - tilesheetSprites[tilesheetIndex]->frameHeight + SPAWN_TILE_SIZE;

		game->SaveEditorSettings();

		previewMap[MODE_TILE] = game->CreateTile(glm::vec2(0, 0), 0,
			glm::vec3(0, 0, 0), DrawingLayer::FRONT);
		previewMap[MODE_TILE]->GetSprite()->color = { 255, 255, 255, 64 };

		previewMap[MODE_FILL] = previewMap[MODE_TILE];

		objectPreview = previewMap[MODE_TILE];

		GRID_SIZE = previewMap[MODE_TILE]->GetGridSize();
	}
}

void Editor::RenderDebug(const Renderer& renderer)
{

}

void Editor::Render(const Renderer& renderer)
{
	// Draw the object or tile that will be placed here, if any
	if (objectPreview != nullptr && objectPreview->GetSprite() != nullptr)
	{	
		objectPreview->GetSprite()->Render(objPreviewPosition, 0, renderer, objectPreview->scale, objectPreview->rotation);
	}

	if (helper != nullptr)
	{
		helper->Render(renderer);
	}

	if (objectMode == "inspect" && selectedEntity != nullptr)
	{
		if (outlineSprite == nullptr)
			outlineSprite = new Sprite(renderer.debugSprite->texture, renderer.debugSprite->shader);

		if (rectSprite == nullptr)
		{
			rectSprite = new Sprite(renderer.shaders[4]);
			rectSprite->keepPositionRelativeToCamera = true;
			rectSprite->keepScaleRelativeToCamera = true;
		}

		glm::vec2 scale;
		// Draw a yellow rectangle around the currently selected object
		outlineSprite->color = { 0, 255, 255, 128 };
		scale = (glm::vec2(selectedEntity->GetBounds()->w / outlineSprite->texture->GetWidth(),
			selectedEntity->GetBounds()->h / outlineSprite->texture->GetWidth()));
		outlineSprite->Render(selectedEntity->position, renderer, scale);

		// Draw the box that goes underneath all the properties
		rectSprite->color = { 0, 255, 255, 128 };
		scale = (glm::vec2(objectPropertiesRect.w, objectPropertiesRect.h));
		rectSprite->Render(glm::vec3(objectPropertiesRect.x, objectPropertiesRect.y, 0), renderer, scale);

		for (unsigned int k = 0; k < properties.size(); k++)
		{
			float targetWidth = properties[k]->text->GetSprite()->frameWidth;
			float targetHeight = properties[k]->text->GetSprite()->frameHeight;
			rectSprite->color = { 0, 0, 255, 128 };
			scale = (glm::vec2(targetWidth, targetHeight));

			if (propertyIndex > -1)
			{
				if (k == propertyIndex)
				{
					rectSprite->color = { 0, 255, 0, 128 };
				}
			}

			rectSprite->Render(properties[k]->text->position, renderer, scale);
		}
		
		// Draw the text for all the properties
		for (unsigned int i = 0; i < properties.size(); i++)
		{
			properties[i]->text->GetSprite()->keepPositionRelativeToCamera = true;
			properties[i]->text->GetSprite()->keepScaleRelativeToCamera = true;
			properties[i]->text->Render(renderer);
		}
	}
	else
	{
		if (objectMode == MODE_TILE || objectMode == MODE_REPLACE || objectMode == MODE_COPY)
		{
			// Draw the tilesheet (only if we are placing a tile)
			tilesheetSprites[tilesheetIndex]->Render(tilesheetPosition, game->renderer, glm::vec2(1,1));

			// Draw a yellow rectangle around the currently selected tileset tile
			game->renderer.debugSprite->color = { 255, 255, 0, 255 };
			game->renderer.debugSprite->keepPositionRelativeToCamera = true;
			game->renderer.debugSprite->keepScaleRelativeToCamera = true;
			
			// Draw the yellow rectangle scaled to the tile size
			glm::vec2 newScale = glm::vec2(SPAWN_TILE_SIZE/24.0f, SPAWN_TILE_SIZE/24.0f);
			game->renderer.debugSprite->Render(selectedTilePosition, renderer, newScale);

			game->renderer.debugSprite->keepPositionRelativeToCamera = false;
			game->renderer.debugSprite->keepScaleRelativeToCamera = false;
		}
	}	

	// Draw all buttons
	for (unsigned int i = 0; i < buttons.size(); i++)
	{
		if (buttons[i]->name == "prevpage")
		{
			if (currentButtonPage == 0)
				continue;
		}

		if (buttons[i]->name == "nextpage")
		{
			if (currentButtonPage > (int)(currentButtonPage/BUTTONS_PER_PAGE))
				continue;
		}

		buttons[i]->Render(renderer);
	}

	// Draw all layer visible buttons
	for (unsigned int i = 0; i < layerVisibleButtons.size(); i++)
	{
		layerVisibleButtons[i]->Render(renderer);
	}

	// Draw all layer buttons
	for (unsigned int i = 0; i < layerButtons.size(); i++)
	{
		layerButtons[i]->Render(renderer);
	}

	if (dialog != nullptr && dialog->visible)
	{
		dialog->Render(renderer);
	}

	if (currentLevelText != nullptr)
	{
		currentLevelText->Render(renderer);
	}
}

void Editor::CreateDialog(const std::string& txt)
{
	if (dialog != nullptr)
	{
		dialog->text->SetText(txt);
		dialog->input->SetText("");
		dialog->visible = true;
		dialog->scale = (game->renderer.CalculateScale(*dialog->sprite,
			dialog->text->GetTextWidth(), dialog->text->GetTextHeight() * 4, dialog->text->scale));
	}
}

void Editor::NewLevel()
{
	CreateDialog("Type in the filename of the new level:");
	game->StartTextInput(*dialog, "new_level");
}

std::string Editor::SaveLevelAsString()
{
	std::ostringstream level;

	std::unordered_map<std::string, std::string> map;
	for (unsigned int i = 0; i < game->entities.size(); i++)
	{
		// Pass in a cleared map, fill it with values,
		// then output all the values according to the loading.dat file
		map.clear();
		game->entities[i]->Save(map);

		// Search through the list of variables in the loading.dat file
		std::vector<std::string> entityList = loadDataMap["entity"];
		std::vector<std::string> list = loadDataMap[game->entities[i]->etype];
		list.insert(list.begin(), entityList.begin(), entityList.end());

		for (size_t k = 0; k < list.size(); k++)
		{
			// If the entity saved any of the variables in the list,
			// then write those to the output file
			if (map.count(list[k]) != 0)
			{
				level << map[list[k]] << " ";
			}
			else
			{
				level << "0 ";
			}
		}

		// We must handle "path" as a special case
		if (game->entities[i]->etype == "path")
		{
			std::cout << "STOI NODE 1" << std::endl;
			int nodeCount = std::stoi(map["nodeCount"]);
			for (int i = 0; i < nodeCount; i++)
			{
				level << map["nodeID_" + std::to_string(i)] << " ";
			}
			std::cout << "STOI NODE 2" << std::endl;
		}

		level << "\n";
	}

	map.clear();
	game->background->Save(map);
	std::vector<std::string> list = { "id", "type", "positionX", "positionY", "subtype" };

	for (size_t k = 0; k < list.size(); k++)
	{
		// If the entity saved any of the variables in the list,
		// then write those to the output file
		if (map.count(list[k]) != 0)
		{
			level << map[list[k]] << " ";
		}
		else
		{
			level << "0 ";
		}
	}

	level << "\n";

	if (game->levelStartCutscene != "")
	{
		level << "1 cutscene-start 0 0 " << game->levelStartCutscene << std::endl;
	}

	// Save the camera
	map.clear();
	game->renderer.camera.Save(map);

	// Search through the list of variables in the loading.dat file
	list.clear();
	list = loadDataMap["camera"];

	for (size_t k = 0; k < list.size(); k++)
	{
		if (map.count(list[k]) != 0)
		{
			level << map[list[k]] << " ";
		}
		else
		{
			level << "0 ";
		}
	}

	level << "\n";

	helper->CustomSave(level);	

	return level.str();
}

void Editor::SaveLevel(const std::string& levelName)
{
	if (levelName != "")
		game->currentLevel = levelName;

	std::ofstream fout;
	fout.open("data/levels/" + game->currentLevel + ".lvl");

	fout << SaveLevelAsString();

	fout.close();
}

void Editor::UndoAction()
{
	std::string levelName = game->currentLevel;
	
	if (levelStringIndex > 0)
	{
		levelStringIndex--;

		glm::vec3 pos = game->renderer.camera.position;

		// Load the level here
		ClearLevelEntities();
		CreateLevelFromString(levelStrings[levelStringIndex], "undo");
		CreateLevelFromVector(levelFilesMap["undo"]);

		game->renderer.camera.position = pos;
	}
}

void Editor::RedoAction()
{
	if (levelStringIndex < levelStrings.size() - 1)
	{
		levelStringIndex++;

		glm::vec3 pos = game->renderer.camera.position;

		// Load the level here
		ClearLevelEntities();
		CreateLevelFromString(levelStrings[levelStringIndex], "redo");
		CreateLevelFromVector(levelFilesMap["redo"]);

		game->renderer.camera.position = pos;
	}
}

void Editor::DoAction()
{
	const int UNDO_LIMIT = 99;

	int timesToPopBack = levelStrings.size() - levelStringIndex - 1;

	for (int i = 0; i < timesToPopBack; i++)
	{
		levelStrings.pop_back();
	}

	levelStrings.push_back(SaveLevelAsString());
	levelStringIndex++;

	while (levelStrings.size() > UNDO_LIMIT && levelStringIndex > 1)
	{
		levelStrings.pop_front();
		levelStringIndex--;
	}	
}

std::string Editor::ReadLevelFromFile(std::string levelName)
{
	std::ifstream fin;
	fin.open("data/levels/" + levelName + ".lvl");

	if (fin.is_open())
	{
		std::string level = "";
		for (std::string line; std::getline(fin, line); )
		{
			level += line + "\n";
		}

		fin.close();
		return level;
	}
	else
	{
		game->logger.Log("ERROR: Failed to read from level file: " + levelName + ".lvl");
	}

	return "";
}

const std::string& Editor::GetTileSheetFileName(const int index) const
{
	return tilesheetFilenames[index];
}

void Editor::GetLevelList()
{
	levelNames.clear();

	std::string levelsFolder = "data/levels/";

	fs::path path = fs::current_path().append(levelsFolder);
	for (const auto& entry : fs::directory_iterator(path))
	{
		if (entry.path().extension().string() == ".lvl")
		{
			std::string levelName = entry.path().stem().string();
			//std::cout << levelName << std::endl;
			levelNames.push_back(levelName);
		}
	}
}

void Editor::CreateLevelFromString(const std::string& level, const std::string& levelName)
{
	int index = 0;
	std::stringstream ss{ level };
	std::vector<std::string> lines;
	std::string line = "";

	lines.reserve(std::count(level.begin(), level.end(), '\n'));

	while (index < level.size())
	{
		if (level[index] == '\n')
		{
			lines.push_back(line);
			line = "";
		}
		else
		{
			line += level[index];
		}
		index++;
	}
	lines.push_back(line);

	levelFilesMap[levelName] = lines;
}

// TODO: This is VERY slow!
void Editor::CreateLevelFromVector(const std::vector<std::string>& lines)
{
	static const std::string STR_ENTITY("entity");
	static const std::string STR_ID("id");
	static const std::string STR_FRAMEX("frameX");
	static const std::string STR_FRAMEY("frameY");
	static const std::string STR_TILESHEET("tilesheet");
	static const std::string STR_POSITIONX("positionX");
	static const std::string STR_POSITIONY("positionY");
	static const std::string STR_LAYER("layer");
	static const std::string STR_PASSABLESTATE("passableState");

	try
	{
		Timer testTimer;
		testTimer.Start(0);

		game->renderer.camera.startingZoom = 1.0f;

		if (game->background == nullptr)
			game->background = new Background("", glm::vec3(0, 0, 0));

		// Remove all backgrounds
		game->background->ResetBackground();

		if (game->editMode)
		{
			GetLevelList();
		}

		if (helper != nullptr)
		{
			helper->CreateLevelStart();
		}

		//cameraTargetID = -1;
		//switchTargetBackToPlayer = false;

		int lineNumber = 0;
		int index = 0;

		std::string etype = "";
		int positionX = 0;
		int positionY = 0;
		std::string subtype = "";
		
		std::unordered_map<std::string, std::string> map;

		int indexOfID = std::distance(loadDataMap["entity"].begin(),
			std::find(loadDataMap["entity"].begin(),
				loadDataMap["entity"].end(), "id"));
	
		int indexOfPositionX = std::distance(loadDataMap["entity"].begin(),
			std::find(loadDataMap["entity"].begin(),
				loadDataMap["entity"].end(), "positionX"));

		int indexOfPositionY = std::distance(loadDataMap["entity"].begin(),
			std::find(loadDataMap["entity"].begin(),
				loadDataMap["entity"].end(), "positionY"));

		int indexOfType = std::distance(loadDataMap["entity"].begin(),
			std::find(loadDataMap["entity"].begin(),
				loadDataMap["entity"].end(), "type"));

		int indexOfSubtype = std::distance(loadDataMap["entity"].begin(),
			std::find(loadDataMap["entity"].begin(),
				loadDataMap["entity"].end(), "subtype"));

		std::string tokens[32];
		std::string word = "";
		std::string line = "";

		std::vector<std::string>* currentDataMap;

		game->SetDuration("Loading Level");

		// Make sure to clear the list of taken IDs 
		// at the start of loading a level
		Entity::takenIDs.clear();
		Entity::nextValidID = 0;

		game->entities.reserve(lines.size());

		lineNumber = 0;
		while (lineNumber < lines.size())
		{

			map.clear();
			index = 0;

			line = lines[lineNumber];
			lineNumber++;

			if (line.size() == 0)
				continue;
			
			size_t wordNumber = 0;
			for (size_t i = 0; i < line.size(); i++)
			{				
				if (line[i] == ' ')
				{
					tokens[wordNumber] = word;
					word = "";
					wordNumber++;
				}
				else
				{
					word += line[i];
				}
			}
			tokens[wordNumber] = word;

			try
			{
				etype = tokens[indexOfType];
				//std::cout << etype << std::endl;

				// Populate the map of data if it does not exist
				if (loadDataMap.count(etype) != 0)
				{
					if (etype != "camera")
					{
						currentDataMap = &loadDataMap[STR_ENTITY];
						for (size_t i = 0; i < currentDataMap->size(); i++)
						{
							map[(*currentDataMap)[i]] = tokens[index++];
						}
					}

					currentDataMap = &loadDataMap[etype];
					for (size_t i = 0; i < currentDataMap->size(); i++)
					{
						map[(*currentDataMap)[i]] = tokens[index++];
					}
				}

				if (etype == "path")
				{
					size_t nodeCount = std::stoi(map["nodeCount"]);
					for (size_t i = 0; i < nodeCount; i++)
					{
						map["nodeID_" + std::to_string(i)] = tokens[index++];
					}
				}

				if (etype == "player" || etype == "Player")
				{
					std::cout << "STOI PLAYER 1" << std::endl;
					positionX = std::stoi(tokens[indexOfPositionX]);
					positionY = std::stoi(tokens[indexOfPositionY]);
					// TODO: If 3D, get position Z too
					game->player = game->SpawnPlayer(glm::vec3(positionX, positionY, 0));

					if (tokens->size() > 5 && game->player != nullptr)
					{
						game->player->Init(*game, "Player");
					}

					std::cout << "STOI PLAYER 2" << std::endl;
				}
				else if (etype == "tile")
				{
					std::cout << "STOI TILE 1" << std::endl;
					Entity::nextValidID = std::stoi(tokens[indexOfID]);

					int tilesheetIndex = std::stoi(map[STR_TILESHEET]);

					Tile* newTile = game->SpawnTile(glm::vec2(std::stoi(map[STR_FRAMEX]), std::stoi(map[STR_FRAMEY])),
						tilesheetIndex, glm::vec3(std::stoi(map[STR_POSITIONX]), std::stoi(map[STR_POSITIONY]), 0),
						(DrawingLayer)std::stoi(map[STR_LAYER]));

					newTile->Load(map, *game);

					if (std::stoi(map[STR_PASSABLESTATE]) == 2)
						newTile->jumpThru = true;

					newTile->subtype = tilesheetIndex;
					std::cout << "STOI TILE 2" << std::endl;
				}
				else if (etype == "cutscene-start")
				{
					index = 4;
					game->levelStartCutscene = tokens[index++];
				}
				else if (etype == "bgm")
				{
					index = 4;
					game->nextBGM = tokens[index++];
				}
				else if (etype == "camera")
				{
					game->renderer.camera.Load(map, *game);
				}
				else if (etype == "bg")
				{
					index = 2;
					std::cout << "STOI BG 1" << std::endl;
					int bgX = std::stoi(tokens[index++]);
					int bgY = std::stoi(tokens[index++]);
					std::cout << "STOI BG 2" << std::endl;
					std::string bgName = tokens[index++];
					game->background->SpawnBackground(bgName, bgX, bgY, *game);
				}
				else if (etype == "particlesystem")
				{
					index = 4;
					CutsceneFunctions::ParticleCommand({ "particle", "system", "create", tokens[index++], tokens[index++], tokens[index++] }, game->cutsceneManager.commands);
					CutsceneFunctions::ParticleCommand({ "particle", "system", tokens[4], "sprite", tokens[index++] }, game->cutsceneManager.commands);
					CutsceneFunctions::ParticleCommand({ "particle", "system", tokens[4], "velocity", tokens[index++], tokens[index++] }, game->cutsceneManager.commands);
					CutsceneFunctions::ParticleCommand({ "particle", "system", tokens[4], "timeToSpawn", tokens[index++] }, game->cutsceneManager.commands);
					CutsceneFunctions::ParticleCommand({ "particle", "system", tokens[4], "timeToLive", tokens[index++] }, game->cutsceneManager.commands);
					CutsceneFunctions::ParticleCommand({ "particle", "system", tokens[4], "maxNumber", tokens[index++] }, game->cutsceneManager.commands);
				}
				// if not handled by helper, load it as a normal entity
				else if (!helper->CustomLoad(etype, tokens))
				{

					std::cout << "ETYPE " << etype << std::endl;
					std::cout << "SUBTYPE " << subtype << std::endl;

					Entity::nextValidID = std::stoi(tokens[indexOfID]);
					std::cout << "STOI CUSTOM 1a" << std::endl;
					positionX = std::stoi(tokens[indexOfPositionX]);
					positionY = std::stoi(tokens[indexOfPositionY]);
					subtype = tokens[indexOfSubtype];

					// To prevent errors
					if (subtype == "")
						subtype = "0";

					std::cout << "STOI CUSTOM 1b" << std::endl;
					Entity* newEntity = game->SpawnEntity(etype, glm::vec3(positionX,positionY, 0), std::stoi(subtype));

					if (newEntity != nullptr)
					{
						newEntity->Load(map, *game);
						std::cout << "STOI CUSTOM 1c" << std::endl;
						newEntity->Init(*game, game->entityTypes[etype][std::stoi(subtype)]);

						// TODO: Use enums for etypes, look up strings in a table
						if (newEntity->etype == "cameraBounds")
						{
							game->cameraBoundsEntities.push_back(newEntity);
						}
					}
					else
					{
						game->logger.Log("FAILED TO SPAWN " + etype);
						std::cout << "LINE: " << lineNumber << std::endl;
						std::cout << "INDEX: " << index << std::endl;
					}


					std::cout << "STOI CUSTOM 2" << std::endl;

				}

			}
			catch (const std::exception& e)
			{
				std::cout << "LOAD LEVEL EXCEPTION: " << e.what() << std::endl;
				std::cout << "LINE: " << lineNumber << std::endl;
				std::cout << "INDEX: " << index << std::endl;
				game->logger.Log(e.what());
			}

			//ss.getline(lineChar, LINE_SIZE);
		}

		game->PrintDuration("Loading Level");

		// Switch the camera's target
		if (game->renderer.camera.startingTargetID >= 0)
		{
			Entity* targetEntity = game->GetEntityFromID(game->renderer.camera.startingTargetID);

			if (targetEntity != nullptr)
			{
				// Switch to the target and teleport to its location
				// so that we know we are within the camera bounds
				// TODO: Is there a way to automatically do this
				// without placing a specific object in the level?
				game->renderer.camera.SwitchTarget(*targetEntity);
				game->renderer.camera.FollowTarget(*game, true);
			}
		}

		// Here we are setting up the player so that once
		// we arrive at the next frame, the camera will
		// now move toward the player, stopping at the bounds
		if (game->renderer.camera.afterStartingTargetID > -1)
		{
			// Get the entity associated with this ID

			Entity* targetEntity = game->GetEntityFromID(game->renderer.camera.afterStartingTargetID);

			if (targetEntity != nullptr)
			{
				// Switch target to that entity
				game->renderer.camera.SwitchTarget(*targetEntity);
			}
		}

		//game->renderer.ConfigureInstanceArray(1000);

		if (helper != nullptr)
		{
			helper->CreateLevelEnd();
		}		
	}
	catch (std::exception ex)
	{
		std::cout << "ERROR CREATING LEVEL: " << ex.what() << std::endl;
		game->logger.Log(ex.what());
	}
}

void Editor::ClearLevelEntities()
{
	for (unsigned int i = 0; i < game->entities.size(); i++)
	{
		//TODO: How to deal with dangling pointers in general?
		if (game->entities[i]->etype == "player")
			game->player = nullptr;
		delete_it(game->entities[i]);
	}
		
	game->entities.clear();
	game->cameraBoundsEntities.clear();

	game->draggedEntity = nullptr;
	grabbedEntities.clear();
	oldGrabbedPositions.clear();
}

//TODO: Display an error message if the file does not exist
void Editor::InitLevelFromFile(const std::string& levelName)
{
	for (auto& [key, val] : game->cutsceneManager.images)
	{
		if (val != nullptr)
			delete_it(val);
	}

	currentLevelText->SetText(levelName);
	game->cutsceneManager.watchingCutscene = false;

	game->cutsceneManager.images.clear();
	game->levelStartCutscene = "";

	ClearLevelEntities();
	entitySubtype = 0;

	if (levelName != "")
		game->currentLevel = levelName;

	// In Release builds, don't reload the level from file, store it in memory

#if _DEBUG
	std::cout << "load 1" << std::endl;

	std::string levelData = ReadLevelFromFile(levelName);
	if (levelData == "")
	{
		// TODO: If failed to load level, handle error somehow
		return;
	}

	CreateLevelFromString(levelData, levelName);
#else
	std::cout << "load 2" << std::endl;
	if (levelFilesMap.count(levelName) == 0)
	{
		std::cout << "load 3" << std::endl;
		CreateLevelFromString(ReadLevelFromFile(levelName), levelName);
	}
#endif


	std::cout << "Loading level \"" + levelName + "\"" << std::endl;
	CreateLevelFromVector(levelFilesMap[levelName]);
	std::cout << "Loaded level \"" + levelName + "\"" << std::endl;

	// Reset the undo/redo queue
	levelStrings.clear();
	levelStringIndex = -1;
	DoAction();

	game->SortEntities(game->entities);

	game->PopulateQuadTree();

	game->gui->ResetText();

	// Play the cutscene for the current level, if any
	std::cout << "Playing start cutscene" << std::endl;
	if (game->levelStartCutscene != "")
	{
		game->cutsceneManager.baseLabel = game->cutsceneManager.JumpToLabel(game->levelStartCutscene);

		if (levelName == "demo")
		{
			if (playOpeningDemoCutscene)
				game->cutsceneManager.PlayCutscene(game->levelStartCutscene.c_str());
			playOpeningDemoCutscene = false;
		}
		else
		{
			game->cutsceneManager.PlayCutscene(game->levelStartCutscene.c_str());
		}
	}
}
