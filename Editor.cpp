#include "Editor.h"
#include "Game.h"
#include "globals.h"


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

	if (SDL_GetMouseState(&mouseX, &mouseY) & SDL_BUTTON(SDL_BUTTON_LEFT))
	{
		//int gameWindowFlags = SDL_GetWindowFlags(game.window);
		//bool clickedGameWindow = //gameWindowFlags & SDL_WINDOW_MOUSE_FOCUS;

		//int toolboxWindowFlags = SDL_GetWindowFlags(toolbox);
		//bool clickedToolboxWindow = toolboxWindowFlags & SDL_WINDOW_MOUSE_FOCUS;

		bool clickedToolboxWindow = mouseX >= toolboxWindowRect.x && mouseY >= toolboxWindowRect.y;

		if (clickedToolboxWindow)
		{
			editorTileX = ((screenWidth - mouseX) - ((screenWidth - mouseX) % (TILE_SIZE * 1))) / (TILE_SIZE * 1) + 1;
			editorTileY = (mouseY - (mouseY % (TILE_SIZE * 1))) / (TILE_SIZE * 1) + 1;
		}
		else // if (clickedToolboxWindow) //TODO: highlight with rectangle
		{
			// mouse has been left-clicked at position X,Y
			game.SpawnTile(Vector2(editorTileX, editorTileY), "assets/tiles/housetiles5.png", Vector2(mouseX, mouseY), true);
			game.SortEntities();
		}
	}
}

void Editor::Render(SDL_Renderer* renderer)
{
	//SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, toolboxTexture, &toolboxTextureRect, &toolboxWindowRect);
	//SDL_RenderPresent(renderer);
}