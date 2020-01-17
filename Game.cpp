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

#include <stdio.h>
#include <time.h>

using std::string;

Game::Game()
{
	startOfGame = std::chrono::steady_clock::now();

	InitSDL();

	spriteManager = new SpriteManager();

	InitOpenGL();

	// Initialize the font before all text
	theFont = TTF_OpenFont("fonts/default.ttf", 20);
	headerFont = TTF_OpenFont("fonts/default.ttf", 32);

	soundManager = new SoundManager();

	// Initialize the cutscene stuff (do this AFTER renderer and sprite manager)
	cutscene = new CutsceneManager(*this);
	cutscene->ParseScene();

	overlayRect.x = 0;
	overlayRect.y = 0;
	overlayRect.w = screenWidth;
	overlayRect.h = screenHeight;

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

	spriteMap["shroom"].push_back("assets/sprites/objects/shroom.png");
	spriteMap["shroom"].push_back("assets/sprites/objects/shroom_potted.png");

	editor = new Editor(*this);
	entities.clear();

	ResetText();

	// Initialize all the menus
	allMenus["Title"] = new MenuScreen("Title", *this);
	allMenus["File Select"] = new MenuScreen("File Select", *this);
	allMenus["Pause"] = new MenuScreen("Pause", *this);
	allMenus["Settings"] = new MenuScreen("Settings", *this);
	allMenus["Spellbook"] = new MenuScreen("Spellbook", *this);
	allMenus["EditorSettings"] = new MenuScreen("EditorSettings", *this);

	timerOverlayColor.Start(1);

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

	now = std::chrono::duration<float, milliseconds::period>(start_time - startOfGame).count();
	renderer->now = now;

	// When we are debugging and hit a breakpoint in an IDE, the timer continues running.
	// This causes the dt to become huge and throw everything around like crazy.
	// So reset the dt if it becomes too big so that we can debug properly.
	if (dt > 100)
		dt = 33;
}

void Game::CreateObjects()
{
	Sprite* spr1 = new Sprite(0, 1, 1, spriteManager, "Textures/but_defa1.png", renderer->shaders["special"], Vector2(0,0), true);
	Sprite* spr2 = new Sprite(0, 1, 1, spriteManager, "Textures/street_1a.jpg", renderer->shaders["default"], Vector2(0,0), false);
	Sprite* spr3 = new Sprite(0, 5, 6, spriteManager, "Textures/wdk_walk.png", renderer->shaders["default"], Vector2(0,0), true);


	Entity* butler = new Entity(Vector2(0, 0), spr1);
	Entity* bg = new Entity(Vector2(0, 0), spr2);
	Entity* kaneko = new Entity(Vector2(0, 0), spr3);

	kaneko->GetSprite()->numberFramesInTexture = 12;
	kaneko->GetSprite()->framesPerRow = 6;
	kaneko->GetSprite()->frameHeight = kaneko->GetSprite()->texture->GetHeight() / (kaneko->GetSprite()->numberFramesInTexture / kaneko->GetSprite()->framesPerRow);

	//entities.push_back(bg);
	//entities.push_back(butler);	
	entities.push_back(kaneko);
	//entities.push_back(cube);
}

void Game::CreateShaders()
{
	ShaderProgram* shader0 = new ShaderProgram();
	shader0->CreateFromFiles("Shaders/shader0.vert", "Shaders/shader0.frag");
	renderer->shaders["default"] = shader0;

	ShaderProgram* shader1 = new ShaderProgram();
	shader1->CreateFromFiles("Shaders/shader1.vert", "Shaders/shader1.frag");
	renderer->shaders["special"] = shader1;

	ShaderProgram* shader2 = new ShaderProgram();
	shader2->CreateFromFiles("Shaders/multiply.vert", "Shaders/multiply.frag");
	renderer->shaders["multiply"] = shader2;
}

