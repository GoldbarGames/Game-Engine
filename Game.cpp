#include "Game.h"
#include "Sprite.h"
#include "Player.h"
#include "Tile.h"
#include "SettingsButton.h"
#include "globals.h"
#include <sstream>
#include <iterator>
#include <ctype.h>

#include <stdio.h>
#include <time.h>
#include "sdl_helpers.h"
#include "PhysicsComponent.h"
#include "Dialog.h"

using std::string;

Mesh* Game::CreateSpriteMesh()
{
	unsigned int quadIndices[] = {
		0, 3, 1,
		1, 3, 2,
		2, 3, 0,
		0, 1, 2
	};

	GLfloat quadVertices[] = {
		-1.0f, -1.0f, 0.0f,  1.0f, 0.0f,
		1.0f, -1.0f, 0.0f,   0.0f, 0.0f,
		-1.0f, 1.0f, 0.0f,   1.0f, 1.0f,
		1.0f, 1.0f, 0.0f,    0.0f, 1.0f
	};

	Mesh* mesh = new Mesh();
	mesh->CreateMesh(quadVertices, quadIndices, 20, 12);

	return mesh;
}

Mesh* Game::CreateCubeMesh()
{
	unsigned int quadIndices[] = {
	0, 3, 1,
	1, 3, 2,
	2, 3, 0,
	0, 1, 2
	};

	GLfloat quadVertices[] = {
		-1.0f, -1.0f, 0.0f,  1.0f, 0.0f,
		1.0f, -1.0f, 0.0f,   0.0f, 0.0f,
		-1.0f, 1.0f, 0.0f,   1.0f, 1.0f,
		1.0f, 1.0f, 0.0f,    0.0f, 1.0f
	};

	Mesh* mesh = new Mesh();
	mesh->CreateMesh(quadVertices, quadIndices, 20, 12);

	return mesh;
}

