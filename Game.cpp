#include "Game.h"
#include "Sprite.h"
#include "Player.h"
#include "debug_state.h"
#include "editor_state.h"
#include "Tile.h"
#include "SettingsButton.h"
#include "globals.h"
#include <sstream>
#include <iterator>

using std::string;

Game::Game()
{
	InitSDL();

	overlayRect.x = 0;
	overlayRect.y = 0;
	overlayRect.w = screenWidth;
	overlayRect.h = screenHeight;

	// Initialize the font before all text
	theFont = TTF_OpenFont("fonts/default.ttf", 20);
	headerFont = TTF_OpenFont("fonts/default.ttf", 32);

	spriteManager = new SpriteManager();

	soundManager = new SoundManager();

	// Initialize the cutscene stuff (do this AFTER renderer and sprite manager)
	cutscene = new CutsceneManager(*this);
	cutscene->ParseScene();

	// Initialize the sprite map (do this BEFORE the editor)
	//TODO: Can this be done automatically by grabbing all files in each folder?
	//(hard-code an array of strings for folder names, then iterate each of them)
	//(turn each spritemap(vector) into a dictionary entry (spritemap["door"])

	std::vector<std::string> mapNames = { "door", "ladder" };

	spriteMap["door"].push_back("assets/sprites/objects/door1.png");
	spriteMap["door"].push_back("assets/sprites/objects/door_house.png");
	spriteMap["door"].push_back("assets/sprites/objects/door_house_outside.png");

	spriteMap["ladder"].push_back("assets/sprites/objects/ladder1.png");
	spriteMap["ladder"].push_back("assets/sprites/objects/ladder_house.png");
	spriteMap["ladder"].push_back("assets/sprites/objects/ladder_b.png");

	spriteMap["npc"].push_back("assets/sprites/npcs/gramps.png");
	spriteMap["npc"].push_back("assets/sprites/npcs/the_man.png");

	spriteMap["bug"].push_back("assets/sprites/bugs/bug1.png");
	spriteMap["bug"].push_back("assets/sprites/bugs/bug2.png");

	spriteMap["goal"].push_back("assets/sprites/objects/door1.png");
	spriteMap["goal"].push_back("assets/sprites/objects/door_house.png");
	spriteMap["goal"].push_back("assets/sprites/objects/door_house_outside.png");

	editor = new Editor(*this);

	ResetText();

	// Initialize all the menus
	allMenus["Title"] = new MenuScreen("Title", *this);
	allMenus["File Select"] = new MenuScreen("File Select", *this);
	allMenus["Pause"] = new MenuScreen("Pause", *this);
	allMenus["Settings"] = new MenuScreen("Settings", *this);
	allMenus["Spellbook"] = new MenuScreen("Spellbook", *this);
	allMenus["EditorSettings"] = new MenuScreen("EditorSettings", *this);

	timerOverlayColor.Start(1);

	// Set up OpenGL stuff
	mainContext = SDL_GL_CreateContext(window);
	SetOpenGLAttributes();
	SDL_GL_SetSwapInterval(1);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	SDL_GL_SwapWindow(window);

	start_time = clock::now();
}

Game::~Game()
{	
	EndSDL();
}

void Game::ResetText()
{
	if (fpsText == nullptr)
		fpsText = new Text(renderer, theFont);

	fpsText->SetText("FPS:");
	fpsText->SetPosition(0, 0);

	if (timerText == nullptr)
		timerText = new Text(renderer, theFont);
	
	timerText->SetText("");
	timerText->SetPosition(100, 0);

	if (bugText == nullptr)
		bugText = new Text(renderer, theFont);
	
	bugText->SetText("Bugs Remaining: " + std::to_string(bugsRemaining));
	bugText->SetPosition(0, 100);

	if (etherText == nullptr)
		etherText = new Text(renderer, theFont);
	
	etherText->SetText("Ether: " + std::to_string(currentEther));
	etherText->SetPosition(0, 150);
}

