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

	editor = new Editor(renderer);

	// Initialize the font before all text
	theFont = TTF_OpenFont("assets/fonts/default.ttf", 20);

	// Initialize all text
	jumpsRemainingText = new Text(renderer, theFont);
	jumpsRemainingText->SetText("Jumps Remaining: 2");
	jumpsRemainingText->SetPosition(0, 100);

	fpsText = new Text(renderer, theFont);
	fpsText->SetText("FPS:");
	fpsText->SetPosition(0, 0);

	timerText = new Text(renderer, theFont);
	timerText->SetText("");
	timerText->SetPosition(0, 100);


	// Initialize all the menus
	allMenus["Title"] = new MenuScreen("Title", *this);
	allMenus["Pause"] = new MenuScreen("Pause", *this);
	allMenus["Settings"] = new MenuScreen("Settings", *this);
	allMenus["Spellbook"] = new MenuScreen("Spellbook", *this);
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

	Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048);
	currentBGM = Mix_LoadMUS("assets/bgm/Witchs_Waltz.ogg");
	 
	window = SDL_CreateWindow("Witch Doctor Kaneko",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, screenWidth, screenHeight, SDL_WINDOW_OPENGL);

	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED );
}

void Game::EndSDL()
{
	// Delete our OpengL context
	SDL_GL_DeleteContext(mainContext);

	SDL_DestroyRenderer(renderer);	
	SDL_DestroyWindow(window);	
	window = nullptr;

	Mix_FreeMusic(currentBGM);
	currentBGM = nullptr;

	TTF_CloseFont(theFont);
	
	Mix_Quit();
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

bool Game::SpawnMissile(Vector2 position)
{
	//TODO: Make a way for this to return false
	Missile* missile = new Missile();
	
	Animator* anim = new Animator("debug_missile", "moving");
	anim->SetBool("destroyed", false);
	missile->SetAnimator(anim);

	anim->MapStateToSprite("moving", new Sprite(8, spriteManager, "assets/sprites/spells/debug_missile.png", renderer, Vector2(14, 7)));
	anim->MapStateToSprite("destroyed", new Sprite(8, spriteManager, "assets/sprites/spells/debug_missile.png", renderer, Vector2(14, 7)));

	missile->SetPosition(position);
	missile->SetVelocity(Vector2(0.5f,0));

	entities.emplace_back(missile);

	return true;
}

void Game::SpawnPerson(Vector2 position)
{
	Sprite* sprite = new Sprite(5, spriteManager, "assets/sprites/wdk_blink.png", renderer, Vector2(16, 24));

	//TODO: Make this a Physics Entity, not just an Entity
	Entity* person = new Entity();
	Animator* anim = new Animator("", "xyz");
	anim->MapStateToSprite("xyz", sprite);
	anim->speed = 50;
	person->SetAnimator(anim);
	person->SetPosition(position);
	person->SetSprite(sprite);
	person->impassable = true;

	//TODO: Also make sure to sort the sprites for drawing in the right order
	entities.emplace_back(person);
}

Tile* Game::SpawnTile(Vector2 frame, string tilesheet, Vector2 position, bool impassable, DrawingLayer drawingLayer)
{
	Tile* tile = new Tile(frame, spriteManager.GetImage(tilesheet), renderer);
	int newTileX = position.x - ((int)position.x % (TILE_SIZE * SCALE));
	int newTileY = position.y - ((int)position.y % (TILE_SIZE * SCALE));
	tile->SetPosition(Vector2(newTileX + camera.x, newTileY + camera.y));
	tile->layer = drawingLayer;
	tile->impassable = impassable;
	tile->etype = "tile";
	tile->tileCoordinates = frame;
	tile->tilesheetIndex = editor->tilesheetIndex;
	entities.emplace_back(tile);
	return tile;
}

//TODO: How can we dynamically get the size of the background so that we can loop them without hardcoding it?
// (low priority / not too important)
Background* Game::SpawnBackground(Vector2 pos)
{
	Background* background = new Background(pos);

	background->AddLayer(spriteManager, renderer, "assets/bg/forest/forest_ground.png", -90);
	background->AddLayer(spriteManager, renderer, "assets/bg/forest/forest_sky1.png", -99);
	background->AddLayer(spriteManager, renderer, "assets/bg/forest/forest_trees_front.png", -10);
	background->AddLayer(spriteManager, renderer, "assets/bg/forest/forest_trees_front_curved.png", -11);
	background->AddLayer(spriteManager, renderer, "assets/bg/forest/forest_trees_back.png", -20);
	background->AddLayer(spriteManager, renderer, "assets/bg/forest/forest_trees_back_curved.png", -21);

	SortEntities(background->layers);

	backgrounds.emplace_back(background);

	return background;
}

Player* Game::SpawnPlayer(Vector2 position)
{
	Player* player = new Player();
	Animator* anim1 = new Animator("kaneko", "idle");

	anim1->SetBool("isGrounded", true);

	anim1->MapStateToSprite("walk", new Sprite(6, spriteManager, "assets/sprites/kaneko/wdk_walk.png", renderer, Vector2(16,24)));
	anim1->MapStateToSprite("blink", new Sprite(5, spriteManager, "assets/sprites/kaneko/wdk_blink.png", renderer, Vector2(16, 24)));
	anim1->MapStateToSprite("idle", new Sprite(2, spriteManager, "assets/sprites/kaneko/wdk_idle.png", renderer, Vector2(16, 24)));
	anim1->MapStateToSprite("jump", new Sprite(2, spriteManager, "assets/sprites/kaneko/wdk_jump.png", renderer, Vector2(24, 24)));
	
	//TODO: Make states for debug in air, up, down, on ladder, etc. (FIX PIVOT POINTS)
	anim1->MapStateToSprite("debug", new Sprite(10, spriteManager, "assets/sprites/kaneko/wdk_debug.png", renderer, Vector2(25, 26)));
	anim1->MapStateToSprite("debug_up", new Sprite(2, spriteManager, "assets/sprites/kaneko/wdk_debug_up.png", renderer, Vector2(25, 26)));
	anim1->MapStateToSprite("debug_down", new Sprite(2, spriteManager, "assets/sprites/kaneko/wdk_debug_down.png", renderer, Vector2(25, 26)));
	anim1->MapStateToSprite("debug_air", new Sprite(7, spriteManager, "assets/sprites/kaneko/wdk_debug_air.png", renderer, Vector2(25, 26)));
	anim1->MapStateToSprite("debug_air_up", new Sprite(2, spriteManager, "assets/sprites/kaneko/wdk_debug_air_up.png", renderer, Vector2(25, 26)));
	anim1->MapStateToSprite("debug_air_down", new Sprite(2, spriteManager, "assets/sprites/kaneko/wdk_debug_air_down.png", renderer, Vector2(25, 26)));
	anim1->MapStateToSprite("debug_climb", new Sprite(2, spriteManager, "assets/sprites/kaneko/wdk_debug_climb.png", renderer, Vector2(25, 26)));



	player->SetAnimator(anim1);
	player->SetPosition(position);
	player->startPosition = position;
	player->drawOrder = 99;
	player->etype = "player";

	entities.emplace_back(player);

	return player;
}

void Game::DeleteEntity(Entity* entity)
{
	std::vector<Entity*>::iterator index = std::find(entities.begin(), entities.end(), entity);
	if (index != entities.end()) // means the element was not found
		entities.erase(index);
}

void Game::DeleteEntity(int index)
{
	delete entities[index];
	entities.erase(entities.begin() + index);
}


void Game::PlayLevel(string gameName, string levelName)
{
	SDL_SetWindowIcon(window, spriteManager.GetImage("assets/gui/icon.png"));

	editor->LoadLevel(*this, levelName);

	MainLoop();
}

void Game::Play(string gameName)
{
	SDL_SetWindowIcon(window, spriteManager.GetImage("assets/gui/icon.png"));

	entities.reserve(5);

	player = SpawnPlayer(Vector2(220, 0));

	//SpawnPerson(Vector2(400, 0));
	//SpawnPerson(Vector2(0, 0));

	//entities.emplace_back(bg);

	//SpawnTile(Vector2(5, 3), "assets/tiles/housetiles5.png", Vector2(100, 180), false);
	//SpawnTile(Vector2(6, 6), "assets/tiles/housetiles5.png", Vector2(300, 180), false);

	MainLoop();
}

void Game::MainLoop()
{
	//Mix_PlayMusic(currentBGM, -1);

	mainContext = SDL_GL_CreateContext(window);

	SetOpenGLAttributes();

	SDL_GL_SetSwapInterval(1);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	SDL_GL_SwapWindow(window);

	const int NUM_BGS = 5;
	const int BG_WIDTH = 636;
	for (int i = 0; i < NUM_BGS; i++)
	{
		SpawnBackground(Vector2(BG_WIDTH * SCALE * -i, 0));
	}

	SortEntities(entities);

	editor->currentEditModeLayer->SetText("Drawing on layer: " + DrawingLayerNames[editor->drawingLayer]);

	//Start counting frames per second
	int countedFrames = 0;
	timer.Start();
	
	quit = false;
	while (!quit)
	{
		fpsLimit.Start();

		//Calculate and correct fps
		float avgFPS = countedFrames / (timer.GetTicks() / 1000.f);
		if (avgFPS > 2000000)
		{
			avgFPS = 0;
		}

		//fpsText->SetText("FPS: " + std::to_string(avgFPS));

		// Reset all inputs here
		pressedJumpButton = false;
		pressedDebugButton = false;

		// Check for inputs
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			if (openedMenus.size() > 0)
				quit = HandleMenuEvent(event);
			else
				quit = HandleEvent(event);

			if (quit)
				break;
		}

		CalcDt();

		//timerText->SetText(std::to_string(timer.GetTicks()/1000.0f));

		if (GetModeEdit())
		{
			HandleEditMode();			
		}
		else if (openedMenus.size() > 0)
		{
			Uint32 ticks = timer.GetTicks();
			if (ticks > lastPressedKeyTicks + 100) //TODO: Check for overflow errors
			{				
				if (openedMenus[openedMenus.size() - 1]->Update() > 0)
					lastPressedKeyTicks = ticks;				
			}

			if (lastPressedKeyTicks > 0)
				int test = 0;
		}
		else
		{
			Update();
		}
			
		Render();

		countedFrames++;

		//If frame finished early
		if (limitFPS)
		{
			int frameTicks = fpsLimit.GetTicks();
			if (frameTicks < SCREEN_TICKS_PER_FRAME)
			{
				//Wait remaining time
				//SDL_Delay(SCREEN_TICKS_PER_FRAME - frameTicks);
			}
		}
	}

}

