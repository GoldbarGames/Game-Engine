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

	overlayRect.x = 0;
	overlayRect.y = 0;
	overlayRect.w = screenWidth;
	overlayRect.h = screenHeight;

	spriteManager = new SpriteManager();

	// Initialize the cutscene stuff (do this AFTER renderer and sprite manager)
	cutscene = new CutsceneManager(*this);
	cutscene->ParseScene();

	// Initialize the sprite map (do this BEFORE the editor)
	spriteMapDoor[0] = "assets/sprites/objects/door1.png";
	spriteMapDoor[1] = "assets/sprites/objects/door_house.png";
	spriteMapDoor[2] = "assets/sprites/objects/door_house_outside.png";

	spriteMapLadder[0] = "assets/sprites/objects/ladder1.png";
	spriteMapLadder[1] = "assets/sprites/objects/ladder_house.png";
	spriteMapLadder[2] = "assets/sprites/objects/ladder_b.png";

	spriteMapNPCs[0] = "assets/sprites/npcs/gramps.png";
	spriteMapNPCs[1] = "assets/sprites/npcs/the_man.png";

	// Initialize the font before all text
	theFont = TTF_OpenFont("assets/fonts/default.ttf", 20);

	editor = new Editor(*this);

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

	timerOverlayColor.Start(1);
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

	renderer = new Renderer();
	renderer->renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED );

	
}