void Game::CalcDt()
{
	dt = std::chrono::duration<float, milliseconds::period>(clock::now() - start_time).count();
	start_time = clock::now();

	// When we are debugging and hit a breakpoint in an IDE, the timer continues running./
	// This causes the dt to become huge and throw everything around like crazy.
	// So reset the dt if it becomes too big so that we can debug properly.
	if (dt > 100)
		dt = 33;
}

void Game::InitSDL()
{
	SDL_Init(SDL_INIT_VIDEO);
	TTF_Init();

	window = SDL_CreateWindow("Witch Doctor Kaneko",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, screenWidth, screenHeight, SDL_WINDOW_OPENGL);

	SDL_SetWindowIcon(window, IMG_Load("assets/gui/icon.png"));

	renderer = new Renderer();
	renderer->CreateSDLRenderer(window, true);
	
}

void Game::EndSDL()
{
	// Delete our OpengL context
	SDL_GL_DeleteContext(mainContext);

	SDL_DestroyRenderer(renderer->renderer);	
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

Ladder* Game::CreateLadder(Vector2 position, int spriteIndex)
{
	Ladder* newLadder = new Ladder(position);
	newLadder->spriteIndex = spriteIndex;

	Animator* anim = new Animator("ladder", "middle");

	anim->MapStateToSprite("middle", new Sprite(2, 2, 5, spriteManager, 
		spriteMap["ladder"][spriteIndex], renderer, Vector2(0, 0)));

	anim->MapStateToSprite("bottom", new Sprite(4, 4, 5, spriteManager,
		spriteMap["ladder"][spriteIndex], renderer, Vector2(0, 0)));

	anim->MapStateToSprite("top", new Sprite(0, 0, 5, spriteManager,
		spriteMap["ladder"][spriteIndex], renderer, Vector2(0, 0)));

	anim->speed = 0;
	newLadder->SetAnimator(anim);

	return newLadder;
}

Vector2 Game::SnapToGrid(Vector2 position)
{
	int x = position.x + camera.x - ((int)(position.x) % (editor->GRID_SIZE * Renderer::GetScale()));
	int y = position.y + camera.y - ((int)(position.y) % (editor->GRID_SIZE * Renderer::GetScale()));

	if (x % 2 != 0)
		x++;

	if (y % 2 != 0)
		y++;

	return Vector2(x, y);
}

Ladder* Game::SpawnLadder(Vector2 position, int spriteIndex)
{
	Ladder* newLadder = CreateLadder(position, spriteIndex);

	if (!newLadder->CanSpawnHere(position, *this))
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
	anim->MapStateToSprite("closed", new Sprite(0, 0, 2, spriteManager, spriteMap["door"][spriteIndex], renderer, pivotPoint));
	anim->MapStateToSprite("opened", new Sprite(1, 1, 2, spriteManager, spriteMap["door"][spriteIndex], renderer, pivotPoint));

	newDoor->SetAnimator(anim);

	return newDoor;
}

Door* Game::SpawnDoor(Vector2 position, int spriteIndex)
{
	Door* newDoor = CreateDoor(position, spriteIndex);

	if (!newDoor->CanSpawnHere(position, *this))
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
		anim->MapStateToSprite("idle", new Sprite(0, 0, 3, spriteManager, spriteMap["npc"][spriteIndex], renderer, pivotPoint));
		anim->MapStateToSprite("sad", new Sprite(1, 1, 3, spriteManager, spriteMap["npc"][spriteIndex], renderer, pivotPoint));
		anim->MapStateToSprite("confused", new Sprite(2, 2, 3, spriteManager, spriteMap["npc"][spriteIndex], renderer, pivotPoint));
		
	}
	else if (name == "the_man")
	{
		pivotPoint = Vector2(23, 36);
		anim->speed = 200;
		anim->MapStateToSprite("idle", new Sprite(0, 7, 8, spriteManager, spriteMap["npc"][spriteIndex], renderer, pivotPoint));
	}

	newNPC->ChangeCollider(anim->GetCurrentSprite()->frameWidth, anim->GetCurrentSprite()->frameHeight);

	newNPC->SetAnimator(anim);
	newNPC->trigger = true;

	return newNPC;
}