void Game::InitOpenGL()
{
	
	// Set up OpenGL stuff
	mainContext = SDL_GL_CreateContext(window);

	// Set our OpenGL version.
	// SDL_GL_CONTEXT_CORE gives us only the newer version, deprecated functions are disabled
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

	// 3.2 is part of the modern versions of OpenGL, but most video cards whould be able to run it
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

	// Turn on double buffering with a 24bit Z buffer.
	// You may need to change this to 16 or 32 for your system
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	// 0 = no vsync, 1 = vsync
	SDL_GL_SetSwapInterval(1);

	glewExperimental = GL_TRUE;
	glewInit();

	glDisable(GL_DEPTH_TEST);
	//glDepthFunc(GL_LESS);

	// Enable blending for transparent textures
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Set up viewport size 
	// TODO: Don't hardcode these numbers
	glViewport(0, 0, 1280, 720);

	SDL_GL_SwapWindow(window);

	renderer->camera = Camera(glm::vec3(0.0f, 0.0f, 0.0f), 
		glm::vec3(0.0f, 1.0f, 0.0f), 90.0f, 0.0f, 1.0f, 0.5f, 1.0f);

	CreateShaders(); // we must create the shaders at this point
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
	return true;
}

Ladder* Game::CreateLadder(Vector2 position, int spriteIndex)
{
	Ladder* newLadder = new Ladder(position);
	newLadder->spriteIndex = spriteIndex;

	std::vector<AnimState*> animStates;

	//ReadAnimData("data/animations/ladder.anim", animStates);

	animStates.push_back(new AnimState("middle", 0, new Sprite(2, 2, 24, 24, spriteManager,
		spriteMap["ladder"][spriteIndex], renderer->shaders["default"], Vector2(0, 0))));

	animStates.push_back(new AnimState("bottom", 0, new Sprite(4, 4, 24, 24, spriteManager,
		spriteMap["ladder"][spriteIndex], renderer->shaders["default"], Vector2(0, 0))));

	animStates.push_back(new AnimState("top", 0, new Sprite(0, 0, 24, 24, spriteManager,
		spriteMap["ladder"][spriteIndex], renderer->shaders["default"], Vector2(0, 0))));

	Animator* anim = new Animator("ladder", animStates, "middle");
	newLadder->SetAnimator(anim);

	return newLadder;
}

