#include "Editor.h"
#include "Game.h"
#include "globals.h"
#include <fstream>
#include <sstream>
#include <iterator>
#include "Tile.h"
#include "debug_state.h"
#include "CutsceneTrigger.h"

using std::string;

Editor::Editor(Game& g)
{
	theFont = g.theFont;

	game = &g;

	editorText["cursorPositionInScreen"] = new Text(game->renderer, theFont);
	editorText["cursorPositionInScreen"]->SetPosition(200, 50);
	editorText["cursorPositionInScreen"]->SetText("");
	editorText["cursorPositionInScreen"]->GetSprite()->keepPositionRelativeToCamera = true;

	editorText["cursorPositionInWorld"] = new Text(game->renderer, theFont);
	editorText["cursorPositionInWorld"]->SetPosition(200, 100);
	editorText["cursorPositionInWorld"]->SetText("");
	editorText["cursorPositionInWorld"]->GetSprite()->keepPositionRelativeToCamera = true;

	editorText["currentEditModeLayer"] = new Text(game->renderer, theFont);
	editorText["currentEditModeLayer"]->SetPosition(200, 200);
	editorText["currentEditModeLayer"]->SetText("");
	editorText["currentEditModeLayer"]->GetSprite()->keepPositionRelativeToCamera = true;

	dialogText = new Text(game->renderer, theFont, "");
	dialogInput = new Text(game->renderer, theFont, "");
	dialogText->SetPosition(dialogRect.x, dialogRect.y + 20);
	dialogInput->SetPosition(dialogRect.x, dialogRect.y + 70);

	npcNames = { "gramps", "the_man" };

	previewMap["tile"] = game->CreateTile(Vector2(0,0), "assets/editor/rect-outline.png", Vector2(0,0), DrawingLayer::FRONT);
	previewMap["tile"]->GetSprite()->color = { 255, 255, 255, 64 };

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

	SetLayer(DrawingLayer::BACK);
}

Editor::~Editor()
{

}

void Editor::CreateEditorButtons()
{
	// Create all the buttons for the bottom of the editor
	for (unsigned int i = 0; i < buttons.size(); i++)
	{
		delete buttons[i];
		buttons[i] = nullptr;
	}

	buttons.clear();

	const int buttonStartX = 50;

	int buttonX = buttonStartX;
	const int buttonWidth = 50;
	const int buttonHeight = 50;
	const int buttonSpacing = 20;

	std::vector<string> buttonNames = { "NewLevel", "Load", "Save", "Tileset", "Inspect", 
		"Grid", "Map", "Door", "Ladder", "NPC", "Goal", "Bug", "Ether", "Undo", "Redo", 
		"Replace", "Copy", "Block", "Grab", "Platform", "Path", "Shroom" };

	unsigned int BUTTON_LIST_START = currentButtonPage * BUTTONS_PER_PAGE;
	unsigned int BUTTON_LIST_END = BUTTON_LIST_START + BUTTONS_PER_PAGE;

	if (BUTTON_LIST_END > buttonNames.size())
		BUTTON_LIST_END = buttonNames.size();

	for (unsigned int i = BUTTON_LIST_START; i < BUTTON_LIST_END; i++)
	{
		if (i > buttonNames.size() - 1)
			break;

		EditorButton* editorButton = new EditorButton("", buttonNames[i], 
			Vector2(buttonX*2, (game->screenHeight - buttonHeight)*2), *game);
		
		editorButton->image->keepPositionRelativeToCamera = true;
		editorButton->image->keepScaleRelativeToCamera = true;
		buttons.emplace_back(editorButton);

		buttonX += buttonWidth + buttonSpacing; // TODO: is there a way to not make this hard-coded? is it worth it?
	}

	EditorButton* previousButton = new EditorButton("", "PrevPage", 
		Vector2(buttonStartX*2, (game->screenHeight - buttonHeight - buttonHeight - buttonSpacing)*2), *game);
	
	previousButton->image->keepScaleRelativeToCamera = true;
	buttons.emplace_back(previousButton);
	
	EditorButton* nextButton = new EditorButton("", "NextPage", 
		Vector2((buttonStartX + (buttonWidth + buttonSpacing) * (BUTTONS_PER_PAGE - 1))*2,
		(game->screenHeight - buttonHeight - buttonHeight - buttonSpacing)*2), *game);
	
	nextButton->image->keepScaleRelativeToCamera = true;
	buttons.emplace_back(nextButton);
}