NPC* Game::SpawnNPC(std::string name, Vector2 position, int spriteIndex)
{
	NPC* newNPC = CreateNPC(name, position, spriteIndex);

	if (!newNPC->CanSpawnHere(position, *this))
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


Goal* Game::CreateGoal(Vector2 position, int spriteIndex)
{
	Goal* newGoal = new Goal(position);
	newGoal->spriteIndex = spriteIndex;

	//TODO: How to make this work for doors that will be related to other tilesets?
	Animator* anim = new Animator("door", "closed");

	Vector2 pivotPoint = Vector2(0, 0);
	anim->MapStateToSprite("closed", new Sprite(0, 0, 2, spriteManager, spriteMap["goal"][spriteIndex], renderer, pivotPoint));
	anim->MapStateToSprite("opened", new Sprite(1, 1, 2, spriteManager, spriteMap["goal"][spriteIndex], renderer, pivotPoint));

	newGoal->SetAnimator(anim);

	return newGoal;
}

Goal* Game::SpawnGoal(Vector2 position, int spriteIndex)
{
	Goal* newGoal = CreateGoal(position, spriteIndex);

	if (!newGoal->CanSpawnHere(position, *this))
	{
		delete newGoal;
		return nullptr;
	}
	else
	{
		entities.emplace_back(newGoal);
		return newGoal;
	}
}


Bug* Game::CreateBug(Vector2 position, int spriteIndex)
{
	Bug* newBug = new Bug(position);
	newBug->spriteIndex = spriteIndex;

	//TODO: How to make this work for doors that will be related to other tilesets?
	Animator* anim = new Animator("bug", "idle");

	Vector2 pivotPoint = Vector2(16, 16);
	anim->MapStateToSprite("idle", new Sprite(0, 0, 1, spriteManager, spriteMap["bug"][spriteIndex], renderer, pivotPoint));

	newBug->SetAnimator(anim);

	return newBug;
}

Bug* Game::SpawnBug(Vector2 position, int spriteIndex)
{
	Bug* newBug = CreateBug(position, spriteIndex);

	if (!newBug->CanSpawnHere(position, *this))
	{
		delete newBug;
		return nullptr;
	}
	else
	{
		entities.emplace_back(newBug);
		return newBug;
	}
}


Ether* Game::CreateEther(Vector2 position, int spriteIndex)
{
	Ether* newEther = new Ether(position);
	newEther->spriteIndex = spriteIndex;

	//TODO: How to make this work for doors that will be related to other tilesets?
	Animator* anim = new Animator("ether", "idle");

	Vector2 pivotPoint = Vector2(0, 0);
	anim->MapStateToSprite("idle", new Sprite(0, 0, 1, spriteManager, "assets/sprites/spells/ether.png", renderer, pivotPoint));

	newEther->SetAnimator(anim);

	return newEther;
}

Ether* Game::SpawnEther(Vector2 position, int spriteIndex)
{
	Ether* newEther = CreateEther(position, spriteIndex);

	if (!newEther->CanSpawnHere(position, *this))
	{
		delete newEther;
		return nullptr;
	}
	else
	{
		entities.emplace_back(newEther);
		return newEther;
	}
}


Block* Game::CreateBlock(Vector2 position, int spriteIndex)
{
	Block* newBlock = new Block(position);
	//newBlock->spriteIndex = spriteIndex;

	//TODO: How to make this work for doors that will be related to other tilesets?
	Animator* anim = new Animator("block", "idle");

	Vector2 pivotPoint = Vector2(24, 32);
	anim->MapStateToSprite("idle", new Sprite(0, 0, 1, spriteManager, "assets/sprites/objects/big_block.png", renderer, pivotPoint));

	newBlock->SetAnimator(anim);

	return newBlock;
}

Block* Game::SpawnBlock(Vector2 position, int spriteIndex)
{
	Block* newBlock = CreateBlock(position, spriteIndex);

	if (!newBlock->CanSpawnHere(position, *this))
	{
		delete newBlock;
		return nullptr;
	}
	else
	{
		entities.emplace_back(newBlock);
		return newBlock;
	}
}



Platform* Game::CreatePlatform(Vector2 position, int spriteIndex)
{
	Platform* newPlatform = new Platform(position);
	newPlatform->spriteIndex = spriteIndex;

	//TODO: How to make this work for doors that will be related to other tilesets?
	Animator* anim = new Animator("platform", "idle");

	Vector2 pivotPoint = Vector2(36, 12);
	anim->MapStateToSprite("idle", new Sprite(0, 0, 1, spriteManager, "assets/sprites/objects/platform.png", renderer, pivotPoint));

	newPlatform->SetAnimator(anim);

	return newPlatform;
}

Platform* Game::SpawnPlatform(Vector2 position, int spriteIndex)
{
	Platform* newPlatform = CreatePlatform(position, spriteIndex);

	if (!newPlatform->CanSpawnHere(position, *this))
	{
		delete newPlatform;
		return nullptr;
	}
	else
	{
		entities.emplace_back(newPlatform);
		return newPlatform;
	}
}


Missile* Game::SpawnMissile(Vector2 position, Vector2 velocity, float angle)
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

	return missile;
}