void Game::EndSDL()
{
	// Delete our OpengL context
	SDL_GL_DeleteContext(mainContext);

	SDL_DestroyRenderer(renderer->renderer);	
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

Ladder* Game::CreateLadder(Vector2 position, int spriteIndex)
{
	Ladder* newLadder = new Ladder(position);
	newLadder->spriteIndex = spriteIndex;

	Animator* anim = new Animator("ladder", "middle");

	anim->MapStateToSprite("middle", new Sprite(2, 2, 5, spriteManager, 
		spriteMapLadder[spriteIndex], renderer, Vector2(0, 0)));

	anim->MapStateToSprite("bottom", new Sprite(4, 4, 5, spriteManager,
		spriteMapLadder[spriteIndex], renderer, Vector2(0, 0)));

	anim->MapStateToSprite("top", new Sprite(0, 0, 5, spriteManager,
		spriteMapLadder[spriteIndex], renderer, Vector2(0, 0)));

	anim->speed = 0;
	newLadder->SetAnimator(anim);

	return newLadder;
}

Vector2 Game::SnapToGrid(Vector2 position)
{
	int x = position.x + camera.x - ((int)(position.x) % (editor->GRID_SIZE * SCALE));
	int y = position.y + camera.y - ((int)(position.y) % (editor->GRID_SIZE * SCALE));

	if (x % 2 != 0)
		x++;

	if (y % 2 != 0)
		y++;

	return Vector2(x, y);
}

Ladder* Game::SpawnLadder(Vector2 position, int spriteIndex)
{
	Vector2 snappedPosition = SnapToGrid(position);

	Ladder* newLadder = CreateLadder(snappedPosition, spriteIndex);	

	if (!newLadder->CanSpawnHere(snappedPosition, *this))
	{
		delete newLadder;
		return nullptr;
	}
	else
	{
		entities.emplace_back(newLadder);
		return newLadder;
	}

	return newLadder;
}

Door* Game::CreateDoor(Vector2 position, int spriteIndex)
{
	Door* newDoor = new Door(position, Vector2(0, 0));
	newDoor->spriteIndex = spriteIndex;

	//TODO: How to make this work for doors that will be related to other tilesets?
	Animator* anim = new Animator("door", "closed");

	Vector2 pivotPoint = Vector2(0, 0);
	anim->MapStateToSprite("closed", new Sprite(0, 0, 2, spriteManager, spriteMapDoor[spriteIndex], renderer, pivotPoint));
	anim->MapStateToSprite("opened", new Sprite(1, 1, 2, spriteManager, spriteMapDoor[spriteIndex], renderer, pivotPoint));

	newDoor->SetAnimator(anim);

	return newDoor;
}

Door* Game::SpawnDoor(Vector2 position, int spriteIndex) // maybe pass in the tileset number for the door?
{
	Vector2 snappedPosition = SnapToGrid(position);

	Door* newDoor = CreateDoor(snappedPosition, spriteIndex);

	if (!newDoor->CanSpawnHere(snappedPosition, *this))
	{
		delete newDoor;
		return nullptr;
	}
	else
	{
		entities.emplace_back(newDoor);
		return newDoor;
	}	
}

NPC* Game::CreateNPC(std::string name, Vector2 position, int spriteIndex)
{
	NPC* newNPC = new NPC(name, position);
	newNPC->spriteIndex = spriteIndex;

	Animator* anim = new Animator(name, "idle");

	Vector2 pivotPoint = Vector2(0, 0);

	if (name == "gramps")
	{
		pivotPoint = Vector2(12, 28);
		anim->MapStateToSprite("idle", new Sprite(0, 0, 3, spriteManager, spriteMapNPCs[spriteIndex], renderer, pivotPoint));
		anim->MapStateToSprite("sad", new Sprite(1, 1, 3, spriteManager, spriteMapNPCs[spriteIndex], renderer, pivotPoint));
		anim->MapStateToSprite("confused", new Sprite(2, 2, 3, spriteManager, spriteMapNPCs[spriteIndex], renderer, pivotPoint));
		
	}
	else if (name == "the_man")
	{
		pivotPoint = Vector2(23, 36);
		anim->speed = 200;
		anim->MapStateToSprite("idle", new Sprite(0, 7, 8, spriteManager, spriteMapNPCs[spriteIndex], renderer, pivotPoint));
	}

	newNPC->ChangeCollider(anim->GetCurrentSprite()->frameWidth, anim->GetCurrentSprite()->frameHeight);

	newNPC->SetAnimator(anim);
	newNPC->trigger = true;

	return newNPC;
}

NPC* Game::SpawnNPC(std::string name, Vector2 position, int spriteIndex)
{
	Vector2 snappedPosition = SnapToGrid(position);

	NPC* newNPC = CreateNPC(name, snappedPosition, spriteIndex);

	if (!newNPC->CanSpawnHere(snappedPosition, *this))
	{
		delete newNPC;
		return nullptr;
	}
	else
	{
		entities.emplace_back(newNPC);
		return newNPC;
	}
}

bool Game::SpawnMissile(Vector2 position, Vector2 velocity, float angle)
{
	//TODO: Make a way for this to return false

	Animator* anim = new Animator("debug_missile", "moving");
	anim->SetBool("destroyed", false);

	Vector2 pivotPoint = Vector2(14, 7);
	anim->MapStateToSprite("moving", new Sprite(0, 3, 8, spriteManager, "assets/sprites/spells/debug_missile.png", renderer, pivotPoint));
	anim->MapStateToSprite("destroyed", new Sprite(4, 7, 8, spriteManager, "assets/sprites/spells/debug_missile.png", renderer, pivotPoint, false));

	Missile* missile = new Missile(position - pivotPoint);

	missile->SetAnimator(anim);
	missile->SetVelocity(velocity);
	missile->angle = angle;
	missile->GetAnimator()->SetState("moving");

	entities.emplace_back(missile);

	return true;
}

Vector2 Game::CalcObjPos(Vector2 pos)
{
	int newTileX = (int)pos.x + (int)(camera.x);
	int newTileY = (int)pos.y + (int)(camera.y);

	if (newTileX % 2 != 0)
		newTileX++;

	if (newTileY % 2 != 0)
		newTileY++;

	return Vector2(newTileX, newTileY);
}

Tile* Game::SpawnTile(Vector2 frame, string tilesheet, Vector2 position, DrawingLayer drawingLayer)
{
	Vector2 newTilePos = CalcObjPos(position);

	//Sprite* tileSprite = new Sprite(Vector2(newTileX, newTileY), spriteManager.GetImage(tilesheet), renderer);
	Tile* tile = new Tile(newTilePos, frame, spriteManager->GetImage(renderer, tilesheet), renderer);

	tile->layer = drawingLayer;
	tile->impassable = drawingLayer == DrawingLayer::COLLISION;
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
	Vector2 snappedPosition = SnapToGrid(position);

	Player* player = new Player(snappedPosition);
	player->game = this;

	Animator* anim1 = new Animator("kaneko", "idle");

	anim1->SetBool("isGrounded", true);

	anim1->MapStateToSprite("walk", new Sprite(6, spriteManager, "assets/sprites/kaneko/wdk_walk.png", renderer, Vector2(16,24)));
	anim1->MapStateToSprite("blink", new Sprite(5, spriteManager, "assets/sprites/kaneko/wdk_blink.png", renderer, Vector2(16, 24)));
	anim1->MapStateToSprite("idle", new Sprite(2, spriteManager, "assets/sprites/kaneko/wdk_idle.png", renderer, Vector2(16, 24)));
	anim1->MapStateToSprite("jump", new Sprite(2, spriteManager, "assets/sprites/kaneko/wdk_jump.png", renderer, Vector2(24, 24)));
	
	anim1->MapStateToSprite("look_up", new Sprite(2, spriteManager, "assets/sprites/kaneko/wdk_lookup.png", renderer, Vector2(16, 24)));
	anim1->MapStateToSprite("look_down", new Sprite(2, spriteManager, "assets/sprites/kaneko/wdk_lookdown.png", renderer, Vector2(16, 24)));

	anim1->MapStateToSprite("ladder_idle", new Sprite(3, 3, 8, spriteManager, "assets/sprites/kaneko/wdk_ladder_climb.png", renderer, Vector2(15, 26)));
	anim1->MapStateToSprite("ladder_climbing", new Sprite(0, 7, 8, spriteManager, "assets/sprites/kaneko/wdk_ladder_climb.png", renderer, Vector2(15, 26)));

	//TODO: Make states for debug in air, up, down, on ladder, etc. (FIX PIVOT POINTS)
	anim1->MapStateToSprite("debug", new Sprite(10, spriteManager, "assets/sprites/kaneko/wdk_debug.png", renderer, Vector2(25, 26)));
	anim1->MapStateToSprite("debug_up", new Sprite(10, spriteManager, "assets/sprites/kaneko/wdk_debug_up.png", renderer, Vector2(25, 26)));
	anim1->MapStateToSprite("debug_down", new Sprite(10, spriteManager, "assets/sprites/kaneko/wdk_debug_down.png", renderer, Vector2(25, 26)));
	anim1->MapStateToSprite("debug_air", new Sprite(7, spriteManager, "assets/sprites/kaneko/wdk_debug_air.png", renderer, Vector2(28, 26)));
	anim1->MapStateToSprite("debug_air_up", new Sprite(7, spriteManager, "assets/sprites/kaneko/wdk_debug_air_up.png", renderer, Vector2(28, 26)));
	anim1->MapStateToSprite("debug_air_down", new Sprite(7, spriteManager, "assets/sprites/kaneko/wdk_debug_air_down.png", renderer, Vector2(28, 26)));
	anim1->MapStateToSprite("debug_climb", new Sprite(2, spriteManager, "assets/sprites/kaneko/wdk_debug_climb.png", renderer, Vector2(25, 26)));

	player->SetAnimator(anim1);
	player->SetPosition(snappedPosition);
	player->startPosition = snappedPosition;

	entities.emplace_back(player);

	return player;
}

void Game::ShouldDeleteEntity(int index)
{
	entities[index]->shouldDelete = true;
}

void Game::ShouldDeleteEntity(Entity* entity)
{
	//std::vector<Entity*>::iterator index = std::find(entities.begin(), entities.end(), entity);
	//if (index != entities.end()) // means the element was not found
	//	index->shouldDelete = true;
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
	entities[index] = nullptr;
	entities.erase(entities.begin() + index);
}

void Game::StartTextInput(std::string reason)
{
	getKeyboardInput = true;
	inputType = reason;
	SDL_StartTextInput();
	inputText = "";
}

void Game::StopTextInput()
{
	getKeyboardInput = false;
	SDL_StopTextInput();

	if (inputType == "properties")
	{
		editor->SetPropertyText();
	}
	else if (inputType == "new_level")
	{
		editor->showDialogPopup = false;
		editor->SaveLevel(inputText);
	}
	else if (inputType == "load_file_as")
	{
		editor->showDialogPopup = false;
		editor->LoadLevel(inputText);
	}
}

void Game::PlayLevel(string gameName, string levelName)
{
	SDL_SetWindowIcon(window, IMG_Load("assets/gui/icon.png"));

	currentLevel = levelName;
	editor->LoadLevel(levelName);

	MainLoop();
}

void Game::Play(string gameName)
{
	SDL_SetWindowIcon(window, IMG_Load("assets/gui/icon.png"));

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

	// Set up OpenGL stuff
	mainContext = SDL_GL_CreateContext(window);
	SetOpenGLAttributes();
	SDL_GL_SetSwapInterval(1);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	SDL_GL_SwapWindow(window);

	// Create the backgrounds
	const int NUM_BGS = 1;
	const int BG_WIDTH = 636;
	for (int i = 0; i < NUM_BGS; i++)
	{
		SpawnBackground(Vector2(BG_WIDTH * SCALE * -i, 0));
	}

	SortEntities(entities);

	editor->currentEditModeLayer->SetText("Drawing on layer: " + GetDrawingLayerName(editor->drawingLayer));

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
		//clickedMouse = false;

		// Check for inputs
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			//if (event.type == SDL_MOUSEBUTTONDOWN)
			//	clickedMouse = true;

			if (openedMenus.size() > 0)
				quit = HandleMenuEvent(event);
			else
				quit = HandleEvent(event);

			if (quit)
				break;
		}

		CalcDt();

		//timerText->SetText(std::to_string(timer.GetTicks()/1000.0f));

		// Destroy entities before we update them
		unsigned int k = 0;
		while (k < entities.size())
		{
			if (entities[k]->shouldDelete)
				DeleteEntity(k);
			else
				k++;
		}


		if (GetModeEdit())
		{
			if (!getKeyboardInput)
			{
				HandleEditMode();
			}
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
		/*
		if (limitFPS)
		{
			int frameTicks = fpsLimit.GetTicks();
			if (frameTicks < SCREEN_TICKS_PER_FRAME)
			{
				//Wait remaining time
				//SDL_Delay(SCREEN_TICKS_PER_FRAME - frameTicks);
			}
		}*/
	}

}