Game::Game()
{
	startOfGame = std::chrono::steady_clock::now();

	logger = new Logger("logs/output.log");

	entityFactory = EntityFactory::Get();

	InitSDL();

	renderer = new Renderer(this);
	spriteManager = new SpriteManager(renderer);

	InitOpenGL();

	//Sprite::mesh = CreateSpriteMesh();
	cubeMesh = CreateCubeMesh();

	// Initialize the font before all text
	theFont = new FontInfo("fonts/space-mono/SpaceMono-Regular.ttf", 24);
	theFont->SetBoldFont("fonts/space-mono/SpaceMono-Bold.ttf");
	theFont->SetItalicsFont("fonts/space-mono/SpaceMono-Italic.ttf");
	theFont->SetBoldItalicsFont("fonts/space-mono/SpaceMono-BoldItalic.ttf");

	//TODO: Header font should be different, at least a bigger size
	headerFont = new FontInfo("fonts/space-mono/SpaceMono-Regular.ttf", 24);
	headerFont->SetBoldFont("fonts/space-mono/SpaceMono-Bold.ttf");
	headerFont->SetItalicsFont("fonts/space-mono/SpaceMono-Italic.ttf");
	headerFont->SetBoldItalicsFont("fonts/space-mono/SpaceMono-BoldItalic.ttf");

	soundManager = new SoundManager();
	soundManager->ReadMusicData("data/bgm.dat");

	// Initialize the cutscene stuff (do this AFTER renderer and sprite manager)
	cutscene = new CutsceneManager(*this);
	cutscene->ParseScene();	

	//ShaderProgram* shader = renderer->shaders[ShaderName::Default];

	// Initialize debug stuff
	renderer->debugSprite = new Sprite(0, 0, 24, 24, spriteManager,
		"assets/editor/rect-outline.png", renderer->shaders[ShaderName::Default], Vector2(0, 0));

	// Initialize overlay sprite
	renderer->overlaySprite = new Sprite(0, 0, 24, 24, spriteManager,
		"assets/gui/white.png", renderer->shaders[ShaderName::Default], Vector2(0, 0));

	// Initialize the sprite map (do this BEFORE the editor)

	//TODO: We don't really need the spriteMap anymore, but we still need the number of sprites
	// (we don't need to know the actual files, just the number of possible choices)

	spriteMap["door"].push_back("assets/sprites/objects/door1.png");
	spriteMap["door"].push_back("assets/sprites/objects/door_house.png");
	spriteMap["door"].push_back("assets/sprites/objects/door_house_outside.png");

	spriteMap["ladder"].push_back("assets/sprites/objects/ladder1.png");
	spriteMap["ladder"].push_back("assets/sprites/objects/ladder_house.png");
	spriteMap["ladder"].push_back("assets/sprites/objects/ladder_b.png");

	spriteMap["bug"].push_back("assets/sprites/bugs/bug1.png");
	spriteMap["bug"].push_back("assets/sprites/bugs/bug2.png");

	spriteMap["goal"].push_back("assets/sprites/objects/door1.png");
	spriteMap["goal"].push_back("assets/sprites/objects/door_house.png");
	spriteMap["goal"].push_back("assets/sprites/objects/door_house_outside.png");

	spriteMap["shroom"].push_back("assets/sprites/objects/shroom.png");
	spriteMap["shroom"].push_back("assets/sprites/objects/shroom_potted.png");

	//TODO: Read these in from a text file
	npcNames = { "gramps", "the_man" };
	enemyNames = { "crawler" };

	for (int i = 0; i < npcNames.size(); i++)
	{
		spriteMap["npc"].push_back("assets/sprites/npcs/" + npcNames[i] + ".png");
	}

	for (int i = 0; i < enemyNames.size(); i++)
	{
		spriteMap["enemy"].push_back("assets/sprites/enemies/" + enemyNames[i] + ".png");
	}

	debugScreen = new DebugScreen(*this);
	
	editor = new Editor(*this);
	
	entities.clear();

	quadTree = new QuadTree();

	ResetText();
	SetScreenResolution(1280, 720);

	// Initialize all the menus
	allMenus["Title"] = new MenuScreen("Title", *this);
	allMenus["File Select"] = new MenuScreen("File Select", *this);
	allMenus["Pause"] = new MenuScreen("Pause", *this);
	allMenus["Settings"] = new MenuScreen("Settings", *this);
	allMenus["Spellbook"] = new MenuScreen("Spellbook", *this);
	allMenus["EditorSettings"] = new MenuScreen("EditorSettings", *this);
	allMenus["Credits"] = new MenuScreen("Credits", *this);

	LoadEditorSettings();

	start_time = clock::now();
}

Game::~Game()
{	
	SaveEditorSettings();
	EndSDL();
}

void Game::ResetText()
{
	if (fpsText == nullptr)
	{
		fpsText = new Text(theFont);
		fpsText->SetText("FPS:");
		fpsText->GetSprite()->keepScaleRelativeToCamera = true;
		fpsText->GetSprite()->keepPositionRelativeToCamera = true;
	}
		
	fpsText->SetText("FPS:");
	fpsText->SetPosition((screenWidth * 2) - (fpsText->GetTextWidth() * 2), fpsText->GetTextHeight());

	if (timerText == nullptr)
	{
		timerText = new Text(theFont);
		timerText->SetText("");
		timerText->GetSprite()->keepScaleRelativeToCamera = true;
		timerText->GetSprite()->keepPositionRelativeToCamera = true;
	}
			
	timerText->SetText("");
	timerText->SetPosition((screenWidth) - (timerText->GetTextWidth() * 2), timerText->GetTextHeight());

	if (bugText == nullptr)
	{
		bugText = new Text(theFont);
		bugText->SetText("");
		bugText->GetSprite()->keepScaleRelativeToCamera = true;
		bugText->GetSprite()->keepPositionRelativeToCamera = true;
	}
	
	//bugText->SetText("Bugs Remaining: " + std::to_string(bugsRemaining));
	//bugText->SetPosition(bugText->GetTextWidth() * 1.25f, bugText->GetTextHeight() * 1.25f);

	if (etherText == nullptr)
	{
		etherText = new Text(theFont);
		etherText->SetText("");
		etherText->GetSprite()->keepScaleRelativeToCamera = true;
		etherText->GetSprite()->keepPositionRelativeToCamera = true;
	}
		
	
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

	glViewport(0, 0, screenWidth, screenHeight);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	SDL_GL_SwapWindow(window);

	bool use2DCamera = true;

	renderer->camera = Camera(glm::vec3(0.0f, 0.0f, 1000.0f),
		glm::vec3(0.0f, 1.0f, 0.0f), 90.0f, 0.0f, 0.5f, 0.5f, 1.0f,
		screenWidth, screenHeight, use2DCamera);

	renderer->guiCamera = Camera(glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f), 90.0f, 0.0f, 0.5f, 0.5f, 1.0f,
		screenWidth, screenHeight, use2DCamera);

	renderer->guiCamera.shouldUpdate = true;
	renderer->guiCamera.useOrthoCamera = true;

	renderer->camera.Update();
	renderer->guiCamera.Update();

	renderer->CreateShaders(); // we must create the shaders at this point
}