Vector2 Game::CalcTileSpawnPos(Vector2 pos)
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
	Vector2 newTilePos = position;// CalcObjPos(position);

	//Sprite* tileSprite = new Sprite(Vector2(newTileX, newTileY), spriteManager.GetImage(tilesheet), renderer);
	Tile* tile = new Tile(newTilePos, frame, spriteManager->GetImage(renderer, tilesheet), renderer);

	tile->layer = drawingLayer;
	tile->impassable = drawingLayer == DrawingLayer::COLLISION;
	//tile->etype = "tile";
	//tile->tileCoordinates = frame;
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
	Player* player = new Player(position);
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

	// Spell animations
	anim1->MapStateToSprite("PUSH", new Sprite(7, spriteManager, "assets/sprites/kaneko/wdk_push.png", renderer, Vector2(21, 26)));


	player->SetAnimator(anim1);
	player->SetPosition(position);
	player->startPosition = position;

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
		editor->propertyIndex = -1;
		editor->DoAction();
	}
	else if (inputType == "new_level")
	{
		editor->showDialogPopup = false;
		editor->SaveLevel(inputText);
	}
	else if (inputType == "load_file_as")
	{
		editor->showDialogPopup = false;
		editor->InitLevelFromFile(inputText);
	}
}

void Game::LoadTitleScreen()
{
	openedMenus.clear();
	editor->InitLevelFromFile("title");
	openedMenus.emplace_back(allMenus["Title"]);

	soundManager->PlayBGM("Witchs_Waltz");
}

void Game::LoadNextLevel()
{
	levelNumber++; //TODO: What should we do with this number? Should we always increase it, or not?

	if (nextLevel == "")
		editor->InitLevelFromFile("test" + std::to_string(levelNumber));
	else
		editor->InitLevelFromFile(nextLevel);
}

void Game::PlayLevel(string levelName)
{
	openedMenus.clear();
	editor->InitLevelFromFile(levelName);

	//TODO: Load different music based on each level

	soundManager->PlayBGM("Forest");
}

bool Game::CheckInputs()
{
	// Reset all inputs here
	pressedDebugButton = false;
	pressedSpellButton = false;
	pressedLeftTrigger = false;
	pressedRightTrigger = false;
	//clickedMouse = false;

	bool quit = false;

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

	return quit;
}

void Game::CheckDeleteEntities()
{
	// Destroy entities before we update them
	unsigned int k = 0;
	while (k < entities.size())
	{
		if (entities[k]->shouldDelete)
			DeleteEntity(k);
		else
			k++;
	}
}