void Game::HandleEditMode()
{
	//TODO: Make this a function
	const Uint8* currentKeyStates = SDL_GetKeyboardState(NULL);
	float cameraSpeed = 1.0f;

	if (currentKeyStates[SDL_SCANCODE_UP] || currentKeyStates[SDL_SCANCODE_W])
	{
		camera.y -= (editor->GRID_SIZE * SCALE);
	}
	else if (currentKeyStates[SDL_SCANCODE_DOWN] || currentKeyStates[SDL_SCANCODE_S])
	{
		camera.y += (editor->GRID_SIZE * SCALE);
	}

	if (currentKeyStates[SDL_SCANCODE_LEFT] || currentKeyStates[SDL_SCANCODE_A])
	{
		camera.x -= (editor->GRID_SIZE * SCALE);

	}
	else if (currentKeyStates[SDL_SCANCODE_RIGHT] || currentKeyStates[SDL_SCANCODE_D])
	{
		camera.x += (editor->GRID_SIZE * SCALE);
	}

	editor->HandleEdit();
}

// PRE-CONDITION: openedMenus.size() > 0
bool Game::HandleMenuEvent(SDL_Event& event)
{
	bool quit = false;

	if (event.type == SDL_QUIT)
		quit = true;

	if (event.type == SDL_KEYDOWN)
	{
		Uint32 ticks = SDL_GetTicks();
		switch (event.key.keysym.sym)
		{
		case SDLK_ESCAPE:
			openedMenus.pop_back();
			
			for (unsigned int i = 0; i < entities.size(); i++)
				entities[i]->Unpause(ticks);
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
		if (getKeyboardInput)
		{
			//Handle backspace
			if (event.key.keysym.sym == SDLK_BACKSPACE && inputText.length() > 0)
			{
				inputText.pop_back();
				UpdateTextInput();
			}
			//Handle copy
			else if (event.key.keysym.sym == SDLK_c && SDL_GetModState() & KMOD_CTRL)
			{
				SDL_SetClipboardText(inputText.c_str());
			}
			//Handle paste
			else if (event.key.keysym.sym == SDLK_v && SDL_GetModState() & KMOD_CTRL)
			{
				inputText += SDL_GetClipboardText();
				UpdateTextInput();
			}
			// Pressed enter, submit the input
			else if (event.key.keysym.sym == SDLK_RETURN)
			{
				UpdateTextInput();
				StopTextInput();				
			}
		}
		else
		{
			switch (event.key.keysym.sym)
			{
			case SDLK_ESCAPE:
				if (!GetModeEdit())
				{
					openedMenus.emplace_back(allMenus["Pause"]);
					Uint32 ticks = SDL_GetTicks();
					for (unsigned int i = 0; i < entities.size(); i++)
						entities[i]->Pause(ticks);
				}
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
					camera.x = camera.x - ((int)camera.x % (editor->GRID_SIZE * SCALE));
					camera.y = camera.y - ((int)camera.y % (editor->GRID_SIZE * SCALE));
					editor->StartEdit();
				}
				else
				{
					editor->StopEdit();
				}
				break;
			case SDLK_3: // toggle drawing layers
				if (GetModeEdit())
					editor->ToggleGridSize();
				break;
			case SDLK_4:
				if (GetModeEdit())
					editor->ToggleTileset();
				break;
			case SDLK_5:
				if (GetModeEdit())
					editor->ToggleSpriteMap();
				break;
			case SDLK_8:
				if (GetModeEdit())
					editor->SaveLevel();
				break;
			case SDLK_9:
				if (GetModeEdit())
					editor->LoadLevel("level");
				break;
			default:
				break;
			}
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
	//Special text input event
	else if (event.type == SDL_TEXTINPUT)
	{
		//Not copy or pasting
		if (!(SDL_GetModState() & KMOD_CTRL && (event.text.text[0] == 'c' || event.text.text[0] == 'C' || 
			event.text.text[0] == 'v' || event.text.text[0] == 'V')))
		{
			//Append character
			inputText += event.text.text;
			UpdateTextInput();
		}
	}

	return quit;
}

void Game::UpdateTextInput()
{
	if (inputType == "properties")
	{
		editor->SetPropertyText();
	}
	else if (inputType == "new_level")
	{
		editor->dialogInput->SetText(inputText);
	}
	else if (inputType == "load_file_as")
	{
		editor->dialogInput->SetText(inputText);
	}
}

void Game::UpdateOverlayColor(int& color, const int& target)
{
	if (color != target)
	{
		changingOverlayColor = true;
		if (target > color)
			color++;
		else
			color--;
	}
}

void Game::Update()
{
	camera = player->GetCenter();
	camera.x -= (screenWidth / 2.0f);  
	camera.y -= (screenHeight / 2.0f);

	if (changingOverlayColor && timerOverlayColor.HasElapsed())
	{
		timerOverlayColor.Start(1);
		changingOverlayColor = false;
		UpdateOverlayColor(overlayColor.r, targetColor.r);
		UpdateOverlayColor(overlayColor.g, targetColor.g);
		UpdateOverlayColor(overlayColor.b, targetColor.b);
		UpdateOverlayColor(overlayColor.a, targetColor.a);
	}

	if (watchingCutscene)
		cutscene->Update();

	// Update all entities
	for (unsigned int i = 0; i < entities.size(); i++)
	{		
		entities[i]->Update(*this);
	}
}

void Game::Render()
{
	SDL_RenderClear(renderer->renderer);

	// Render all backgrounds and their layers
	for (unsigned int i = 0; i < backgrounds.size(); i++)
	{
		backgrounds[i]->Render(renderer, camera);
	}

	// Render editor grid
	if (GetModeEdit())
	{
		editor->DrawGrid();
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

	// Draw the screen overlay
	SDL_SetRenderDrawBlendMode(renderer->renderer, SDL_BLENDMODE_BLEND);
	SDL_SetRenderDrawColor(renderer->renderer, overlayColor.r, overlayColor.g, overlayColor.b, overlayColor.a);
	SDL_RenderFillRect(renderer->renderer, &overlayRect);
	SDL_SetRenderDrawColor(renderer->renderer, 0, 0, 0, 255);
	SDL_SetRenderDrawBlendMode(renderer->renderer, SDL_BLENDMODE_NONE);

	if (watchingCutscene)
		cutscene->Render(renderer);

	// Render all menu screens
	if (openedMenus.size() > 0)
	{
		openedMenus[openedMenus.size() - 1]->Render(renderer);
	}
		
	SDL_RenderPresent(renderer->renderer);
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
		while (j > 0)
		{
			if (entityVector[j - 1]->layer > entityVector[j]->layer)
			{
				std::swap(entityVector[j], entityVector[j - 1]);
			}
			else if (entityVector[j - 1]->layer == entityVector[j]->layer)
			{
				if (entityVector[j - 1]->drawOrder > entityVector[j]->drawOrder)
				{
					std::swap(entityVector[j], entityVector[j - 1]);
				}
			}
			j--;
		}
	}
}