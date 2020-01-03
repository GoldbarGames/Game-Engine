#include "Editor.h"
#include "Game.h"
#include "globals.h"
#include <fstream>
#include <sstream>
#include <iterator>
#include "Tile.h"
#include "debug_state.h"

using std::string;

Editor::Editor(Game& g)
{
	theFont = g.theFont;

	game = &g;

	currentEditModeLayer = new Text(game->renderer, theFont);
	cursorPosition = new Text(game->renderer, theFont);
	cursorPosition->SetPosition(0, 50);

	dialogText = new Text(game->renderer, theFont, "");
	dialogInput = new Text(game->renderer, theFont, "");
	dialogText->SetPosition(dialogRect.x, dialogRect.y + 20);
	dialogInput->SetPosition(dialogRect.x, dialogRect.y + 70);

	npcNames = { "gramps", "the_man" };

	previewMap["tile"] = nullptr;
	previewMap["door"] = game->CreateDoor(Vector2(0,0), spriteMapIndex);
	previewMap["ladder"] = game->CreateLadder(Vector2(0, 0), spriteMapIndex);

	previewMap["goal"] = game->CreateGoal(Vector2(0, 0), spriteMapIndex);
	previewMap["bug"] = game->CreateBug(Vector2(0, 0), spriteMapIndex);
	previewMap["ether"] = game->CreateEther(Vector2(0, 0), spriteMapIndex);
	previewMap["block"] = game->CreateBlock(Vector2(0, 0), spriteMapIndex);
	previewMap["platform"] = game->CreatePlatform(Vector2(0, 0), spriteMapIndex);
	previewMap["shroom"] = game->CreateShroom(Vector2(0, 0), spriteMapIndex);
	//TODO: Make the indexes different numbers for the names and sprite sheets?
	previewMap["npc"] = game->CreateNPC(npcNames[spriteMapIndex], Vector2(0, 0), spriteMapIndex);
	

	game->entities.clear();	
}

Editor::~Editor()
{

}

void Editor::CreateEditorButtons()
{
	// Create all the buttons for the bottom of the editor
	for (unsigned int i = 0; i < buttons.size(); i++)
		delete buttons[i];
	buttons.clear();

	int buttonX = 0;
	const int buttonWidth = 50;
	const int buttonHeight = 50;
	const int buttonSpacing = 20;

	std::vector<string> buttonNames = { "NewLevel", "Load", "Save", "Tileset", "Inspect", "Grid", "Map", "Door", "Ladder", "NPC", "Goal", "Bug", "Ether", "Undo", "Redo", "Replace", "Copy", "Block", "Grab", "Platform", "Path", "Shroom" };

	unsigned int BUTTON_LIST_START = currentButtonPage * BUTTONS_PER_PAGE;
	unsigned int BUTTON_LIST_END = BUTTON_LIST_START + BUTTONS_PER_PAGE;

	if (BUTTON_LIST_END > buttonNames.size())
		BUTTON_LIST_END = buttonNames.size();

	for (unsigned int i = BUTTON_LIST_START; i < BUTTON_LIST_END; i++)
	{
		if (i > buttonNames.size() - 1)
			break;

		buttons.emplace_back(new EditorButton("", buttonNames[i], Vector2(buttonX, screenHeight - buttonHeight), *game));
		buttonX += buttonWidth + buttonSpacing; // TODO: is there a way to not make this hard-coded? is it worth it?
	}

	buttons.emplace_back(new EditorButton("", "PrevPage", Vector2(0, screenHeight - buttonHeight - buttonHeight - buttonSpacing), *game));
	buttons.emplace_back(new EditorButton("", "NextPage", Vector2((buttonWidth + buttonSpacing) * (BUTTONS_PER_PAGE - 1),
		screenHeight - buttonHeight - buttonHeight - buttonSpacing), *game));
}

void Editor::StartEdit()
{
	//toolbox = SDL_CreateWindow("Toolbox", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 100, 100, SDL_WINDOW_OPENGL);
	//rendererToolbox = SDL_CreateRenderer(toolbox, -1, SDL_RENDERER_ACCELERATED);

	game->LoadEditorSettings();

	// TILE SHEET FOR TOOLBOX

	toolboxTexture = game->spriteManager->GetImage(game->renderer, "assets/tiles/" + tilesheets[tilesheetIndex] + ".png");
	toolboxTextureRect.x = 0;
	toolboxTextureRect.y = 0;

	SDL_QueryTexture(toolboxTexture, NULL, NULL, &toolboxTextureRect.w, &toolboxTextureRect.h);

	toolboxTextureRect.w *= 1;
	toolboxTextureRect.h *= 1;

	toolboxWindowRect.x = screenWidth - toolboxTextureRect.w;
	toolboxWindowRect.y = 0;
	toolboxWindowRect.w = toolboxTextureRect.w;
	toolboxWindowRect.h = toolboxTextureRect.h;

	objectPropertiesRect.w = 400;
	objectPropertiesRect.h = 600;
	objectPropertiesRect.x = screenWidth - objectPropertiesRect.w;
	objectPropertiesRect.y = 0;

	dialogRect.w = 400;
	dialogRect.h = 200;
	dialogRect.x = (screenWidth / 2) - (objectPropertiesRect.w / 2);
	dialogRect.y = (screenHeight / 2) - (objectPropertiesRect.h / 2);

	dialogText->SetPosition(dialogRect.x, dialogRect.y + 20);
	dialogInput->SetPosition(dialogRect.x, dialogRect.y + 70);

	//SDL_SetWindowSize(toolbox, toolboxWindowRect.w, toolboxWindowRect.h);

	selectedRect.x = toolboxWindowRect.x;
	selectedRect.y = 0;
	selectedRect.w = 24;
	selectedRect.h = 24;

	hoveredTileRect.w = 24 * Renderer::GetScale();
	hoveredTileRect.h = 24 * Renderer::GetScale();

	CreateEditorButtons();

	// Create the layer buttons

	for (unsigned int i = 0; i < layerButtons.size(); i++)
		delete layerButtons[i];
	layerButtons.clear();

	for (unsigned int i = 0; i < layerVisibleButtons.size(); i++)
		delete layerVisibleButtons[i];
	layerVisibleButtons.clear();

	int buttonY = 200;
	const int layerButtonWidth = 100;
	const int layerButtonHeight = 50;
	const int layerButtonSpacing = 20;

	std::vector<string> layerButtonNames = { "BACK", "MIDDLE", "OBJECT", "COLLISION", "FRONT" };

	for (unsigned int i = 0; i < layerButtonNames.size(); i++)
	{
		layerButtons.emplace_back(new EditorButton(layerButtonNames[i], "Layer", Vector2(0, buttonY), *game, Vector2(layerButtonWidth, 50), { 128, 128, 128, 255 }));
		layerVisibleButtons.emplace_back(new EditorButton("", "Visible", Vector2(layerButtonWidth, buttonY), *game, Vector2(50, 50)));
		buttonY += layerButtonHeight + layerButtonSpacing; // TODO: is there a way to not make this hard-coded? is it worth it?
	}

	ClickedLayerButton("BACK");
	//currentEditModeLayer->SetText("Active Mode: " + objectMode); // GetDrawingLayerName(drawingLayer));
}

