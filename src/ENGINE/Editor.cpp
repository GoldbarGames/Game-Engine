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
#include <filesystem>
#include "Renderer.h"
#include "DebugScreen.h"
#include "Quadtree.h"
#include "Property.h"
#include "Logger.h"
#include "CutsceneManager.h"
#include "Background.h"

#include "HealthComponent.h"
#include "PhysicsComponent.h"

#include "EditorHelper.h"

Editor::Editor(Game& g)
{
	game = &g;

	tilesheetFilenames = game->ReadStringsFromFile("data/lists/tilesheet.list");
	for (int i = 0; i < tilesheetFilenames.size(); i++)
	{
		tilesheetFilenames[i] = "assets/tiles/" + tilesheetFilenames[i] + ".png";
	}

	GetLevelList();

	currentLevelText = neww Text(game->theFont, "");
	currentLevelText->SetPosition(100, 50);
	currentLevelText->GetSprite()->keepPositionRelativeToCamera = true;
	currentLevelText->GetSprite()->keepScaleRelativeToCamera = true;

	playOpeningDemoCutscene = false;
	dialog = neww Dialog(Vector2(g.screenWidth, g.screenHeight), &g.spriteManager);
	dialog->text = neww Text(game->theFont, "");
	dialog->input = neww Text(game->theFont, "");

	dialog->text->SetPosition(dialog->position.x, dialog->position.y + 20);
	dialog->input->SetPosition(dialog->position.x, dialog->position.y + 70);

	dialog->sprite->SetShader(game->renderer.shaders[ShaderName::SolidColor]);
	dialog->sprite->color = { 255, 0, 0, 255 };
	dialog->sprite->keepPositionRelativeToCamera = true;
	dialog->sprite->keepScaleRelativeToCamera = true;
	dialog->scale = (game->renderer.CalculateScale(*dialog->sprite, 
		dialog->text->GetTextWidth(), dialog->text->GetTextHeight() * 4, dialog->text->scale));

	dialog->text->GetSprite()->keepPositionRelativeToCamera = true;
	dialog->input->GetSprite()->keepPositionRelativeToCamera = true;
	dialog->text->GetSprite()->keepScaleRelativeToCamera = true;
	dialog->input->GetSprite()->keepScaleRelativeToCamera = true;


	grid = neww Sprite(game->renderer.shaders[ShaderName::Grid]);
	//grid->SetScale(Vector2(game->screenWidth, game->screenHeight));

	previewMap["tile"] = game->CreateTile(Vector2(0,0), "assets/editor/rect-outline.png", 
		Vector2(0,0), DrawingLayer::FRONT);

	previewMap["tile"]->GetSprite()->color = { 255, 255, 255, 64 };

	// Create a preview object of every entity type
	previewMapObjectNames = game->ReadStringsFromFile("data/lists/entityTypes.list");

	for (int i = 0; i < previewMapObjectNames.size(); i++)
	{
		previewMap[previewMapObjectNames[i]] = game->CreateEntity(previewMapObjectNames[i], 
			Vector2(0, 0), entitySubtype);
	}

	objectPreview = previewMap["tile"];

	game->entities.clear();	

	SetLayer(DrawingLayer::BACK);

	objectPropertiesRect.w = 400;
	objectPropertiesRect.h = 600;
	objectPropertiesRect.y = 100;

	// Check to see if the object properties (saving/loading)
	// require the level to be re-calculated
#if _DEBUG
	UpdateLevelFiles();
#endif

}

Editor::~Editor()
{
	for (auto& [key, val] : previewMap)
	{
		if (val != nullptr)
			delete_it(val);
	}

	for (int i = 0; i < buttons.size(); i++)
	{
		if (buttons[i] != nullptr)
			delete_it(buttons[i]);
	}

	for (int i = 0; i < layerButtons.size(); i++)
	{
		if (layerButtons[i] != nullptr)
			delete_it(layerButtons[i]);
	}

	for (int i = 0; i < layerVisibleButtons.size(); i++)
	{
		if (layerVisibleButtons[i] != nullptr)
			delete_it(layerVisibleButtons[i]);
	}

	for (int i = 0; i < tilesheetSprites.size(); i++)
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
}