void Game::HandleEditMode()
{
	//TODO: Make this a function
	const Uint8* currentKeyStates = SDL_GetKeyboardState(NULL);
	float cameraSpeed = 1.0f;

	if (currentKeyStates[SDL_SCANCODE_UP] || currentKeyStates[SDL_SCANCODE_W])
	{
		camera.y -= (TILE_SIZE * SCALE);
	}
	else if (currentKeyStates[SDL_SCANCODE_DOWN] || currentKeyStates[SDL_SCANCODE_S])
	{
		camera.y += (TILE_SIZE * SCALE);
	}

	if (currentKeyStates[SDL_SCANCODE_LEFT] || currentKeyStates[SDL_SCANCODE_A])
	{
		camera.x -= (TILE_SIZE * SCALE);

	}
	else if (currentKeyStates[SDL_SCANCODE_RIGHT] || currentKeyStates[SDL_SCANCODE_D])
	{
		camera.x += (TILE_SIZE * SCALE);
	}

	editor->HandleEdit(*this);
}

// PRE-CONDITION: openedMenus.size() > 0
bool Game::HandleMenuEvent(SDL_Event& event)
{
	bool quit = false;

	if (event.type == SDL_QUIT)
		quit = true;

	if (event.type == SDL_KEYDOWN)
	{
		switch (event.key.keysym.sym)
		{
		case SDLK_ESCAPE:
			openedMenus.pop_back();
			break;
		case SDLK_q:
			quit = true;
			break;
		case SDLK_RETURN:
			quit = openedMenus[openedMenus.size() - 1]->PressSelectedButton(*this);
			break;
		default:
			break;
		}
	}

	return quit;
}