void Editor::StopEdit()
{
	selectedEntity = nullptr;
	//inspectionMode = false;	
	propertyIndex = -1;
}

void Editor::LeftClick(Vector2 clickedPosition, int mouseX, int mouseY)
{
	bool clickedToolboxWindow = mouseX >= toolboxWindowRect.x && mouseY <= toolboxWindowRect.h;

	// Get name of the button that was clicked, if any
	string clickedButton = "";
	for (unsigned int i = 0; i < buttons.size(); i++)
	{
		if (buttons[i]->IsClicked(mouseX, mouseY))
			clickedButton = buttons[i]->name;
	}

	string clickedLayerButton = "";
	for (unsigned int i = 0; i < layerButtons.size(); i++)
	{
		if (layerButtons[i]->IsClicked(mouseX, mouseY))
			clickedLayerButton = layerButtons[i]->text->txt;
	}

	string clickedLayerVisibleButton = "";
	for (unsigned int i = 0; i < layerVisibleButtons.size(); i++)
	{
		if (layerVisibleButtons[i]->IsClicked(mouseX, mouseY))
			clickedLayerVisibleButton = layerButtons[i]->text->txt;
	}

	// Allow the tile sheet to be clicked when in certain modes
	if ( (objectMode == "tile" || objectMode == "replace" || objectMode == "copy") && clickedToolboxWindow)
	{
		int xOffset = (mouseX - toolboxWindowRect.x);
		selectedRect.x = (xOffset - (xOffset % (TILE_SIZE * 1)));
		selectedRect.y = (mouseY - (mouseY % (TILE_SIZE * 1)));

		editorTileX = (selectedRect.x / (TILE_SIZE * 1)) + 1;
		editorTileY = (selectedRect.y / (TILE_SIZE * 1)) + 1;

		selectedRect.x += toolboxWindowRect.x;
	}
	else if (clickedButton != "")
	{
		if (!(previousMouseState & SDL_BUTTON(SDL_BUTTON_LEFT)))
		{
			ClickedButton(clickedButton);
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
			game->renderer->ToggleVisibility(clickedLayerVisibleButton);

			for (unsigned int i = 0; i < layerVisibleButtons.size(); i++)
			{
				if (layerButtons[i]->text->txt == clickedLayerVisibleButton)
				{
					// Toggle between white and black colors
					if (layerVisibleButtons[i]->buttonColor.b == 255)
						layerVisibleButtons[i]->buttonColor = { 0, 0, 0, 0 };
					else
						layerVisibleButtons[i]->buttonColor = { 255, 255, 255, 255 };
				}
			}
		}
	}
	else if (objectMode == "inspect")
	{
		clickedPosition += game->camera;
		InspectObject(mouseX, mouseY);
	}
	else if (objectMode == "grab")
	{
		if (!(previousMouseState & SDL_BUTTON(SDL_BUTTON_LEFT)))
		{
			// Either grab a new entity, or place the currently grabbed one
			if (grabbedEntity == nullptr)
			{
				SDL_Point point;
				point.x = mouseX;
				point.y = mouseY;

				//std::cout << "x: " + point.x << std::endl;
				//std::cout << "y:" + point.y << std::endl;

				// Find the selected entity and grab it
				for (unsigned int i = 0; i < game->entities.size(); i++)
				{
					if (game->entities[i]->etype != "tile" &&
						SDL_PointInRect(&point, game->entities[i]->GetBounds()))
					{
						grabbedEntity = game->entities[i];
						oldGrabbedPosition = grabbedEntity->GetPosition();
						break;
					}
				}
			}
			else
			{
				// If the entity is allowed to spawn here, then place it there
				if (grabbedEntity->CanSpawnHere(Vector2(mouseX, mouseY), *game, false))
				{
					grabbedEntity = nullptr;
					DoAction();
				}
			}
		}
		
	}
	else // we clicked somewhere in the game world, so place a tile/object
	{
		clickedPosition += game->camera;

		// if we are placing a tile...
		if (objectMode == "tile")
		{
			PlaceTile(clickedPosition, mouseX, mouseY);
			DoAction();
		}
		else if (objectMode == "replace")
		{
			bool foundTile = false;
			Vector2 coordsToReplace = Vector2(0, 0);
			Vector2 coordsToSet = Vector2(editorTileX, editorTileY);

			for (unsigned int i = 0; i < game->entities.size(); i++)
			{
				if (RoundToInt(game->entities[i]->GetPosition()) == RoundToInt(clickedPosition) &&
					game->entities[i]->layer == drawingLayer &&
					game->entities[i]->etype == "tile")
				{
					Tile* tile = static_cast<Tile*>(game->entities[i]);

					// Save the index of the tile
					coordsToReplace = tile->tileCoordinates;
					foundTile = true;
				}
			}

			if (foundTile)
			{
				// Replace the tile with the one selected in the sprite sheet
				for (unsigned int i = 0; i < game->entities.size(); i++)
				{
					if (game->entities[i]->etype == "tile"
						&& game->entities[i]->tileCoordinates == coordsToReplace)
					{
						Tile* tile = dynamic_cast<Tile*>(game->entities[i]);

						// Set the index of the tile
						tile->ChangeSprite(coordsToSet,
							game->spriteManager->GetImage(game->renderer,
								"assets/tiles/" + tilesheets[tilesheetIndex] + ".png"),
							game->renderer);
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
				if (RoundToInt(game->entities[i]->GetPosition()) == RoundToInt(clickedPosition) &&
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

				selectedRect.x = (tile->tileCoordinates.x - 1) * TILE_SIZE;
				selectedRect.y = (tile->tileCoordinates.y - 1) * TILE_SIZE;

				editorTileX = tile->tileCoordinates.x;
				editorTileY = tile->tileCoordinates.y;

				selectedRect.x += toolboxWindowRect.x;
			}

		}
		else // when placing an object
		{
			PlaceObject(clickedPosition, mouseX, mouseY);
			DoAction();
		}
		
	}
}


void Editor::InspectObject(int mouseX, int mouseY)
{
	SDL_Point point;
	point.x = mouseX;
	point.y = mouseY;

	//std::cout << "x: " + point.x << std::endl;
	//std::cout << "y:" + point.y << std::endl;

	if (SDL_PointInRect(&point, &objectPropertiesRect))
	{
		for (unsigned int i = 0; i < properties.size(); i++)
		{
			if (SDL_PointInRect(&point, &properties[i]->text->textWindowRect))
			{
				if (selectedEntity != nullptr)
				{
					Color red = { 255, 0, 0, 255 };
					if (properties[i]->text->textColor != red)
					{
						propertyIndex = i;
						game->StartTextInput("properties");
						SetPropertyText();
					}					
				}
				break;
			}
		}
	}
	else
	{
		// Find the selected entity
		for (unsigned int i = 0; i < game->entities.size(); i++)
		{
			if (game->entities[i]->etype != "tile" &&
				SDL_PointInRect(&point, game->entities[i]->GetBounds()))
			{
				selectedEntity = game->entities[i];
				break;
			}
		}

		// If selected entity was found, then generate text for all properties of it
		if (selectedEntity != nullptr)
		{
			selectedEntity->GetProperties(game->renderer, theFont, properties);
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

void Editor::SetPropertyText()
{	
	selectedEntity->SetProperty(properties[propertyIndex]->text->txt, game->inputText);
	selectedEntity->GetProperties(game->renderer, theFont, properties);
	SetPropertyPositions();
}

void Editor::SetPropertyPositions()
{
	// Set the text position of all properties
	int propertyX = objectPropertiesRect.x + 10;
	int propertyY = objectPropertiesRect.y + 10;

	for (unsigned int i = 0; i < properties.size(); i++)
	{
		properties[i]->text->SetPosition(propertyX, propertyY);
		propertyY += 50;
	}
}

void Editor::PlaceObject(Vector2 clickedPosition, int mouseX, int mouseY)
{
	bool canPlaceObjectHere = true;

	clickedPosition.x = (int)clickedPosition.x;
	clickedPosition.y = (int)clickedPosition.y;

	for (unsigned int i = 0; i < game->entities.size(); i++)
	{
		if (game->entities[i]->GetPosition() == clickedPosition &&
			game->entities[i]->etype == objectMode)
		{
			canPlaceObjectHere = false;
			break;
		}
	}

	if (canPlaceObjectHere)
	{
		Vector2 snappedPosition = game->SnapToGrid(Vector2(mouseX, mouseY));
		if (objectMode == "npc")
		{
			currentNPC = game->SpawnNPC(npcNames[spriteMapIndex], snappedPosition, spriteMapIndex);
			if (currentNPC != nullptr)
			{
				game->SortEntities(game->entities);
			}
		}
		else if (objectMode == "goal")
		{
			Goal* currentGoal = game->SpawnGoal(snappedPosition, spriteMapIndex);
			if (currentGoal != nullptr)
				game->SortEntities(game->entities);
		}
		else if (objectMode == "bug")
		{
			Bug* currentBug = game->SpawnBug(snappedPosition, spriteMapIndex);
			if (currentBug != nullptr)
				game->SortEntities(game->entities);
		}
		else if (objectMode == "ether")
		{
			Ether* currentEther = game->SpawnEther(snappedPosition, spriteMapIndex);
			if (currentEther != nullptr)
				game->SortEntities(game->entities);
		}
		else if (objectMode == "block")
		{
			Block* currentblock = game->SpawnBlock(snappedPosition, spriteMapIndex);
			if (currentblock != nullptr)
				game->SortEntities(game->entities);
		}
		else if (objectMode == "platform")
		{
			Platform* currentPlatform = game->SpawnPlatform(snappedPosition, spriteMapIndex);
			if (currentPlatform != nullptr)
				game->SortEntities(game->entities);
		}
		else if (objectMode == "shroom")
		{
			Shroom* currentShroom = game->SpawnShroom(snappedPosition, spriteMapIndex);
			if (currentShroom != nullptr)
				game->SortEntities(game->entities);
		}
		else if (objectMode == "path")
		{
			// If we do not have a path, create a new one
			if (currentPath == nullptr)
			{
				currentPath = new Path(snappedPosition);
				currentPath->AddPointToPath(snappedPosition);
				game->entities.push_back(currentPath);
				game->SortEntities(game->entities);
			}
			else // otherwise, add to the current path
			{
				currentPath->AddPointToPath(snappedPosition);
			}
		}
		else if (objectMode == "door")
		{
			if (!placingDoor)
			{
				std::cout << "trying to spawn entrance" << std::endl;
				currentDoor = game->SpawnDoor(snappedPosition, spriteMapIndex);
				if (currentDoor != nullptr)
				{
					std::cout << "placing door set true" << std::endl;
					placingDoor = true;
					game->SortEntities(game->entities);
				}

			}
			else
			{
				std::cout << "trying to spawn destination" << std::endl;
				Door* destination = game->SpawnDoor(snappedPosition, spriteMapIndex);
				if (destination != nullptr)
				{
					std::cout << "placing door set false" << std::endl;
					placingDoor = false;

					currentDoor->SetDestination(destination->GetPosition());
					destination->SetDestination(currentDoor->GetPosition());
					currentDoor = nullptr;

					game->SortEntities(game->entities);
				}
			}

		}
		else if (objectMode == "ladder")
		{
			if (!placingLadder)
			{
				std::cout << "trying to spawn ladder start" << std::endl;
				currentLadder = game->SpawnLadder(snappedPosition, spriteMapIndex);
				if (currentLadder != nullptr)
				{
					std::cout << "placing ladder set true" << std::endl;
					placingLadder = true;
					game->SortEntities(game->entities);
				}
			}
			else
			{
				// only spawn if the position we clicked at is on the same column as the ladder start
				if (snappedPosition.x == currentLadder->GetPosition().x)
				{
					std::cout << "trying to spawn ladder end" << std::endl;
					Ladder* ladderEnd = game->SpawnLadder(snappedPosition, spriteMapIndex);
					if (ladderEnd != nullptr)
					{
						std::cout << "placing ladder set false" << std::endl;
						placingLadder = false;

						// Define which edges of the ladder are the top and bottom
						bool ladderGoesUp = false;
						if (ladderEnd->GetPosition().y > currentLadder->GetPosition().y)
						{
							ladderEnd->GetAnimator()->SetState("bottom");
							currentLadder->GetAnimator()->SetState("top");
						}
						else
						{
							ladderEnd->GetAnimator()->SetState("top");
							currentLadder->GetAnimator()->SetState("bottom");
							ladderGoesUp = true;
						}

						if (ladderGoesUp)
						{
							// Connect the two edges by spawning the middle parts
							mouseY += GRID_SIZE * Renderer::GetScale();

							while (snappedPosition.y < currentLadder->GetPosition().y)
							{
								game->SpawnLadder(snappedPosition, spriteMapIndex);
								snappedPosition.y += GRID_SIZE * Renderer::GetScale();
							}
						}
						else
						{
							// Connect the two edges by spawning the middle parts
							mouseY -= GRID_SIZE * Renderer::GetScale();

							while (snappedPosition.y > currentLadder->GetPosition().y)
							{
								game->SpawnLadder(snappedPosition, spriteMapIndex);
								snappedPosition.y -= GRID_SIZE * Renderer::GetScale();
							}
						}

						currentLadder = nullptr;

						game->SortEntities(game->entities);
					}
				}
			}
		}
	}
}

void Editor::PlaceTile(Vector2 clickedPosition, int mouseX, int mouseY)
{
	bool canPlaceTileHere = true;
	for (unsigned int i = 0; i < game->entities.size(); i++)
	{
		if (RoundToInt(game->entities[i]->GetPosition()) == RoundToInt(clickedPosition) &&
			game->entities[i]->layer == drawingLayer &&
			game->entities[i]->etype == "tile")
		{

			if (replaceSettingIndex == 0) //TODO: Can we replace these numbers with strings?
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
		int mod = (GRID_SIZE * Renderer::GetScale());

		int afterModX = ((int)(mouseX) % mod);
		int afterModY = ((int)(mouseY) % mod);

		Vector2 spawnPos = game->CalcTileSpawnPos(Vector2(mouseX - afterModX, mouseY - afterModY));

		game->SpawnTile(Vector2(editorTileX, editorTileY), "assets/tiles/" + tilesheets[tilesheetIndex] + ".png",
			spawnPos, drawingLayer);
		game->SortEntities(game->entities);
	}
}

// Toggle special properties of the selected entity
void Editor::MiddleClick(Vector2 clickedPosition)
{
	if (previousMouseState & SDL_BUTTON(SDL_BUTTON_MIDDLE))
		return;

	clickedPosition.x += (int)game->camera.x;
	clickedPosition.y += (int)game->camera.y;

	for (int i = game->entities.size() - 1; i >= 0; i--)
	{
		Vector2 entityPosition = RoundToInt(game->entities[i]->GetPosition());
		Vector2 clickedInt = RoundToInt(clickedPosition);

		bool samePosition = entityPosition.x >= clickedInt.x - 1 &&
			entityPosition.x <= clickedInt.x + 1 &&
			entityPosition.y >= clickedInt.y - 1 &&
			entityPosition.y <= clickedInt.y + 1;

		if (samePosition)
		{
			// Toggle the jump thru property of tiles
			if (game->entities[i]->etype == "tile")
			{
				game->entities[i]->jumpThru = !game->entities[i]->jumpThru;
			}
		}
	}
}

//TODO: Figure out how to structure this so we can add deleting stuff as an Action
void Editor::RightClick(Vector2 clickedPosition)
{
	// If we have grabbed an entity, return it to its old position and immediately exit
	if (grabbedEntity != nullptr)
	{
		grabbedEntity->SetPosition(oldGrabbedPosition);
		grabbedEntity = nullptr;
		return;
	}

	clickedPosition.x += (int)game->camera.x;
	clickedPosition.y += (int)game->camera.y;

	int ladderIndex = -1;

	// Iterate backwards so that we prioritize objects that are rendered closest to the camera
	for (int i = game->entities.size() - 1; i >= 0; i--)
	{
		Vector2 entityPosition = RoundToInt(game->entities[i]->GetPosition());
		Vector2 clickedInt = RoundToInt(clickedPosition);

		bool shouldDeleteThis = false;
		bool sameMode = game->entities[i]->etype == objectMode;
		bool sameLayer = game->entities[i]->layer == drawingLayer;
		bool samePosition = entityPosition.x >= clickedInt.x - 1 &&
			entityPosition.x <= clickedInt.x + 1 &&
			entityPosition.y >= clickedInt.y - 1 &&
			entityPosition.y <= clickedInt.y + 1;

		if (deleteSettingIndex == 0) // TODO: Can we change this number to a string?
		{
			// Same layer, same mode
			shouldDeleteThis = samePosition && sameLayer && sameMode;
		}
		else if (deleteSettingIndex == 1)
		{
			// Only same layer
			shouldDeleteThis = samePosition && sameLayer;
		}
		else if (deleteSettingIndex == 2)
		{
			// Only same mode
			shouldDeleteThis = samePosition && sameMode;
		}
		else if (deleteSettingIndex == 3)
		{
			// Can delete if at same position
			shouldDeleteThis = samePosition;
		}

		// If this entity is a path, check all points in the path
		// (This must be dealt with outside of the shouldDelete section
		// because each path contains multiple points that must each be
		// deleted individually if any of them have been clicked on)
		if (game->entities[i]->etype == "path")
		{
			Path* path = dynamic_cast<Path*>(game->entities[i]);
			if (path->IsPointInPath(clickedInt))
			{
				path->RemovePointFromPath(clickedInt);

				// Only if there are no points in the path do we remove the path
				if (path->nodes.size() == 0)
				{
					game->ShouldDeleteEntity(i);
					return;
				}
			}
		}

		if (shouldDeleteThis)
		{
			if (game->entities[i]->etype == "door")
			{
				// Save destination and delete the entry door
				Door* door = static_cast<Door*>(game->entities[i]);
				Vector2 dest = door->GetDestination();

				// Only delete if both doors have been placed
				if (dest != Vector2(0, 0))
				{
					game->ShouldDeleteEntity(i);

					// Delete the exit door
					for (unsigned int k = 0; k < game->entities.size(); k++)
					{
						if (game->entities[k]->GetPosition() == dest)
						{
							game->ShouldDeleteEntity(k);
							return;
						}
					}
				}				
			}
			else if (game->entities[i]->etype == "ladder")
			{
				ladderIndex = i;
			}
			else
			{
				game->ShouldDeleteEntity(i);
				return;
			}
		}
	}

	if (ladderIndex >= 0 && !placingLadder)
	{
		std::string startingState = game->entities[ladderIndex]->GetAnimator()->currentState->name;
		Vector2 lastPosition = game->entities[ladderIndex]->GetPosition();
		game->ShouldDeleteEntity(ladderIndex);

		if (startingState == "top")
			DestroyLadder("top", lastPosition);
		else if (startingState == "bottom")
			DestroyLadder("bottom", lastPosition);
		else if (startingState == "middle")
		{
			DestroyLadder("top", lastPosition);
			DestroyLadder("bottom", lastPosition);
		}
	}
}

void Editor::SetLayerButtonColor(Color color)
{
	for (int i = 0; i < layerButtons.size(); i++)
	{
		layerButtons[i]->buttonColor = color;
	}
}

void Editor::DestroyLadder(std::string startingState, Vector2 lastPosition)
{
	bool exit = false;
	while (!exit)
	{
		// go over all the entities and check to see if there is one at the next position
		// if it is, and it is a ladder, then delete it and keep going
		// otherwise, we are done, and can exit the loop

		if (startingState == "top")
			lastPosition.y += GRID_SIZE * Renderer::GetScale();
		else if (startingState == "bottom")
			lastPosition.y -= GRID_SIZE * Renderer::GetScale();

		exit = true;

		for (unsigned int i = 0; i < game->entities.size(); i++)
		{
			if (game->entities[i]->etype == "ladder")
			{
				if (RoundToInt(game->entities[i]->GetPosition()) == RoundToInt(lastPosition))
				{
					game->ShouldDeleteEntity(i);
					exit = false;
					break;
				}
			}
			
		}
	}
}

void Editor::HandleEdit()
{
	int mouseX = 0;
	int mouseY = 0;

	const Uint32 mouseState = SDL_GetMouseState(&mouseX, &mouseY);

	int clickedX = mouseX - ((int)mouseX % (GRID_SIZE * Renderer::GetScale()));
	int clickedY = mouseY - ((int)mouseY % (GRID_SIZE * Renderer::GetScale()));

	Vector2 clickedPosition(clickedX, clickedY);

	hoveredTileRect.x = clickedX;
	hoveredTileRect.y = clickedY;

	std::string clickedText = std::to_string(clickedX + (int)game->camera.x) + " " + std::to_string(clickedY + (int)game->camera.y);
	cursorPosition->SetText("Position: " + clickedText);

	if (mouseState & SDL_BUTTON(SDL_BUTTON_LEFT))
	{
		LeftClick(clickedPosition, mouseX, mouseY);
	}
	else if (mouseState & SDL_BUTTON(SDL_BUTTON_RIGHT)) // deletes tiles in order, nearest first
	{
		RightClick(clickedPosition);
	}
	else if (mouseState & SDL_BUTTON(SDL_BUTTON_MIDDLE)) // toggles special properties
	{
		MiddleClick(clickedPosition);
	}

	if (grabbedEntity != nullptr)
	{
		Vector2 snappedPosition = game->SnapToGrid(Vector2(mouseX, mouseY));
		grabbedEntity->SetPosition(snappedPosition);
	}

	previousMouseState = mouseState;
}

void Editor::ClickedButton(string buttonName)
{
	//objectMode = buttonName; //TODO: Make a better way to do this
	if (buttonName == "Tileset")
	{
		ToggleTileset();
	}
	else if (buttonName == "Map")
	{
		//TODO: Maybe make this use the mouse wheel too?
		ToggleSpriteMap();
	}
	else if (buttonName == "Grid")
	{
		ToggleGridSize();
	}
	else if (buttonName == "Door")
	{
		ToggleObjectMode("door");
	}
	else if (buttonName == "Ladder")
	{
		ToggleObjectMode("ladder");
	}	
	else if (buttonName == "NPC")
	{
		ToggleObjectMode("npc");
	}
	else if (buttonName == "Goal")
	{
		ToggleObjectMode("goal");
	}
	else if (buttonName == "Bug")
	{
		ToggleObjectMode("bug");
	}
	else if (buttonName == "Ether")
	{
		ToggleObjectMode("ether");
	}
	else if (buttonName == "Block")
	{
		ToggleObjectMode("block");
	}
	else if (buttonName == "Inspect")
	{
		ToggleInspectionMode();
	}
	else if (buttonName == "NewLevel")
	{
		NewLevel();
	}
	else if (buttonName == "Load")
	{
		CreateDialog("Type in the name of the file to load:");
		game->StartTextInput("load_file_as");
	}
	else if (buttonName == "Save")
	{
		SaveLevel();
	}
	else if (buttonName == "Undo")
	{
		UndoAction();
	}
	else if (buttonName == "Redo")
	{
		RedoAction();
	}
	else if (buttonName == "Replace")
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
	else if (buttonName == "Copy")
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
	else if (buttonName == "Grab")
	{
		ToggleObjectMode("grab");
	}
	else if (buttonName == "PrevPage")
	{
		currentButtonPage--;
		CreateEditorButtons();
	}
	else if (buttonName == "NextPage")
	{
		currentButtonPage++;
		CreateEditorButtons();
	}
	else if (buttonName == "Platform")
	{
		ToggleObjectMode("platform");
	}
	else if (buttonName == "Path")
	{
		if (currentPath != nullptr)
		{
			currentPath = nullptr;
		}

		ToggleObjectMode("path");
	}
	else if (buttonName == "Shroom")
	{
		ToggleObjectMode("shroom");
	}
}

void Editor::ToggleSpriteMap()
{
	spriteMapIndex++;

	if (game->spriteMap.count(objectMode) > 0 && spriteMapIndex >= game->spriteMap[objectMode].size())
		spriteMapIndex = 0;

	if (previewMap.count(objectMode) > 0 && previewMap[objectMode] != nullptr)
		delete previewMap[objectMode];

	// Update the preview sprites accordingly
	if (objectMode == "door")
	{
		previewMap[objectMode] = game->CreateDoor(Vector2(0, 0), spriteMapIndex);
	}
	else if (objectMode == "ladder")
	{
		previewMap[objectMode] = game->CreateLadder(Vector2(0, 0), spriteMapIndex);
	}
	else if (objectMode == "npc")
	{
		previewMap[objectMode] = game->CreateNPC(npcNames[spriteMapIndex], Vector2(0, 0), spriteMapIndex);
	}
	else if (objectMode == "goal")
	{
		previewMap[objectMode] = game->CreateGoal(Vector2(0, 0), spriteMapIndex);
	}
	else if (objectMode == "bug")
	{
		previewMap[objectMode] = game->CreateBug(Vector2(0, 0), spriteMapIndex);
	}
	else if (objectMode == "shroom")
	{
		previewMap[objectMode] = game->CreateShroom(Vector2(0, 0), spriteMapIndex);
	}

	objectPreview = previewMap[objectMode];
}


void Editor::ClickedLayerButton(string buttonText)
{
	// Highlight the current layer, return all others to normal
	for (unsigned int i = 0; i < layerButtons.size(); i++)
	{		
		layerButtons[i]->buttonColor = { 128, 128, 128, 255 };
		if (layerButtons[i]->text->txt == buttonText)
		{
			layerButtons[i]->buttonColor = { 0, 0, 255, 255 };
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
		if (mode == "npc")
			SetLayer(DrawingLayer::COLLISION);
		else
			SetLayer(DrawingLayer::OBJECT);

		objectMode = mode;
	}

	currentEditModeLayer->SetText("Active Mode: " + objectMode);
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

	currentEditModeLayer->SetText("Active Mode: " + objectMode);
	//inspectionMode = !inspectionMode;
}

void Editor::SetLayer(DrawingLayer layer)
{
	drawingLayer = layer;
	currentEditModeLayer->SetText("Drawing on layer: " + GetDrawingLayerName(drawingLayer));
}

void Editor::ToggleTileset()
{
	tilesheetIndex++;
	if (tilesheetIndex > 1)
		tilesheetIndex = 0;
	game->SaveEditorSettings();
	StartEdit();	
}

void Editor::DrawGrid()
{
	SDL_SetRenderDrawColor(game->renderer->renderer, 64, 64, 64, 64);
	for (int x = 0; x < 100; x++)
	{
		for (int y = 0; y < 100; y++)
		{
			SDL_Rect rect;
			rect.x = x * GRID_SIZE * Renderer::GetScale();
			rect.y = y * GRID_SIZE * Renderer::GetScale();
			SDL_RenderDrawRect(game->renderer->renderer, &rect);
		}
	}
	SDL_SetRenderDrawColor(game->renderer->renderer, 0, 0, 0, 255);
}

void Editor::Render(Renderer* renderer)
{
	// Draw a white rectangle around the currently highlighted grid tile
	SDL_SetRenderDrawColor(renderer->renderer, 255, 255, 255, 255);
	SDL_RenderDrawRect(renderer->renderer, &hoveredTileRect);
	SDL_SetRenderDrawColor(renderer->renderer, 0, 0, 0, 255);

	// Draw the object or tile that will be placed here, if any
	if (objectPreview != nullptr && objectPreview->GetSprite() != nullptr)
	{	
		if (GetModeDebug())
		{
			SDL_SetRenderDrawBlendMode(renderer->renderer, SDL_BLENDMODE_BLEND);
			if (!objectPreview->CanSpawnHere(Vector2(hoveredTileRect.x, hoveredTileRect.y), *game, false))
				SDL_SetRenderDrawColor(renderer->renderer, 255, 0, 0, 128);
			else
				SDL_SetRenderDrawColor(renderer->renderer, 0, 255, 0, 128);

			SDL_RenderFillRect(renderer->renderer, objectPreview->GetSprite()->GetRect());
			SDL_SetRenderDrawBlendMode(renderer->renderer, SDL_BLENDMODE_NONE);
			SDL_SetRenderDrawColor(renderer->renderer, 0, 0, 0, 255);
		}

		objectPreview->GetSprite()->Render(Vector2(hoveredTileRect.x, hoveredTileRect.y),
			0, -1, SDL_FLIP_NONE, renderer, 0);

		if (placingDoor && currentDoor != nullptr)
		{
			SDL_SetRenderDrawColor(renderer->renderer, 255, 255, 255, 255);

			Vector2 doorCenter = currentDoor->GetCenter();
			Vector2 doorPos = currentDoor->GetPosition() + doorCenter - game->camera;
			SDL_RenderDrawLine(renderer->renderer, doorPos.x, doorPos.y, hoveredTileRect.x + doorCenter.x, hoveredTileRect.y + doorCenter.y);

			SDL_SetRenderDrawColor(renderer->renderer, 0, 0, 0, 255);
		}
	}

	SDL_SetRenderDrawColor(renderer->renderer, 255, 255, 255, 255);
	for (unsigned int i = 0; i < game->entities.size(); i++)
	{
		if (game->entities[i]->etype == "door")
		{			
			Door* door = static_cast<Door*>(game->entities[i]);

			Vector2 destPos = door->GetDestination();
			if (destPos.x != 0 && destPos.y != 0)
			{
				destPos = destPos - game->camera;
				Vector2 doorCenter = door->GetCenter();
				Vector2 doorPos = door->GetPosition() + doorCenter - game->camera;
				SDL_RenderDrawLine(renderer->renderer, doorPos.x, doorPos.y, destPos.x + doorCenter.x, destPos.y + doorCenter.y);
			}	
		}
	}
	SDL_SetRenderDrawColor(renderer->renderer, 0, 0, 0, 255);

	if (objectMode == "inspect" && selectedEntity != nullptr)
	{
		// Draw a yellow rectangle around the currently selected object
		SDL_SetRenderDrawColor(renderer->renderer, 255, 255, 0, 255);
		SDL_RenderDrawRect(renderer->renderer, selectedEntity->GetBounds());

		// Draw the box that goes underneath all the properties
		SDL_SetRenderDrawColor(renderer->renderer, 128, 128, 128, 255);
		SDL_RenderFillRect(renderer->renderer, &objectPropertiesRect);
		
		// Draw the text for all the properties
		SDL_SetRenderDrawColor(renderer->renderer, 255, 255, 255, 255);

		for (unsigned int i = 0; i < properties.size(); i++)
		{
			if (propertyIndex > -1)
			{
				if (i == propertyIndex)
					SDL_RenderDrawRect(renderer->renderer, &properties[i]->text->textWindowRect);
			}

			properties[i]->text->Render(renderer);
		}

		SDL_SetRenderDrawColor(renderer->renderer, 0, 0, 0, 255);
	}
	else
	{
		if (objectMode == "tile" || objectMode == "replace" || objectMode == "copy")
		{
			// Draw a white rectangle around the entire tilesheet
			SDL_SetRenderDrawColor(renderer->renderer, 255, 0, 255, 255);
			SDL_RenderFillRect(renderer->renderer, &toolboxWindowRect);

			// Draw the tilesheet (only if we are placing a tile)
			SDL_RenderCopy(renderer->renderer, toolboxTexture, &toolboxTextureRect, &toolboxWindowRect);

			// Draw a yellow rectangle around the currently selected tileset tile
			SDL_SetRenderDrawColor(renderer->renderer, 255, 255, 0, 255);
			SDL_RenderDrawRect(renderer->renderer, &selectedRect);
			SDL_SetRenderDrawColor(renderer->renderer, 0, 0, 0, 255);
		}
	}	

	// Draw text
	currentEditModeLayer->Render(renderer);
	cursorPosition->Render(renderer);

	// Draw all buttons
	for (unsigned int i = 0; i < buttons.size(); i++)
	{
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

	if (showDialogPopup)
	{
		// Draw the box that goes underneath the popup dialog
		SDL_SetRenderDrawColor(renderer->renderer, 128, 128, 128, 255);
		SDL_RenderFillRect(renderer->renderer, &dialogRect);

		// Draw the text for the popup dialog
		SDL_SetRenderDrawColor(renderer->renderer, 255, 255, 255, 255);
		dialogText->Render(renderer);
		dialogInput->Render(renderer);
		SDL_SetRenderDrawColor(renderer->renderer, 0, 0, 0, 255);
	}
}

void Editor::SetText(string newText)
{
	//TODO: Check what to do here to avoid memory leaks
	//if (textSurface != nullptr)
	//	delete textSurface;
	//if (textTexture != nullptr)
	//	delete textTexture;

	currentEditModeLayer->SetText(newText);
}

void Editor::DestroyDialog()
{
	if (dialogText != nullptr)
		delete dialogText;

	if (dialogInput != nullptr)
		delete dialogInput;
}

void Editor::CreateDialog(std::string txt)
{
	//DestroyDialog();

	//TODO: There might be a small memory leak here, but it's not too bad right now.

	dialogText->SetText(txt);
	dialogInput->SetText("");

	showDialogPopup = true;
}

void Editor::NewLevel()
{
	for (unsigned int i = 0; i < game->entities.size(); i++)
		delete game->entities[i];
	game->entities.clear();

	game->player = game->SpawnPlayer(Vector2(0, 0));

	CreateDialog("Type in the filename of the new level:");
	game->StartTextInput("new_level");
}

std::string Editor::SaveLevelAsString()
{
	std::ostringstream level;

	for (unsigned int i = 0; i < game->entities.size(); i++)
	{
		game->entities[i]->Save(level);
	}

	return level.str();
}

void Editor::SaveLevel(std::string levelName)
{
	if (levelName != "")
		game->currentLevel = levelName;

	std::ofstream fout;
	fout.open("data/" + game->currentLevel + ".lvl");

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
	fin.open("data/" + levelName + ".lvl");

	std::string level = "";
	for (std::string line; std::getline(fin, line); )
	{
		level += line + "\n";
	}

	fin.close();

	return level;
}

void Editor::CreateLevelFromString(std::string level)
{
	std::vector<Path*> paths;
	std::vector<Platform*> movingPlatforms;

	int SCALE = Renderer::GetScale();
	std::stringstream ss{ level };

	char lineChar[256];
	ss.getline(lineChar, 256);

	while (ss.good() && !ss.eof())
	{
		std::istringstream buf(lineChar);
		std::istream_iterator<std::string> beg(buf), end;
		std::vector<std::string> tokens(beg, end);

		int index = 0;
		const int id = std::stoi(tokens[index++]);
		Entity::nextValidID = id;

		const std::string etype = tokens[index++];

		int positionX = std::stoi(tokens[index++]);
		int positionY = std::stoi(tokens[index++]);

		if (etype == "tile")
		{
			index++;
			int layer = std::stoi(tokens[index++]);

			int passableState = std::stoi(tokens[index++]);

			int tilesheet = std::stoi(tokens[index++]);
			int frameX = std::stoi(tokens[index++]);
			int frameY = std::stoi(tokens[index++]);

			Tile* newTile = game->SpawnTile(Vector2(frameX, frameY), "assets/tiles/" + tilesheets[tilesheet] + ".png",
				Vector2(positionX * SCALE, positionY * SCALE), (DrawingLayer)layer);

			if (passableState == 2)
				newTile->jumpThru = true;

			newTile->tilesheetIndex = tilesheet;
		}
		else if (etype == "door")
		{
			int destX = std::stoi(tokens[index++]);
			int destY = std::stoi(tokens[index++]);
			int spriteIndex = std::stoi(tokens[index++]);
			Door* newDoor = game->SpawnDoor(Vector2(positionX * SCALE, positionY * SCALE), spriteIndex);			
			newDoor->SetDestination(Vector2(destX * SCALE, destY * SCALE));
		}
		else if (etype == "ladder")
		{
			std::string ladderState = tokens[index++];
			int spriteIndex = std::stoi(tokens[index++]);
			Ladder* newLadder = game->SpawnLadder(Vector2(positionX * SCALE, positionY * SCALE), spriteIndex);
			newLadder->GetAnimator()->SetState(ladderState);
		}
		else if (etype == "player")
		{
			game->player = game->SpawnPlayer(Vector2(positionX, positionY));
		}
		else if (etype == "npc")
		{
			std::string npcName = tokens[index++];
			std::string npcCutscene = tokens[index++];
			int spriteIndex = std::stoi(tokens[index++]);
			NPC* newNPC = game->SpawnNPC(npcName, Vector2(positionX, positionY), spriteIndex);
			newNPC->cutsceneLabel = npcCutscene;

			newNPC->drawOrder = std::stoi(tokens[index++]);
			newNPC->layer = (DrawingLayer)std::stoi(tokens[index++]);
			newNPC->impassable = std::stoi(tokens[index++]);
		}
		else if (etype == "goal")
		{
			int spriteIndex = std::stoi(tokens[index++]);
			Goal* entity = game->SpawnGoal(Vector2(positionX * SCALE, positionY * SCALE), spriteIndex);
		}
		else if (etype == "bug")
		{
			int spriteIndex = std::stoi(tokens[index++]);
			Bug* entity = game->SpawnBug(Vector2(positionX * SCALE, positionY * SCALE), spriteIndex);
		}
		else if (etype == "ether")
		{
			int spriteIndex = std::stoi(tokens[index++]);
			Ether* entity = game->SpawnEther(Vector2(positionX * SCALE, positionY * SCALE), spriteIndex);
		}
		else if (etype == "block")
		{
			int spriteIndex = std::stoi(tokens[index++]);
			Block* block = game->SpawnBlock(Vector2(positionX * SCALE, positionY * SCALE), spriteIndex);
		}
		else if (etype == "shroom")
		{
			int spriteIndex = std::stoi(tokens[index++]);
			Shroom* entity = game->SpawnShroom(Vector2(positionX * SCALE, positionY * SCALE), spriteIndex);
		}
		else if (etype == "path")
		{
			bool shouldLoop = std::stoi(tokens[index++]);
			int nodeCount = std::stoi(tokens[index++]);

			Path* path = new Path(Vector2(positionX, positionY));

			for (int i = 0; i < nodeCount; i++)
			{
				int pointX = std::stoi(tokens[index++]);
				int pointY = std::stoi(tokens[index++]);
				path->AddPointToPath(Vector2(pointX, pointY));
			}

			game->entities.emplace_back(path);
			paths.emplace_back(path);
		}
		else if (etype == "platform")
		{
			int spriteIndex = std::stoi(tokens[index++]);
			Platform* platform = game->SpawnPlatform(Vector2(positionX * SCALE, positionY * SCALE), spriteIndex);

			platform->platformType = tokens[index++];

			if (platform->platformType == "Path")
			{				
				platform->pathID = std::stoi(tokens[index++]);
				platform->pathSpeed = std::stof(tokens[index++]);
				platform->endPathBehavior = tokens[index++];
				movingPlatforms.emplace_back(platform);
			}
			else
			{
				float vx = std::stof(tokens[index++]);
				float vy = std::stof(tokens[index++]);
				platform->startVelocity = Vector2(vx, vy);
				platform->tilesToMove = std::stoi(tokens[index++]);
				platform->shouldLoop = std::stoi(tokens[index++]);
				platform->SetVelocity(platform->startVelocity);
			}	
		}

		ss.getline(lineChar, 256);
	}

	int id2 = Entity::nextValidID;

	// Match all platforms moving on paths with their assigned path
	for (int i = 0; i < movingPlatforms.size(); i++)
	{
		for (int k = 0; k < paths.size(); k++)
		{
			if (movingPlatforms[i]->pathID == paths[k]->id)
			{
				movingPlatforms[i]->currentPath = paths[k];
				movingPlatforms[i]->SetPosition(paths[k]->nodes[0]->point);
				break;
			}
		}
	}

}

void Editor::ClearLevelEntities()
{
	for (unsigned int i = 0; i < game->entities.size(); i++)
		delete game->entities[i];
	game->entities.clear();
}


void Editor::InitLevelFromFile(std::string levelName)
{
	//std::cout << game->camera.y << std::endl;

	game->debugRectangles.clear();

	//const int OFFSET = -4;
	//game->camera = Vector2(0, OFFSET * TILE_SIZE * Renderer::GetScale());

	//std::cout << game->camera.y << std::endl;

	ClearLevelEntities();

	if (levelName != "")
		game->currentLevel = levelName;

	game->currentEther = game->startingEther;

	levelStrings.clear();
	levelStringIndex = -1;

	std::string level = ReadLevelFromFile(levelName);	
	
	CreateLevelFromString(level);

	DoAction();

	// Remove all backgrounds
	for (unsigned int i = 0; i < game->backgrounds.size(); i++)
	{
		game->backgrounds[i]->DeleteLayers(*game);
		delete game->backgrounds[i];
	}

	game->backgrounds.clear();
		
	// Create the backgrounds
	const int NUM_BGS = 4;
	const int BG_WIDTH = 636;
	const int BG_OFFSET = (BG_WIDTH * 2);

	int Y_OFFSET = -4 * TILE_SIZE * Renderer::GetScale();
	if (levelName == "title")
		Y_OFFSET = 0;

	game->camera = Vector2(0, 0);

	for (unsigned int i = 0; i < NUM_BGS; i++)
	{
		game->SpawnBackground(Vector2( (BG_WIDTH * Renderer::GetScale() * i) - BG_OFFSET, Y_OFFSET));
	}

	game->SortEntities(game->entities);

	// Count all bugs
	game->bugsRemaining = 0;
	for (int i = 0; i < game->entities.size(); i++)
	{
		if (game->entities[i]->etype == "bug")
			game->bugsRemaining++;
	}

	game->ResetText();
}