void Game::HandleEditMode()
{
	//TODO: Make this a function
	const Uint8* currentKeyStates = SDL_GetKeyboardState(NULL);
	float cameraSpeed = 1.0f;

	if (currentKeyStates[SDL_SCANCODE_UP] || currentKeyStates[SDL_SCANCODE_W])
	{
		camera.y -= (editor->GRID_SIZE * Renderer::GetScale());
	}
	else if (currentKeyStates[SDL_SCANCODE_DOWN] || currentKeyStates[SDL_SCANCODE_S])
	{
		camera.y += (editor->GRID_SIZE * Renderer::GetScale());
	}

	if (currentKeyStates[SDL_SCANCODE_LEFT] || currentKeyStates[SDL_SCANCODE_A])
	{
		camera.x -= (editor->GRID_SIZE * Renderer::GetScale());

	}
	else if (currentKeyStates[SDL_SCANCODE_RIGHT] || currentKeyStates[SDL_SCANCODE_D])
	{
		camera.x += (editor->GRID_SIZE * Renderer::GetScale());
	}

	editor->HandleEdit();
}

void Game::EscapeMenu()
{
	if (openedMenus.size() > 0)
	{
		MenuScreen* currentMenu = openedMenus.back();

		//TODO: Maybe have a Screen base class with virtual OnPop, OnPush functions
		//TOOD: Maybe instead, we limit the size to 1 and put a GUI in here
		if (currentMenu->name == "Title")
			return;

		// Resume time when unpausing
		if (currentMenu->name == "Pause")
		{
			Uint32 ticks = SDL_GetTicks();
			for (unsigned int i = 0; i < entities.size(); i++)
				entities[i]->Unpause(ticks);
		}

		// Close the current menu
		openedMenus.pop_back();

		// Open a menu after closing this one
		if (currentMenu->name == "File Select")
		{
			openedMenus.emplace_back(allMenus["Title"]);
		}

		if (currentMenu->name == "Settings")
		{
			if (currentLevel == "title")
				openedMenus.emplace_back(allMenus["Title"]);
		}
	}
}


void Game::SaveEditorSettings()
{
	std::ofstream fout;
	fout.open("data/editor.config");

	// Autoreplace = When we click to place a tile, automatically replace the one that was there before it
	// Or if this setting is off, then you must right-click to delete it first (can only place when empty)
	fout << "replacing " << editor->replaceSettingIndex << std::endl;

	// Autodelete = When we right click to delete, you do not need to be on the same layer,
	// instead, it just deletes whatever is in the highest layer first
	fout << "deleting " << editor->deleteSettingIndex << std::endl;

	// Color palette for the buttons
	fout << "colors " << editor->colorSettingIndex << std::endl;

	// The currently selected tilesheet
	fout << "tilesheet " << editor->tilesheetIndex << std::endl;

	//fout << "display_fps " << showFPS << std::endl;
	//fout << "display_timer " << showTimer << std::endl;
	//fout << "language " << soundManager->soundVolumeIndex << std::endl;

	fout.close();
}


void Game::LoadEditorSettings()
{
	std::ifstream fin;
	fin.open("data/editor.config");

	char line[256];
	fin.getline(line, 256);

	while (fin.good())
	{
		std::istringstream buf(line);
		std::istream_iterator<std::string> beg(buf), end;
		std::vector<std::string> tokens(beg, end);

		if (tokens[0] == "replacing")
		{
			editor->replaceSettingIndex = std::stoi(tokens[1]);
			SettingsButton* button = dynamic_cast<SettingsButton*>(allMenus["EditorSettings"]->
				GetButtonByName("Replacing"));
			button->selectedOption = editor->replaceSettingIndex;
		}
		else if (tokens[0] == "deleting")
		{
			editor->deleteSettingIndex = std::stoi(tokens[1]);
			SettingsButton* button = dynamic_cast<SettingsButton*>(allMenus["EditorSettings"]->
				GetButtonByName("Deleting"));
			button->selectedOption = editor->deleteSettingIndex;
		}
		else if (tokens[0] == "colors")
		{
			editor->colorSettingIndex = std::stoi(tokens[1]);
			SettingsButton* button = dynamic_cast<SettingsButton*>(allMenus["EditorSettings"]->
				GetButtonByName("Button Color"));
			button->selectedOption = editor->colorSettingIndex;
		}
		else if (tokens[0] == "tilesheet")
		{
			editor->tilesheetIndex = std::stoi(tokens[1]);
		}

		fin.getline(line, 256);
	}

	fin.close();
}

