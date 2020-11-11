#include "leak_check.h"

#include "Game.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cmath>
#include <vector>
#include <unordered_map>
#include <string>
#include <iostream>
#include <memory>
#include <cmath>
#include <chrono>
#include "sdl_helpers.h"

#include "Mesh.h"
#include "Shader.h"
#include "Camera.h"
#include "Texture.h"
#include "Logger.h"
#include "FileManager.h"
#include "MenuManager.h"

#include "Sprite.h"
#include "Entity.h"
#include "SpriteManager.h"

#include "Background.h"
#include "Editor.h"
#include "Tile.h"
#include "Timer.h"
#include "MenuScreen.h"
#include "Renderer.h"

#include "SettingsButton.h" // TODO: Allow for customization

#include "DebugScreen.h"
#include "EntityFactory.h"
#include "QuadTree.h"
#include "GUI.h"
#include "Dialog.h"
#include "CutsceneManager.h"
#include "SoundManager.h"
#include "RandomManager.h"
#include <filesystem>

static unsigned int allocationCount = 0;

/*
void* operator new(size_t size)
{
	allocationCount++;
	//if (allocationCount < 1000)
	//	std::cout << allocationCount << " Allocating " << size << " bytes\n";
	return malloc(size);
}

void operator delete(void* p)
{
	free(p);
}
*/

int Game::MainLoop()
{
	// Load settings
	LoadSettings();
	LoadTitleScreen();
	SortEntities(entities);
	timer.Start();

	const int updateInterval = 500; // update fps every X ms
	float fpsSum = 0.0f; // 
	float timeLeft = updateInterval; // time left before updating
	int frames = 0; // number of frames counted

	int drawCallsLastFrame = 0;
	int previousNumberOfFrames = 0;
	int currentNumberOfFrames = 0;

	const std::string guiFPS = "FPS";
	const std::string guiFPS2 = "FPS: ";
	const std::string guiTimer = "timer";

	while (!shouldQuit)
	{
		renderer.drawCallsPerFrame = 0;

		CalcDt();

		//std::cout << "---" << std::endl;
		allocationCount = 0;

		//game.showFPS = true;
		if (showFPS)
		{
			timeLeft -= dt;
			fpsSum += 1000 / dt;
			if (timeLeft <= 0)
			{
				currentNumberOfFrames = (int)(fpsSum / frames);
				if (currentNumberOfFrames != previousNumberOfFrames)
				{
					gui->texts[guiFPS]->SetText(guiFPS2 + std::to_string(currentNumberOfFrames));
					previousNumberOfFrames = currentNumberOfFrames;
				}

				//std::cout << game.fpsText->txt << std::endl;
				timeLeft = updateInterval;
				fpsSum = 0;
				frames = 0;
			}
			frames++;
		}

		if (showTimer)
		{
			gui->texts[guiTimer]->SetText(std::to_string(timer.GetTicks() / 1000.0f));
		}

		switch (state)
		{
		case GameState::RESET_LEVEL:
			editor->InitLevelFromFile(currentLevel);
			state = GameState::NORMAL;
			break;
		case GameState::LOAD_NEXT_LEVEL:
			LoadLevel(nextLevel);
			state = GameState::NORMAL;
			break;
		default:
			break;
		}

		Update();

		Render();

		if (renderer.drawCallsPerFrame != drawCallsLastFrame)
		{
			drawCallsLastFrame = renderer.drawCallsPerFrame;
			//std::cout << "Draw calls: " << renderer.drawCallsPerFrame << std::endl;
		}
	}

	return 0;
}

Mesh* Game::CreateQuadMesh()
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

	Mesh* mesh = neww Mesh();
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

	Mesh* mesh = neww Mesh();
	mesh->CreateMesh(quadVertices, quadIndices, 20, 12);

	return mesh;
}

Game::Game(const std::string& n, const std::string& title, const std::string& icon, 
	const EntityFactory& e, const FileManager& f, GUI& g, MenuManager& m) : logger("logs/output.log")
{
	currentGame = n;
	startOfGame = std::chrono::steady_clock::now();
	entityFactory = &e;

	fileManager = &f;
	fileManager->Init(*this);

	menuManager = &m;

	windowTitle = title;
	windowIconFilepath = icon;

	InitSDL();

	renderer.Init(this);
	spriteManager.Init(&renderer);
	soundManager.Init(this);

	InitOpenGL();

	//Sprite::mesh = CreateSpriteMesh();
	cubeMesh = CreateCubeMesh();

	// Initialize the font before all text

	// TODO: Load all fonts from a file
	CreateFont("SpaceMono", m.GetFontSize());
	theFont = fonts["SpaceMono"];
	headerFont = fonts["SpaceMono"];

	soundManager.ReadMusicData("data/bgm.dat");

	// Initialize the cutscene stuff (do this AFTER renderer and sprite manager and random)
	cutsceneManager.Init(*this);
	cutsceneManager.ParseCutsceneFile();	

	//ShaderProgram* shader = renderer.shaders[ShaderName::Default];

	// Initialize debug stuff
	renderer.debugSprite = CreateSprite("assets/editor/rect-outline.png");

	// Initialize overlay sprite
	renderer.overlaySprite = CreateSprite("assets/gui/white.png");
	renderer.overlaySprite->keepPositionRelativeToCamera = true;
	//renderer.overlaySprite->keepScaleRelativeToCamera = true;

	// Initialize the sprite map (do this BEFORE the editor)
	ReadEntityLists();

	editor = neww Editor(*this);
	debugScreen = neww DebugScreen(*this);

	entities.clear();

	SetScreenResolution(renderer.camera.startScreenWidth, renderer.camera.startScreenHeight);

	// Initialize GUI (do this AFTER fonts and resolution)
	gui = &g;
	gui->Init(this);

	// Initialize all the menus (do this AFTER fonts and resolution)
	menuManager->Init(*this);

	LoadEditorSettings();

	start_time = clock::now();
}