// Updates the level file based on changes in how entities are saved/loaded
void Editor::UpdateLevelFiles()
{
	// Fill in the neww array
	loadDataMap.clear();
	const std::string LOAD_FILE = "data/loading.dat";
	std::string newData = ReadLoadingData(LOAD_FILE, loadDataMap);

	const std::string OLD_FILEPATH = "data/load_old.dat";
	std::unordered_map<std::string, std::vector<std::string>> oldMap;
	std::string oldData = ReadLoadingData(OLD_FILEPATH, oldMap);

	// If no changes, return now
	if (newData == oldData)
	{
		return;
	}

	const std::string STR_ENTITY("entity");

	std::string levelsFolder = "data\\levels\\";
	std::filesystem::path path = std::filesystem::current_path().append(levelsFolder);
	for (const auto& entry : std::filesystem::directory_iterator(path))
	{
		if (entry.path().extension().string() == ".lvl")
		{
			std::string levelName = entry.path().stem().string();
			std::cout << levelName << std::endl;

			std::unordered_map<std::string, std::vector<Vector2>> reorderMap;
			std::unordered_map<std::string, int> resizeMap;

			// Store the neww size of the list in order to resize it (add/remove)
			int oldEntitySize = oldMap[STR_ENTITY].size();
			int newEntitySize = loadDataMap[STR_ENTITY].size();

			for (int i = 0; i < oldEntitySize; i++)
			{
				// Check for any elements that have been re-ordered or re-named
				std::vector<std::string>::iterator it = std::find(loadDataMap[STR_ENTITY].begin(),
					loadDataMap[STR_ENTITY].end(), oldMap[STR_ENTITY][i]);

				// If we can find this element in the neww data, 
				// keep track of where to place it in the neww array
				if (it != loadDataMap[STR_ENTITY].end())
				{
					int newIndex = std::distance(loadDataMap[STR_ENTITY].begin(), it);
					reorderMap[STR_ENTITY].push_back(Vector2(i, newIndex));
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
						// Store the neww size of the list in order to resize it (add/remove)
						resizeMap[entityType] = loadDataMap[entityType].size();

						// Check for any elements that have been re-ordered
						std::vector<std::string>::iterator it = std::find(loadDataMap[entityType].begin(),
							loadDataMap[entityType].end(), currentList[i]);

						// If we can find this element in the neww data...
						if (it != loadDataMap[entityType].end())
						{
							// keep track of the indices here and swap them in the level file
							int newIndex = std::distance(loadDataMap[entityType].begin(), it);
							reorderMap[entityType].push_back(Vector2(i, newIndex));
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
				etype = tokens[1];

				// Depending on the type of entity, figure out the rest

				// 1. Create a neww vector of strings equal to the neww size
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

				// 2. For each reorder, take the value in the old index, place it in neww index
				// 3. For anything that is not a reorder, just keep the values where they are
				int reorderEntitySize = reorderMap[STR_ENTITY].size();
				for (int i = 0; i < reorderEntitySize; i++)
				{
					int oldIndex = reorderMap[STR_ENTITY][i].x + difference;
					int newIndex = reorderMap[STR_ENTITY][i].y + difference;
					newTokens[newIndex] = tokens[oldIndex];
				}

				int reorderTypeSize = reorderMap[etype].size();
				for (int i = 0; i < reorderTypeSize; i++)
				{
					int oldIndex = reorderMap[etype][i].x + reorderEntitySize;
					int newIndex = reorderMap[etype][i].y + reorderEntitySize + difference;
					newTokens[newIndex] = tokens[oldIndex];
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

			// Output the neww level to a file
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

		for (int i = 1; i < tokens.size(); i++)
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
	const int buttonHeight = 50;
	const int buttonSpacing = 20;

	int buttonX = buttonStartX + buttonWidth + buttonSpacing;

	std::vector<string> buttonNames = game->ReadStringsFromFile("data/lists/editorbuttons.list"); 

	buttonNames.insert(buttonNames.begin() + 7, previewMapObjectNames.begin(), previewMapObjectNames.end());

	unsigned int BUTTON_LIST_START = currentButtonPage * BUTTONS_PER_PAGE;
	unsigned int BUTTON_LIST_END = BUTTON_LIST_START + BUTTONS_PER_PAGE;

	if (BUTTON_LIST_END > buttonNames.size())
		BUTTON_LIST_END = buttonNames.size();

	for (unsigned int i = BUTTON_LIST_START; i < BUTTON_LIST_END; i++)
	{
		if (i > buttonNames.size() - 1)
			break;

		EditorButton* editorButton = neww EditorButton("", buttonNames[i], 
			Vector2(buttonX*2, (game->screenHeight - buttonHeight)*2), *game);
		
		editorButton->image->keepPositionRelativeToCamera = true;
		editorButton->image->keepScaleRelativeToCamera = true;
		buttons.emplace_back(editorButton);

		// TODO: is there a way to not make this hard-coded? is it worth it?
		buttonX += buttonWidth + buttonSpacing; 
	}

	EditorButton* previousButton = neww EditorButton("", "prevpage", 
		Vector2(buttonStartX*2, (game->screenHeight - buttonHeight)*2), *game);
	
	previousButton->image->keepScaleRelativeToCamera = true;
	buttons.emplace_back(previousButton);
	
	EditorButton* nextButton = neww EditorButton("", "nextpage", 
		Vector2((buttonStartX + (buttonWidth + buttonSpacing) * (BUTTONS_PER_PAGE + 1)) * 2,
		(game->screenHeight - buttonHeight)*2), *game);
	
	nextButton->image->keepScaleRelativeToCamera = true;
	buttons.emplace_back(nextButton);

	// Level navigation

	EditorButton* previousLevelButton = neww EditorButton("", "prevlevel",
		Vector2(buttonStartX * Camera::MULTIPLIER, 
			(game->screenHeight - buttonHeight - buttonHeight - buttonSpacing) * Camera::MULTIPLIER), *game);

	previousLevelButton->image->keepScaleRelativeToCamera = true;
	buttons.emplace_back(previousLevelButton);

	EditorButton* nextLevelButton = neww EditorButton("", "nextlevel",
		Vector2((buttonStartX + (buttonWidth + buttonSpacing)) * Camera::MULTIPLIER,
			(game->screenHeight - buttonHeight - buttonHeight - buttonSpacing) * Camera::MULTIPLIER), *game);

	nextLevelButton->image->keepScaleRelativeToCamera = true;
	buttons.emplace_back(nextLevelButton);
}

void Editor::StartEdit()
{
	game->LoadEditorSettings();

	//game->renderer.camera.ResetProjection();
	currentLevelText->SetText(game->currentLevel);

	// TILE SHEET FOR TOOLBOX
	if (tilesheetSprites.empty())
	{
		for (int i = 0; i < tilesheetFilenames.size(); i++)
		{
			tilesheetSprites.push_back(new Sprite(1, game->spriteManager,
				tilesheetFilenames[i], game->renderer.shaders[ShaderName::NoAlpha], Vector2(0, 0)));

			tilesheetSprites[i]->keepPositionRelativeToCamera = true;
			tilesheetSprites[i]->keepScaleRelativeToCamera = true;
		}
	}
	
	tilesheetPosition.x = (game->screenWidth * 2) - tilesheetSprites[tilesheetIndex]->frameWidth;
	tilesheetPosition.y = tilesheetSprites[tilesheetIndex]->frameHeight;

	// this centers the yellow rectangle on the top left tile in the tilesheet
	// (we need to subtract the width/height to get to the top left corner,
	// and then add the tile size to center it within the actual tile)
	selectedTilePosition.x = tilesheetPosition.x - tilesheetSprites[tilesheetIndex]->frameWidth + TILE_SIZE;
	selectedTilePosition.y = tilesheetPosition.y - tilesheetSprites[tilesheetIndex]->frameHeight + TILE_SIZE;

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

	// To add a neww layer:
	// 1. Add the name here
	// 2. Add the enum
	// 3. In renderer, add the layersVisible for it to true
	// 4. In globals.cpp, add the switch/case for it

	std::vector<string> layerButtonNames = { "BACK", "MIDDLE", "OBJECT", "COLLISION", "COLLISION2", "FRONT" };

	for (unsigned int i = 0; i < layerButtonNames.size(); i++)
	{
		EditorButton* layerButton = neww EditorButton(layerButtonNames[i], "Layer", 
			Vector2(buttonX, buttonY), *game, Vector2(layerButtonWidth, 50), { 255, 255, 255, 255 });

		layerButton->image->keepScaleRelativeToCamera = true;
		layerButton->text->GetSprite()->keepScaleRelativeToCamera = true;
		layerButtons.emplace_back(layerButton);
		
		EditorButton* layerVisibleButton = neww EditorButton("", "Visible", 
			Vector2(buttonX + (layerButtonWidth * 2), buttonY), *game, Vector2(50, 50), { 255, 255, 255, 255 });

		layerVisibleButton->image->keepScaleRelativeToCamera = true;
		layerVisibleButton->text->GetSprite()->keepScaleRelativeToCamera = true;
		layerVisibleButtons.emplace_back(layerVisibleButton);

		buttonY += layerButtonHeight + layerButtonSpacing; // TODO: is there a way to not make this hard-coded? is it worth it?
	}

	ClickedLayerButton("BACK");
	//currentEditModeLayer->SetText("Active Mode: " + objectMode); // GetDrawingLayerName(drawingLayer));
}

void Editor::StopEdit()
{
	//game->renderer.camera.Zoom(0.0f, game->screenWidth, game->screenHeight);	
	//inspectionMode = false;	
	game->SaveEditorSettings();
	selectedEntity = nullptr;
	propertyIndex = -1;
}

void Editor::LeftClick(Vector2 clickedScreenPosition, int mouseX, int mouseY, Vector2 clickedWorldPosition)
{
	bool clickedToolboxWindow = mouseX >= tilesheetPosition.x - tilesheetSprites[tilesheetIndex]->frameWidth
		&& mouseY <= tilesheetSprites[tilesheetIndex]->frameHeight * Camera::MULTIPLIER;

	bool clickedNewButton = false;
	if (!(previousMouseState & SDL_BUTTON(SDL_BUTTON_LEFT)))
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

	string clickedLayerButton = "";
	for (unsigned int i = 0; i < layerButtons.size(); i++)
	{
		if (layerButtons[i]->IsPointInsideButton(mouseX, mouseY))
			clickedLayerButton = layerButtons[i]->text->txt;
	}

	string clickedLayerVisibleButton = "";
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
	if ( (objectMode == "tile" || objectMode == "replace" || objectMode == "copy") && clickedToolboxWindow)
	{
		mouseX *= Camera::MULTIPLIER;
		mouseY *= Camera::MULTIPLIER;

		const int topLeftX = tilesheetPosition.x - tilesheetSprites[tilesheetIndex]->frameWidth;
		const int topLeftY = tilesheetPosition.y - tilesheetSprites[tilesheetIndex]->frameHeight;

		int xOffset = (mouseX - topLeftX);
		int yOffset = (mouseY - topLeftY);

		// Calculate the position in the tilesheet texture to use for drawing the tile
		float x2 = (xOffset / (float)(TILE_SIZE));
		float y2 = (yOffset / (float)(TILE_SIZE));

		spriteSheetTileFrame.x = (int)(roundf(x2)/ Camera::MULTIPLIER) + 1;
		spriteSheetTileFrame.y = (int)(roundf(y2)/ Camera::MULTIPLIER) + 1;

		int moveRight = ( (spriteSheetTileFrame.x - 1) * TILE_SIZE * Camera::MULTIPLIER);
		int moveDown = ( (spriteSheetTileFrame.y - 1) * TILE_SIZE * Camera::MULTIPLIER);

		//std::cout << "(" << x2 << "," << y2 << ")" << std::endl;
		//std::cout << "(" << spriteSheetTileFrame.x << "," << spriteSheetTileFrame.y << ")" << std::endl;

		// Set the location of the yellow rectangle indicating which tile will be drawn
		selectedTilePosition.x = topLeftX + TILE_SIZE + moveRight;
		selectedTilePosition.y = topLeftY + TILE_SIZE + moveDown;

		//TODO: Make this section a function we can call to refresh the current tile preview
		Entity*& prev = previewMap[objectMode];
		if (prev != nullptr)
			delete_it(prev);

		prev = game->CreateTile(spriteSheetTileFrame, tilesheetFilenames[tilesheetIndex],
			Vector2(0, 0), DrawingLayer::FRONT);

		objectPreview = prev;
	}
	else if (clickedNewButton && clickedButton != nullptr)
	{
		if (!(previousMouseState & SDL_BUTTON(SDL_BUTTON_LEFT)))
		{
			ClickedButton();
			objectPreview = previewMap[objectMode];
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
		Vector2 inspectPosition = Vector2(mouseX, mouseY);
		inspectPosition.x += game->renderer.camera.position.x;
		inspectPosition.y += game->renderer.camera.position.y;
		InspectObject(inspectPosition, clickedScreenPosition);
	}
	else if (objectMode == "rotate")
	{
		if (!(previousMouseState & SDL_BUTTON(SDL_BUTTON_LEFT)))
		{
			Vector2 rotatePosition = Vector2(mouseX, mouseY);
			rotatePosition.x += game->renderer.camera.position.x;
			rotatePosition.y += game->renderer.camera.position.y;

			Entity* rotatedEntity = GetClickedEntity(rotatePosition);

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
			Vector2 grabPosition = Vector2(mouseX, mouseY);
			grabPosition.x += game->renderer.camera.position.x;
			grabPosition.y += game->renderer.camera.position.y;

			// Either grab a neww entity, or place the currently grabbed one
			if (grabbedEntity == nullptr)
			{
				grabbedEntity = GetClickedEntity(grabPosition);

				if (grabbedEntity != nullptr)
				{
					oldGrabbedPosition = grabbedEntity->GetPosition();
				}
			}
			else
			{
				// If the entity is allowed to spawn here, then place it there
				if (grabbedEntity->CanSpawnHere(Vector2(mouseX, mouseY), *game, false))
				{
					if (grabbedEntity->physics != nullptr)
					{
						grabbedEntity->startPosition = grabbedEntity->position;
						grabbedEntity->CalculateCollider();
					}
					grabbedEntity = nullptr;
					DoAction();
				}
			}
		}
		
	}
	else if (mouseY < 1290/Camera::MULTIPLIER) // we clicked somewhere in the game world, so place a tile/object
	{
		//clickedPosition += game->camera;

		// if we are placing a tile...
		if (objectMode == "tile")
		{
			PlaceTile(clickedScreenPosition, mouseX, mouseY);
			DoAction();
		}
		else if (objectMode == "replace")
		{
			bool foundTile = false;
			Vector2 coordsToReplace = Vector2(0, 0);
			Vector2 roundedPosition = RoundToInt(clickedWorldPosition);

			std::vector<Tile*> tilesInLevel;

			for (unsigned int i = 0; i < game->entities.size(); i++)
			{
				if (game->entities[i]->etype == "tile")
				{
					Tile* tile = static_cast<Tile*>(game->entities[i]);
					tilesInLevel.push_back(tile);

					if (!foundTile)
					{
						if (RoundToInt(game->entities[i]->GetPosition()) == roundedPosition &&
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
					if (tilesInLevel[i]->tileCoordinates == coordsToReplace)
					{
						// Set the index of the tile
						tilesInLevel[i]->ChangeSprite(spriteSheetTileFrame,
							game->spriteManager.GetImage(tilesheetFilenames[tilesheetIndex]), game->renderer);
					}
				}

				DoAction();
			}
		}
		else if (objectMode == "copy")
		{
			// We want to set the active tilesheet to the copied tile's
			// and we want to set the selected tile to the copied tile's
			Vector2 coordsToCopy = Vector2(0, 0);
			Tile* tile = nullptr;

			for (unsigned int i = 0; i < game->entities.size(); i++)
			{
				if (RoundToInt(game->entities[i]->GetPosition()) == RoundToInt(clickedWorldPosition) &&
					game->entities[i]->layer == drawingLayer &&
					game->entities[i]->etype == "tile")
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
				objectMode = "tile";

				selectedTilePosition.x = (int)((tile->tileCoordinates.x - 1) * TILE_SIZE);
				selectedTilePosition.y = (int)((tile->tileCoordinates.y - 1) * TILE_SIZE);

				spriteSheetTileFrame.x = (int)tile->tileCoordinates.x;
				spriteSheetTileFrame.y = (int)tile->tileCoordinates.y;

				selectedTilePosition.x += tilesheetPosition.x;
			}

		}
		else // when placing an object
		{
			PlaceObject(clickedScreenPosition, mouseX, mouseY);
			DoAction();
		}
		
	}
}

Entity* Editor::GetClickedEntity(const Vector2& clickedWorldPosition, bool includeTiles)
{
	SDL_Rect point;
	point.x = clickedWorldPosition.x;
	point.y = clickedWorldPosition.y;
	point.w = 1;
	point.h = 1;

	// Find the selected entity
	for (int i = game->entities.size() - 1; i >= 0; i--)
	{
		// Without this code, it would be using the center as the top-left corner,
		// so we need to convert the coordinates to get the correct rectangle
		SDL_Rect bounds = *(game->entities[i]->GetBounds());
		bounds.x -= bounds.w;
		bounds.y -= bounds.h;
		bounds.w *= 2;
		bounds.h *= 2;

		if (includeTiles)
		{
			if (HasIntersection(point, bounds))
			{
				return game->entities[i];
			}
		}
		else
		{
			if (game->entities[i]->etype != "tile" && 
				HasIntersection(point, bounds))
			{
				return game->entities[i];
			}
		}
	}

	return nullptr;
}

void Editor::InspectObject(const Vector2& clickedWorldPosition, const Vector2& clickedScreenPosition)
{
	SDL_Rect screenPoint;
	screenPoint.x = clickedScreenPosition.x * Camera::MULTIPLIER;
	screenPoint.y = clickedScreenPosition.y * Camera::MULTIPLIER;
	screenPoint.w = 1;
	screenPoint.h = 1;

	bool clickedOnProperty = false;

	for (unsigned int i = 0; i < properties.size(); i++)
	{
		SDL_Rect textRect;
		textRect.w = properties[i]->text->GetTextWidth();
		textRect.h = properties[i]->text->GetTextHeight();
		textRect.x = properties[i]->text->position.x - (textRect.w);
		textRect.y = properties[i]->text->position.y - (textRect.h);
		textRect.w *= 2;

		if (HasIntersection(screenPoint, textRect))
		{
			if (selectedEntity != nullptr)
			{
				clickedOnProperty = true;
				if (properties[i]->pType != PropertyType::ReadOnly)
				{
					propertyIndex = i;
					CreateDialog("Edit property '" + properties[i]->key + "': ");
					game->StartTextInput("properties");

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
		selectedEntity = GetClickedEntity(clickedWorldPosition);

		// If selected entity was found, then generate text for all properties of it
		if (selectedEntity != nullptr)
		{
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
	selectedEntity->SetProperty(properties[propertyIndex]->key, newText);
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

void Editor::PlaceObject(Vector2 clickedPosition, int mouseX, int mouseY)
{
	bool canPlaceObjectHere = true;

	Vector2 snappedPosition = game->CalculateObjectSpawnPosition(Vector2(mouseX, mouseY), GRID_SIZE);

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
	}
}

void Editor::PlaceTile(Vector2 clickedPosition, int mouseX, int mouseY)
{
	bool canPlaceTileHere = true;

	Vector2 spawnPos = game->CalculateObjectSpawnPosition(clickedPosition, GRID_SIZE);

	for (unsigned int i = 0; i < game->entities.size(); i++)
	{
		Vector2 entityPosition = RoundToInt(game->entities[i]->GetPosition());

		if (entityPosition == spawnPos &&
			game->entities[i]->layer == drawingLayer &&
			game->entities[i]->etype == "tile")
		{

			if (replaceSettingIndex == 0)
			{
				canPlaceTileHere = false;
			}
			else // overwrite
			{
				//TODO: Delete this tile, then break
				game->DeleteEntity(i);
			}

			break;
		}
	}

	if (canPlaceTileHere)
	{
		//TODO: Can't be this simple. Our zoom level affects the mouse position!
		// Do we need to use raycasts or something else here?

		//glm::mat4 invertedProjection = glm::inverse(game->renderer.camera.projection);
		//glm::vec4 spawnPos = (invertedProjection * glm::vec4(mouseX, mouseY, 0, 1));

		game->SpawnTile(spriteSheetTileFrame, tilesheetFilenames[tilesheetIndex], spawnPos, drawingLayer);
		game->SortEntities(game->entities);

		std::cout << "Spawned at (" << spawnPos.x << "," << spawnPos.y << "!" << std::endl;
	}
}

// Toggle special properties of the selected entity
void Editor::MiddleClick(Vector2 clickedPosition, int mouseX, int mouseY, Vector2 clickedWorldPosition)
{
	if (previousMouseState & SDL_BUTTON(SDL_BUTTON_MIDDLE))
		return;

	const Uint8* input = SDL_GetKeyboardState(NULL);

	// TODO: Maybe assign this to a different button or something?
	if (input[SDL_SCANCODE_LCTRL])
	{
		Vector2 worldPosition = Vector2(clickedPosition.x + game->renderer.camera.position.x,
			clickedPosition.y + game->renderer.camera.position.y);

		SDL_Rect mouseRect;
		mouseRect.x = worldPosition.x;
		mouseRect.y = worldPosition.y;
		mouseRect.w = TILE_SIZE;
		mouseRect.h = TILE_SIZE;

		// NOTE: You will need to exit out of edit mode and then go back in
		// to change tiles that are being placed via the editor
		for (unsigned int i = 0; i < game->entities.size(); i++)
		{
			if (HasIntersection(mouseRect, *game->entities[i]->GetBounds()))
			{
				// Toggle the jump thru property of tiles
				if (game->entities[i]->etype == "tile")
				{
					game->entities[i]->jumpThru = !game->entities[i]->jumpThru;
					break;
				}
			}
		}
	}
	else
	{
		previewMap[objectMode]->rotation.z -= 90;
		if (previewMap[objectMode]->rotation.z < 0)
			previewMap[objectMode]->rotation.z += 360;
	}

}

//TODO: Figure out how to structure this so we can add deleting stuff as an Action
void Editor::RightClick(Vector2 clickedPosition, int mouseX, int mouseY, Vector2 clickedWorldPosition)
{
	// If we have grabbed an entity, return it to its old position and immediately exit
	if (grabbedEntity != nullptr)
	{
		grabbedEntity->SetPosition(oldGrabbedPosition);
		grabbedEntity = nullptr;
		return;
	}

	clickedWorldPosition = Vector2(mouseX / Camera::MULTIPLIER, mouseY / Camera::MULTIPLIER);
	clickedWorldPosition.x += game->renderer.camera.position.x;
	clickedWorldPosition.y += game->renderer.camera.position.y;

	if (objectMode == "rotate")
	{
		Entity* rotatedEntity = GetClickedEntity(clickedWorldPosition);

		if (rotatedEntity != nullptr)
		{
			rotatedEntity->rotation.z -= 90;
			if (rotatedEntity->rotation.z < 0)
				rotatedEntity->rotation.z += 360;
		}
	}
	else
	{
		Entity* entityToDelete = GetClickedEntity(clickedWorldPosition, true);

		if (entityToDelete != nullptr)
		{
			bool sameMode = entityToDelete->etype == objectMode;
			bool sameLayer = entityToDelete->layer == drawingLayer;
			bool shouldDeleteThis = false;

			if (deleteSettingIndex == 0)
			{
				// Same layer, same mode
				shouldDeleteThis = sameLayer && sameMode;
			}
			else if (deleteSettingIndex == 1)
			{
				// Only same layer
				shouldDeleteThis = sameLayer;
			}
			else if (deleteSettingIndex == 2)
			{
				// Only same mode
				shouldDeleteThis = sameMode;
			}
			else if (deleteSettingIndex == 3)
			{
				// Can delete if at same position
				shouldDeleteThis = true;
			}

			if (helper != nullptr)
			{
				helper->DeleteObject(shouldDeleteThis, entityToDelete);
			}			
		}
	}

	
}

void Editor::HandleEdit()
{
	int mouseX = 0;
	int mouseY = 0;

	const Uint32 mouseState = SDL_GetMouseState(&mouseX, &mouseY);

	int clickedX = mouseX - ((int)mouseX % (GRID_SIZE));
	int clickedY = mouseY - ((int)mouseY % (GRID_SIZE));

	// snapped position in screen space (0,0) to (1280,720)
	Vector2 clickedScreenPosition(clickedX, clickedY); 

	objPreviewPosition = game->CalculateObjectSpawnPosition(clickedScreenPosition, GRID_SIZE);

	std::string clickedText = std::to_string(mouseX) + " " + std::to_string(mouseY);
	game->debugScreen->debugText[DebugText::cursorPositionInScreen]->SetText("Mouse Screen: " + clickedText);
	game->debugScreen->debugText[DebugText::cursorPositionInScreen]->GetSprite()->keepScaleRelativeToCamera = true;

	glm::mat4 invertedProjection = glm::inverse(game->renderer.camera.projection);
	glm::vec4 spawnPos = (invertedProjection * glm::vec4(mouseX, mouseY, 0, 1));

	if (mouseState & SDL_BUTTON(SDL_BUTTON_LEFT))
	{
		// We multiply X and Y by 2 because the guiProjection is multiplied by 2
		// TODO: Maybe remove the multiplier
		LeftClick(clickedScreenPosition, mouseX*Camera::MULTIPLIER, mouseY* Camera::MULTIPLIER, objPreviewPosition);
	}
	else if (mouseState & SDL_BUTTON(SDL_BUTTON_RIGHT)) // deletes tiles in order, nearest first
	{
		RightClick(clickedScreenPosition, mouseX * Camera::MULTIPLIER, mouseY * Camera::MULTIPLIER, objPreviewPosition);
	}
	else if (mouseState & SDL_BUTTON(SDL_BUTTON_MIDDLE)) // toggles special properties
	{
		MiddleClick(clickedScreenPosition, mouseX * Camera::MULTIPLIER, mouseY * Camera::MULTIPLIER, objPreviewPosition);
	}
	else // no button was clicked, just check for hovering
	{
		for (unsigned int i = 0; i < buttons.size(); i++)
		{
			buttons[i]->isHovered = false;
			if (buttons[i]->IsPointInsideButton(mouseX*2, mouseY*2))			
				buttons[i]->isHovered = true;		
		}
	}

	if (grabbedEntity != nullptr)
	{
		Vector2 grabPosition = Vector2(mouseX, mouseY);
		grabPosition.x += game->renderer.camera.position.x;
		grabPosition.y += game->renderer.camera.position.y;

		grabbedEntity->SetPosition(game->SnapToGrid(grabPosition));
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
		//TODO: Maybe make this use the mouse wheel too?
		ToggleSpriteMap(1);
	}
	else if (clickedButton->name == "grid")
	{
		ToggleGridSize();
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
		game->StartTextInput("load_file_as");
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
	else if (clickedButton->name == "replace")
	{
		if (objectMode == "replace")
		{
			//SetLayer(DrawingLayer::BACK);
			objectMode = "tile";
		}
		else
		{
			objectMode = "replace";
		}
	}
	else if (clickedButton->name == "copy")
	{
		if (objectMode == "copy")
		{
			//SetLayer(DrawingLayer::BACK);
			objectMode = "tile";
		}
		else
		{
			objectMode = "copy";
		}
	}
	else if (clickedButton->name == "grab")
	{
		// TODO: Should this be a special function
		// for toggling the grab mode, since it's not an object?
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
		}
	}
	else if (clickedButton->name == "nextpage")
	{
		if (currentButtonPage <= (int)(buttons.size() / BUTTONS_PER_PAGE))
		{
			currentButtonPage++;
			CreateEditorButtons();
			clickedButton->isClicked = false;
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

		int index = 0;
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
	else if (clickedButton->name == "path")
	{
		//if (currentPath != nullptr)
		//{
		//	currentPath = nullptr;
		//}

		// TODO: Fix this
		ToggleObjectMode("path");
	}
}

void Editor::ToggleSpriteMap(int num)
{
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
	if (objectMode == "tile")
	{
		prev = game->CreateTile(spriteSheetTileFrame, tilesheetFilenames[tilesheetIndex],
			Vector2(0, 0), DrawingLayer::FRONT);

		prev->GetSprite()->color = { 255, 255, 255, 64 };
	}
	else
	{		
		prev = game->CreateEntity(objectMode, Vector2(0, 0), entitySubtype);
		prev->Init(game->entityTypes[objectMode][entitySubtype]);
	}

	//TODO: How to deal with object modes that return nullptr?
	//if (prev == nullptr)
	//	prev = previewMap[objectMode];

	objectPreview = prev;
}


void Editor::ClickedLayerButton(string buttonText)
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

void Editor::ToggleObjectMode(std::string mode)
{
	if (objectMode == mode)
	{
		SetLayer(DrawingLayer::BACK);
		objectMode = "tile";
	}
	else
	{
		entitySubtype = 0;

		if (mode == "npc" || mode == "enemy" || mode == "collectible")
			SetLayer(DrawingLayer::COLLISION);
		else
			SetLayer(DrawingLayer::OBJECT);

		objectMode = mode;
	}

	game->debugScreen->debugText[DebugText::currentEditModeLayer]->SetText("Active Mode: " + objectMode);
	game->debugScreen->debugText[DebugText::currentEditModeLayer]->GetSprite()->keepScaleRelativeToCamera = true;
}

void Editor::ToggleGridSize()
{
	if (GRID_SIZE == 24)
		GRID_SIZE = 12;
	else
		GRID_SIZE = 24;
}

void Editor::ToggleInspectionMode()
{
	//TODO: Is this a good idea?
	SetLayer(DrawingLayer::BACK);

	if (objectMode != "inspect")
		objectMode = "inspect";
	else
		objectMode = "tile";

	// If we already have an entity selected, deselect it
	if (selectedEntity != nullptr)
	{
		selectedEntity->DeleteProperties(properties);
		selectedEntity = nullptr;
	}

	game->debugScreen->debugText[DebugText::currentEditModeLayer]->SetText("Active Mode: " + objectMode);
	//inspectionMode = !inspectionMode;
}

void Editor::SetLayer(DrawingLayer layer)
{
	drawingLayer = layer;
}

void Editor::ToggleTileset()
{
	tilesheetIndex++;
	if (tilesheetIndex > 1)
		tilesheetIndex = 0;

	// Calculate and set positions for the selected tilesheet
	//TODO: Maybe make this its own function?
	tilesheetPosition.x = (game->screenWidth * 2) - tilesheetSprites[tilesheetIndex]->frameWidth;
	tilesheetPosition.y = tilesheetSprites[tilesheetIndex]->frameHeight;
	selectedTilePosition.x = tilesheetPosition.x - tilesheetSprites[tilesheetIndex]->frameWidth + TILE_SIZE;
	selectedTilePosition.y = tilesheetPosition.y - tilesheetSprites[tilesheetIndex]->frameHeight + TILE_SIZE;
	
	game->SaveEditorSettings();
	StartEdit();	
}

void Editor::RenderDebug(const Renderer& renderer)
{
	//TODO: Only set each text if the number has changed from last time



	
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
			outlineSprite = neww Sprite(renderer.debugSprite->texture, renderer.debugSprite->shader);

		if (rectSprite == nullptr)
		{
			rectSprite = neww Sprite(renderer.shaders[ShaderName::SolidColor]);
			rectSprite->keepPositionRelativeToCamera = true;
			rectSprite->keepScaleRelativeToCamera = true;
		}

		Vector2 scale;
		// Draw a yellow rectangle around the currently selected object
		outlineSprite->color = { 0, 255, 255, 128 };
		scale = (Vector2(selectedEntity->GetBounds()->w / outlineSprite->texture->GetWidth(),
			selectedEntity->GetBounds()->h / outlineSprite->texture->GetWidth()));
		outlineSprite->Render(selectedEntity->position, renderer, scale);

		// Draw the box that goes underneath all the properties
		rectSprite->color = { 0, 255, 255, 128 };
		scale = (Vector2(objectPropertiesRect.w, objectPropertiesRect.h));
		rectSprite->Render(Vector2(objectPropertiesRect.x, objectPropertiesRect.y), renderer, scale);

		for (unsigned int k = 0; k < properties.size(); k++)
		{
			float targetWidth = properties[k]->text->GetSprite()->frameWidth;
			float targetHeight = properties[k]->text->GetSprite()->frameHeight;
			rectSprite->color = { 0, 0, 255, 128 };
			scale = (Vector2(targetWidth, targetHeight));

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
		if (objectMode == "tile" || objectMode == "replace" || objectMode == "copy")
		{
			// Draw the tilesheet (only if we are placing a tile)
			tilesheetSprites[tilesheetIndex]->Render(tilesheetPosition, game->renderer, Vector2(1,1));

			// Draw a yellow rectangle around the currently selected tileset tile
			game->renderer.debugSprite->color = { 255, 255, 0, 255 };
			//game->renderer.debugSprite->scale = Vector2(1, 1);
			game->renderer.debugSprite->keepPositionRelativeToCamera = true;
			game->renderer.debugSprite->keepScaleRelativeToCamera = true;
			game->renderer.debugSprite->Render(selectedTilePosition, renderer, Vector2(1, 1));
			game->renderer.debugSprite->keepPositionRelativeToCamera = false;
			game->renderer.debugSprite->keepScaleRelativeToCamera = false;
		}
	}	

	// Draw all buttons
	for (unsigned int i = 0; i < buttons.size(); i++)
	{
		// TODO: Maybe offset the buttons
		// For now, always draw the previous/next page buttons
		// If we don't, then clicking in the empty space will 
		// accidentally instantiate something in the level

		/*
		if (buttons[i]->name == "PrevPage")
		{
			if (currentButtonPage == 0)
				continue;
		}

		if (buttons[i]->name == "NextPage")
		{
			if (currentButtonPage > (int)(currentButtonPage/BUTTONS_PER_PAGE))
				continue;
		}
		*/

		buttons[i]->Render(renderer);
	}

	// Draw all layer buttons
	for (unsigned int i = 0; i < layerButtons.size(); i++)
	{
		layerButtons[i]->Render(renderer);
	}

	// Draw all layer visible buttons
	for (unsigned int i = 0; i < layerVisibleButtons.size(); i++)
	{
		layerVisibleButtons[i]->Render(renderer);
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

void Editor::DestroyDialog()
{
	if (dialog != nullptr)
		dialog->visible = false;
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
	CreateDialog("Type in the filename of the neww level:");
	game->StartTextInput("new_level");
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

		for (int k = 0; k < list.size(); k++)
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
	}

	// TODO: Maybe don't separate the bg from the other types?
	map.clear();
	game->background->Save(map);
	std::vector<std::string> list = { "id", "type", "positionX", "positionY", "subtype" }; // loadDataMap["bg"];
	for (int k = 0; k < list.size(); k++)
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

	// TODO: Refactor this better
	if (game->levelStartCutscene != "")
	{
		level << "1 cutscene-start 0 0 " << game->levelStartCutscene << std::endl;
	}

	return level.str();
}

void Editor::SaveLevel(std::string levelName)
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
		// Load the level here
		ClearLevelEntities();
		CreateLevelFromString(levelStrings[levelStringIndex]);
	}
}

void Editor::RedoAction()
{
	if (levelStringIndex < levelStrings.size() - 1)
	{
		levelStringIndex++;
		// Load the level here
		ClearLevelEntities();
		CreateLevelFromString(levelStrings[levelStringIndex]);
	}
}

void Editor::DoAction()
{
	const int UNDO_LIMIT = 99;

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

	std::string level = "";
	for (std::string line; std::getline(fin, line); )
	{
		level += line + "\n";
	}

	fin.close();

	return level;
}

const std::string& Editor::GetTileSheetFileName(const int index) const
{
	return tilesheetFilenames[index];
}

void Editor::GetLevelList()
{
	levelNames.clear();

	std::string levelsFolder = "data\\levels\\";
	std::filesystem::path path = std::filesystem::current_path().append(levelsFolder);
	for (const auto& entry : std::filesystem::directory_iterator(path))
	{
		if (entry.path().extension().string() == ".lvl")
		{
			std::string levelName = entry.path().stem().string();
			//std::cout << levelName << std::endl;
			levelNames.push_back(levelName);
		}
	}
}

// TODO: This is VERY slow!
void Editor::CreateLevelFromString(std::string level)
{
	try
	{
		if (game->background == nullptr)
			game->background = neww Background("", Vector2(0, 0));

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

		std::stringstream ss{ level };
		
		int lineNumber = 0;
		int index = 0;

		std::vector<std::string> lines;
		std::string line = "";

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

		index = 0;

		//const int LINE_SIZE = 1024;
		//char lineChar[LINE_SIZE];
		//ss.getline(lineChar, LINE_SIZE);

		std::string etype = "";
		int positionX = 0;
		int positionY = 0;
		std::string subtype = "";
		
		std::unordered_map<std::string, std::string> map;
	
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

		//std::istringstream buf(lineChar);
		//std::istream_iterator<std::string> beg(buf), end;
		std::string tokens[32];
		std::string word = "";

		const std::string STR_ENTITY("entity");
		const std::string STR_ID("id");
		const std::string STR_FRAMEX("frameX");
		const std::string STR_FRAMEY("frameY");
		const std::string STR_TILESHEET("tilesheet");
		const std::string STR_POSITIONX("positionX");
		const std::string STR_POSITIONY("positionY");
		const std::string STR_LAYER("layer");
		const std::string STR_PASSABLESTATE("passableState");

		std::vector<std::string>* currentDataMap;

		std::cout << "START" << std::endl;

		// Make sure to clear the list of taken IDs 
		// at the start of loading a level
		Entity::takenIDs.clear();
		Entity::nextValidID = 0;

		lineNumber = 0;
		while (lineNumber < lines.size())
		{
			map.clear();
			index = 0;

			line = lines[lineNumber];
			lineNumber++;

			if (line.size() == 0)
				continue;
			
			int wordNumber = 0;
			for (int i = 0; i < line.size(); i++)
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

				// Populate the map of data if it does not exist
				if (loadDataMap.count(etype) != 0)
				{
					currentDataMap = &loadDataMap[STR_ENTITY];
					for (int i = 0; i < currentDataMap->size(); i++)
					{
						map[(*currentDataMap)[i]] = tokens[index++];
					}

					currentDataMap = &loadDataMap[etype];
					for (int i = 0; i < currentDataMap->size(); i++)
					{
						map[(*currentDataMap)[i]] = tokens[index++];
					}
				}

				if (etype == "pathnode")
					int test = 0;

				if (etype == "player")
				{
					positionX = std::stoi(tokens[indexOfPositionX]);
					positionY = std::stoi(tokens[indexOfPositionY]);
					game->player = game->SpawnPlayer(Vector2(positionX, positionY));
				}
				else if (etype == "tile")
				{
					//map["id"] = tokens[index++];
					//map["type"] = tokens[index++];
					//map["positionX"] = tokens[index++];
					//map["positionY"] = tokens[index++];

					//map["drawOrder"] = tokens[index++];
					//map["layer"] = tokens[index++];

					//map["passableState"] = tokens[index++];
					//map["tilesheet"] = tokens[index++];
					//map["frameX"] = tokens[index++];
					//map["frameY"] = tokens[index++];

					Entity::nextValidID = std::stoi(map[STR_ID]);

					int tilesheetIndex = std::stoi(map[STR_TILESHEET]);

					Tile* newTile = game->SpawnTile(Vector2(std::stoi(map[STR_FRAMEX]), std::stoi(map[STR_FRAMEY])),
						GetTileSheetFileName(tilesheetIndex),
						Vector2(std::stoi(map[STR_POSITIONX]), std::stoi(map[STR_POSITIONY])), 
						(DrawingLayer)std::stoi(map[STR_LAYER]));

					newTile->Load(map, *game);

					if (std::stoi(map[STR_PASSABLESTATE]) == 2)
						newTile->jumpThru = true;

					newTile->subtype = tilesheetIndex;
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
				else if (etype == "bg")
				{
					index = 4;
					std::string bgName = tokens[index++];

					// Create the backgrounds
					unsigned int NUM_BGS = 8;

					//TODO: Get the width of the texture automatically
					// or read it in from the level file
					unsigned int BG_WIDTH = 636 * 2;
					unsigned int X_OFFSET = 636 * -4;
					unsigned int Y_OFFSET = 0;

					if (bgName == "title")
					{
						X_OFFSET = 640; // half the width of the texture
						Y_OFFSET = 360; // half the height of the texture
						NUM_BGS = 1;
					}

					for (unsigned int i = 0; i < NUM_BGS; i++)
					{
						//Background* bg = game->SpawnBackground(Vector2((BG_WIDTH * i) + X_OFFSET, Y_OFFSET), bgName);
						Vector2 bgPos = Vector2((BG_WIDTH * i) + X_OFFSET, Y_OFFSET);
						game->background->CreateBackground(bgName, bgPos, game->spriteManager, game->renderer);
					}

					game->SortEntities(game->background->layers);
				}
				else
				{
					positionX = std::stoi(tokens[indexOfPositionX]);
					positionY = std::stoi(tokens[indexOfPositionY]);
					subtype = tokens[indexOfSubtype];

					// To prevent errors
					if (subtype == "")
						subtype = "0";

					Entity* newEntity = game->SpawnEntity(etype,
						Vector2(positionX,positionY), std::stoi(subtype));

					if (newEntity != nullptr)
					{
						newEntity->Load(map, *game);
						newEntity->Init(game->entityTypes[etype][std::stoi(tokens[5])]);
					}
				}
			}
			catch (const std::exception& e)
			{
				std::cout << "EXCEPTION: " << e.what() << std::endl;
				std::cout << "LINE: " << lineNumber << std::endl;
				std::cout << "INDEX: " << index << std::endl;
				game->logger.Log(e.what());
			}

			//ss.getline(lineChar, LINE_SIZE);
		}

		std::cout << "FINISH" << std::endl;

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
}

// TODO: Loading levels is kind of slow
void Editor::InitLevelFromFile(std::string levelName)
{
	for (auto& [key, val] : game->cutsceneManager.images)
	{
		if (val != nullptr)
			delete_it(val);
	}

	currentLevelText->SetText(levelName);
	game->cutsceneManager.watchingCutscene = false;

	game->cutsceneManager.images.clear();

	game->debugRectangles.clear();
	game->levelStartCutscene = "";

	game->quadTree.~QuadTree();
	game->quadTree = QuadTree(-4000, -4000, 8000, 8000);

	ClearLevelEntities();
	entitySubtype = 0;

	if (levelName != "")
		game->currentLevel = levelName;

	levelStrings.clear();
	levelStringIndex = -1;		

	CreateLevelFromString(ReadLevelFromFile(levelName));

	DoAction();
	game->SortEntities(game->entities);

	for (int i = 0; i < game->entities.size(); i++)
	{
		//TODO: Only add entities that have colliders or are impassable
		game->quadTree.Insert(game->entities[i]);
	}

	game->gui->ResetText();

	// Play the cutscene for the current level, if any
	if (game->levelStartCutscene != "")
	{
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

	//NOTE: Figure out why removing this glitches out at the start
	game->SetScreenResolution(game->screenWidth, game->screenHeight);
}
