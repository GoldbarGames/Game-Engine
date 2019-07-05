#include "Game.h"
#include "Sprite.h"
#include "Player.h"
#include "debug_state.h"
#include "editor_state.h"
#include "Tile.h"
#include "globals.h"

using std::string;

Game::Game()
{
	InitSDL();
}

Game::~Game()
{
	EndSDL();
}

void Game::CalcDt()
{
	timePrev = timeNow;
	timeNow = SDL_GetPerformanceCounter();

	dt = (double)((timeNow - timePrev) * 1000 / (double)SDL_GetPerformanceFrequency());
}

void Game::InitSDL()
{
	SDL_Init(SDL_INIT_VIDEO);
	TTF_Init();

	window = SDL_CreateWindow("Witch Doctor Kaneko",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, screenWidth, screenHeight, SDL_WINDOW_OPENGL);

	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
}

void Game::EndSDL()
{
	// Delete our OpengL context
	SDL_GL_DeleteContext(mainContext);

	SDL_DestroyRenderer(renderer);	
	SDL_DestroyWindow(window);	
	window = nullptr;

	TTF_CloseFont(theFont);
	
	TTF_Quit();
	SDL_Quit();
	IMG_Quit();
}

bool Game::SetOpenGLAttributes()
{
	int success = 0;	

	// Set our OpenGL version.
	// SDL_GL_CONTEXT_CORE gives us only the newer version, deprecated functions are disabled
	success += SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

	// 3.2 is part of the modern versions of OpenGL, but most video cards whould be able to run it
	success +=  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	success +=  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);

	// Turn on double buffering with a 24bit Z buffer.
	// You may need to change this to 16 or 32 for your system
	success +=  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	return success == 0;
}


void Game::SpawnPerson(Vector2 position)
{
	//TODO: Make a sprite factory that can check to see if we have already loaded a sprite
	// and if so, we use that one, instead of creating a new one
	Sprite* sprite = new Sprite(5, spriteManager.GetImage("assets/sprites/wdk_blink.png"), renderer);

	//TODO: Make this a Physics Entity, not just an Entity
	Entity* person = new Entity();
	Animator* anim = new Animator("xyz");
	anim->MapStateToSprite("xyz", sprite);
	anim->speed = 50;
	person->SetAnimator(anim);
	person->SetPosition(position);
	person->SetSprite(sprite);
	person->impassable = true;

	//TODO: Also make sure to sort the sprites for drawing in the right order
	entities.emplace_back(person);
}

void Game::SpawnTile(Vector2 frame, string tilesheet, Vector2 position, bool impassable, DrawingLayer drawingLayer)
{
	Tile* tile = new Tile(frame, spriteManager.GetImage(tilesheet), renderer);
	int newTileX = position.x - ((int)position.x % (TILE_SIZE * SCALE));
	int newTileY = position.y - ((int)position.y % (TILE_SIZE * SCALE));
	tile->SetPosition(Vector2(newTileX, newTileY));
	tile->layer = drawingLayer;
	tile->impassable = impassable;
	entities.emplace_back(tile);
}

Player* Game::SpawnPlayer(Vector2 position)
{
	Player* player = new Player();
	Animator* anim1 = new Animator("blink");

	anim1->MapStateToSprite("walk", new Sprite(6, spriteManager.GetImage("assets/sprites/wdk_walk.png"), renderer));
	anim1->MapStateToSprite("blink", new Sprite(5, spriteManager.GetImage("assets/sprites/wdk_blink.png"), renderer));

	player->SetAnimator(anim1);
	player->SetPosition(position);
	player->startPosition = position;
	player->drawOrder = 99;

	entities.emplace_back(player);

	return player;
}

void Game::SetText(string newText)
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