void Game::SaveSettings()
{
	std::ofstream fout;
	fout.open("data/settings.config");

	fout << "music_volume " << soundManager->bgmVolumeIndex << std::endl;
	fout << "sound_volume " << soundManager->soundVolumeIndex << std::endl;
	fout << "screen_resolution " << isFullscreen << std::endl;
	fout << "display_fps " << showFPS << std::endl;
	fout << "display_timer " << showTimer << std::endl;
	//fout << "language " << soundManager->soundVolumeIndex << std::endl;

	fout.close();
}

void Game::LoadSettings()
{
	std::ifstream fin;
	fin.open("data/settings.config");

	char line[256];
	fin.getline(line, 256);

	while (fin.good())
	{
		std::istringstream buf(line);
		std::istream_iterator<std::string> beg(buf), end;
		std::vector<std::string> tokens(beg, end);

		if (tokens[0] == "music_volume")
		{
			soundManager->SetVolumeBGM(std::stoi(tokens[1]));

			//TODO: Refactor to avoid the dynamic cast
			SettingsButton* button = dynamic_cast<SettingsButton*>(allMenus["Settings"]->GetButtonByName("Music Volume"));
			button->selectedOption = std::stoi(tokens[1]);
		}
		else if (tokens[0] == "sound_volume")
		{
			soundManager->SetVolumeSound(std::stoi(tokens[1]));

			//TODO: Refactor to avoid the dynamic cast
			SettingsButton* button = dynamic_cast<SettingsButton*>(allMenus["Settings"]->GetButtonByName("Sound Volume"));
			button->selectedOption = std::stoi(tokens[1]);
		}
		else if (tokens[0] == "screen_resolution")
		{
			isFullscreen = std::stoi(tokens[1]);

			if (isFullscreen)
				SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
			else
				SDL_SetWindowFullscreen(window, 0);				

			//TODO: Refactor to avoid the dynamic cast
			SettingsButton* button = dynamic_cast<SettingsButton*>(allMenus["Settings"]->GetButtonByName("Screen Resolution"));
			button->selectedOption = std::stoi(tokens[1]);
		}
		else if (tokens[0] == "display_fps")
		{
			showFPS = std::stoi(tokens[1]);

			//TODO: Refactor to avoid the dynamic cast
			SettingsButton* button = dynamic_cast<SettingsButton*>(allMenus["Settings"]->GetButtonByName("Display FPS"));
			button->selectedOption = std::stoi(tokens[1]);
		}
		else if (tokens[0] == "display_timer")
		{
			showTimer = std::stoi(tokens[1]);

			//TODO: Refactor to avoid the dynamic cast
			SettingsButton* button = dynamic_cast<SettingsButton*>(allMenus["Settings"]->GetButtonByName("Display Timer"));
			button->selectedOption = std::stoi(tokens[1]);
		}
		else if (tokens[0] == "language")
		{
			//TODO: Deal with this later

			//TODO: Refactor to avoid the dynamic cast
			SettingsButton* button = dynamic_cast<SettingsButton*>(allMenus["Settings"]->GetButtonByName("Language"));
			button->selectedOption = std::stoi(tokens[1]);
		}

		fin.getline(line, 256);
	}

	fin.close();
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
				EscapeMenu();
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
			//TODO: Find a way to map these keys to customizable buttons and controllers
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
			case SDLK_c:
				pressedDebugButton = true;
				break;
			case SDLK_v:
				pressedSpellButton = true;
				break;
			case SDLK_q:
				pressedLeftTrigger = true;
				break;
			case SDLK_e:
				pressedRightTrigger = true;
				break;
			case SDLK_r:
				//if (player != nullptr)
				//	player->ResetPosition();
				editor->InitLevelFromFile(currentLevel);
				break;
			case SDLK_1: // toggle Debug mode
				SetModeDebug(!GetModeDebug());
				break;
			case SDLK_2: // toggle Editor mode
				SetModeEdit(!GetModeEdit());
				if (GetModeEdit())
				{
					camera.x = camera.x - ((int)camera.x % (editor->GRID_SIZE * Renderer::GetScale()));
					camera.y = camera.y - ((int)camera.y % (editor->GRID_SIZE * Renderer::GetScale()));
					editor->StartEdit();
				}
				else
				{
					editor->StopEdit();
				}
				break;
			case SDLK_3: // toggle Editor settings
				if (GetModeEdit())
				{
					openedMenus.emplace_back(allMenus["EditorSettings"]);
				}
				break;
			case SDLK_4: // Undo Button
				editor->UndoAction();
				break;
			case SDLK_5: // Redo Button
				editor->RedoAction();
				break;
			case SDLK_7: // TODO: Zoom camera button
				if (Renderer::GetScale() == 2)
					Renderer::SetScale(1);
				else
					Renderer::SetScale(2);
				break;
			default:
				break;
			}
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

