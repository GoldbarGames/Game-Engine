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

	//TODO: Make the indexes different numbers for the names and sprite sheets?
	previewMap["npc"] = game->CreateNPC(npcNames[spriteMapIndex], Vector2(0, 0), spriteMapIndex);

	game->entities.clear();

	currentEditModeLayer->SetText("Drawing on layer: " + GetDrawingLayerName(drawingLayer));
}

Editor::~Editor()
{

}

void Editor::StartEdit()
{
	//toolbox = SDL_CreateWindow("Toolbox", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 100, 100, SDL_WINDOW_OPENGL);
	//rendererToolbox = SDL_CreateRenderer(toolbox, -1, SDL_RENDERER_ACCELERATED);

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

	// Create all the buttons for the editor
	for (unsigned int i = 0; i < buttons.size(); i++)
		delete buttons[i];
	buttons.clear();

	int buttonX = 0;
	const int buttonWidth = 50;
	const int buttonHeight = 50;
	const int buttonSpacing = 20;

	std::vector<string> buttonNames = { "NewLevel", "Load", "Save", "Tileset", "Inspect", "Layer", "Door", "Ladder", "NPC", "Goal", "Bug", "Ether" };

	for (unsigned int i = 0; i < buttonNames.size(); i++)
	{
		buttons.emplace_back(new EditorButton("", buttonNames[i], Vector2(buttonX, screenHeight-buttonHeight), *game));
		buttonX += buttonWidth + buttonSpacing; // TODO: is there a way to not make this hard-coded? is it worth it?
	}

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
		layerButtons.emplace_back(new EditorButton(layerButtonNames[i], "Layer", Vector2(0, buttonY), *game, Vector2(layerButtonWidth, 0)));
		layerVisibleButtons.emplace_back(new EditorButton("", "Visible", Vector2(layerButtonWidth, buttonY), *game, Vector2(50, 0)));
		buttonY += layerButtonHeight + buttonSpacing; // TODO: is there a way to not make this hard-coded? is it worth it?
	}
}

void Editor::StopEdit()
{

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

	if (objectMode == "tile" && clickedToolboxWindow)
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
		}
	}
	else // we clicked somewhere in the game world, so place a tile/object
	{
		clickedPosition += game->camera;

		if (inspectionMode)
		{
			InspectObject(mouseX, mouseY);
		}
		else
		{
			// if we are placing a tile...
			if (objectMode == "tile")
			{
				PlaceTile(clickedPosition, mouseX, mouseY);
			}
			else // when placing an object
			{
				PlaceObject(clickedPosition, mouseX, mouseY);
			}
		}

		
	}
}


