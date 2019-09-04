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
	theFont = TTF_OpenFont("assets/fonts/default.ttf", 20);

	game = &g;

	currentEditModeLayer = new Text(game->renderer, theFont);
	cursorPosition = new Text(game->renderer, theFont);
	cursorPosition->SetPosition(0, 50);

	npcNames = { "gramps", "the_man" };

	previewMap["tile"] = nullptr;
	previewMap["door"] = game->CreateDoor(Vector2(0,0), spriteMapIndex);
	previewMap["ladder"] = game->CreateLadder(Vector2(0, 0), spriteMapIndex);

	//TODO: Make the indexes different numbers for the names and sprite sheets?
	previewMap["NPC"] = game->CreateNPC(npcNames[spriteMapIndex], Vector2(0, 0), spriteMapIndex);

	game->entities.clear();
}

Editor::~Editor()
{

}

void Editor::StartEdit()
{
	//toolbox = SDL_CreateWindow("Toolbox", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 100, 100, SDL_WINDOW_OPENGL);
	//rendererToolbox = SDL_CreateRenderer(toolbox, -1, SDL_RENDERER_ACCELERATED);

	// TILE SHEET FOR TOOLBOX

	toolboxTexture = game->spriteManager->GetImage("assets/tiles/" + tilesheets[tilesheetIndex] + ".png");
	toolboxTextureRect.x = 0;
	toolboxTextureRect.y = 0;

	SDL_QueryTexture(toolboxTexture, NULL, NULL, &toolboxTextureRect.w, &toolboxTextureRect.h);

	toolboxTextureRect.w *= 1;
	toolboxTextureRect.h *= 1;

	toolboxWindowRect.x = screenWidth - toolboxTextureRect.w;
	toolboxWindowRect.y = 0;
	toolboxWindowRect.w = toolboxTextureRect.w;
	toolboxWindowRect.h = toolboxTextureRect.h;

	//SDL_SetWindowSize(toolbox, toolboxWindowRect.w, toolboxWindowRect.h);

	selectedRect.x = toolboxWindowRect.x;
	selectedRect.y = 0;
	selectedRect.w = 24;
	selectedRect.h = 24;

	hoveredTileRect.w = 24 * SCALE;
	hoveredTileRect.h = 24 * SCALE;

	// Create all the buttons for the editor
	for (unsigned int i = 0; i < buttons.size(); i++)
		delete buttons[i];
	buttons.clear();

	int buttonX = 0;
	const int buttonWidth = 50;
	const int buttonHeight = 50;
	const int buttonSpacing = 20;

	std::vector<string> buttonNames = { "Tileset", "Layer", "Door", "Ladder", "NPC" };

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

		// if we are placing a tile...
		if (objectMode == "tile")
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
				int mod = (GRID_SIZE * SCALE);

				int afterModX = ((int)(mouseX) % mod);
				int afterModY = ((int)(mouseY) % mod);

				game->SpawnTile(Vector2(editorTileX, editorTileY), "assets/tiles/" + tilesheets[tilesheetIndex] + ".png",
					Vector2(mouseX - afterModX, mouseY - afterModY), drawingLayer);
				game->SortEntities(game->entities);
			}
		}
		else // when placing an object
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
				if (objectMode == "NPC")
				{
					currentNPC = game->SpawnNPC(npcNames[spriteMapIndex], Vector2(mouseX, mouseY), spriteMapIndex);
					if (currentNPC != nullptr)
					{
						game->SortEntities(game->entities);
					}
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
									mouseY += GRID_SIZE * SCALE;

									int snappedY = game->SnapToGrid(Vector2(mouseY, mouseY)).y;

									while (snappedY < currentLadder->GetPosition().y)
									{
										game->SpawnLadder(Vector2(mouseX, mouseY), spriteMapIndex);
										mouseY += GRID_SIZE * SCALE;
										snappedY = game->SnapToGrid(Vector2(mouseY, mouseY)).y;
									}
								}
								else
								{
									// Connect the two edges by spawning the middle parts
									mouseY -= GRID_SIZE * SCALE;

									int snappedY = game->SnapToGrid(Vector2(mouseY, mouseY)).y;

									while (snappedY > currentLadder->GetPosition().y)
									{
										game->SpawnLadder(Vector2(mouseX, mouseY), spriteMapIndex);
										mouseY -= GRID_SIZE * SCALE;
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
			lastPosition.y += GRID_SIZE * SCALE;
		else if (startingState == "bottom")
			lastPosition.y -= GRID_SIZE * SCALE;

		exit = true;

		for (int i = 0; i < game->entities.size(); i++)
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

	int clickedX = mouseX - ((int)mouseX % (GRID_SIZE * SCALE));
	int clickedY = mouseY - ((int)mouseY % (GRID_SIZE * SCALE));

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
		ToggleObjectMode("NPC");
	}
}

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
	else if (objectMode == "NPC")
	{
		if (spriteMapIndex > 1)
			spriteMapIndex = 0;

		delete previewMap["NPC"];
		previewMap["NPC"] = game->CreateNPC(npcNames[spriteMapIndex], Vector2(0, 0), spriteMapIndex);
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
		if (mode == "NPC")
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
			rect.x = x * GRID_SIZE * SCALE;
			rect.y = y * GRID_SIZE * SCALE;
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

	
	if (objectMode == "tile")
	{
		// Draw the tilesheet (only if we are placing a tile)
		SDL_RenderCopy(renderer->renderer, toolboxTexture, &toolboxTextureRect, &toolboxWindowRect);

		// Draw a yellow rectangle around the currently selected tileset tile
		SDL_SetRenderDrawColor(renderer->renderer, 255, 255, 0, 255);
		SDL_RenderDrawRect(renderer->renderer, &selectedRect);
		SDL_SetRenderDrawColor(renderer->renderer, 0, 0, 0, 255);
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

void Editor::SaveLevel()
{
	std::ofstream fout;
	fout.open("data/" + game->currentLevel + ".wdk");

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
		else
		{
			fout << game->entities[i]->etype << " " << (game->entities[i]->GetPosition().x / SCALE) <<
				" " << (game->entities[i]->GetPosition().y / SCALE) << " " << game->entities[i]->drawOrder <<
				" " << game->entities[i]->layer << " " << game->entities[i]->impassable << std::endl;
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

	std::ifstream fin;
	fin.open("data/" + levelName + ".wdk");

	char line[256];

	fin.getline(line, 256);

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

		fin.getline(line, 256);
	}

	fin.close();
}