void Editor::StartEdit()
{
	game->LoadEditorSettings();

	game->renderer->camera.ResetProjection();

	// TILE SHEET FOR TOOLBOX
	if (tilesheetSprites.empty())
	{
		for (int i = 0; i < tilesheetFilenames.size(); i++)
		{
			tilesheetSprites.push_back(new Sprite(1, game->spriteManager,
				"assets/tiles/" + tilesheetFilenames[i] + ".png",
				game->renderer->shaders["noalpha"], Vector2(0, 0)));

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

	objectPropertiesRect.w = 400;
	objectPropertiesRect.h = 600;
	objectPropertiesRect.x = (game->screenWidth * 2) - objectPropertiesRect.w;
	objectPropertiesRect.y = 100;

	dialogRect.x = (game->screenWidth / 2) - (objectPropertiesRect.w / 2);
	dialogRect.y = (game->screenHeight / 2) - (objectPropertiesRect.h / 2);
	dialogRect.w = 400;
	dialogRect.h = 200;

	dialogText->SetPosition(dialogRect.x, dialogRect.y + 20);
	dialogInput->SetPosition(dialogRect.x, dialogRect.y + 70);


	CreateEditorButtons();

	// Create the layer buttons

	for (unsigned int i = 0; i < layerButtons.size(); i++)
		delete layerButtons[i];
	layerButtons.clear();

	for (unsigned int i = 0; i < layerVisibleButtons.size(); i++)
		delete layerVisibleButtons[i];
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

	std::vector<string> layerButtonNames = { "BACK", "MIDDLE", "OBJECT", "COLLISION", "COLLISION2", "FRONT" };

	for (unsigned int i = 0; i < layerButtonNames.size(); i++)
	{
		EditorButton* layerButton = new EditorButton(layerButtonNames[i], "Layer", 
			Vector2(buttonX, buttonY), *game, Vector2(layerButtonWidth, 50), { 255, 255, 255, 255 });

		layerButton->image->keepScaleRelativeToCamera = true;
		layerButton->text->GetSprite()->keepScaleRelativeToCamera = true;
		layerButtons.emplace_back(layerButton);
		
		EditorButton* layerVisibleButton = new EditorButton("", "Visible", 
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
	game->renderer->camera.Zoom(0.0f, game->screenWidth, game->screenHeight);
	selectedEntity = nullptr;
	//inspectionMode = false;	
	propertyIndex = -1;
}

void Editor::LeftClick(Vector2 clickedScreenPosition, int mouseX, int mouseY, Vector2 clickedWorldPosition)
{
	bool clickedToolboxWindow = mouseX >= tilesheetPosition.x - tilesheetSprites[tilesheetIndex]->frameWidth
		&& mouseY <= tilesheetSprites[tilesheetIndex]->frameHeight * 2;

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
			clickedLayerVisibleButton = layerButtons[i]->text->txt;
	}

	mouseX /= 2;
	mouseY /= 2;

	// Allow the tile sheet to be clicked when in certain modes
	if ( (objectMode == "tile" || objectMode == "replace" || objectMode == "copy") && clickedToolboxWindow)
	{
		mouseX *= 2;
		mouseY *= 2;

		const int topLeftX = tilesheetPosition.x - tilesheetSprites[tilesheetIndex]->frameWidth;
		const int topLeftY = tilesheetPosition.y - tilesheetSprites[tilesheetIndex]->frameHeight;

		int xOffset = (mouseX - topLeftX);
		int yOffset = (mouseY - topLeftY);

		// Calculate the position in the tilesheet texture to use for drawing the tile
		float x2 = (xOffset / (float)(TILE_SIZE));
		float y2 = (yOffset / (float)(TILE_SIZE));

		spriteSheetTileFrame.x = (int)(roundf(x2)/2.0f) + 1;
		spriteSheetTileFrame.y = (int)(roundf(y2)/2.0f) + 1;

		int moveRight = ( (spriteSheetTileFrame.x - 1) * TILE_SIZE * 2);
		int moveDown = ( (spriteSheetTileFrame.y - 1) * TILE_SIZE * 2);

		//std::cout << "(" << x2 << "," << y2 << ")" << std::endl;
		//std::cout << "(" << spriteSheetTileFrame.x << "," << spriteSheetTileFrame.y << ")" << std::endl;

		// Set the location of the yellow rectangle indicating which tile will be drawn
		selectedTilePosition.x = topLeftX + TILE_SIZE + moveRight;
		selectedTilePosition.y = topLeftY + TILE_SIZE + moveDown;
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
					game->renderer->ToggleVisibility(DrawingLayer::BACK);
				}
				else if (clickedLayerVisibleButton == "MIDDLE")
				{
					game->renderer->ToggleVisibility(DrawingLayer::MIDDLE);
				}
				else if (clickedLayerVisibleButton == "OBJECT")
				{
					game->renderer->ToggleVisibility(DrawingLayer::OBJECT);
				}
				else if (clickedLayerVisibleButton == "COLLISION")
				{
					game->renderer->ToggleVisibility(DrawingLayer::COLLISION);
				}
				else if (clickedLayerVisibleButton == "COLLISION2")
				{
					game->renderer->ToggleVisibility(DrawingLayer::COLLISION2);
				}
				else if (clickedLayerVisibleButton == "FRONT")
				{
					game->renderer->ToggleVisibility(DrawingLayer::FRONT);
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
		InspectObject(clickedWorldPosition.x, clickedWorldPosition.y);
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
	else if (mouseY < 1290/2) // we clicked somewhere in the game world, so place a tile/object
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

			for (unsigned int i = 0; i < game->entities.size(); i++)
			{
				if (RoundToInt(game->entities[i]->GetPosition()) == RoundToInt(clickedWorldPosition) &&
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
						tile->ChangeSprite(spriteSheetTileFrame,
							game->spriteManager->GetImage("assets/tiles/" + tilesheetFilenames[tilesheetIndex] + ".png"),
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
			//TODO: Fix for OpenGL		
			SDL_Rect textRect;
			textRect.w = properties[i]->text->GetTextWidth();
			textRect.h = properties[i]->text->GetTextHeight();
			textRect.x = properties[i]->text->position.x - (textRect.w/2);
			textRect.y = properties[i]->text->position.y - (textRect.h/2);			

			if (SDL_PointInRect(&point, &textRect))
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
							while (snappedPosition.y < currentLadder->GetPosition().y)
							{
								game->SpawnLadder(snappedPosition, spriteMapIndex);
								snappedPosition.y += TILE_SIZE * 2;
							}
						}
						else
						{
							// Connect the two edges by spawning the middle parts
							while (snappedPosition.y > currentLadder->GetPosition().y)
							{
								game->SpawnLadder(snappedPosition, spriteMapIndex);
								snappedPosition.y -= TILE_SIZE * 2;
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

	Vector2 spawnPos = game->CalculateObjectSpawnPosition(clickedPosition, GRID_SIZE);

	for (unsigned int i = 0; i < game->entities.size(); i++)
	{
		Vector2 entityPosition = RoundToInt(game->entities[i]->GetPosition());

		if (entityPosition == spawnPos &&
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
		//TODO: Can't be this simple. Our zoom level affects the mouse position!
		// Do we need to use raycasts or something else here?

		//glm::mat4 invertedProjection = glm::inverse(game->renderer->camera.projection);
		//glm::vec4 spawnPos = (invertedProjection * glm::vec4(mouseX, mouseY, 0, 1));

		game->SpawnTile(spriteSheetTileFrame, "assets/tiles/" + tilesheetFilenames[tilesheetIndex] + ".png",
			Vector2(spawnPos.x, spawnPos.y), drawingLayer);
		game->SortEntities(game->entities);

		std::cout << "Spawned at (" << spawnPos.x << "," << spawnPos.y << "!" << std::endl;
	}
}

// Toggle special properties of the selected entity
void Editor::MiddleClick(Vector2 clickedPosition)
{
	if (previousMouseState & SDL_BUTTON(SDL_BUTTON_MIDDLE))
		return;

	//clickedPosition.x += (int)game->camera.x;
	//clickedPosition.y += (int)game->camera.y;

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

	//clickedPosition.x += (int)game->camera.x;
	//clickedPosition.y += (int)game->camera.y;

	int ladderIndex = -1;

	// Iterate backwards so that we prioritize objects that are rendered closest to the camera
	for (int i = game->entities.size() - 1; i >= 0; i--)
	{
		Vector2 entityPosition = RoundToInt(game->entities[i]->GetPosition());
		Vector2 clickedInt = RoundToInt(clickedPosition);

		bool shouldDeleteThis = false;
		bool sameMode = game->entities[i]->etype == objectMode;
		bool sameLayer = game->entities[i]->layer == drawingLayer;
		
		bool samePosition = (entityPosition == game->CalculateObjectSpawnPosition(clickedInt, GRID_SIZE));
		
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

void Editor::DestroyLadder(std::string startingState, Vector2 lastPosition)
{
	bool exit = false;
	while (!exit)
	{
		// go over all the entities and check to see if there is one at the next position
		// if it is, and it is a ladder, then delete it and keep going
		// otherwise, we are done, and can exit the loop

		if (startingState == "top")
			lastPosition.y += GRID_SIZE;
		else if (startingState == "bottom")
			lastPosition.y -= GRID_SIZE;

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

	int clickedX = mouseX - ((int)mouseX % (GRID_SIZE));
	int clickedY = mouseY - ((int)mouseY % (GRID_SIZE));

	// snapped position in screen space (0,0) to (1280,720)
	Vector2 clickedScreenPosition(clickedX, clickedY); 

	objPreviewPosition = game->CalculateObjectSpawnPosition(Vector2(mouseX, mouseY), GRID_SIZE);

	std::string clickedText = std::to_string(mouseX) + " " + std::to_string(mouseY);
	editorText["cursorPositionInScreen"]->SetText("Mouse Screen: " + clickedText);
	editorText["cursorPositionInScreen"]->GetSprite()->keepScaleRelativeToCamera = true;

	glm::mat4 invertedProjection = glm::inverse(game->renderer->camera.projection);
	glm::vec4 spawnPos = (invertedProjection * glm::vec4(mouseX, mouseY, 0, 1));

	std::string clickedText2 = std::to_string((int)spawnPos.x) + " " + std::to_string((int)spawnPos.y);
	editorText["cursorPositionInWorld"]->SetText("Mouse World: " + clickedText2);
	editorText["cursorPositionInWorld"]->GetSprite()->keepScaleRelativeToCamera = true;

	if (mouseState & SDL_BUTTON(SDL_BUTTON_LEFT))
	{
		// We multiply X and Y by 2 because the guiProjection is multiplied by 2
		// TODO: Maybe remove the multiplier
		LeftClick(clickedScreenPosition, mouseX*2, mouseY*2, objPreviewPosition);
	}
	else if (mouseState & SDL_BUTTON(SDL_BUTTON_RIGHT)) // deletes tiles in order, nearest first
	{
		RightClick(clickedScreenPosition);
	}
	else if (mouseState & SDL_BUTTON(SDL_BUTTON_MIDDLE)) // toggles special properties
	{
		MiddleClick(clickedScreenPosition);
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
		Vector2 snappedPosition = game->SnapToGrid(Vector2(mouseX, mouseY));
		grabbedEntity->SetPosition(snappedPosition);
	}

	previousMouseState = mouseState;
}

void Editor::ClickedButton()
{
	if (clickedButton == nullptr)
		return;

	//objectMode = buttonName;
	//TODO: Make a better way to do this
	// (Use a switch/case instead of if-else

	if (clickedButton->name == "Tileset")
	{
		ToggleTileset();
		clickedButton->isClicked = false;
	}
	else if (clickedButton->name == "Map")
	{
		//TODO: Maybe make this use the mouse wheel too?
		ToggleSpriteMap();
	}
	else if (clickedButton->name == "Grid")
	{
		ToggleGridSize();
	}
	else if (clickedButton->name == "Door")
	{
		ToggleObjectMode("door");
	}
	else if (clickedButton->name == "Ladder")
	{
		ToggleObjectMode("ladder");
	}	
	else if (clickedButton->name == "NPC")
	{
		ToggleObjectMode("npc");
	}
	else if (clickedButton->name == "Goal")
	{
		ToggleObjectMode("goal");
	}
	else if (clickedButton->name == "Bug")
	{
		ToggleObjectMode("bug");
	}
	else if (clickedButton->name == "Ether")
	{
		ToggleObjectMode("ether");
	}
	else if (clickedButton->name == "Block")
	{
		ToggleObjectMode("block");
	}
	else if (clickedButton->name == "Inspect")
	{
		ToggleInspectionMode();
	}
	else if (clickedButton->name == "NewLevel")
	{
		NewLevel();
		clickedButton->isClicked = false;
	}
	else if (clickedButton->name == "Load")
	{
		CreateDialog("Type in the name of the file to load:");
		game->StartTextInput("load_file_as");
		clickedButton->isClicked = false;
	}
	else if (clickedButton->name == "Save")
	{
		SaveLevel();
		clickedButton->isClicked = false;
	}
	else if (clickedButton->name == "Undo")
	{
		UndoAction();
		clickedButton->isClicked = false;
	}
	else if (clickedButton->name == "Redo")
	{
		RedoAction();
		clickedButton->isClicked = false;
	}
	else if (clickedButton->name == "Replace")
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
	else if (clickedButton->name == "Copy")
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
	else if (clickedButton->name == "Grab")
	{
		ToggleObjectMode("grab");
	}
	else if (clickedButton->name == "PrevPage")
	{
		currentButtonPage--;
		CreateEditorButtons();
		clickedButton->isClicked = false;
	}
	else if (clickedButton->name == "NextPage")
	{
		currentButtonPage++;
		CreateEditorButtons();
		clickedButton->isClicked = false;
	}
	else if (clickedButton->name == "Platform")
	{
		ToggleObjectMode("platform");
	}
	else if (clickedButton->name == "Path")
	{
		if (currentPath != nullptr)
		{
			currentPath = nullptr;
		}

		ToggleObjectMode("path");
	}
	else if (clickedButton->name == "Shroom")
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
		delete_it(previewMap[objectMode]);

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
		if (mode == "npc")
			SetLayer(DrawingLayer::COLLISION);
		else
			SetLayer(DrawingLayer::OBJECT);

		objectMode = mode;
	}

	editorText["currentEditModeLayer"]->SetText("Active Mode: " + objectMode);
	editorText["currentEditModeLayer"]->GetSprite()->keepScaleRelativeToCamera = true;
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

	editorText["currentEditModeLayer"]->SetText("Active Mode: " + objectMode);
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

void Editor::DrawGrid()
{
	//SDL_SetRenderDrawColor(game->renderer->renderer, 64, 64, 64, 64);

	for (int x = 0; x < 100; x++)
	{
		for (int y = 0; y < 100; y++)
		{
			SDL_Rect rect;
			rect.x = x * GRID_SIZE;
			rect.y = y * GRID_SIZE;
			//SDL_RenderDrawRect(game->renderer->renderer, &rect);
		}
	}

	//SDL_SetRenderDrawColor(game->renderer->renderer, 0, 0, 0, 255);
}

void Editor::Render(Renderer* renderer)
{
	Vector2 cameraOffset = Vector2(renderer->camera.position.x, renderer->camera.position.y);

	// Draw a white rectangle around the currently highlighted grid tile
	//SDL_SetRenderDrawColor(renderer->renderer, 255, 255, 255, 255);
	//SDL_RenderDrawRect(renderer->renderer, &hoveredTileRect);
	//SDL_SetRenderDrawColor(renderer->renderer, 0, 0, 0, 255);

	// Draw the object or tile that will be placed here, if any
	if (objectPreview != nullptr && objectPreview->GetSprite() != nullptr)
	{	
		if (GetModeDebug())
		{
			//SDL_SetRenderDrawBlendMode(renderer->renderer, SDL_BLENDMODE_BLEND);
			
			/*
			if (!objectPreview->CanSpawnHere(Vector2(hoveredTileRect.x, hoveredTileRect.y), *game, false))
				SDL_SetRenderDrawColor(renderer->renderer, 255, 0, 0, 128);
			else
				SDL_SetRenderDrawColor(renderer->renderer, 0, 255, 0, 128);
				*/

			//SDL_RenderFillRect(renderer->renderer, objectPreview->GetSprite()->GetRect());
			//SDL_SetRenderDrawBlendMode(renderer->renderer, SDL_BLENDMODE_NONE);
			//SDL_SetRenderDrawColor(renderer->renderer, 0, 0, 0, 255);
		}

		objectPreview->GetSprite()->Render(Vector2(objPreviewPosition.x, objPreviewPosition.y),
			0, -1, SDL_FLIP_NONE, renderer, objectPreview->rotation);

		if (placingDoor && currentDoor != nullptr)
		{
			//SDL_SetRenderDrawColor(renderer->renderer, 255, 255, 255, 255);

			Vector2 doorCenter = currentDoor->GetCenter();
			Vector2 doorPos = currentDoor->GetPosition() + doorCenter;
			//SDL_RenderDrawLine(renderer->renderer, doorPos.x, doorPos.y, 
			//hoveredTileRect.x + doorCenter.x, hoveredTileRect.y + doorCenter.y);

			//SDL_SetRenderDrawColor(renderer->renderer, 0, 0, 0, 255);
		}
	}

	//SDL_SetRenderDrawColor(renderer->renderer, 255, 255, 255, 255);

	for (unsigned int i = 0; i < game->entities.size(); i++)
	{
		if (game->entities[i]->etype == "door")
		{			
			Door* door = static_cast<Door*>(game->entities[i]);

			Vector2 destPos = door->GetDestination();
			if (destPos.x != 0 && destPos.y != 0)
			{
				destPos = destPos;
				Vector2 doorCenter = door->GetCenter();
				Vector2 doorPos = door->GetPosition() + doorCenter;
				//SDL_RenderDrawLine(renderer->renderer, doorPos.x, doorPos.y, destPos.x + doorCenter.x, destPos.y + doorCenter.y);
			}	
		}
	}
	
	if (objectMode == "inspect" && selectedEntity != nullptr)
	{
		// Draw a yellow rectangle around the currently selected object
		//SDL_SetRenderDrawColor(renderer->renderer, 255, 255, 0, 255);
		//SDL_RenderDrawRect(renderer->renderer, selectedEntity->GetBounds());

		// Draw the box that goes underneath all the properties
		//SDL_SetRenderDrawColor(renderer->renderer, 128, 128, 128, 255);
		//SDL_RenderFillRect(renderer->renderer, &objectPropertiesRect);
		
		// Draw the text for all the properties
		//SDL_SetRenderDrawColor(renderer->renderer, 255, 255, 255, 255);

		for (unsigned int i = 0; i < properties.size(); i++)
		{
			if (propertyIndex > -1)
			{
				//TODO: Fix for OpenGL
				//if (i == propertyIndex)
				//	SDL_RenderDrawRect(renderer->renderer, &properties[i]->text->);
			}

			properties[i]->text->GetSprite()->keepPositionRelativeToCamera = true;
			properties[i]->text->GetSprite()->keepScaleRelativeToCamera = true;
			properties[i]->text->Render(renderer);
		}

		//SDL_SetRenderDrawColor(renderer->renderer, 0, 0, 0, 255);
	}
	else
	{
		if (objectMode == "tile" || objectMode == "replace" || objectMode == "copy")
		{
			// Draw the tilesheet (only if we are placing a tile)
			tilesheetSprites[tilesheetIndex]->Render(tilesheetPosition, game->renderer);

			// Draw a yellow rectangle around the currently selected tileset tile
			game->renderer->debugSprite->color = { 255, 255, 0, 255 };
			game->renderer->debugSprite->scale = Vector2(1, 1);
			game->renderer->debugSprite->keepPositionRelativeToCamera = true;
			game->renderer->debugSprite->keepScaleRelativeToCamera = true;
			game->renderer->debugSprite->Render(selectedTilePosition, renderer);
			game->renderer->debugSprite->keepPositionRelativeToCamera = false;
			game->renderer->debugSprite->keepScaleRelativeToCamera = false;
		}
	}	

	// Draw text
	editorText["cursorPositionInScreen"]->Render(renderer);
	editorText["cursorPositionInWorld"]->Render(renderer);

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
		//SDL_SetRenderDrawColor(renderer->renderer, 128, 128, 128, 255);
		//SDL_RenderFillRect(renderer->renderer, &dialogRect);

		// Draw the text for the popup dialog
		//SDL_SetRenderDrawColor(renderer->renderer, 255, 255, 255, 255);
		dialogText->Render(renderer);
		dialogInput->Render(renderer);
		//SDL_SetRenderDrawColor(renderer->renderer, 0, 0, 0, 255);
	}
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

	game->background->Save(level);

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

void Editor::CreateLevelFromString(std::string level)
{
	if (game->background == nullptr)
		game->background = new Background("", Vector2(0,0));

	// Remove all backgrounds
	game->background->ResetBackground();

	std::vector<Path*> paths;
	std::vector<Platform*> movingPlatforms;

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

			Tile* newTile = game->SpawnTile(Vector2(frameX, frameY), "assets/tiles/" + tilesheetFilenames[tilesheet] + ".png",
				Vector2(positionX, positionY), (DrawingLayer)layer);

			if (passableState == 2)
				newTile->jumpThru = true;

			newTile->tilesheetIndex = tilesheet;
		}
		else if (etype == "door")
		{
			int destX = std::stoi(tokens[index++]);
			int destY = std::stoi(tokens[index++]);
			int spriteIndex = std::stoi(tokens[index++]);
			Door* newDoor = game->SpawnDoor(Vector2(positionX, positionY), spriteIndex);			
			newDoor->SetDestination(Vector2(destX, destY));
		}
		else if (etype == "ladder")
		{
			std::string ladderState = tokens[index++];
			int spriteIndex = std::stoi(tokens[index++]);
			Ladder* newLadder = game->SpawnLadder(Vector2(positionX, positionY), spriteIndex);
			newLadder->GetAnimator()->SetState(ladderState.c_str());
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
			Goal* entity = game->SpawnGoal(Vector2(positionX, positionY), spriteIndex);
		}
		else if (etype == "bug")
		{
			int spriteIndex = std::stoi(tokens[index++]);
			Bug* entity = game->SpawnBug(Vector2(positionX, positionY), spriteIndex);
		}
		else if (etype == "ether")
		{
			int spriteIndex = std::stoi(tokens[index++]);
			Ether* entity = game->SpawnEther(Vector2(positionX, positionY), spriteIndex);
		}
		else if (etype == "block")
		{
			int spriteIndex = std::stoi(tokens[index++]);
			Block* block = game->SpawnBlock(Vector2(positionX, positionY), spriteIndex);
		}
		else if (etype == "shroom")
		{
			int spriteIndex = std::stoi(tokens[index++]);
			Shroom* entity = game->SpawnShroom(Vector2(positionX, positionY), spriteIndex);
		}
		else if (etype == "cutscene-trigger")
		{
			const std::string label = tokens[index++];
			int w = std::stoi(tokens[index++]);
			int h = std::stoi(tokens[index++]);
			CutsceneTrigger* entity = new CutsceneTrigger(label, Vector2(positionX, positionY), w, h);
			game->entities.push_back(entity);
		}
		else if (etype == "cutscene-start")
		{
			const std::string label = tokens[index++];
			game->levelStartCutscene = label;
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
			Platform* platform = game->SpawnPlatform(Vector2(positionX, positionY), spriteIndex);

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
				platform->physics->SetVelocity(platform->startVelocity);
			}	
		}
		else if (etype == "bg")
		{
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

		ss.getline(lineChar, 256);
	}

	int id2 = Entity::nextValidID;

	// Match all platforms moving on paths with their assigned path
	for (unsigned int i = 0; i < movingPlatforms.size(); i++)
	{
		for (unsigned int k = 0; k < paths.size(); k++)
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

	game->levelStartCutscene = "";

	ClearLevelEntities();

	if (levelName != "")
		game->currentLevel = levelName;

	game->currentEther = game->startingEther;

	levelStrings.clear();
	levelStringIndex = -1;

	std::string level = ReadLevelFromFile(levelName);	
	
	CreateLevelFromString(level);

	DoAction();
	game->SortEntities(game->entities);

	// Count all bugs
	game->bugsRemaining = 0;
	for (unsigned int i = 0; i < game->entities.size(); i++)
	{
		if (game->entities[i]->etype == "bug")
			game->bugsRemaining++;
	}

	game->ResetText();

	if (game->levelStartCutscene != "")
	{
		if (levelName == "demo")
		{
			if (playOpeningDemoCutscene)
				game->cutscene->PlayCutscene(game->levelStartCutscene);
			playOpeningDemoCutscene = false;
		}
		else
		{
			game->cutscene->PlayCutscene(game->levelStartCutscene);
		}
	}
}