void Game::InitSDL()
{
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER);
	TTF_Init();

	if (SDL_NumJoysticks() < 1)
	{
		std::cout << "No controller connected." << std::endl;
	}
	else
	{
		for (int i = 0; i < SDL_NumJoysticks(); i++)
		{
			if (SDL_IsGameController(i))
			{
				std::cout << "Controller connected!" << std::endl;				
				controller = SDL_GameControllerOpen(i);
				std::cout << SDL_GameControllerMapping(controller) << std::endl;
				break;
			}
		}

		SDL_Joystick* joystick = SDL_JoystickOpen(0);
		std::cout << "Controller Name: " << SDL_JoystickName(joystick) << std::endl;
		std::cout << "Num Axes: " << SDL_JoystickNumAxes(joystick) << std::endl;
		std::cout << "Num Buttons: " << SDL_JoystickNumButtons(joystick) << std::endl;
		SDL_JoystickClose(joystick);
	}

	windowTitle = "Witch Doctor Kaneko";
	windowIconFilepath = "assets/gui/icon.png";

	window = SDL_CreateWindow(windowTitle.c_str(),
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, screenWidth, screenHeight, SDL_WINDOW_OPENGL);

	SDL_SetWindowIcon(window, IMG_Load(windowIconFilepath.c_str()));
}

void Game::EndSDL()
{
	// Delete our OpengL context
	SDL_GL_DeleteContext(mainContext);

	//SDL_DestroyRenderer(renderer->renderer);	
	SDL_DestroyWindow(window);	
	window = nullptr;

	if (controller != nullptr)
	{
		SDL_GameControllerClose(controller);
	}
	
	delete theFont;
	delete headerFont;
	
	TTF_Quit();
	SDL_Quit();
	IMG_Quit();
}