void Game::GetMenuInput()
{
	Uint32 ticks = timer.GetTicks();
	if (ticks > lastPressedKeyTicks + 100) //TODO: Check for overflow errors
	{
		// If we have pressed any key on the menu, add a delay between presses
		if (openedMenus[openedMenus.size() - 1]->Update(*this))
			lastPressedKeyTicks = ticks;
	}
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
	// For non-moving camera, set offset based on tile size and scale
	const int OFFSET = -4;
	camera = Vector2(0, OFFSET * TILE_SIZE * Renderer::GetScale());
	//camera = Vector2(0, 0);
	//std::cout << camera.y << std::endl;

	if (player != nullptr)
	{
		// Update the camera
		//camera = player->GetCenter();
		//camera.x -= (screenWidth / 2.0f);
		//camera.y -= (screenHeight / 2.0f);
	}
	else // to handle the title screen (maybe change this later)
	{
		//camera = Vector2(0, 0);
	}

	if (changingOverlayColor && timerOverlayColor.HasElapsed())
	{
		timerOverlayColor.Start(1);
		changingOverlayColor = false;
		UpdateOverlayColor(overlayColor.r, targetColor.r);
		UpdateOverlayColor(overlayColor.g, targetColor.g);
		UpdateOverlayColor(overlayColor.b, targetColor.b);
		UpdateOverlayColor(overlayColor.a, targetColor.a);
		std::cout << overlayColor.r << std::endl;
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

	// Draw stuff for debugging purposes here
	if (GetModeDebug())
	{
		SDL_SetRenderDrawColor(renderer->renderer, 255, 255, 0, 255);
		for (unsigned int i = 0; i < debugRectangles.size(); i++)
		{
			SDL_RenderDrawRect(renderer->renderer, debugRectangles[i]);
		}
		SDL_SetRenderDrawColor(renderer->renderer, 255, 255, 255, 255);
	}

	if (showFPS)
		fpsText->Render(renderer);
	
	if (showTimer)
		timerText->Render(renderer);

	if (currentLevel != "title")
	{
		bugText->Render(renderer);
		etherText->Render(renderer);
	}	

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
		SDL_SetRenderDrawBlendMode(renderer->renderer, SDL_BLENDMODE_BLEND);
		SDL_SetRenderDrawColor(renderer->renderer, 0, 0, 0, 128);
		SDL_RenderFillRect(renderer->renderer, &overlayRect);
		SDL_SetRenderDrawBlendMode(renderer->renderer, SDL_BLENDMODE_NONE);
		openedMenus[openedMenus.size() - 1]->Render(renderer);
	}

	SDL_SetRenderDrawColor(renderer->renderer, 0, 0, 0, 255);
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