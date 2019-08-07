#include "Editor.h"
#include "Game.h"
#include "globals.h"
#include <fstream>
#include <sstream>
#include <iterator>
#include "Tile.h"

using std::string;

Editor::Editor(SDL_Renderer* renderer)
{
	theFont = TTF_OpenFont("assets/fonts/default.ttf", 20);

	currentEditModeLayer = new Text(renderer, theFont);
	cursorPosition = new Text(renderer, theFont);
	cursorPosition->SetPosition(0, 50);
}

Editor::~Editor()
{

}

void Editor::StartEdit(Game &game)
{
	//toolbox = SDL_CreateWindow("Toolbox", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 100, 100, SDL_WINDOW_OPENGL);
	//rendererToolbox = SDL_CreateRenderer(toolbox, -1, SDL_RENDERER_ACCELERATED);

	// TILE SHEET FOR TOOLBOX

	toolboxTexture = SDL_CreateTextureFromSurface(game.renderer, game.spriteManager.GetImage("assets/tiles/" + tilesheets[tilesheetIndex] + ".png"));
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
	for (int i = 0; i < buttons.size(); i++)
		delete buttons[i];
	buttons.clear();

	int buttonX = 0;
	const int buttonWidth = 50;
	const int buttonHeight = 50;
	const int buttonSpacing = 20;

	std::vector<string> buttonNames = { "tileset", "layer" };

	for (int i = 0; i < buttonNames.size(); i++)
	{
		buttons.emplace_back(new EditorButton("assets/gui/menu.png", buttonNames[i],
			Vector2(buttonX, screenHeight-buttonHeight), game));
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

void Editor::HandleEdit(Game& game)
{
	int mouseX = 0;
	int mouseY = 0;

	const Uint32 mouseState = SDL_GetMouseState(&mouseX, &mouseY);

	int clickedX = mouseX - ((int)mouseX % (TILE_SIZE * SCALE));
	int clickedY = mouseY - ((int)mouseY % (TILE_SIZE * SCALE));

	Vector2 clickedPosition(clickedX, clickedY);

	hoveredTileRect.x = clickedX;
	hoveredTileRect.y = clickedY;

	std::string clickedPos = std::to_string(clickedX + (int)game.camera.x) + " " + std::to_string(clickedY + (int)game.camera.y);

	cursorPosition->SetText("Position: " + clickedPos);

	if (mouseState & SDL_BUTTON(SDL_BUTTON_LEFT))
	{
		//int gameWindowFlags = SDL_GetWindowFlags(game.window);
		//bool clickedGameWindow = //gameWindowFlags & SDL_WINDOW_MOUSE_FOCUS;

		//int toolboxWindowFlags = SDL_GetWindowFlags(toolbox);
		//bool clickedToolboxWindow = toolboxWindowFlags & SDL_WINDOW_MOUSE_FOCUS;

		bool clickedToolboxWindow = mouseX >= toolboxWindowRect.x && mouseY <= toolboxWindowRect.h;

		// Get name of the button that was clicked, if any
		string clickedButton = "";
		for (int i = 0; i < buttons.size(); i++)
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
		else if ( !(previousMouseState & SDL_BUTTON(SDL_BUTTON_LEFT)) && clickedButton != "")
		{
			ClickedButton(clickedButton, game);
		}
		else // we clicked somewhere in the game world, so place a tile/object
		{
			clickedPosition += game.camera;

			bool canPlaceTileHere = true;
			for (unsigned int i = 0; i < game.entities.size(); i++)
			{
				if (game.entities[i]->GetPosition() == clickedPosition && 
					game.entities[i]->layer == drawingLayer)
				{
					canPlaceTileHere = false;
					break;
				}
			}

			if (canPlaceTileHere)
			{
				bool impassable = (drawingLayer == FOREGROUND);
				game.SpawnTile(Vector2(editorTileX, editorTileY), "assets/tiles/" + tilesheets[tilesheetIndex] + ".png", 
					Vector2(mouseX, mouseY), impassable, drawingLayer);
				game.SortEntities(game.entities);
			}
		}
	}
	else if (mouseState & SDL_BUTTON(SDL_BUTTON_RIGHT)) // deletes tiles in order, nearest first
	{
		clickedPosition += game.camera;
		//clickedPosition.RoundToInt(); // without this line, would not delete tiles from previous edit

		for (int i = game.entities.size() - 1; i >= 0; i--)
		{
			if (game.entities[i]->GetPosition() == clickedPosition &&
				game.entities[i]->layer == drawingLayer)
			{
				game.DeleteEntity(i);
				break;
			}
		}
	}

	previousMouseState = mouseState;
}

void Editor::ClickedButton(string buttonName, Game& game)
{
	if (buttonName == "tileset")
	{
		ToggleTileset(game);
	}
	else if (buttonName == "layer")
	{
		ToggleLayer();
	}
}

void Editor::ToggleLayer()
{
	if (drawingLayer == BACKGROUND)
		drawingLayer = FOREGROUND;
	else if (drawingLayer == FOREGROUND)
		drawingLayer = BACKGROUND;

	currentEditModeLayer->SetText("Drawing on layer: " + DrawingLayerNames[drawingLayer]);
}

void Editor::ToggleTileset(Game& game)
{
	tilesheetIndex++;
	if (tilesheetIndex > 1)
		tilesheetIndex = 0;
	StartEdit(game);
}

void Editor::Render(SDL_Renderer* renderer)
{
	// Draw a white rectangle around the currently highlighted grid tile
	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
	SDL_RenderDrawRect(renderer, &hoveredTileRect);
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
	for (int i = 0; i < buttons.size(); i++)
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

void Editor::SaveLevel(Game& game)
{
	std::ofstream fout;
	fout.open("data/level.wdk");

	for (unsigned int i = 0; i < game.entities.size(); i++)
	{
		if (game.entities[i]->etype == "tile")
		{
			fout << game.entities[i]->id << " " << game.entities[i]->etype << " " << (game.entities[i]->GetPosition().x / SCALE) <<
				" " << (game.entities[i]->GetPosition().y / SCALE) << " " << game.entities[i]->drawOrder <<
				" " << game.entities[i]->layer << " " << game.entities[i]->impassable << 
				" " << game.entities[i]->tilesheetIndex << " " << game.entities[i]->tileCoordinates.x << 
				" " << game.entities[i]->tileCoordinates.y << "" << std::endl;
		}
		else
		{
			fout << game.entities[i]->id << " " << game.entities[i]->etype << " " << (game.entities[i]->GetPosition().x / SCALE) <<
				" " << (game.entities[i]->GetPosition().y / SCALE) << " " << game.entities[i]->drawOrder <<
				" " << game.entities[i]->layer << " " << game.entities[i]->impassable << std::endl;
		}		
	}

	fout.close();
}

void Editor::LoadLevel(Game& game, std::string levelName)
{
	// Clear the old level
	for (unsigned int i = 0; i < game.entities.size(); i++)
		delete game.entities[i];		
	game.entities.clear();

	std::ifstream fin;
	fin.open("data/" + levelName + ".wdk");

	char line[256];

	fin.getline(line, 256);

	while (fin.good())
	{
		std::istringstream buf(line);
		std::istream_iterator<std::string> beg(buf), end;
		std::vector<std::string> tokens(beg, end);

		int positionX = std::stoi(tokens[2]);
		int positionY = std::stoi(tokens[3]);

		if (tokens[1] == "tile")
		{
			int layer = std::stoi(tokens[5]);
			int impassable = std::stoi(tokens[6]);
			int tilesheet = std::stoi(tokens[7]);
			int frameX = std::stoi(tokens[8]);
			int frameY = std::stoi(tokens[9]);			
			
			Tile* newTile = game.SpawnTile(Vector2(frameX, frameY), "assets/tiles/" + tilesheets[tilesheet] + ".png",
				Vector2(positionX * SCALE, positionY * SCALE), impassable, (DrawingLayer)layer);

			newTile->tilesheetIndex = tilesheet;
		}
		else if (tokens[1] == "player")
		{			
			game.player = game.SpawnPlayer(Vector2(positionX, positionY));
		}

		fin.getline(line, 256);
	}

	fin.close();
}