void Game::Play(string gameName)
{
	SDL_SetWindowIcon(window, spriteManager.GetImage("assets/gui/icon.png"));

	//TODO: Implement scrolling camera
	SDL_Rect camera = { 0, 0, screenWidth, screenHeight };



	entities.reserve(5);

	Player* player = SpawnPlayer(Vector2(220, 0));

	Sprite* background = new Sprite(1, spriteManager.GetImage("assets/bg/bg.png"), renderer);
	Entity* bg = new Entity();
	bg->SetSprite(background);

	Sprite* floorSprite = new Sprite(1, spriteManager.GetImage("assets/sprites/floor.png"), renderer);
	Entity* floor = new Entity();
	floor->SetSprite(floorSprite);
	floor->SetPosition(Vector2(0, 300));
	floor->impassable = true;

	//SpawnPerson(Vector2(400, 0));
	//SpawnPerson(Vector2(0, 0));

	entities.emplace_back(floor);

	//entities.emplace_back(bg);

	//SpawnTile(Vector2(5, 3), "assets/tiles/housetiles5.png", Vector2(100, 180), false);
	//SpawnTile(Vector2(6, 6), "assets/tiles/housetiles5.png", Vector2(300, 180), false);

	mainContext = SDL_GL_CreateContext(window);

	SetOpenGLAttributes();

	SDL_GL_SetSwapInterval(1);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	SDL_GL_SwapWindow(window);

	SortEntities();

	editor.SetText("Drawing on layer: " + DrawingLayerNames[editor.drawingLayer], renderer);

	bool quit = false;
	while (!quit)
	{
		// Reset all inputs here
		pressedJumpButton = false;

		// Check for inputs
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			if (event.type == SDL_QUIT)
				quit = true;

			if (event.type == SDL_KEYDOWN)
			{
				switch (event.key.keysym.sym)
				{
				case SDLK_ESCAPE:
					quit = true;
					break;
				case SDLK_x:
					pressedJumpButton = true;
					break;
				case SDLK_r:
					player->ResetPosition();
					break;
				case SDLK_1: // toggle Debug mode
					SetModeDebug(!GetModeDebug());
					break;
				case SDLK_2: // toggle Editor mode
					SetModeEdit(!GetModeEdit());
					if (GetModeEdit())
						editor.StartEdit(renderer, spriteManager.GetImage("assets/tiles/" + editor.tilesheets[editor.tilesheetIndex] + ".png"));
					else
						editor.StopEdit();
					break;
				case SDLK_3: // toggle drawing layers
					
					if (GetModeEdit())
					{
						if (editor.drawingLayer == BACKGROUND)
							editor.drawingLayer = FOREGROUND;
						else if (editor.drawingLayer == FOREGROUND)
							editor.drawingLayer = BACKGROUND;

						editor.SetText("Drawing on layer: " + DrawingLayerNames[editor.drawingLayer], renderer);

					}

					
					break;
				case SDLK_4:

					if (GetModeEdit())
					{
						editor.tilesheetIndex++;
						if (editor.tilesheetIndex > 1)
							editor.tilesheetIndex = 0;
						editor.StartEdit(renderer, spriteManager.GetImage("assets/tiles/" + editor.tilesheets[editor.tilesheetIndex] + ".png"));
					}

					break;
				default:
					break;
				}
			}
			else if (event.type == SDL_KEYUP)
			{
				switch (event.key.keysym.sym)
				{
				case SDLK_x:
					pressedJumpButton = false;
					break;
				default:
					break;
				}
			}
		}

		CalcDt();

		if (!GetModeEdit())
			Update();
		else
			editor.HandleEdit(*this);

		Render();
	}

}

void Game::Update()
{
	for (int i = 0; i < entities.size(); i++)
	{
		entities[i]->Update(*this);
	}
}

void Game::Render()
{
	SDL_RenderClear(renderer);

	// Render editor grid
	if (GetModeEdit())
	{
		SDL_SetRenderDrawColor(renderer, 64, 64, 64, 64);
		for (int x = 0; x < 100; x++)
		{
			for (int y = 0; y < 100; y++)
			{
				SDL_Rect rect;
				rect.x = x * TILE_SIZE * SCALE;
				rect.y = y * TILE_SIZE * SCALE;
		
				SDL_RenderDrawRect(renderer, &rect);				
			}
		}
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	}

	// Render all entities
	for (int i = 0; i < entities.size(); i++)
	{
		entities[i]->Render(renderer);
	}

	// Render editor toolbox
	if (GetModeEdit())
	{
		editor.Render(renderer);
	}
		
	SDL_RenderPresent(renderer);
}

// Implementation of insertion sort:
// Splits the list into two portions - sorted and unsorted.
// Then steps through the unsorted list, checking where the next one fits.
void Game::SortEntities()
{
	const int n = entities.size();
	for (int i = 0; i < n; i++)
	{
		int j = i;
		while (j > 0 && entities[j - 1]->drawOrder > entities[j]->drawOrder)
		{
			std::swap(entities[j], entities[j - 1]);
			j--;
		}
	}
}