Game::~Game()
{	
	for (int i = 0; i < entities.size(); i++)
	{
		if (entities[i] != nullptr)
			delete_it(entities[i]);
	}

	for (auto& [key, animInfo] : Animator::mapTypeToInfo)
	{
		if (animInfo != nullptr)
			delete_it(animInfo);
	}

	for (auto& [key, val] : allMenus)
	{
		if (val != nullptr)
			delete_it(val);
	}

	if (background != nullptr)
		delete_it(background);

	// NOTE: Need to delete textures manually for the screen textures
	// which are created using frame buffers (not the usual way)
	// TODO: Maybe automate this somehow
	if (prevScreenSprite != nullptr)
	{
		if (prevScreenSprite->texture != nullptr)
			delete_it(prevScreenSprite->texture);
		delete_it(prevScreenSprite);
	}
	if (screenSprite != nullptr)
	{
		if (screenSprite->texture != nullptr)
			delete_it(screenSprite->texture);
		delete_it(screenSprite);
	}
		

	if (debugScreen != nullptr)
		delete_it(debugScreen);

	if (cubeMesh != nullptr)
		delete_it(cubeMesh);

	if (Sprite::meshLine != nullptr)
		delete_it(Sprite::meshLine);

	if (Sprite::meshTri != nullptr)
		delete_it(Sprite::meshTri);

	if (Sprite::meshQuad != nullptr)
		delete_it(Sprite::meshQuad);

	SaveEditorSettings();

	if (editor != nullptr)
		delete_it(editor);

	for (auto& [key, val] : fonts)
	{
		if (val != nullptr)
			delete_it(val);
	}

	EndSDL();
}

Sprite* Game::CreateSprite(const std::string& filepath, const ShaderName shaderName)
{
	return neww Sprite(spriteManager.GetImage(filepath), renderer.shaders[shaderName]);
}

// IMPORTANT INSTRUCTIONS:
// entityTypes.list is a list of all the different types of entities in the game.
// Each entity type has its own list file, which contains the list of different subtypes.
// The subtypes are toggled through when placing objects via the editor.
// Thus you can modify these things outside of the engine to customize different games.
void Game::ReadEntityLists()
{
	std::vector<std::string> listNames = ReadStringsFromFile("data/lists/entityTypes.list");

	entityTypes.clear();
	for (int i = 0; i < listNames.size(); i++)
	{
		entityTypes[listNames[i]] = ReadStringsFromFile("data/lists/" + listNames[i] + ".list");
	}

	entityTypes["cameraBounds"] = { "cameraBounds" };

	spriteMap.clear();
	for (auto const& [key, val] : entityTypes)
	{
		for (int i = 0; i < val.size(); i++)
		{
			spriteMap[key].push_back("assets/sprites/" + key + "/" + val[i] + ".png");
		}
	}
}

void Game::CreateFont(const std::string& fontName, int size)
{
	if (fonts.count(fontName) == 0)
	{
		fonts[fontName] = neww FontInfo("fonts/" + fontName + "/" + fontName + "-Regular.ttf", size);
		fonts[fontName]->SetBoldFont("fonts/" + fontName + "/" + fontName + "-Bold.ttf");
		fonts[fontName]->SetItalicsFont("fonts/" + fontName + "/" + fontName + "-Italic.ttf");
		fonts[fontName]->SetBoldItalicsFont("fonts/" + fontName + "/" + fontName + "-BoldItalic.ttf");
	}
}

void Game::CalcDt()
{
	dt = std::chrono::duration<float, milliseconds::period>(clock::now() - start_time).count();
	start_time = clock::now();

	now = std::chrono::duration<float, milliseconds::period>(start_time - startOfGame).count();
	renderer.now = now;

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

	renderer.camera = Camera(glm::vec3(0.0f, 0.0f, 1000.0f),
		glm::vec3(0.0f, 1.0f, 0.0f), 90.0f, 0.0f, 0.5f, 0.5f, 1.0f,
		screenWidth, screenHeight, use2DCamera);

	renderer.guiCamera = Camera(glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f), 90.0f, 0.0f, 0.5f, 0.5f, 1.0f,
		screenWidth, screenHeight, use2DCamera);

	renderer.guiCamera.shouldUpdate = true;
	renderer.guiCamera.useOrthoCamera = true;

	renderer.camera.Update();
	renderer.guiCamera.Update();

	renderer.CreateShaders(); // we must create the shaders at this point

	screenSprite = InitFramebuffer(framebuffer, textureColorBuffer, renderBufferObject);
	prevScreenSprite = InitFramebuffer(prevFramebuffer, prevTextureColorBuffer, prevRenderBufferObject);

	/*
	prevScreenSprite = neww Sprite(screenSprite->texture, renderer.shaders[ShaderName::Default]);	
	prevScreenSprite->keepPositionRelativeToCamera = true;
	prevScreenSprite->keepScaleRelativeToCamera = true;
	prevScreenSprite->color = { 255, 255, 255, 0 };
	prevScreenSprite->SetScale(Vector2(1.0f, -1.0f));
	*/

}