void Editor::InspectObject(int mouseX, int mouseY)
{
	SDL_Point point;
	point.x = mouseX;
	point.y = mouseY;

	if (SDL_PointInRect(&point, &objectPropertiesRect))
	{
		for (unsigned int i = 0; i < properties.size(); i++)
		{
			if (SDL_PointInRect(&point, &properties[i]->textWindowRect))
			{
				if (selectedEntity != nullptr)
				{
					propertyIndex = i;
					game->StartTextInput("properties");
					SetPropertyText();
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

void Editor::SetPropertyText()
{
	selectedEntity->SetProperty(properties[propertyIndex]->txt, game->inputText);
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
		properties[i]->SetPosition(propertyX, propertyY);
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
		if (objectMode == "npc")
		{
			currentNPC = game->SpawnNPC(npcNames[spriteMapIndex], Vector2(mouseX, mouseY), spriteMapIndex);
			if (currentNPC != nullptr)
			{
				game->SortEntities(game->entities);
			}
		}
		else if (objectMode == "goal")
		{
			Goal* currentGoal = game->SpawnGoal(Vector2(mouseX, mouseY), spriteMapIndex);
			if (currentGoal != nullptr)
				game->SortEntities(game->entities);
		}
		else if (objectMode == "bug")
		{
			Bug* currentBug = game->SpawnBug(Vector2(mouseX, mouseY), spriteMapIndex);
			if (currentBug != nullptr)
				game->SortEntities(game->entities);
		}
		else if (objectMode == "ether")
		{
			Ether* currentEther = game->SpawnEther(Vector2(mouseX, mouseY), spriteMapIndex);
			if (currentEther != nullptr)
				game->SortEntities(game->entities);
		}
		else if (objectMode == "door")
		{
			if (!placingDoor)
			{
				std::cout << "trying to spawn entrance" << std::endl;
				currentDoor = game->SpawnDoor(Vector2(mouseX, mouseY), spriteMapIndex);
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
				Door* destination = game->SpawnDoor(Vector2(mouseX, mouseY), spriteMapIndex);
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
				currentLadder = game->SpawnLadder(Vector2(mouseX, mouseY), spriteMapIndex);
				if (currentLadder != nullptr)
				{
					std::cout << "placing ladder set true" << std::endl;
					placingLadder = true;
					game->SortEntities(game->entities);
				}
			}
			else
			{
				Vector2 snappedPosition = game->SnapToGrid(Vector2(mouseX, mouseY));

				// only spawn if the position we clicked at is on the same column as the ladder start
				if (snappedPosition.x == currentLadder->GetPosition().x)
				{
					std::cout << "trying to spawn ladder end" << std::endl;
					Ladder* ladderEnd = game->SpawnLadder(Vector2(mouseX, mouseY), spriteMapIndex);
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

							int snappedY = game->SnapToGrid(Vector2(mouseY, mouseY)).y;

							while (snappedY < currentLadder->GetPosition().y)
							{
								game->SpawnLadder(Vector2(mouseX, mouseY), spriteMapIndex);
								mouseY += GRID_SIZE * Renderer::GetScale();
								snappedY = game->SnapToGrid(Vector2(mouseY, mouseY)).y;
							}
						}
						else
						{
							// Connect the two edges by spawning the middle parts
							mouseY -= GRID_SIZE * Renderer::GetScale();

							int snappedY = game->SnapToGrid(Vector2(mouseY, mouseY)).y;

							while (snappedY > currentLadder->GetPosition().y)
							{
								game->SpawnLadder(Vector2(mouseX, mouseY), spriteMapIndex);
								mouseY -= GRID_SIZE * Renderer::GetScale();
								snappedY = game->SnapToGrid(Vector2(mouseY, mouseY)).y;
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
		if (game->entities[i]->GetPosition().RoundToInt() == clickedPosition.RoundToInt() &&
			game->entities[i]->layer == drawingLayer &&
			game->entities[i]->etype == "tile")
		{
			canPlaceTileHere = false;
			break;
		}
	}

	if (canPlaceTileHere)
	{
		int mod = (GRID_SIZE * Renderer::GetScale());

		int afterModX = ((int)(mouseX) % mod);
		int afterModY = ((int)(mouseY) % mod);

		game->SpawnTile(Vector2(editorTileX, editorTileY), "assets/tiles/" + tilesheets[tilesheetIndex] + ".png",
			Vector2(mouseX - afterModX, mouseY - afterModY), drawingLayer);
		game->SortEntities(game->entities);
	}
}

void Editor::RightClick(Vector2 clickedPosition)
{
	clickedPosition.x += (int)game->camera.x;
	clickedPosition.y += (int)game->camera.y;

	int ladderIndex = -1;

	for (int i = game->entities.size() - 1; i >= 0; i--)
	{
		Vector2 entityPosition = game->entities[i]->GetPosition().RoundToInt();
		Vector2 clickedInt = clickedPosition.RoundToInt();

		//

		//TODO: Is there a better way than this?
		if (game->entities[i]->etype == objectMode &&
			game->entities[i]->layer == drawingLayer &&
			entityPosition.x >= clickedInt.x - 1 && 
			entityPosition.x <= clickedInt.x + 1 &&
			entityPosition.y >= clickedInt.y - 1 && 
			entityPosition.y <= clickedInt.y + 1 )
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
		std::string startingState = game->entities[ladderIndex]->GetAnimator()->currentState;
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
			lastPosition.y += GRID_SIZE * Renderer::GetScale();
		else if (startingState == "bottom")
			lastPosition.y -= GRID_SIZE * Renderer::GetScale();

		exit = true;

		for (unsigned int i = 0; i < game->entities.size(); i++)
		{
			if (game->entities[i]->GetPosition() == lastPosition &&
				game->entities[i]->etype == "ladder")
			{
				game->ShouldDeleteEntity(i);
				exit = false;
				break;
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

	previousMouseState = mouseState;
}

void Editor::ClickedButton(string buttonName)
{
	//objectMode = buttonName; //TODO: Make a better way to do this
	if (buttonName == "Tileset")
	{
		ToggleTileset();
	}
	else if (buttonName == "Layer")
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
}

//TODO: Refactor this


void Editor::ToggleSpriteMap()
{
	spriteMapIndex++; //TODO: Make this better, how to get length of each map?
		
	// Update the preview sprites accordingly
	if (objectMode == "door")
	{
		if (spriteMapIndex > 2)
			spriteMapIndex = 0;

		delete previewMap["door"];
		previewMap["door"] = game->CreateDoor(Vector2(0, 0), spriteMapIndex);		
	}
	else if (objectMode == "ladder")
	{
		if (spriteMapIndex > 2)
			spriteMapIndex = 0;

		delete previewMap["ladder"];
		previewMap["ladder"] = game->CreateLadder(Vector2(0, 0), spriteMapIndex);
	}
	else if (objectMode == "npc")
	{
		if (spriteMapIndex > 1)
			spriteMapIndex = 0;

		delete previewMap["npc"];
		previewMap["npc"] = game->CreateNPC(npcNames[spriteMapIndex], Vector2(0, 0), spriteMapIndex);
	}
	else if (objectMode == "goal")
	{
		if (spriteMapIndex > 2)
			spriteMapIndex = 0;

		delete previewMap["goal"];
		previewMap["goal"] = game->CreateGoal(Vector2(0, 0), spriteMapIndex);
	}
	else if (objectMode == "bug")
	{
		if (spriteMapIndex > 1)
			spriteMapIndex = 0;

		delete previewMap["bug"];
		previewMap["bug"] = game->CreateBug(Vector2(0, 0), spriteMapIndex);
	}
	else if (objectMode == "ether")
	{
		if (spriteMapIndex > 0)
			spriteMapIndex = 0;

		delete previewMap["ether"];
		previewMap["ether"] = game->CreateEther(Vector2(0, 0), spriteMapIndex);
	}

	objectPreview = previewMap[objectMode];
}


void Editor::ClickedLayerButton(string buttonText)
{
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
	objectMode = "tile";

	// If we already have an entity selected, deselect it
	if (selectedEntity != nullptr)
	{
		selectedEntity->DeleteProperties(properties);
		selectedEntity = nullptr;
	}

	inspectionMode = !inspectionMode;
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

	if (inspectionMode)
	{
		if (selectedEntity != nullptr)
		{
			// Draw a yellow rectangle around the currently selected object
			SDL_SetRenderDrawColor(renderer->renderer, 255, 255, 0, 255);
			SDL_RenderDrawRect(renderer->renderer, selectedEntity->GetBounds());
		}

		// Draw the box that goes underneath all the properties
		SDL_SetRenderDrawColor(renderer->renderer, 128, 128, 128, 255);
		SDL_RenderFillRect(renderer->renderer, &objectPropertiesRect);
		
		// Draw the text for all the properties
		SDL_SetRenderDrawColor(renderer->renderer, 255, 255, 255, 255);
		for (unsigned int i = 0; i < properties.size(); i++)
		{
			properties[i]->Render(renderer);
		}
		SDL_SetRenderDrawColor(renderer->renderer, 0, 0, 0, 255);

	}
	else
	{
		if (objectMode == "tile")
		{
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

void Editor::SaveLevel(std::string levelName)
{
	if (levelName != "")
		game->currentLevel = levelName;

	std::ofstream fout;
	fout.open("data/" + game->currentLevel + ".lvl");

	int SCALE = Renderer::GetScale();

	for (unsigned int i = 0; i < game->entities.size(); i++)
	{
		if (game->entities[i]->etype == "tile")
		{
			float x = game->entities[i]->GetPosition().x / SCALE;
			float y = game->entities[i]->GetPosition().y / SCALE;
			fout << game->entities[i]->etype << " " << (x) <<
				" " << (y) << " " << game->entities[i]->drawOrder <<
				" " << game->entities[i]->layer << " " << game->entities[i]->impassable << 
				" " << game->entities[i]->tilesheetIndex << " " << game->entities[i]->tileCoordinates.x << 
				" " << game->entities[i]->tileCoordinates.y << "" << std::endl;
		}
		else if (game->entities[i]->etype == "door")
		{
			Door* door = static_cast<Door*>(game->entities[i]);
			Vector2 pos = door->GetPosition();//game->CalcObjPos();

			fout << door->etype << " " << (pos.x / SCALE) << " " <<
				(pos.y / SCALE) << " " << (door->GetDestination().x / SCALE) <<
				" " << (door->GetDestination().y / SCALE) << " " << door->spriteIndex << "" << std::endl;
		}
		else if (game->entities[i]->etype == "ladder")
		{
			Ladder* ladder = static_cast<Ladder*>(game->entities[i]);
			Vector2 pos = ladder->GetPosition();//game->CalcObjPos();

			fout << ladder->etype << " "  << (pos.x / SCALE) << " " <<
				(pos.y / SCALE) << " " << ladder->GetAnimator()->currentState
				<< " " << ladder->spriteIndex << "" << std::endl;
		}
		else if (game->entities[i]->etype == "npc")
		{
			NPC* npc = static_cast<NPC*>(game->entities[i]);
			Vector2 pos = npc->GetPosition();

			std::string npcLabel = npc->cutsceneLabel;
			if (npcLabel == "")
				npcLabel = "null";

			fout << npc->etype << " " << (pos.x / SCALE) <<
				" " << (pos.y / SCALE) << " " << npc->name << " " << npc->cutsceneLabel <<
				" " << npc->spriteIndex << " " << npc->drawOrder <<
				" " << npc->layer << " " << npc->impassable << std::endl;
		}
		else if (game->entities[i]->etype == "player")
		{
			Player* player = static_cast<Player*>(game->entities[i]);

			fout << player->etype << " " << (player->startPosition.x) <<
				" " << (player->startPosition.y) << " " << player->drawOrder <<
				" " << player->layer << " " << player->impassable << std::endl;
		}
		else if (game->entities[i]->etype == "goal")
		{
			Goal* goal = static_cast<Goal*>(game->entities[i]);

			fout << goal->etype << " " << (goal->GetPosition().x / SCALE) <<
				" " << (goal->GetPosition().y / SCALE) << " " << goal->spriteIndex << std::endl;
		}
		else if (game->entities[i]->etype == "bug")
		{
			Bug* bug = static_cast<Bug*>(game->entities[i]);

			fout << bug->etype << " " << (bug->GetPosition().x / SCALE) <<
				" " << (bug->GetPosition().y / SCALE) << " " << bug->spriteIndex << std::endl;
		}
		else if (game->entities[i]->etype == "ether")
		{
			Ether* ether = static_cast<Ether*>(game->entities[i]);

			fout << ether->etype << " " << (ether->GetPosition().x / SCALE) <<
				" " << (ether->GetPosition().y / SCALE) << " " << ether->spriteIndex << std::endl;
		}
		else
		{
			// Don't save anything else, because they are probably temp objects like missiles, etc.

			//fout << game->entities[i]->etype << " " << (game->entities[i]->GetPosition().x / SCALE) <<
			//	" " << (game->entities[i]->GetPosition().y / SCALE) << " " << game->entities[i]->drawOrder <<
			//	" " << game->entities[i]->layer << " " << game->entities[i]->impassable << std::endl;
		}		
	}

	fout.close();
}


void Editor::LoadLevel(std::string levelName)
{
	// Clear the old level
	for (unsigned int i = 0; i < game->entities.size(); i++)
		delete game->entities[i];		
	game->entities.clear();	

	if (levelName != "")
		game->currentLevel = levelName;

	game->currentEther = game->startingEther;

	std::ifstream fin;
	fin.open("data/" + levelName + ".lvl");

	char line[256];

	fin.getline(line, 256);

	int SCALE = Renderer::GetScale();

	while (fin.good())
	{
		std::istringstream buf(line);
		std::istream_iterator<std::string> beg(buf), end;
		std::vector<std::string> tokens(beg, end);

		const std::string etype = tokens[0];

		int positionX = std::stoi(tokens[1]);
		int positionY = std::stoi(tokens[2]);

		if (etype == "tile")
		{
			int layer = std::stoi(tokens[4]);
			int impassable = std::stoi(tokens[5]);
			int tilesheet = std::stoi(tokens[6]);
			int frameX = std::stoi(tokens[7]);
			int frameY = std::stoi(tokens[8]);
			
			Tile* newTile = game->SpawnTile(Vector2(frameX, frameY), "assets/tiles/" + tilesheets[tilesheet] + ".png",
				Vector2(positionX * SCALE, positionY * SCALE), (DrawingLayer)layer);

			newTile->tilesheetIndex = tilesheet;
		}
		else if (etype == "door")
		{
			int spriteIndex = std::stoi(tokens[5]);
			Door* newDoor = game->SpawnDoor(Vector2(positionX * SCALE, positionY * SCALE), spriteIndex);
			int destX = std::stoi(tokens[3]);
			int destY = std::stoi(tokens[4]);
			newDoor->SetDestination(Vector2(destX * SCALE, destY * SCALE));
		}
		else if (etype == "ladder")
		{
			int spriteIndex = std::stoi(tokens[4]);
			Ladder* newLadder = game->SpawnLadder(Vector2(positionX * SCALE, positionY * SCALE), spriteIndex);
			newLadder->GetAnimator()->SetState(tokens[3]);
		}
		else if (etype == "player")
		{			
			game->player = game->SpawnPlayer(Vector2(positionX, positionY));
		}
		else if (etype == "npc")
		{
			std::string npcName = tokens[3];
			std::string npcCutscene = tokens[4];
			int spriteIndex = std::stoi(tokens[5]);
			NPC* newNPC = game->SpawnNPC(npcName, Vector2(positionX, positionY), spriteIndex);
			newNPC->cutsceneLabel = npcCutscene;

			newNPC->drawOrder = std::stoi(tokens[6]);
			newNPC->layer = (DrawingLayer)std::stoi(tokens[7]);
			newNPC->impassable = std::stoi(tokens[8]);
		}
		else if (etype == "goal")
		{			
			int spriteIndex = std::stoi(tokens[3]);
			Goal* entity = game->SpawnGoal(Vector2(positionX * SCALE, positionY * SCALE), spriteIndex);
		}
		else if (etype == "bug")
		{
			int spriteIndex = std::stoi(tokens[3]);
			Bug* entity = game->SpawnBug(Vector2(positionX * SCALE, positionY * SCALE), spriteIndex);
		}
		else if (etype == "ether")
		{
			int spriteIndex = std::stoi(tokens[3]);
			Ether* entity = game->SpawnEther(Vector2(positionX * SCALE, positionY * SCALE), spriteIndex);
		}

		fin.getline(line, 256);
	}

	fin.close();

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
	for (unsigned int i = 0; i < NUM_BGS; i++)
	{
		game->SpawnBackground(Vector2( (BG_WIDTH * Renderer::GetScale() * i) - BG_OFFSET, 0));
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