Vector2 Game::SnapToGrid(Vector2 position)
{
	int x = position.x - ((int)(position.x) % (editor->GRID_SIZE));
	int y = position.y - ((int)(position.y) % (editor->GRID_SIZE));

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

	Vector2 pivotPoint = Vector2(0, 0);
	std::vector<AnimState*> animStates;
	//ReadAnimData("data/animations/door.anim", animStates);
	animStates.push_back(new AnimState("closed", 100, new Sprite(0, 0, 48, 96, spriteManager, spriteMap["door"][spriteIndex], renderer->shaders["default"], pivotPoint)));
	animStates.push_back(new AnimState("opened", 100, new Sprite(1, 1, 48, 96, spriteManager, spriteMap["door"][spriteIndex], renderer->shaders["default"], pivotPoint)));
	
	//TODO: How to make this work for doors that will be related to other tilesets?
	Animator* anim = new Animator("door", animStates, "closed");
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

	Vector2 pivotPoint = Vector2(0, 0);
	std::vector<AnimState*> animStates;

	if (name == "gramps")
	{
		//ReadAnimData("data/animations/gramps.anim", animStates);
		pivotPoint = Vector2(12, 28);
		animStates.push_back(new AnimState("idle", 100, new Sprite(0, 0, 27, 46, spriteManager, spriteMap["npc"][spriteIndex], renderer->shaders["default"], pivotPoint)));
		animStates.push_back(new AnimState("sad", 100, new Sprite(1, 1, 27, 46, spriteManager, spriteMap["npc"][spriteIndex], renderer->shaders["default"], pivotPoint)));
		animStates.push_back(new AnimState("confused", 100, new Sprite(2, 2, 27, 46, spriteManager, spriteMap["npc"][spriteIndex], renderer->shaders["default"], pivotPoint)));
		
	}
	else if (name == "the_man")
	{
		//ReadAnimData("data/animations/the_man.anim", animStates);
		pivotPoint = Vector2(23, 36);
		animStates.push_back(new AnimState("idle", 200, new Sprite(0, 7, 50, 70, spriteManager, spriteMap["npc"][spriteIndex], renderer->shaders["default"], pivotPoint)));
	}

	Animator* anim = new Animator(name, animStates, "idle");
	newNPC->SetAnimator(anim);
	newNPC->trigger = true;

	float w = anim->GetCurrentSprite()->frameWidth;
	float h = anim->GetCurrentSprite()->frameHeight;
	newNPC->ChangeCollider(0, 0, w, h);

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

	Vector2 pivotPoint = Vector2(0, 0);
	std::vector<AnimState*> animStates;
	//ReadAnimData("data/animations/goal.anim", animStates);
	animStates.push_back(new AnimState("closed", 100, new Sprite(0, 0, 48, 96, spriteManager, spriteMap["goal"][spriteIndex], renderer->shaders["default"], pivotPoint)));
	animStates.push_back(new AnimState("opened", 100, new Sprite(1, 1, 48, 96, spriteManager, spriteMap["goal"][spriteIndex], renderer->shaders["default"], pivotPoint)));
	
	Animator* anim = new Animator("door", animStates, "closed");
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

	Vector2 pivotPoint = Vector2(16, 16);
	std::vector<AnimState*> animStates;
	//ReadAnimData("data/animations/bug.anim", animStates);
	animStates.push_back(new AnimState("idle", 100, new Sprite(0, 0, 32, 32, spriteManager, spriteMap["bug"][spriteIndex], renderer->shaders["default"], pivotPoint)));
	Animator* anim = new Animator("bug", animStates, "idle");
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

	Vector2 pivotPoint = Vector2(0, 0);
	std::vector<AnimState*> animStates;
	//ReadAnimData("data/animations/ether.anim", animStates);
	animStates.push_back(new AnimState("idle", 100, new Sprite(0, 0, 1, spriteManager, "assets/sprites/spells/ether.png", renderer->shaders["default"], pivotPoint)));

	Animator* anim = new Animator("ether", animStates, "idle");
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

	Vector2 pivotPoint = Vector2(24, 32);
	std::vector<AnimState*> animStates;
	//ReadAnimData("data/animations/block.anim", animStates);
	animStates.push_back(new AnimState("idle", 100, new Sprite(0, 0, 1, spriteManager, "assets/sprites/objects/big_block.png", renderer->shaders["default"], pivotPoint)));
	
	Animator* anim = new Animator("block", animStates, "idle");
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
	

	Vector2 pivotPoint = Vector2(36, 12);
	std::vector<AnimState*> animStates;
	//ReadAnimData("data/animations/platform.anim", animStates);
	animStates.push_back(new AnimState("idle", 100, new Sprite(0, 0, 1, spriteManager, "assets/sprites/objects/platform.png", renderer->shaders["default"], pivotPoint)));

	Animator* anim = new Animator("platform", animStates, "idle");
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

Shroom* Game::CreateShroom(Vector2 position, int spriteIndex)
{
	Shroom* newObject = new Shroom(position);
	newObject->spriteIndex = spriteIndex;
	
	Vector2 pivotPoint = Vector2(15, 30);

	std::vector<AnimState*> animStates;
	//ReadAnimData("data/animations/shroom.anim", animStates);
	animStates.push_back(new AnimState("idle", 200, new Sprite(0, 3, 32, 48, spriteManager, 
		spriteMap["shroom"][spriteIndex], renderer->shaders["default"], pivotPoint)));

	newObject->SetAnimator(new Animator("shroom", animStates));
	newObject->canBePushed = (spriteIndex == 1); // can push if it is in the pot

	return newObject;
}

Shroom* Game::SpawnShroom(Vector2 position, int spriteIndex)
{
	Shroom* newObject = CreateShroom(position, spriteIndex);

	if (!newObject->CanSpawnHere(position, *this))
	{
		delete newObject;
		return nullptr;
	}
	else
	{
		entities.emplace_back(newObject);
		return newObject;
	}
}

Missile* Game::SpawnMissile(Vector2 position, Vector2 velocity, float angle)
{
	//TODO: Make a way for this to return false

	Vector2 pivotPoint = Vector2(14, 7);

	std::vector<AnimState*> animStates;
	//ReadAnimData("data/animations/missile.anim", animStates);
	animStates.push_back(new AnimState("moving", 100, new Sprite(0, 3, 23, 16, spriteManager, "assets/sprites/spells/debug_missile.png", renderer->shaders["default"], pivotPoint)));
	animStates.push_back(new AnimState("destroyed", 100, new Sprite(4, 7, 23, 16, spriteManager, "assets/sprites/spells/debug_missile.png", renderer->shaders["default"], pivotPoint, false)));

	Missile* missile = new Missile(position - pivotPoint);

	Animator* anim = new Animator("debug_missile", animStates, "moving");
	anim->SetBool("destroyed", false);

	missile->SetAnimator(anim);
	missile->SetVelocity(velocity);
	missile->angle = angle;
	missile->GetAnimator()->SetState("moving");

	entities.emplace_back(missile);

	return missile;
}

Vector2 Game::CalcTileSpawnPos(Vector2 pos)
{
	int newTileX = (int)pos.x;
	int newTileY = (int)pos.y;

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
	Tile* tile = new Tile(newTilePos, frame, spriteManager->GetImage(tilesheet), renderer);

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

//TODO: Only read this data once at the beginning and then store it for lookup later
void Game::ReadAnimData(std::string dataFilePath, std::vector<AnimState*> & animStates)
{
	// Get anim data from the file
	std::ifstream fin;
	fin.open(dataFilePath);

	std::string animData = "";
	for (std::string line; std::getline(fin, line); )
	{
		animData += line + "\n";
	}

	fin.close();

	// Go through the data and add all states

	std::stringstream ss{ animData };

	char lineChar[256];
	ss.getline(lineChar, 256);

	while (ss.good() && !ss.eof())
	{
		std::istringstream buf(lineChar);
		std::istream_iterator<std::string> beg(buf), end;
		std::vector<std::string> tokens(beg, end);

		int index = 0;
		std::string stateName = tokens[index++];
		int stateSpeed = std::stoi(tokens[index++]);
		int spriteStartFrame = std::stoi(tokens[index++]);
		int spriteEndFrame = std::stoi(tokens[index++]);
		int spriteFrameWidth = std::stoi(tokens[index++]);
		int spriteFrameHeight = std::stoi(tokens[index++]);
		std::string spriteFilePath = tokens[index++];
		int spritePivotX = std::stoi(tokens[index++]);
		int spritePivotY = std::stoi(tokens[index++]);

		animStates.push_back(new AnimState(stateName, stateSpeed,
			new Sprite(spriteStartFrame, spriteEndFrame, spriteFrameWidth, spriteFrameHeight,
				spriteManager, spriteFilePath, renderer->shaders["default"], Vector2(spritePivotX, spritePivotY))));

		ss.getline(lineChar, 256);
	}
}

Player* Game::SpawnPlayer(Vector2 position)
{
	Player* player = new Player(position);
	player->game = this;

	std::vector<AnimState*> animStates;
	ReadAnimData("data/animations/player.anim", animStates);

	Animator* anim1 = new Animator("kaneko", animStates, "idle");
	anim1->SetBool("isGrounded", true);
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
		//camera.y -= (editor->GRID_SIZE * Renderer::GetScale());
	}
	else if (currentKeyStates[SDL_SCANCODE_DOWN] || currentKeyStates[SDL_SCANCODE_S])
	{
		//camera.y += (editor->GRID_SIZE * Renderer::GetScale());
	}

	if (currentKeyStates[SDL_SCANCODE_LEFT] || currentKeyStates[SDL_SCANCODE_A])
	{
		//camera.x -= (editor->GRID_SIZE * Renderer::GetScale());

	}
	else if (currentKeyStates[SDL_SCANCODE_RIGHT] || currentKeyStates[SDL_SCANCODE_D])
	{
		//camera.x += (editor->GRID_SIZE * Renderer::GetScale());
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
			else if (event.key.keysym.sym == SDLK_DOWN)
			{
				std::string optionString = editor->GetCurrentPropertyOptionString(1);
				if (optionString != "")
				{
					inputText = optionString;
					UpdateTextInput();
				}
			}
			else if (event.key.keysym.sym == SDLK_UP)
			{
				std::string optionString = editor->GetCurrentPropertyOptionString(-1);
				if (optionString != "")
				{
					inputText = optionString;
					UpdateTextInput();
				}
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
			case SDLK_t:
				LoadNextLevel();
				break;
			case SDLK_1: // toggle Debug mode
				SetModeDebug(!GetModeDebug());
				break;
			case SDLK_2: // toggle Editor mode
				SetModeEdit(!GetModeEdit());
				if (GetModeEdit())
				{
					//camera.x = camera.x - ((int)camera.x % (editor->GRID_SIZE * Renderer::GetScale()));
					//camera.y = camera.y - ((int)camera.y % (editor->GRID_SIZE * Renderer::GetScale()));
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
			case SDLK_6: // Screenshot Button
				SaveScreenshot();
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

void Game::SaveScreenshot()
{
	const std::string counterfilepath = "screenshots/counter.txt";

	std::ifstream fin;
	fin.open(counterfilepath);
	std::string timestamp = "";
	fin >> timestamp;
	fin.close();

	std::ofstream fout;
	fout.open(counterfilepath);
	fout << (std::stoi(timestamp) + 1) << std::endl;
	fout.close();
	
	//TODO: Can we get this working based on a date time?

	std::string filepath = "screenshots/screenshot-" + timestamp + ".bmp";

	const unsigned int bytesPerPixel = 3;

	unsigned char* pixels = new unsigned char[screenWidth * screenHeight * bytesPerPixel]; // 4 bytes for RGBA
	glReadPixels(0, 0, screenHeight, screenHeight, GL_RGB, GL_UNSIGNED_BYTE, pixels);

	SDL_Surface* screenshot = SDL_CreateRGBSurfaceFrom(pixels, screenWidth, screenHeight, 8 * bytesPerPixel, screenWidth * bytesPerPixel, 0, 0, 0, 0);

	//SDL_Surface * screenshot = SDL_CreateRGBSurface(0, screenWidth, screenHeight, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000);
	//SDL_RenderReadPixels(renderer->renderer, NULL, SDL_PIXELFORMAT_ARGB8888, screenshot->pixels, screenshot->pitch);
	
	SDL_SaveBMP(screenshot, filepath.c_str());
	SDL_FreeSurface(screenshot);

	delete[] pixels;
}

void Game::GetMenuInput()
{
	const Uint8* input = SDL_GetKeyboardState(NULL);
	renderer->camera.KeyControl(input, dt);

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
	const Uint8* input = SDL_GetKeyboardState(NULL);
	renderer->camera.KeyControl(input, dt);

	// For non-moving camera, set offset based on tile size and scale
	const int OFFSET = -4;
	//camera = Vector2(0, OFFSET * TILE_SIZE * Renderer::GetScale());
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

void Game::RenderEntities(glm::mat4 projection, std::vector<Entity*> renderedEntities)
{
	
}

void Game::Render()
{
	// Clear window
	glClearColor(0.0f, 0.0f, 0.2f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Render editor grid
	if (GetModeEdit())
	{
		//editor->DrawGrid();
	}

	// Render all backgrounds and their layers
	for (int i = 0; i < backgrounds.size(); i++)
	{
		for (unsigned int k = 0; k < backgrounds[i]->layers.size(); k++)
		{
			backgrounds[i]->layers[k]->Render(renderer);
		}
	}	

	// Render all entities
	for (unsigned int i = 0; i < entities.size(); i++)
	{
		entities[i]->Render(renderer);
	}

	// LAST THING
	// Render all menu screens
	if (openedMenus.size() > 0)
	{
		openedMenus[openedMenus.size() - 1]->Render(renderer);
	}

	/*
	

	// Render editor toolbox
	if (GetModeEdit())
	{
		editor->Render(renderer, uniformModel);
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
		fpsText->Render(renderer, uniformModel);
	
	if (showTimer)
		timerText->Render(renderer, uniformModel);

	if (currentLevel != "title")
	{
		bugText->Render(renderer, uniformModel);
		etherText->Render(renderer, uniformModel);
	}	

	// Draw the screen overlay
	SDL_SetRenderDrawBlendMode(renderer->renderer, SDL_BLENDMODE_BLEND);
	SDL_SetRenderDrawColor(renderer->renderer, overlayColor.r, overlayColor.g, overlayColor.b, overlayColor.a);
	SDL_RenderFillRect(renderer->renderer, &overlayRect);
	SDL_SetRenderDrawColor(renderer->renderer, 0, 0, 0, 255);
	SDL_SetRenderDrawBlendMode(renderer->renderer, SDL_BLENDMODE_NONE);

	if (watchingCutscene)
		cutscene->Render(renderer);



		*/

	glUseProgram(0);
	SDL_GL_SwapWindow(window);
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