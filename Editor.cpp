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

	previewMap["tile"] = nullptr;
	previewMap["door"] = game->CreateDoor(Vector2(0,0));


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

	toolboxTexture = SDL_CreateTextureFromSurface(game->renderer, game->spriteManager.GetImage("assets/tiles/" + tilesheets[tilesheetIndex] + ".png"));
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

	std::vector<string> buttonNames = { "Tileset", "Layer", "Door", "Ladder" };

	for (unsigned int i = 0; i < buttonNames.size(); i++)
	{
		buttons.emplace_back(new EditorButton(buttonNames[i], Vector2(buttonX, screenHeight-buttonHeight), *game));
		buttonX += buttonWidth + buttonSpacing; // TODO: is there a way to not make this hard-coded? is it worth it?
	}
}

void Editor::StopEdit()
{
	SDL_DestroyTexture(toolboxTexture);
	//SDL_DestroyRenderer(rendererToolbox);
	//SDL_DestroyWindow(toolbox);
	//toolbox = nullptr;
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

	if (clickedToolboxWindow)
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
				bool impassable = (drawingLayer == FOREGROUND);
				game->SpawnTile(Vector2(editorTileX, editorTileY), "assets/tiles/" + tilesheets[tilesheetIndex] + ".png",
					Vector2(mouseX, mouseY), impassable, drawingLayer);
				game->SortEntities(game->entities);
			}
		}
		else // when placing an object
		{
			bool canPlaceObjectHere = true; //TODO: actually check for this

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
				if (objectMode == "door")
				{
					if (!placingDoor)
					{
						std::cout << "trying to spawn entrance" << std::endl;
						currentDoor = game->SpawnDoor(Vector2(mouseX, mouseY));
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
						Door* destination = game->SpawnDoor(Vector2(mouseX, mouseY));
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
			}


		}
	}
}

void Editor::RightClick(Vector2 clickedPosition)
{
	clickedPosition += game->camera;

	//TODO: Fix this
	//clickedPosition.RoundToInt(); // with this line, cannot delete tiles from previous edit

	for (int i = game->entities.size() - 1; i >= 0; i--)
	{
		if (game->entities[i]->etype == objectMode &&
			game->entities[i]->layer == drawingLayer &&
			game->entities[i]->GetPosition().RoundToInt() == clickedPosition.RoundToInt())
		{
			if (game->entities[i]->etype == "door")
			{
				// Save destination and delete the entry door
				Door* door = static_cast<Door*>(game->entities[i]);
				Vector2 dest = door->GetDestination();

				// Only delete if both doors have been placed
				if (dest != Vector2(0, 0))
				{
					game->DeleteEntity(i);

					// Delete the exit door
					for (unsigned int k = 0; k < game->entities.size(); k++)
					{
						if (game->entities[k]->GetPosition() == dest)
						{
							game->DeleteEntity(k);
							return;
						}
					}
				}				
			}
			else
			{
				game->DeleteEntity(i);
				return;
			}
		}
	}
}

void Editor::HandleEdit()
{
	int mouseX = 0;
	int mouseY = 0;

	const Uint32 mouseState = SDL_GetMouseState(&mouseX, &mouseY);

	int clickedX = mouseX - ((int)mouseX % (TILE_SIZE * SCALE));
	int clickedY = mouseY - ((int)mouseY % (TILE_SIZE * SCALE));

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
		ToggleLayer();
	}
	else if (buttonName == "Door")
	{
		if (objectMode == "door")
		{
			SetLayer(DrawingLayer::BACKGROUND);
			objectMode = "tile";
		}
		else
		{
			SetLayer(DrawingLayer::OBJECT);
			objectMode = "door";
		}
	}
	else if (buttonName == "Ladder")
	{

	}	
}

void Editor::ToggleLayer()
{
	int layer = (int)drawingLayer;
	layer++;
	if (layer >= DrawingLayerNames.size())
		layer = 0;
	
	SetLayer((DrawingLayer)layer);
}

void Editor::SetLayer(DrawingLayer layer)
{
	drawingLayer = layer;
	currentEditModeLayer->SetText("Drawing on layer: " + DrawingLayerNames[drawingLayer]);
}