bool Game::HandleEvent(SDL_Event& event)
{
	bool quit = false;

	if (event.type == SDL_QUIT)
		quit = true;

	if (event.type == SDL_MOUSEWHEEL)
	{
		//TODO: Zooming in and out
	}

	if (event.type == SDL_KEYDOWN)
	{
		switch (event.key.keysym.sym)
		{
		case SDLK_ESCAPE:
			if (!GetModeEdit())
				openedMenus.emplace_back(allMenus["Pause"]);
			break;
		case SDLK_q:
			quit = true;
			break;
		case SDLK_x:
			pressedJumpButton = true;
			break;
		case SDLK_c:
			pressedDebugButton = true;
			break;
		case SDLK_m:
			if (Mix_PausedMusic())
				Mix_ResumeMusic();
			else
				Mix_PauseMusic();
			break;
		case SDLK_r:
			if (player != nullptr)
				player->ResetPosition();
			break;
		case SDLK_1: // toggle Debug mode
			SetModeDebug(!GetModeDebug());
			break;
		case SDLK_2: // toggle Editor mode
			SetModeEdit(!GetModeEdit());
			if (GetModeEdit())
			{
				camera.x = camera.x - ((int)camera.x % (TILE_SIZE * SCALE));
				camera.y = camera.y - ((int)camera.y % (TILE_SIZE * SCALE));
				editor->StartEdit(renderer, spriteManager.GetImage("assets/tiles/" + editor->tilesheets[editor->tilesheetIndex] + ".png"));
			}
			else
			{
				editor->StopEdit();
			}

			break;
		case SDLK_3: // toggle drawing layers

			if (GetModeEdit())
			{
				if (editor->drawingLayer == BACKGROUND)
					editor->drawingLayer = FOREGROUND;
				else if (editor->drawingLayer == FOREGROUND)
					editor->drawingLayer = BACKGROUND;

				editor->currentEditModeLayer->SetText("Drawing on layer: " + DrawingLayerNames[editor->drawingLayer]);
			}

			break;
		case SDLK_4:

			if (GetModeEdit())
			{
				editor->tilesheetIndex++;
				if (editor->tilesheetIndex > 1)
					editor->tilesheetIndex = 0;
				editor->StartEdit(renderer, spriteManager.GetImage("assets/tiles/" + editor->tilesheets[editor->tilesheetIndex] + ".png"));
			}

			break;
		case SDLK_9:
			if (GetModeEdit())
				editor->SaveLevel(*this);
			break;
		case SDLK_l:
			if (GetModeEdit())
				editor->LoadLevel(*this, "level");
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

	return quit;
}

void Game::Update()
{
	camera = player->GetCenter();
	camera.x -= (screenWidth / 2.0f);  
	camera.y -= (screenHeight / 2.0f);

	// Option 1: Destroy entities before we update them
	unsigned int k = 0;
	while(k < entities.size())
	{
		if (entities[k]->shouldDelete)
			DeleteEntity(k);
		else
			k++;
	}

	for (unsigned int i = 0; i < entities.size(); i++)
	{		
		// Option 2: Destroy entities as we update them
		entities[i]->Update(*this);
	}

	// Option 3: Destroy entities after we update them
}

void Game::Render()
{
	SDL_RenderClear(renderer);

	// Render all backgrounds and their layers
	for (unsigned int i = 0; i < backgrounds.size(); i++)
	{
		backgrounds[i]->Render(renderer, camera);
	}

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
	for (unsigned int i = 0; i < entities.size(); i++)
	{
		entities[i]->Render(renderer, camera);
	}

	// Render editor toolbox
	if (GetModeEdit())
	{
		editor->Render(renderer);
	}

	if (GetModeDebug())
	{
		//jumpsRemainingText->Render(renderer);
	}

	fpsText->Render(renderer);
	timerText->Render(renderer);

	// Render all menu screens
	if (openedMenus.size() > 0)
	{
		openedMenus[openedMenus.size() - 1]->Render(renderer);
	}
		
	SDL_RenderPresent(renderer);
}

// Implementation of insertion sort:
// Splits the list into two portions - sorted and unsorted.
// Then steps through the unsorted list, checking where the next one fits.
void Game::SortEntities(std::vector<Entity*>& entityVector)
{
	const int n = entityVector.size();
	for (int i = 0; i < n; i++)
	{
		int j = i;
		while (j > 0 && entityVector[j - 1]->drawOrder > entityVector[j]->drawOrder)
		{
			std::swap(entityVector[j], entityVector[j - 1]);
			j--;
		}
	}
}