#include "Editor.h"
#include "Game.h"
#include "globals.h"
#include <fstream>
#include <sstream>
#include <iterator>

using std::string;

Editor::Editor()
{
	
}

Editor::~Editor()
{

}

void Editor::StartEdit(SDL_Renderer* renderer, SDL_Surface* tilesheet)
{
	//toolbox = SDL_CreateWindow("Toolbox", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 100, 100, SDL_WINDOW_OPENGL);
	//rendererToolbox = SDL_CreateRenderer(toolbox, -1, SDL_RENDERER_ACCELERATED);

	// TILE SHEET FOR TOOLBOX

	toolboxTexture = SDL_CreateTextureFromSurface(renderer, tilesheet);
	toolboxTextureRect.x = 0;
	toolboxTextureRect.y = 0;

	SDL_QueryTexture(toolboxTexture, NULL, NULL, &toolboxTextureRect.w, &toolboxTextureRect.h);

	toolboxTextureRect.w *= 1;
	toolboxTextureRect.h *= 1;

	toolboxWindowRect.x = screenWidth - toolboxTextureRect.w;
	toolboxWindowRect.y = 0;
	toolboxWindowRect.w = toolboxTextureRect.w;
	toolboxWindowRect.h = toolboxTextureRect.h;

	SDL_SetWindowSize(toolbox, toolboxWindowRect.w, toolboxWindowRect.h);

	selectedRect.x = toolboxWindowRect.x;
	selectedRect.y = 0;
	selectedRect.w = 24;
	selectedRect.h = 24;
}

void Editor::StopEdit()
{
	SDL_DestroyTexture(toolboxTexture);
	//SDL_DestroyRenderer(rendererToolbox);
	SDL_DestroyWindow(toolbox);
	toolbox = nullptr;
}

void Editor::HandleEdit(Game& game)
{
	int mouseX = 0;
	int mouseY = 0;

	const int mouseState = SDL_GetMouseState(&mouseX, &mouseY);

	int clickedX = mouseX - ((int)mouseX % (TILE_SIZE * SCALE));
	int clickedY = mouseY - ((int)mouseY % (TILE_SIZE * SCALE));

	Vector2 clickedPosition(clickedX, clickedY);

	if (mouseState & SDL_BUTTON(SDL_BUTTON_LEFT))
	{
		//int gameWindowFlags = SDL_GetWindowFlags(game.window);
		//bool clickedGameWindow = //gameWindowFlags & SDL_WINDOW_MOUSE_FOCUS;

		//int toolboxWindowFlags = SDL_GetWindowFlags(toolbox);
		//bool clickedToolboxWindow = toolboxWindowFlags & SDL_WINDOW_MOUSE_FOCUS;

		bool clickedToolboxWindow = mouseX >= toolboxWindowRect.x && mouseY <= toolboxWindowRect.h;

		if (clickedToolboxWindow)
		{
			int xOffset = (mouseX - toolboxWindowRect.x);
			selectedRect.x = (xOffset - (xOffset % (TILE_SIZE * 1)));
			selectedRect.y = (mouseY - (mouseY % (TILE_SIZE * 1)));

			editorTileX = (selectedRect.x / (TILE_SIZE * 1)) + 1;
			editorTileY = (selectedRect.y / (TILE_SIZE * 1)) + 1;

			selectedRect.x += toolboxWindowRect.x;
		}
		else // if (clickedToolboxWindow) //TODO: highlight with rectangle
		{
			bool canPlaceTileHere = true;
			for (int i = 0; i < game.entities.size(); i++)
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
				game.SortEntities();
			}
		}
	}
	else if (mouseState & SDL_BUTTON(SDL_BUTTON_RIGHT)) // deletes tiles in order, nearest first
	{
		for (int i = game.entities.size() - 1; i >= 0; i--)
		{
			if (game.entities[i]->GetPosition() == clickedPosition &&
				game.entities[i]->layer == drawingLayer)
			{
				delete game.entities[i];
				game.entities.erase(game.entities.begin() + i);
				break;
			}
		}
	}
}

void Editor::Render(SDL_Renderer* renderer)
{
	//SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, toolboxTexture, &toolboxTextureRect, &toolboxWindowRect);
	//SDL_RenderPresent(renderer);

	// Draw a yellow rectangle around the currently selected tile
	SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
	SDL_RenderDrawRect(renderer, &selectedRect);
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);	

	SDL_RenderCopy(renderer, textTexture, &textTextureRect, &textWindowRect);
}

void Editor::SetText(string newText, SDL_Renderer* renderer)
{
	//TODO: Check what to do here to avoid memory leaks
	//if (textSurface != nullptr)
	//	delete textSurface;
	//if (textTexture != nullptr)
	//	delete textTexture;

	//TODO: Clean up this code, put it in a better location
	theFont = TTF_OpenFont("assets/fonts/default.ttf", 20);
	textSurface = TTF_RenderText_Solid(theFont, newText.c_str(), { 255, 255, 255, 255 });
	textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
	textTextureRect.x = 0;
	textTextureRect.y = 0;
	SDL_QueryTexture(textTexture, NULL, NULL, &textTextureRect.w, &textTextureRect.h);
	textWindowRect.w = textTextureRect.w;
	textWindowRect.h = textTextureRect.h;
}

void Editor::SaveLevel(Game& game)
{
	std::ofstream fout;
	fout.open("data/level.wdk");

	for (int i = 0; i < game.entities.size(); i++)
	{
		if (game.entities[i]->etype == "tile")
		{
			fout << game.entities[i]->id << " " << game.entities[i]->etype << " " << game.entities[i]->GetPosition().x <<
				" " << game.entities[i]->GetPosition().y << " " << game.entities[i]->drawOrder <<
				" " << game.entities[i]->layer << " " << game.entities[i]->impassable << 
				" " << game.entities[i]->tilesheetIndex << " " << game.entities[i]->tileCoordinates.x << 
				" " << game.entities[i]->tileCoordinates.y << "" << std::endl;
		}
		else
		{
			fout << game.entities[i]->id << " " << game.entities[i]->etype << " " << game.entities[i]->GetPosition().x <<
				" " << game.entities[i]->GetPosition().y << " " << game.entities[i]->drawOrder <<
				" " << game.entities[i]->layer << " " << game.entities[i]->impassable << std::endl;
		}
		
	}

	fout.close();
}

void Editor::LoadLevel(Game& game, std::string levelName)
{
	// Clear the old level
	for (int i = 0; i < game.entities.size(); i++)
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
			
			game.SpawnTile(Vector2(frameX, frameY), "assets/tiles/" + tilesheets[tilesheet] + ".png",
				Vector2(positionX, positionY), impassable, (DrawingLayer)layer);
		}
		else if (tokens[1] == "player")
		{			
			game.player = game.SpawnPlayer(Vector2(positionX, positionY));
		}

		fin.getline(line, 256);
	}

	fin.close();
}