bool Game::SetOpenGLAttributes()
{
	return true;
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

Entity* Game::CreateEntity(const std::string& entityName, const Vector2& position, int spriteIndex)
{
	Entity* newEntity = entityFactory->Create(entityName, position);

	if (newEntity != nullptr)
	{
		newEntity->spriteIndex = spriteIndex;

		std::vector<AnimState*> animStates;
		std::unordered_map<std::string, std::string> args;
		args["0"] = std::to_string(spriteIndex);

		if (entityName == "npc")
		{
			args["1"] = npcNames[spriteIndex];
			spriteManager->ReadAnimData("data/animators/npc/" + args["1"] + "/" + args["1"] + ".animations", animStates, args);
		}
		else if (entityName == "enemy")
		{
			args["1"] = enemyNames[spriteIndex];
			spriteManager->ReadAnimData("data/animators/enemies/" + args["1"] + "/" + args["1"] + ".animations", animStates, args);
		}
		else
		{
			spriteManager->ReadAnimData("data/animators/" + entityName + "/" + entityName + ".animations", animStates, args);
		}


		//TODO: Make this better...
		// - Allow for conditions that are always true/false 
		// so that you can write Animator files that 
		// immediately go to other states
		// (such as "notidle: bool true == true")
		// - OR add a simple way to define the starting state in the animator file itself
		// (such as "^*unpressed*") and otherwise just default to the topmost state

		std::string initialState = "idle";
		if (newEntity->etype == "ladder")
			initialState = "middle";
		if (newEntity->etype == "switch")
			initialState = "unpressed";

		Animator* newAnimator = new Animator(entityName, animStates, initialState);
		newEntity->SetAnimator(*newAnimator);
	}

	return newEntity;
}

Entity* Game::SpawnEntity(const std::string& entityName, const Vector2& position, const int spriteIndex)
{
	Entity* entity = CreateEntity(entityName, position, spriteIndex); //entityFactory->Create(entityName, position);

	if (entity != nullptr)
	{
		entity->spriteIndex = spriteIndex;
		if (!entity->CanSpawnHere(position, *this))
		{
			delete entity;
			return nullptr;
		}
		else
		{
			entities.emplace_back(entity);
			return entity;
		}
	}

	return nullptr;
}


Missile* Game::SpawnMissile(Vector2 position)
{
	//TODO: Make a way for this to return false

	Vector2 pivotPoint = Vector2(14, 7);

	std::vector<AnimState*> animStates;
	//ReadAnimData("data/animations/missile.anim", animStates);
	animStates.push_back(new AnimState("moving", 100, new Sprite(0, 3, 23, 16, spriteManager, "assets/sprites/spells/debug_missile.png", renderer->shaders[ShaderName::Default], pivotPoint)));
	animStates.push_back(new AnimState("destroyed", 100, new Sprite(4, 7, 23, 16, spriteManager, "assets/sprites/spells/debug_missile.png", renderer->shaders[ShaderName::Default], pivotPoint, false)));

	Missile* missile = new Missile(position - pivotPoint);

	Animator* newAnimator = new Animator("debugmissile", animStates, "moving");
	newAnimator->SetBool("destroyed", false);

	missile->SetAnimator(*newAnimator);
	missile->GetAnimator()->SetState("moving");

	entities.emplace_back(missile);

	return missile;
}

// This function converts from screen to world coordinates
// and then immediately aligns the object on the grid
Vector2 Game::CalculateObjectSpawnPosition(Vector2 mousePos, const int GRID_SIZE)
{
	mousePos.x += renderer->camera.position.x;
	mousePos.y += renderer->camera.position.y;

	int afterModX = ((int)(mousePos.x) % (GRID_SIZE * 2));
	int afterModY = ((int)(mousePos.y) % (GRID_SIZE * 2));

	Vector2 snappedPos = Vector2(mousePos.x - afterModX, mousePos.y - afterModY);

	int newTileX = (int)snappedPos.x;
	int newTileY = (int)snappedPos.y;

	if (newTileX % 2 != 0)
		newTileX++;

	if (newTileY % 2 != 0)
		newTileY++;

	//TODO: Not sure if this should go here or somewhere else
	// but the tile is not centered on the grid
	newTileX += GRID_SIZE;
	newTileY += GRID_SIZE;

	return Vector2(newTileX, newTileY);
}

Tile* Game::CreateTile(Vector2 frame, string tilesheet, Vector2 position, DrawingLayer drawingLayer)
{
	Tile* tile = new Tile(position, frame, spriteManager->GetImage(tilesheet), renderer);

	tile->layer = drawingLayer;
	tile->impassable = drawingLayer == DrawingLayer::COLLISION
		|| drawingLayer == DrawingLayer::COLLISION2;
	//tile->tilesheetIndex = editor->tilesheetIndex;

	return tile;
}

Tile* Game::SpawnTile(Vector2 frame, string tilesheet, Vector2 position, DrawingLayer drawingLayer)
{
	//Sprite* tileSprite = new Sprite(Vector2(newTileX, newTileY), spriteManager.GetImage(tilesheet), renderer);
	Tile* tile = new Tile(position, frame, spriteManager->GetImage(tilesheet), renderer);

	tile->layer = drawingLayer;
	tile->impassable = drawingLayer == DrawingLayer::COLLISION 
		|| drawingLayer == DrawingLayer::COLLISION2;

	//tile->etype = "tile";
	//tile->tileCoordinates = frame;
	tile->tilesheetIndex = editor->tilesheetIndex;
	entities.emplace_back(tile);

	//std::cout << tile->Size() << std::endl;

	return tile;
}

Player* Game::SpawnPlayer(Vector2 position)
{
	Player* player = new Player(position);
	player->game = this;

	std::vector<AnimState*> animStates;
	spriteManager->ReadAnimData("data/animators/player/player.animations", animStates);

	Animator* newAnimator = new Animator("player", animStates, "idle");
	newAnimator->SetBool("isGrounded", true);
	player->SetAnimator(*newAnimator);
	player->SetPosition(position);
	player->physics->startPosition = position;

	entities.emplace_back(player);

	renderer->camera.target = player;
	renderer->guiCamera.target = player;

	renderer->camera.FollowTarget(*this);
	renderer->guiCamera.FollowTarget(*this);

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

void Game::StartTextInput(const std::string& reason)
{
	getKeyboardInput = true;
	inputReason = reason;
	SDL_StartTextInput();
	inputText = "";
	inputType = "String";
}

void Game::StopTextInput()
{
	getKeyboardInput = false;
	SDL_StopTextInput();
	editor->DestroyDialog();

	if (inputReason == "properties")
	{		
		editor->SetPropertyText(inputText);
		editor->propertyIndex = -1;
		editor->DoAction();
	}
	else if (inputReason == "new_level")
	{
		if (inputText != "")
		{
			for (unsigned int i = 0; i < entities.size(); i++)
				delete entities[i];
			entities.clear();
			player = SpawnPlayer(Vector2(0, 0));

			editor->SaveLevel(inputText);
		}			
	}
	else if (inputReason == "load_file_as")
	{
		if (inputText != "")
		{
			editor->InitLevelFromFile(inputText);
		}
	}
}

void Game::LoadTitleScreen()
{
	renderer->camera.ResetCamera();
	openedMenus.clear();
	editor->InitLevelFromFile("title");
	openedMenus.emplace_back(allMenus["Title"]);
}

// If we have a name for the next level, then load that level
// Otherwise, load the level corresponding to the number
void Game::LoadLevel(const std::string& level, int onExit, int onEnter)
{
	//TODO: What happens if the level fails to load, or the file does not exist?
	// Should it load the same level again, an error screen, or something else?

	transitionExit = onExit;
	transitionEnter = onEnter;
	transitionState = 1; // exit state

	nextLevel = level;
}

void Game::TransitionLevel()
{
	bool conditionMet = false;

	if (transitionState == 1) // exit the old level, start condition
	{
		//std::cout << "t1" << std::endl;

		if (transitionExit == 0)
		{
			conditionMet = true;
		}
		else if (transitionExit == 1)
		{
			renderer->changingOverlayColor = true;
			renderer->overlayStartTime = timer.GetTicks();
			renderer->overlayEndTime = renderer->overlayStartTime + 1000;
			renderer->startColor = renderer->overlayColor;
			renderer->targetColor = Color{ 0, 0, 0, 255 };

			soundManager->FadeOutBGM(1000);
		}

		transitionState = 2;
	}
	else if (transitionState == 2) // check exit condition
	{
		//std::cout << "t2" << std::endl;

		if (transitionExit == 0)
		{
			conditionMet = true;
		}
		else if (transitionExit == 1)
		{
			conditionMet = !renderer->changingOverlayColor;
		}

		// if condition is met, go to next state
		if (conditionMet)
		{
			transitionState = 3;
			if (nextLevel == "")
			{
				levelNumber++;
				editor->InitLevelFromFile("test" + std::to_string(levelNumber));
			}
			else
			{
				openedMenus.clear();
				editor->InitLevelFromFile(nextLevel);
			}
		}
	}
	else if (transitionState == 3) // enter the new level, start condition
	{
		//std::cout << "t3" << std::endl;

		if (transitionEnter == 0)
		{
			conditionMet = true;
		}
		else if (transitionEnter == 1)
		{
			renderer->changingOverlayColor = true;
			renderer->overlayStartTime = timer.GetTicks();
			renderer->overlayEndTime = renderer->overlayStartTime + 1000;
			renderer->startColor = renderer->overlayColor;
			renderer->targetColor = Color{ 0, 0, 0, 0 };
		}

		soundManager->PlayBGM(soundManager->bgmNames[nextBGM]);

		transitionState = 4;
	}
	else if (transitionState == 4) // check enter condition
	{
		//std::cout << "t4" << std::endl;

		if (transitionEnter == 0)
		{
			conditionMet = true;
		}
		else if (transitionEnter == 1)
		{
			conditionMet = !renderer->changingOverlayColor;
		}

		// if condition is met, go to next state
		// player resumes control
		if (conditionMet)
		{
			transitionState = 0;
			transitionEnter = -1;
			transitionExit = -1;
		}
	}
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
		if (event.type == SDL_MOUSEBUTTONUP)
		{
			cutscene->previousMouseState = 0;
		}
		else if (event.type == SDL_CONTROLLERBUTTONDOWN)
		{
			if (event.cbutton.button == SDL_CONTROLLER_BUTTON_A)
			{
				std::cout << "PRESSED A" << std::endl;
			}
			else if (event.cbutton.button == SDL_CONTROLLER_BUTTON_B)
			{
				std::cout << "PRESSED B" << std::endl;
			}
		}

		if (openedMenus.size() > 0)
		{
			if (!cutscene->watchingCutscene)
				quit = HandleMenuEvent(event);
		}			
		else
		{
			quit = HandleEvent(event);
		}

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
	if (!getKeyboardInput)
	{
		const Uint8* input = SDL_GetKeyboardState(NULL);
		renderer->camera.KeyControl(input, dt, screenWidth, screenHeight);
		renderer->guiCamera.KeyControl(input, dt, screenWidth, screenHeight);

		editor->HandleEdit();
	}
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

		if (currentMenu->name == "Credits")
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

	// The currently opened level
	fout << "level " << currentLevel << std::endl;

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
		else if (tokens[0] == "level")
		{
			editor->startEditorLevel = tokens[1];
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
	fout << "windowed " << isFullscreen << std::endl;
	fout << "screen_resolution " << indexScreenResolution << std::endl;
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
		else if (tokens[0] == "fullscreen")
		{
			isFullscreen = std::stoi(tokens[1]);

			if (isFullscreen)
				SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
			else
				SDL_SetWindowFullscreen(window, 0);				

			//TODO: Refactor to avoid the dynamic cast
			SettingsButton* button = dynamic_cast<SettingsButton*>(allMenus["Settings"]->GetButtonByName("Windowed"));
			button->selectedOption = std::stoi(tokens[1]);
		}
		else if (tokens[0] == "screen_resolution")
		{
			indexScreenResolution = std::stoi(tokens[1]);

			//TODO: Refactor to avoid the dynamic cast
			SettingsButton* button = dynamic_cast<SettingsButton*>(allMenus["Settings"]->GetButtonByName("Screen Resolution"));
			button->selectedOption = indexScreenResolution;
			button->ExecuteSelectedOption(*this);
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
		if (event.wheel.y > 0)
		{
			if (editMode)
			{
				editor->ToggleSpriteMap(1);
			}
			else
			{
				renderer->camera.Zoom(-0.1f, screenWidth, screenHeight);
			}
		}
		else if (event.wheel.y < 0)
		{
			if (editMode)
			{
				editor->ToggleSpriteMap(-1);
			}
			else
			{
				renderer->camera.Zoom(0.1f, screenWidth, screenHeight);
			}
		}
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
				//TODO: Handle type checking here
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
				if (!editMode && !cutscene->watchingCutscene)
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

#if _DEBUG
			// NOT IMPLEMENTED YET
				
			case SDLK_v:
				pressedSpellButton = true;
				break;
			case SDLK_q:
				pressedLeftTrigger = true;
				break;
			case SDLK_e:
				pressedRightTrigger = true;
				break;

			// DEVELOPER BUTTONS

			case SDLK_r:
				//if (player != nullptr)
				//	player->ResetPosition();
				editor->InitLevelFromFile(currentLevel);
				break;
			case SDLK_t:
				LoadLevel(nextLevel);
				break;
			case SDLK_1: // toggle Debug mode
				debugMode = !debugMode;
				break;
			case SDLK_2: // toggle Editor mode
				editMode = !editMode;
				
				if (editMode)
				{
					editor->StartEdit();
				}
				else
				{
					editor->StopEdit();
				}
				break;
			case SDLK_3: // toggle Editor settings
				if (editMode)
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
#endif
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
			char c = *event.text.text;
			bool valid = true;

			// Check if the character is valid for the text we are getting
			if (inputType == "Integer")
			{
				valid = std::isdigit(c);
			}
			else if (inputType == "Float")
			{
				valid = std::isdigit(c) || (c == '.' && inputText.find('.') == string::npos);
			}

			if (valid)
			{
				//Append character
				inputText += c;
				UpdateTextInput();
			}
		}
	}

	return quit;
}


void Game::SaveScreenshot(std::string filepath)
{
	if (filepath == "")
	{
		std::string timestamp = CurrentDate() + "-" + CurrentTime();
		for (int i = 0; i < timestamp.size(); i++)
		{
			if (timestamp[i] == ':')
				timestamp[i] = '-';
		}

		filepath = "screenshots/screenshot-" + timestamp + ".bmp";
	}		

	const unsigned int bytesPerPixel = 3;

	unsigned char* pixels = new unsigned char[screenWidth * screenHeight * bytesPerPixel]; // 4 bytes for RGBA
	glReadPixels(0, 0, screenWidth, screenHeight, GL_BGR, GL_UNSIGNED_BYTE, pixels);

	SDL_Surface* screenshot = SDL_CreateRGBSurfaceFrom(pixels, screenWidth, screenHeight, 8 * bytesPerPixel, screenWidth * bytesPerPixel, 0, 0, 0, 0);

	//SDL_Surface * screenshot = SDL_CreateRGBSurface(0, screenWidth, screenHeight, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000);
	//SDL_RenderReadPixels(renderer->renderer, NULL, SDL_PIXELFORMAT_ARGB8888, screenshot->pixels, screenshot->pitch);
	invertSDLSurfaceVertically(screenshot);
	SDL_SaveBMP(screenshot, filepath.c_str());
	SDL_FreeSurface(screenshot);

	delete[] pixels;
}

void Game::GetMenuInput()
{
	const Uint8* input = SDL_GetKeyboardState(NULL);

	if (cutscene->watchingCutscene && cutscene->currentLabel->name == "title")
	{
		cutscene->Update();
	}
	else
	{
		if (debugMode)
		{
			renderer->camera.KeyControl(input, dt, screenWidth, screenHeight);
			renderer->guiCamera.KeyControl(input, dt, screenWidth, screenHeight);
		}

		Uint32 ticks = timer.GetTicks();
		if (ticks > lastPressedKeyTicks + 100) //TODO: Check for overflow errors
		{
			// If we have pressed any key on the menu, add a delay between presses
			if (openedMenus[openedMenus.size() - 1]->Update(*this))
				lastPressedKeyTicks = ticks;
		}
	}


}

void Game::UpdateTextInput()
{
	if (inputReason == "properties")
	{
		editor->dialog->input->SetText(inputText);
		editor->SetPropertyText(inputText);
	}
	else if (inputReason == "new_level")
	{
		editor->dialog->input->SetText(inputText);
	}
	else if (inputReason == "load_file_as")
	{
		editor->dialog->input->SetText(inputText);
	}
}

void Game::Update()
{
	shouldQuit = CheckInputs();
	CheckDeleteEntities();
	renderer->Update();

	if (openedMenus.size() > 0)
	{
		GetMenuInput();
		return;
	}
	else if (editMode)
	{
		HandleEditMode();
		return;
	}
	else if (transitionState > 0)
	{
		TransitionLevel();
		return;
	}

	const Uint8* input = SDL_GetKeyboardState(NULL);

	if (debugMode)
	{
		debugScreen->Update();
	}

	if (cutscene->watchingCutscene)
	{
		cutscene->Update();
	}
	else
	{
		// Get position of mouse and any clicks
		previousMouseState = mouseState;
		mouseState = SDL_GetMouseState(&mouseRect.x, &mouseRect.y);

		// If left click
		if (mouseState & SDL_BUTTON(SDL_BUTTON_LEFT) && 
			!(previousMouseState & SDL_BUTTON(SDL_BUTTON_LEFT)))
		{
			Vector2 worldPosition = Vector2(mouseRect.x + renderer->camera.position.x, mouseRect.y + renderer->camera.position.y);

			mouseRect.x = worldPosition.x;
			mouseRect.y = worldPosition.y;

			//TODO: Only iterate over entities that are clickable
			for (unsigned int i = 0; i < entities.size(); i++)
			{
				if (entities[i]->clickable)
				{
					if (HasIntersection(mouseRect, *entities[i]->GetBounds()))
					{
						entities[i]->OnClickPressed(mouseState, *this);
						break;
					}
				}				
			}
		}


	}

	if (quadTree != nullptr)
	{
		//quadTree->Update();
		quadTree->Reset();
		for (int i = 0; i < entities.size(); i++)
		{
			if (entities[i]->impassable || entities[i]->trigger || entities[i]->jumpThru)
				quadTree->Insert(entities[i]);
		}
	}
		
	// Update all entities
	updateCalls = 0;
	collisionChecks = 0;
	for (unsigned int i = 0; i < entities.size(); i++)
	{		
		entities[i]->Update(*this);
	}

	

	// Update the camera last
	// We need to use the original screen resolution here (for some reason)
	// which in our case is 1280 x 720
	if (!cutscene->watchingCutscene)
	{
		if (renderer->camera.useOrthoCamera)
			renderer->camera.FollowTarget(*this);

		renderer->guiCamera.FollowTarget(*this);
	}
}

void Game::SetScreenResolution(const unsigned int width, const unsigned int height)
{
	screenWidth = width;
	screenHeight = height;

	SDL_SetWindowSize(window, screenWidth, screenHeight);
	renderer->camera.Zoom(0.0f, screenWidth, screenHeight);
	renderer->guiCamera.Zoom(0.0f, screenWidth, screenHeight);

	glViewport(0, 0, screenWidth, screenHeight);
	renderer->screenScale = Vector2(screenWidth / 1280.0f, screenHeight / 720.0f);
}

void Game::Render()
{
	// Clear window	
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Render editor grid
	if (editMode)
	{
		//editor->DrawGrid();
	}

	// Render all backgrounds and their layers
	background->Render(*renderer);

	if (quadTree != nullptr)
	{
		quadTree->Render(*renderer);
	}

	// Render all entities
	if (renderer->camera.useOrthoCamera && !editMode)
	{
		const int maxWidth = (30 * TILE_SIZE) * 2;
		const int maxHeight = (17 * TILE_SIZE) * 2;
		for (unsigned int i = 0; i < entities.size(); i++)
		{
			if (entities[i]->position.x > renderer->camera.position.x - TILE_SIZE &&
				entities[i]->position.y > renderer->camera.position.y - TILE_SIZE &&
				entities[i]->position.x < renderer->camera.position.x + maxWidth + TILE_SIZE &&
				entities[i]->position.y < renderer->camera.position.y + maxHeight + TILE_SIZE)
			{
				entities[i]->Render(*renderer);
				if (debugMode)
					entities[i]->RenderDebug(*renderer);
			}
		}

	}
	else
	{
		for (unsigned int i = 0; i < entities.size(); i++)
		{
			entities[i]->Render(*renderer);
			if (debugMode)
				entities[i]->RenderDebug(*renderer);
		}
	}

	renderer->RenderLate();

	// LAST THING
	// Render all menu screens
	if (openedMenus.size() > 0)
	{
		openedMenus[openedMenus.size() - 1]->Render(*renderer);
	}

	if (showFPS)
		fpsText->Render(*renderer);
	
	if (showTimer)
		timerText->Render(*renderer);

	if (currentLevel != "title" && !cutscene->watchingCutscene)
	{
		//bugText->Render(renderer);
		//etherText->Render(renderer);
	}	

	// Draw anything in the cutscenes
	cutscene->Render(*renderer); // includes the overlay

	// Render editor toolbox
	if (editMode)
	{
		editor->Render(*renderer);		
	}

	//if (GetModeDebug())
#if _DEBUG
	editor->RenderDebug(*renderer);
	if (debugMode)
	{
		if (quadTree != nullptr)
		{
			//quadTree->Render(*renderer);
			if (quadrantEntities.size() > 0)
				quadTree->RenderEntities(*renderer, quadrantEntities);
		}
			
		debugScreen->Render(*renderer);
	}
#endif



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