Sprite* Game::InitFramebuffer(unsigned int& fbo, unsigned int& tex, unsigned int& rbo)
{
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	Texture* screenTexture = neww Texture("");
	screenTexture->LoadTexture(tex, screenWidth, screenHeight);

	Sprite* sprite = neww Sprite(screenTexture, renderer.shaders[ShaderName::Default]);
	sprite->keepPositionRelativeToCamera = true;
	sprite->keepScaleRelativeToCamera = true;

	// attach it to currently bound framebuffer object
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);

	glGenRenderbuffers(1, &rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, rbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, screenWidth, screenHeight);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return sprite;
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

	window = SDL_CreateWindow(windowTitle.c_str(),
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, screenWidth, screenHeight, SDL_WINDOW_OPENGL);
	SDL_SetWindowIcon(window, IMG_Load(windowIconFilepath.c_str()));
}

void Game::EndSDL()
{
	// Delete our OpenGL context
	SDL_GL_DeleteContext(mainContext);

	SDL_DestroyWindow(window);	
	window = nullptr;

	if (controller != nullptr)
	{
		SDL_GameControllerClose(controller);
	}
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

Entity* Game::CreateEntity(const std::string& entityName, const Vector2& position, int subtype)
{
	Entity* newEntity = entityFactory->Create(entityName, position);

	if (newEntity != nullptr)
	{
		newEntity->subtype = subtype;

		std::unordered_map<std::string, std::string> args;

		std::string filepath = entityName + "/";

		args["0"] = entityName; // std::to_string(subtype);
		args["1"] = "";
		
		if (entityTypes.count(entityName) > 0 && entityTypes[entityName].size() > subtype)
		{
			args["1"] = entityTypes[entityName][subtype];
		}

		// TODO: Don't hardcode this part, store these in an external file
		// and check to see if it matches any entity type in that file...
		if (args["1"] != "" && (entityName == "enemy" || entityName == "npc" || entityName == "collectible"))
		{
			filepath += args["1"] + "/" + args["1"];
		}
		else
		{
			filepath += entityName;
		}

		std::vector<AnimState*> animStates = spriteManager.ReadAnimData("data/animators/" + filepath + ".animations", args);

		//TODO: Make this better...
		// - Allow for conditions that are always true/false 
		// so that you can write Animator files that 
		// immediately go to other states
		// (such as "notidle: bool true == true")
		// - OR add a simple way to define the starting state in the animator file itself
		// (such as "^*unpressed*") and otherwise just default to the topmost state

		// TODO: Don't hardcode this part
		std::string initialState = "idle";
		if (newEntity->etype == "ladder")
			initialState = "middle";
		if (newEntity->etype == "switch")
			initialState = "unpressed";

		if (animStates.size() > 0)
		{
			Animator* newAnimator = nullptr;

			// TODO: It seems like we need a neww animator for each entity
			// because each one will be in different states at different times,
			// and the entity's sprite is based on its animation state.
			// However, this also seems wasteful, is there a way to optimize this?

			/*
			for (auto const& [key, val] : animators)
			{
				if (key == filepath)
				{
					newAnimator = val;
				}
			}
			*/

			if (newAnimator == nullptr)
			{
				newAnimator = neww Animator(filepath, animStates, initialState);
				//animators[filepath] = newAnimator;
			}

			newEntity->SetAnimator(*newAnimator);			
		}
	}

	return newEntity;
}

// TODO: We want to sort the entities every time we spawn one
// as long as the game is running, but not when we are first loading the level
Entity* Game::SpawnEntity(const std::string& entityName, const Vector2& position, const int spriteIndex)
{
	Entity* entity = CreateEntity(entityName, position, spriteIndex); //entityFactory->Create(entityName, position);

	if (entity != nullptr)
	{
		entity->subtype = spriteIndex;
		if (!entity->CanSpawnHere(position, *this))
		{
			delete_it(entity);
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

// This function converts from screen to world coordinates
// and then immediately aligns the object on the grid
Vector2 Game::CalculateObjectSpawnPosition(Vector2 mousePos, const int GRID_SIZE)
{
	mousePos.x += renderer.camera.position.x;
	mousePos.y += renderer.camera.position.y;

	int afterModX = ((int)(mousePos.x) % (GRID_SIZE * (int)Camera::MULTIPLIER));
	int afterModY = ((int)(mousePos.y) % (GRID_SIZE * (int)Camera::MULTIPLIER));

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

Tile* Game::CreateTile(const Vector2& frame, const std::string& tilesheet, 
	const Vector2& position, DrawingLayer drawingLayer) const
{
	Tile* tile = neww Tile(position, frame, spriteManager.GetImage(tilesheet), renderer);

	tile->layer = drawingLayer;
	tile->impassable = drawingLayer == DrawingLayer::COLLISION
		|| drawingLayer == DrawingLayer::COLLISION2;

	return tile;
}

Tile* Game::SpawnTile(const Vector2& frame, const std::string& tilesheet, 
	const Vector2& position, DrawingLayer drawingLayer) const
{
	Tile* tile = neww Tile(position, frame, spriteManager.GetImage(tilesheet), renderer);

	tile->layer = drawingLayer;
	tile->impassable = drawingLayer == DrawingLayer::COLLISION 
		|| drawingLayer == DrawingLayer::COLLISION2;

	tile->tilesheetIndex = editor->tilesheetIndex;
	entities.emplace_back(tile);

	return tile;
}

// NOTE: Spawning more than one player this way breaks things
Entity* Game::SpawnPlayer(const Vector2& position)
{
	Entity* player = SpawnEntity("player", position, 0);
		//static_cast<Player*>(SpawnEntity("player", position, 0));
	//player->game = this;
	renderer.camera.target = player;
	renderer.camera.FollowTarget(*this);
	return player;
}

void Game::ShouldDeleteEntity(int index)
{
	entities[index]->shouldDelete = true;
}

void Game::ShouldDeleteEntity(Entity* entity)
{
	std::vector<Entity*>::iterator iter = std::find(entities.begin(), entities.end(), entity);
	if (iter != entities.end())
	{
		int index = std::distance(entities.begin(), iter);
		entities[index]->shouldDelete = true;
	}
}

void Game::DeleteEntity(Entity* entity)
{
	std::vector<Entity*>::iterator index = std::find(entities.begin(), entities.end(), entity);
	if (index != entities.end()) // means the element was not found
		entities.erase(index);
}

void Game::DeleteEntity(int index)
{
	delete_it(entities[index]);
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
				delete_it(entities[i]);
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
	else if (inputReason == "set_background")
	{
		// If a BG with that name exists, then use it
		if (background->bgData.count(inputText) != 0)
		{
			background->SpawnBackground(inputText, *this);
		}
		else
		{
			logger.Log("ERROR: Failed to load BG with name " + inputText);
		}
	}
	else if (inputReason == "new_entity_type")
	{
		if (inputText == "")
			return;

		std::string newEntityName = Trim(inputText);

		// 1. Add this entity to the entityTypes.list
		std::ifstream fin;
		std::ofstream fout;
		std::string data = "";

		fin.open("data/lists/entityTypes.list");
		// TODO: Create file if does not exist
		for (std::string line; std::getline(fin, line); )
		{
			data += line + "\n";
		}

		fin.close();

		// Don't add if already in file
		if (data.find(newEntityName) != std::string::npos)
		{
			return;
		}

		fout.open("data/lists/entityTypes.list");
		fout << data << newEntityName;
		fout.close();

		// 2. Create a new .list file
		fout.open("data/lists/" + newEntityName + ".list");
		fout << newEntityName + "\n";
		fout.close();

		// 3. Create folder in animations dir, two new files
		std::string directory = std::filesystem::current_path().string() + "\\data\\animators\\";
		std::filesystem::create_directories(directory + newEntityName + "\\");

		fout.open("data/animators/" + newEntityName + "/" + newEntityName + ".animations");
		fout << "idle 100 0 0 " << Globals::TILE_SIZE << " " << Globals::TILE_SIZE
			<< " assets/sprites/{0}/{1}.png 0 0\n";
		fout.close();

		fout.open("data/animators/" + newEntityName + "/" + newEntityName + ".animator");
		fout << "*idle*\n";
		fout.close();

		// 4. Create new class

		data = "";
		fin.open("data/editor/NewEntityType-h.template");
		for (std::string line; std::getline(fin, line); )
		{
			data += line + "\n";
		}
		fin.close();
		ReplaceAll(data, "NewEntityType", newEntityName);
		fout.open(newEntityName + ".h");
		fout << data;
		fout.close();

		data = "";
		fin.open("data/editor/NewEntityType-cpp.template");
		for (std::string line; std::getline(fin, line); )
		{
			data += line + "\n";
		}
		fin.close();
		ReplaceAll(data, "NewEntityType", newEntityName);
		fout.open(newEntityName + ".cpp");
		fout << data;
		fout.close();

		// 5. Modify EntityFactory
		data = "";
		fin.open("data/editor/MyEntityFactory-cpp.template");
		for (std::string line; std::getline(fin, line); )
		{
			data += line + "\n";
		}
		fin.close();

		std::string includes = "";
		std::string functions = "";
		ReadEntityLists();

		std::filesystem::path path = std::filesystem::current_path();
		std::vector<std::string> classNames;
		for (const auto& entry : std::filesystem::directory_iterator(path))
		{
			if (entry.path().extension().string() == ".h")
			{
				std::string s = entry.path().filename().string();
				classNames.push_back(s.substr(0, s.size() - 2));
			}
		}

		for (auto const& [key, val] : entityTypes)
		{
			// TODO: Don't hardcode this
			if (key == "cameraBounds" || key == "path" || key == "pathnode")
				continue;

			std::string fixedKey = key;
			std::string loweredKey = key;
			transform(loweredKey.begin(), loweredKey.end(), loweredKey.begin(), ::tolower);

			for (int i = 0; i < classNames.size(); i++)
			{
				std::string loweredName = classNames[i];
				transform(loweredName.begin(), loweredName.end(), loweredName.begin(), ::tolower);
				if (loweredName == loweredKey)
				{
					fixedKey = classNames[i];
					break;
				}
			}

			includes += "#include \"" + fixedKey + ".h\"\n";
			functions += "\tRegister(\"" + key + "\", &" + fixedKey + "::Create);\n";
		}

		ReplaceAll(data, "$includes$", includes);
		ReplaceAll(data, "$functions$", functions);
		
		fout.open("MyEntityFactory.cpp");
		fout << data;
		fout.close();
	}
}

void Game::LoadTitleScreen()
{
	renderer.camera.ResetCamera();
	openedMenus.clear();
	editor->InitLevelFromFile("title");

	if (allMenus.count("Title") != 0)
		openedMenus.emplace_back(allMenus["Title"]);
	else
		std::cout << "ERROR: No title menu found!" << std::endl;

	if (soundManager.bgmFilepath != soundManager.bgmNames[nextBGM])
		soundManager.PlayBGM(soundManager.bgmNames[nextBGM]);
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
			renderer.changingOverlayColor = true;
			renderer.overlayStartTime = timer.GetTicks();
			renderer.overlayEndTime = renderer.overlayStartTime + 1000;
			renderer.startColor = renderer.overlayColor;
			renderer.targetColor = Color{ 0, 0, 0, 255 };

			soundManager.FadeOutBGM(1000);
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
			conditionMet = !renderer.changingOverlayColor;
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

			if (loadingFromSaveFile)
			{
				loadingFromSaveFile = false;
				// Load data from the current save file
				if (player != nullptr)
				{
					player->position.x = cutsceneManager.commands.numberVariables[202];
					player->position.y = cutsceneManager.commands.numberVariables[203];
					player->startPosition = player->position;

					/*
					if (player->health != nullptr)
					{
						player->health->SetMaxHP(cutsceneManager.commands.numberVariables[204]);
						player->health->SetCurrentHP(cutsceneManager.commands.numberVariables[205]);
					}
					*/
				}
			}			
		}
	}
	else if (transitionState == 3) // enter the neww level, start condition
	{
		//std::cout << "t3" << std::endl;

		if (transitionEnter == 0)
		{
			conditionMet = true;
		}
		else if (transitionEnter == 1)
		{
			renderer.changingOverlayColor = true;
			renderer.overlayStartTime = timer.GetTicks();
			renderer.overlayEndTime = renderer.overlayStartTime + 1000;
			renderer.startColor = renderer.overlayColor;
			renderer.targetColor = Color{ 0, 0, 0, 0 };
		}

		if (soundManager.bgmFilepath != soundManager.bgmNames[nextBGM])
			soundManager.PlayBGM(soundManager.bgmNames[nextBGM]);

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
			conditionMet = !renderer.changingOverlayColor;
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
			cutsceneManager.previousMouseState = 0;
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
			if (!cutsceneManager.watchingCutscene)
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
		renderer.camera.KeyControl(input, dt, screenWidth, screenHeight);
		renderer.guiCamera.KeyControl(input, dt, screenWidth, screenHeight);

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
	//fout << "language " << soundManager.soundVolumeIndex << std::endl;

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

		const std::string editorSettings = "EditorSettings";

		if (tokens[0] == "replacing" && allMenus.count(editorSettings) != 0)
		{
			editor->replaceSettingIndex = std::stoi(tokens[1]);
			SettingsButton* button = dynamic_cast<SettingsButton*>(allMenus[editorSettings]->
				GetButtonByName("Replacing"));
			button->selectedOption = editor->replaceSettingIndex;
		}
		else if (tokens[0] == "deleting" && allMenus.count(editorSettings) != 0)
		{
			editor->deleteSettingIndex = std::stoi(tokens[1]);
			SettingsButton* button = dynamic_cast<SettingsButton*>(allMenus[editorSettings]->
				GetButtonByName("Deleting"));
			button->selectedOption = editor->deleteSettingIndex;
		}
		else if (tokens[0] == "colors" && allMenus.count(editorSettings) != 0)
		{
			editor->colorSettingIndex = std::stoi(tokens[1]);
			SettingsButton* button = dynamic_cast<SettingsButton*>(allMenus[editorSettings]->
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

	fout << "music_volume " << soundManager.bgmVolumeIndex << std::endl;
	fout << "sound_volume " << soundManager.soundVolumeIndex << std::endl;
	fout << "windowed " << isFullscreen << std::endl;
	fout << "screen_resolution " << indexScreenResolution << std::endl;
	fout << "display_fps " << showFPS << std::endl;
	fout << "display_timer " << showTimer << std::endl;
	//fout << "language " << soundManager.soundVolumeIndex << std::endl;

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

		// TODO: Refactor to avoid the dynamic cast
		// 1. All buttons should just be one class, no inheritance
		// 2. Match tokens[0] perfectly with the button name (i.e. [Music Volume])

		if (tokens[0] == "music_volume")
		{
			soundManager.SetVolumeBGM(std::stoi(tokens[1]));

			SettingsButton* button = dynamic_cast<SettingsButton*>(allMenus["Settings"]->GetButtonByName("Music Volume"));
			button->selectedOption = std::stoi(tokens[1]);
		}
		else if (tokens[0] == "sound_volume")
		{
			soundManager.SetVolumeSound(std::stoi(tokens[1]));

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

			SettingsButton* button = dynamic_cast<SettingsButton*>(allMenus["Settings"]->GetButtonByName("Windowed"));
			button->selectedOption = std::stoi(tokens[1]);
		}
		else if (tokens[0] == "screen_resolution")
		{
			indexScreenResolution = std::stoi(tokens[1]);

			SettingsButton* button = dynamic_cast<SettingsButton*>(allMenus["Settings"]->GetButtonByName("Screen Resolution"));
			button->selectedOption = indexScreenResolution;
			button->ExecuteSelectedOption(*this);
		}
		else if (tokens[0] == "display_fps")
		{
			showFPS = std::stoi(tokens[1]);

			SettingsButton* button = dynamic_cast<SettingsButton*>(allMenus["Settings"]->GetButtonByName("Display FPS"));
			button->selectedOption = std::stoi(tokens[1]);
		}
		else if (tokens[0] == "display_timer")
		{
			showTimer = std::stoi(tokens[1]);

			SettingsButton* button = dynamic_cast<SettingsButton*>(allMenus["Settings"]->GetButtonByName("Display Timer"));
			button->selectedOption = std::stoi(tokens[1]);
		}
		else if (tokens[0] == "language")
		{
			//TODO: Deal with this later

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

void Game::ResetLevel()
{
	editor->InitLevelFromFile(currentLevel);
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
				renderer.camera.Zoom(-0.1f, screenWidth, screenHeight);
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
				renderer.camera.Zoom(0.1f, screenWidth, screenHeight);
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
				if (!editMode && !cutsceneManager.watchingCutscene)
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
				// pressedSpellButton = true;
				break;
			case SDLK_q:
				pressedLeftTrigger = true;
				break;
			case SDLK_e:
				pressedRightTrigger = true;
				break;

#if _DEBUG
			// DEVELOPER BUTTONS

			case SDLK_r:
				//if (player != nullptr)
				//	player->ResetPosition();
				if (!editMode)
					ResetLevel();
				break;			
			case SDLK_t:
				if (!editMode)
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
				//editor->UndoAction();
				if (savingGIF)
				{
					EndGIF();
					savingGIF = false;
				}
				else
				{
					StartGIF();
					savingGIF = true;					
				}				
				break;
			case SDLK_5: // Redo Button
				//editor->RedoAction();
				//EndGIF();
				break;
			case SDLK_6: // Screenshot Button
				SaveScreenshot();
				break;
			case SDLK_7:
				//_CrtDumpMemoryLeaks();
				break;
			case SDLK_8: // save game
				fileManager->SaveFile(currentSaveFileName);
				break;
			case SDLK_9: // load game
				if (!editMode)
					fileManager->LoadFile(currentSaveFileName);
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
				valid = std::isdigit(c) || (c == '-' && inputText.size() == 0);
			}
			else if (inputType == "Float") // check for negative numbers, decimal point
			{
				valid = std::isdigit(c) 
					|| (c == '-' && inputText.size() == 0) 
					|| (c == '.' && inputText.size() > 1 && inputText[inputText.size()-1] != '-' && inputText.find('.') == string::npos) ;
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

void Game::StartGIF(const std::string& filepath)
{
	//std::vector<uint8_t> black(width * height * 4, 0);
	//std::vector<uint8_t> white(width * height * 4, 255);

	gifData.clear();
	std::string fileName = "test.gif";
	int delay = 0;

	if (filepath == "")
	{
		std::string timestamp = CurrentDate() + "-" + CurrentTime();
		for (int i = 0; i < timestamp.size(); i++)
		{
			if (timestamp[i] == ':')
				timestamp[i] = '-';
		}

		fileName = timestamp + ".gif";
	}
	
	gifAnim.GifBegin(&gifWriter, fileName.c_str(), screenWidth, screenHeight, delay);
	//GifWriteFrame(&gifWriter, black.data(), width, height, delay);
	//GifWriteFrame(&gifWriter, white.data(), width, height, delay);
}

void Game::EndGIF()
{
	//gifAnim.GifWriteFrame(&gifWriter, gifData.data(), screenWidth, screenHeight, 100);
	gifAnim.GifEnd(&gifWriter);
	gifData.clear();
}

void Game::SaveGIF()
{
	const int DELAY = 0;
	static int gifDelay = DELAY;

	gifDelay--;
	if (gifDelay < 0)
	{
		gifDelay = DELAY;

		const unsigned int bytesPerPixel = 4;
		int pixelsSize = screenWidth * screenHeight * bytesPerPixel;
		uint8_t* pixels = neww uint8_t[pixelsSize]; // 4 bytes for RGBA
		glReadPixels(0, 0, screenWidth, screenHeight, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

		// TODO: This part is EXTREMELY slow! What can we do here?
		// TODO: Also, the output file is laggy and does not loop.
		//gifData.emplace_back(pixels);
		//std::copy(pixels, pixels + pixelsSize, std::back_inserter(gifData));
		gifAnim.GifWriteFrame(&gifWriter, pixels, screenWidth, screenHeight, gifDelay);

		delete[] pixels;
	}
}

void Game::SaveScreenshot(const std::string& filepath)
{
	const unsigned int bytesPerPixel = 3;

	unsigned char* pixels = neww unsigned char[screenWidth * screenHeight * bytesPerPixel]; // 4 bytes for RGBA
	glReadPixels(0, 0, screenWidth, screenHeight, GL_BGR, GL_UNSIGNED_BYTE, pixels);

	SDL_Surface* screenshot = SDL_CreateRGBSurfaceFrom(pixels, screenWidth, screenHeight, 8 * bytesPerPixel, screenWidth * bytesPerPixel, 0, 0, 0, 0);

	//SDL_Surface * screenshot = SDL_CreateRGBSurface(0, screenWidth, screenHeight, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000);
	//SDL_RenderReadPixels(renderer.renderer, NULL, SDL_PIXELFORMAT_ARGB8888, screenshot->pixels, screenshot->pitch);
	invertSDLSurfaceVertically(screenshot);
	
	if (filepath == "")
	{
		std::string timestamp = CurrentDate() + "-" + CurrentTime();
		for (int i = 0; i < timestamp.size(); i++)
		{
			if (timestamp[i] == ':')
				timestamp[i] = '-';
		}

		SDL_SaveBMP(screenshot, ("screenshots/screenshot-" + timestamp + ".bmp").c_str());
	}
	else
	{
		SDL_SaveBMP(screenshot, filepath.c_str());
	}
	
	SDL_FreeSurface(screenshot);

	delete[] pixels;
}

void Game::GetMenuInput()
{
	const Uint8* input = SDL_GetKeyboardState(NULL);

	if (cutsceneManager.watchingCutscene && cutsceneManager.currentLabel->name == "title")
	{
		cutsceneManager.Update();
	}
	else
	{
		if (debugMode)
		{
			renderer.camera.KeyControl(input, dt, screenWidth, screenHeight);
			renderer.guiCamera.KeyControl(input, dt, screenWidth, screenHeight);
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
	editor->dialog->input->SetText(inputText);
	if (inputReason == "properties")
	{
		editor->SetPropertyText(inputText);
	}
}

void Game::Update()
{
	shouldQuit = CheckInputs();
	CheckDeleteEntities();

	if (!updateScreenTexture)
	{
		updateScreenTexture = !cutsceneManager.watchingCutscene || cutsceneManager.printNumber > 0;
	}

	renderer.Update();

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

	if (cutsceneManager.watchingCutscene)
	{
		cutsceneManager.Update();
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
			Vector2 worldPosition = Vector2(mouseRect.x + renderer.camera.position.x, mouseRect.y + renderer.camera.position.y);

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

	quadTree.Reset();
	for (int i = 0; i < entities.size(); i++)
	{
		if (entities[i]->impassable || entities[i]->trigger 
			|| entities[i]->jumpThru || entities[i]->etype == "player")
			quadTree.Insert(entities[i]);
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
	if (!cutsceneManager.watchingCutscene)
	{
		if (renderer.camera.useOrthoCamera)
			renderer.camera.FollowTarget(*this);

		//renderer.guiCamera.FollowTarget(*this);
	}
}

void Game::SetScreenResolution(const unsigned int width, const unsigned int height)
{
	screenWidth = width;
	screenHeight = height;

	SDL_SetWindowSize(window, screenWidth, screenHeight);
	renderer.camera.ResetProjection(); // Zoom(0.0f, 1280.0f * Camera::MULTIPLIER, 720.0f * Camera::MULTIPLIER);
	renderer.guiCamera.Zoom(0.0f, screenWidth * Camera::MULTIPLIER, screenHeight * Camera::MULTIPLIER);

	if (prevScreenSprite != nullptr)
	{
		if (prevScreenSprite->texture != nullptr)
			delete_it(prevScreenSprite->texture);
		delete_it(prevScreenSprite);
	}
	if (screenSprite != nullptr)
	{
		if (screenSprite->texture != nullptr)
			delete_it(screenSprite->texture);
		delete_it(screenSprite);
	}

	screenSprite = InitFramebuffer(framebuffer, textureColorBuffer, renderBufferObject);
	prevScreenSprite = InitFramebuffer(prevFramebuffer, prevTextureColorBuffer, prevRenderBufferObject);

	glViewport(0, 0, screenWidth, screenHeight);
}


void Game::Render()
{
	// first pass
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // we're not using the stencil buffer now
	//glEnable(GL_DEPTH_TEST);

	RenderScene();

	// second pass
	bool renderSecondFrameBuffer = false;
	
	// Don't render the scene twice outside of cutscenes
	if (!cutsceneManager.watchingCutscene)
	{
		updateScreenTexture = false;
	}

	if (updateScreenTexture)
	{
		renderSecondFrameBuffer = true;
		float difference = cutsceneManager.printTimer.endTime - cutsceneManager.printTimer.startTicks;

		float t = 1.0f;
		if (difference != 0)
		{
			t = (cutsceneManager.printTimer.GetTicks() / difference); // percentage of passed time
		}

		if (t > 1.0f)
			t = 1.0f;

		float alpha = prevScreenSprite->color.a;
		LerpCoord(alpha, 255, 0, t);
		prevScreenSprite->color.a = alpha;

		if (prevScreenSprite->color.a <= 0)
		{
			//std::cout << "RENDER SECOND SCENE" << std::endl;
			updateScreenTexture = false;
			glBindFramebuffer(GL_FRAMEBUFFER, prevFramebuffer);
			glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // we're not using the stencil buffer now
			//glEnable(GL_DEPTH_TEST);
			RenderScene();
			prevScreenSprite->color.a = 255;
		}
	}

	// final pass
	glBindFramebuffer(GL_FRAMEBUFFER, 0); // back to default
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//glDisable(GL_DEPTH_TEST);

	// TODO: Set post-processing shaders here

	screenSprite->SetShader(renderer.shaders[ShaderName::Default]);

	Vector2 screenPos = Vector2(renderer.camera.startScreenWidth, renderer.camera.startScreenHeight);
	Vector2 screenScale = Vector2(renderer.camera.startScreenWidth / screenWidth, renderer.camera.startScreenHeight / -screenHeight);
	screenSprite->Render(screenPos, renderer, screenScale);

	if (renderSecondFrameBuffer)
	{
		prevScreenSprite->Render(Vector2(screenWidth, screenHeight), renderer, Vector2(1,-1));
	}

	/*
	screenSprite->SetShader(renderer.shaders[ShaderName::Sharpen]);
	screenSprite->Render(Vector2(screenWidth/2, screenHeight/2), renderer, Vector2(0.5f, -0.5f));
	screenSprite->SetShader(renderer.shaders[ShaderName::Test]);
	screenSprite->Render(Vector2(screenWidth + (screenWidth / 2), screenHeight + (screenHeight / 2)), renderer, Vector2(0.5f, -0.5f));
	screenSprite->SetShader(renderer.shaders[ShaderName::Grayscale]);
	screenSprite->Render(Vector2(screenWidth/2, screenHeight + (screenHeight/2)), renderer, Vector2(0.5f, -0.5f));
	screenSprite->SetShader(renderer.shaders[ShaderName::Edge]);
	screenSprite->Render(Vector2(screenWidth + (screenWidth / 2), screenHeight/2), renderer, Vector2(0.5f, -0.5f));
	*/

	glUseProgram(0);
	SDL_GL_SwapWindow(window);

	if (savingGIF)
	{
		SaveGIF();
	}
}


void Game::RenderScene()
{
	gui->RenderStart();

	// Render editor grid
	if (editMode)
	{
		//editor->DrawGrid();
	}

	// Render all backgrounds and their layers
	background->Render(renderer);
	quadTree.Render(renderer);

	// Render all entities
	if (renderer.camera.useOrthoCamera && !editMode)
	{
		entitiesToRender.clear();

		// TODO: Don't hardcode these numbers
		SDL_Rect cameraBounds;
		cameraBounds.x = renderer.camera.position.x - (6 * Globals::TILE_SIZE);
		cameraBounds.y = renderer.camera.position.y - (6 * Globals::TILE_SIZE);
		cameraBounds.w = (32 * Globals::TILE_SIZE) * Camera::MULTIPLIER;
		cameraBounds.h = (20 * Globals::TILE_SIZE) * Camera::MULTIPLIER;

		for (unsigned int i = 0; i < entities.size(); i++)
		{
			const SDL_Rect* theirBounds = entities[i]->GetBounds();

			if (HasIntersection(cameraBounds, *theirBounds))
			{
				entities[i]->Render(renderer);
				if (debugMode && (entities[i]->etype == "player"
					|| entities[i]->impassable || entities[i]->trigger || entities[i]->jumpThru))
					entitiesToRender.push_back(entities[i]);
			}
		}

		if (debugMode)
		{
			for (unsigned int i = 0; i < entitiesToRender.size(); i++)
			{
				entitiesToRender[i]->RenderDebug(renderer);
			}
		}

	}
	else
	{
		for (unsigned int i = 0; i < entities.size(); i++)
		{
			entities[i]->Render(renderer);
			if (debugMode)
				entities[i]->RenderDebug(renderer);
		}
	}

	if (currentLevel != "title" && !cutsceneManager.watchingCutscene && !editMode)
	{
		gui->Render(renderer);
	}

	// LAST THING
	// Render all menu screens
	if (openedMenus.size() > 0)
	{
		openedMenus[openedMenus.size() - 1]->Render(renderer);
	}

	// Draw anything in the cutscenes
	cutsceneManager.Render(renderer); // includes the overlay

	// Render editor toolbox
	if (editMode)
	{
		editor->Render(renderer);
	}

	//if (GetModeDebug())
#if _DEBUG
	editor->RenderDebug(renderer);
	if (debugMode)
	{
		//quadTree.Render(*renderer);
		if (quadrantEntities.size() > 0)
			quadTree.RenderEntities(renderer, quadrantEntities);

		debugScreen->Render(renderer);
	}
#endif
}

std::vector<std::string> Game::ReadStringsFromFile(const std::string& filepath)
{
	std::vector<std::string> result;

	std::ifstream fin;
	char token[256];

	fin.open(filepath);
	if (fin.is_open())
	{
		while (!fin.eof())
		{
			fin.getline(token, 256);
			result.push_back(token);
		}
	}

	return result;
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