void Editor::ToggleTileset()
{
	tilesheetIndex++;
	if (tilesheetIndex > 1)
		tilesheetIndex = 0;
	StartEdit();
}



void Editor::Render(SDL_Renderer* renderer)
{
	// Draw a white rectangle around the currently highlighted grid tile
	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
	SDL_RenderDrawRect(renderer, &hoveredTileRect);
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

	// Draw the object or tile that will be placed here, if any
	if (objectPreview != nullptr && objectPreview->GetSprite() != nullptr)
	{	
		if (GetModeDebug())
		{
			SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
			if (!objectPreview->CanSpawnHere(Vector2(hoveredTileRect.x, hoveredTileRect.y), *game, false))
				SDL_SetRenderDrawColor(renderer, 255, 0, 0, 128);
			else
				SDL_SetRenderDrawColor(renderer, 0, 255, 0, 128);

			SDL_RenderFillRect(renderer, objectPreview->GetSprite()->GetRect());
			SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
			SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		}

		objectPreview->GetSprite()->Render(Vector2(hoveredTileRect.x, hoveredTileRect.y), 0, -1, SDL_FLIP_NONE, renderer, 0);

		if (placingDoor && currentDoor != nullptr)
		{
			SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

			Vector2 doorCenter = currentDoor->GetCenter();
			Vector2 doorPos = currentDoor->GetPosition() + doorCenter - game->camera;
			SDL_RenderDrawLine(renderer, doorPos.x, doorPos.y, hoveredTileRect.x + doorCenter.x, hoveredTileRect.y + doorCenter.y);

			SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		}
	}

	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
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
				SDL_RenderDrawLine(renderer, doorPos.x, doorPos.y, destPos.x + doorCenter.x, destPos.y + doorCenter.y);
			}	
		}
	}
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

	// Draw the tilesheet
	SDL_RenderCopy(renderer, toolboxTexture, &toolboxTextureRect, &toolboxWindowRect);

	// Draw a yellow rectangle around the currently selected tileset tile
	SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
	SDL_RenderDrawRect(renderer, &selectedRect);
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

	// Draw text
	currentEditModeLayer->Render(renderer);
	cursorPosition->Render(renderer);

	// Draw all buttons
	for (unsigned int i = 0; i < buttons.size(); i++)
	{
		buttons[i]->Render(renderer);
	}
}

void Editor::SetText(string newText, SDL_Renderer* renderer)
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
	fout.open("data/level.wdk");

	for (unsigned int i = 0; i < game->entities.size(); i++)
	{
		if (game->entities[i]->etype == "tile")
		{
			fout << game->entities[i]->etype << " " << (game->entities[i]->GetPosition().x / SCALE) <<
				" " << (game->entities[i]->GetPosition().y / SCALE) << " " << game->entities[i]->drawOrder <<
				" " << game->entities[i]->layer << " " << game->entities[i]->impassable << 
				" " << game->entities[i]->tilesheetIndex << " " << game->entities[i]->tileCoordinates.x << 
				" " << game->entities[i]->tileCoordinates.y << "" << std::endl;
		}
		else if (game->entities[i]->etype == "door")
		{
			Door* door = static_cast<Door*>(game->entities[i]);
			fout << door->etype << " " << (door->GetPosition().x / SCALE) << " " <<
				(door->GetPosition().y / SCALE) << " " << (door->GetDestination().x / SCALE) <<
				" " << (door->GetDestination().y / SCALE) << "" << std::endl;
		}
		else
		{
			fout << " " << game->entities[i]->etype << " " << (game->entities[i]->GetPosition().x / SCALE) <<
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
				Vector2(positionX * SCALE, positionY * SCALE), impassable, (DrawingLayer)layer);

			newTile->tilesheetIndex = tilesheet;
		}
		else if (etype == "door")
		{
			Door* newDoor = game->SpawnDoor(Vector2(positionX * SCALE, positionY * SCALE));
			int destX = std::stoi(tokens[3]);
			int destY = std::stoi(tokens[4]);
			newDoor->SetDestination(Vector2(destX * SCALE, destY * SCALE));
		}
		else if (etype == "player")
		{			
			game->player = game->SpawnPlayer(Vector2(positionX, positionY));
		}

		fin.getline(line, 256);
	}

	fin.close();
}
