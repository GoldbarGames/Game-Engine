#include "leak_check.h"

#include "Game.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iterator>
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
#include "Light.h"
#include "DirectionalLight.h"
#include "PointLight.h"

#include "SoundTest.h"
#include "SettingsButton.h"

#include "DebugScreen.h"
#include "EntityFactory.h"
#include "QuadTree.h"
#include "GUI.h"
#include "Dialog.h"
#include "CutsceneManager.h"
#include "SoundManager.h"
#include "RandomManager.h"

#if USE_NETWORKING
#include <nlohmann/json.hpp>
#endif

// For WebAssembly builds only
#ifdef EMSCRIPTEN

#include <emscripten/emscripten.h>
#include <emscripten/html5.h>

#endif

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

void WebGLMainLoop(Game* game)
{
	game->MainLoop();
}

int Game::BeforeMainLoop()
{
	// Load settings
	LoadSettings();
	LoadTitleScreen();
	SortEntities(entities);
	timer.Start();

	if (autoGIFsDelay > 0 && autoGIFsDuration > 0)
	{
		autoGifDelayTimer.Start(autoGIFsDelay);
	}
	else if (autoScreenshots > 0)
	{
		screenshotTimer.Start(autoScreenshots);
	}

#ifdef EMSCRIPTEN
	//emscripten_request_animation_frame_loop(one_iter, 0);
	std::cout << "Attempting to set and enter the main loop..." << std::endl;
	emscripten_set_main_loop_arg((em_arg_callback_func)WebGLMainLoop, this, 0, 1);
#else
	while (!shouldQuit)
	{
		MainLoop();
	}
#endif

	return 0;
}



int Game::MainLoop()
{
	renderer.drawCallsPerFrame = 0;

	CalcDt();

	//std::cout << "---" << std::endl;
	allocationCount = 0;

#if _DEBUG
	showFPS = true;
	if (showFPS)
	{
		timeLeft -= dt;
		fpsSum += 1000 / dt;
		if (timeLeft <= 0)
		{
			currentNumberOfFrames = (int)(fpsSum / frames);
			if (currentNumberOfFrames != previousNumberOfFrames)
			{
				gui->GetText(guiFPS)->SetText(guiFPS2 + std::to_string(currentNumberOfFrames));
				previousNumberOfFrames = currentNumberOfFrames;
				if (currentNumberOfFrames < 59)
					std::cout << currentNumberOfFrames << std::endl;
			}

			timeLeft = updateInterval;
			fpsSum = 0;
			frames = 0;
		}
		frames++;
	}
#endif

	if (showTimer)
	{
		gui->GetText(guiTimer)->SetText(std::to_string(timer.GetTicks() / 1000.0f));
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

#if _DEBUG
	if (savingGIF)
	{
		if (autoGIFsDuration > 0 && autoGifDurationTimer.HasElapsed())
		{
			EndGIF();
		}
	}
	else if (autoGIFsDelay > 0 && autoGifDelayTimer.HasElapsed())
	{
		StartGIF("screenshots/gif/frames/");
		autoGifDurationTimer.Start(autoGIFsDuration);
	}
	else if (autoScreenshots > 0 && screenshotTimer.HasElapsed())
	{
		screenshotTimer.Start(autoScreenshots);

		SaveScreenshot("screenshots/auto/", "", ".png");
	}
#endif

	return 0;
}


Game::Game(const std::string& n, const std::string& title, const std::string& icon, bool is2D,
	const EntityFactory& e, const FileManager& f, GUI& g, MenuManager& m, CutsceneHelper* ch, NetworkManager* net) : logger("logs/output.log")
{
	currentGame = n;
	startOfGame = std::chrono::steady_clock::now();
	entityFactory = &e;
	networkManager = net;

	use2DCamera = is2D;

	fileManager = &f;
	fileManager->Init(*this);

	menuManager = &m;

	windowTitle = title;
	windowIconFilepath = icon;

	InitSDL();

	renderer.Init(this);
	spriteManager.Init(&renderer);

	InitOpenGL();

	std::cout << "After OpenGL Init..." << std::endl;

	//Sprite::mesh = CreateSpriteMesh();
	cubeMesh = CreateCubeMesh();

	// Initialize the font before all text

	// TODO: Load all fonts from a file (fonts.list)
	theFont = CreateFont("SazanamiGothic", m.GetFontSize());
	headerFont = CreateFont("SazanamiGothic", m.GetFontSize() * 2);

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

	// Initialize the translation maps
	ReadTranslationData();

	editor = new Editor(*this);
	debugScreen = new DebugScreen(*this);

	// Initialize this AFTER OpenGL, Fonts, and Editor
	soundManager.Init(this);

	dirNames = ReadStringsFromFile("data/dirs.dat");
	initialStates = GetMapStringsFromFile("data/istates.dat");

	entities.clear();

	SetScreenResolution(renderer.camera.startScreenWidth, renderer.camera.startScreenHeight);

	// Initialize GUI (do this AFTER fonts and resolution)
	gui = &g;
	gui->Init(this);

	inputManager.Init();

	// Initialize all the menus (do this AFTER fonts and resolution)
	menuManager->Init(*this);

	LoadEditorSettings();

	previousTime = clock::now();

	if (!use2DCamera)
	{
		glm::vec3 lColor2 = glm::vec3(0.0f, 1.0f, 0.0f);

		glm::vec3 lColor = glm::vec3(1.0f, 1.0f, 1.0f);
		glm::vec3 lDir = glm::vec3(2.0f, 2.0f, 2.0f);

		shinyMaterial = Material(1.0f, 16);
		dullMaterial = Material(0.3f, 4);

		renderer.light = new DirectionalLight(lColor, 0.4f, 0.2f, lDir);

		glm::vec3 pos1 = glm::vec3(0.0f, 0.0f, 0.0f);
		glm::vec3 pos2 = glm::vec3(5.0f, 0.0f, 0.0f);
		glm::vec3 attenuation = glm::vec3(0.3f, 0.2f, 0.1f);

		renderer.pointLightCount = 0;
		renderer.pointLights[0] = new PointLight(lColor2, 0.4f, 0.2f, pos1, attenuation);
		renderer.pointLightCount++;

		renderer.spotLightCount = 0;
		renderer.spotLights[0] = new SpotLight(lColor2, 0.4f, 0.2f, pos2, attenuation, lDir, 20.0f);
		renderer.spotLightCount++;

		// shaders[5] = Diffuse
		triangle3D = new Sprite(renderer.shaders[5], MeshType::Pyramid);
		triangle3D->color = { 255, 0, 0, 255 };

		// Shiny Material
		triangle3D->material = &shinyMaterial;

		// Dull Material
		//triangle3D->material = &dullMaterial;

		//cutsceneManager.commands.ExecuteCommand("shader pyramid data/shaders/default.vert data/shaders/pyramid.frag");
		//triangle3D->SetShader(cutsceneManager.commands.customShaders["pyramid"]);

#ifdef USE_ASSIMP
		modelChopper.LoadModel("assets/models/chopper/chopper.obj");
#endif

	}
	else
	{
		glm::vec3 lColor = glm::vec3(1.0f, 1.0f, 1.0f);
		glm::vec3 lDir = glm::vec3(2.0f, 2.0f, 2.0f);
		renderer.light = new DirectionalLight(lColor, 1.0f, 1.0f, lDir);
	}

	cutsceneManager.commands.helper = ch;
	cutsceneManager.commands.helper->SetFunctions(cutsceneManager.commands);

	std::cout << "Game Created" << std::endl;
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

	if (mainFrameBuffer != nullptr)
	{
		delete_it(mainFrameBuffer);
	}

	if (prevMainFrameBuffer != nullptr)
	{
		delete_it(prevMainFrameBuffer);
	}

	if (cutsceneFrameBuffer != nullptr)
	{
		delete_it(cutsceneFrameBuffer);
	}

	if (prevCutsceneFrameBuffer != nullptr)
	{
		delete_it(prevCutsceneFrameBuffer);
	}

	if (cursorSprite != nullptr)
		delete_it(cursorSprite);		

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
		delete fonts[key];
	}

	EndSDL();
}

Sprite* Game::CreateSprite(const std::string& filepath, const int shaderName)
{
	return new Sprite(spriteManager.GetImage(filepath), renderer.shaders[shaderName]);
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

	// TODO: Don't hardcode these? What if we want to add new ones?
	entityTypes["cameraBounds"] = { "cameraBounds" };
	entityTypes["particle"] = { "particle" };

	spriteMap.clear();
	for (auto const& [key, val] : entityTypes)
	{
		for (int i = 0; i < val.size(); i++)
		{
			spriteMap[key].push_back("assets/sprites/" + key + "/" + val[i] + ".png");
		}
	}
}

FontInfo* Game::CreateFont(const std::string& fontName, int size)
{
	std::string key = fontName + std::to_string(size);
	if (fonts.count(key) == 0)
	{
		fonts[key] = new FontInfo("fonts/" + fontName + "/" + fontName + "-Regular.ttf", size);
		fonts[key]->SetBoldFont("fonts/" + fontName + "/" + fontName + "-Bold.ttf");
		fonts[key]->SetItalicsFont("fonts/" + fontName + "/" + fontName + "-Italic.ttf");
		fonts[key]->SetBoldItalicsFont("fonts/" + fontName + "/" + fontName + "-BoldItalic.ttf");
	}

	return fonts[key];
}

void Game::CalcDt()
{
	// NOTE: Because this only gets updated once per loop,
	// multiple calls to it within a single loop will not match the real time
	Globals::CurrentTicks = SDL_GetTicks();

	dt = std::chrono::duration<float, milliseconds::period>(clock::now() - previousTime).count();
	previousTime = clock::now();

	// When we are debugging and hit a breakpoint in an IDE, the timer continues running.
	// This causes the dt to become huge and throw everything around like crazy.
	// So reset the dt if it becomes too big so that we can debug properly.
	if (dt > 100)
		dt = 33;

	dtUnscaled = dt;
	dt *= timeScale;

	now = std::chrono::duration<float, milliseconds::period>(previousTime - startOfGame).count();
	renderer.now = now;
}

void Game::InitOpenGL()
{
	std::cout << "Init OpenGL..." << std::endl;


#if EMSCRIPTEN

	std::cout << "Attempting to create emscripten WebGL context..." << std::endl;

	// 3.2 is part of the modern versions of OpenGL, but most video cards whould be able to run it
	if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3) != 0)
	{
		logger.Log("ERROR: SDL_GL_SetAttribute SDL_GL_CONTEXT_MAJOR_VERSION failed. " + std::string(SDL_GetError()));
	}

	if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2) != 0)
	{
		logger.Log("ERROR: SDL_GL_SetAttribute SDL_GL_CONTEXT_MINOR_VERSION failed. " + std::string(SDL_GetError()));
}

	// Set our OpenGL version.
	// SDL_GL_CONTEXT_CORE gives us only the newer version, deprecated functions are disabled
	if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES) != 0)
	{
		logger.Log("ERROR: SDL_GL_SetAttribute SDL_GL_CONTEXT_PROFILE_MASK failed. " + std::string(SDL_GetError()));
	}

	EmscriptenWebGLContextAttributes attrs;
	attrs.antialias = true;
	attrs.majorVersion = 3;
	attrs.minorVersion = 2;
	attrs.alpha = true;
	attrs.powerPreference = EM_WEBGL_POWER_PREFERENCE_DEFAULT;

	// The following lines must be done in exact order, or it will break!
	emscripten_webgl_init_context_attributes(&attrs); // you MUST init the attributes before creating the context
	attrs.majorVersion = 3; // you MUST set the version AFTER the above line
	EMSCRIPTEN_WEBGL_CONTEXT_HANDLE webgl_context = emscripten_webgl_create_context("#canvas", &attrs);
	emscripten_webgl_make_context_current(webgl_context);
	mainContext = SDL_GL_CreateContext(window);

#else
	// 3.2 is part of the modern versions of OpenGL, but most video cards whould be able to run it
	if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3) != 0)
	{
		logger.Log("ERROR: SDL_GL_SetAttribute SDL_GL_CONTEXT_MAJOR_VERSION failed. " + std::string(SDL_GetError()));
	}

	if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2) != 0)
	{
		logger.Log("ERROR: SDL_GL_SetAttribute SDL_GL_CONTEXT_MINOR_VERSION failed. " + std::string(SDL_GetError()));
	}

	// Set our OpenGL version.
	// SDL_GL_CONTEXT_CORE gives us only the newer version, deprecated functions are disabled
	if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE) != 0)
	{
		logger.Log("ERROR: SDL_GL_SetAttribute SDL_GL_CONTEXT_PROFILE_MASK failed. " + std::string(SDL_GetError()));
	}

	std::cout << "Creating context..." << std::endl;

	// Set up OpenGL context - CALL THIS AFTER SETTING THE ATTRIBUTES!
	// If you call in the wrong order, won't display correctly on many devices!
	mainContext = SDL_GL_CreateContext(window);

	if (SDL_GL_MakeCurrent(window, mainContext) != 0)
	{
		std::cout << "Context failed..." << std::endl;
		logger.Log("ERROR: SDL_GL_MakeCurrent failed. " + std::string(SDL_GetError()));
	}
#endif

	// Turn on double buffering with a 24bit Z buffer.
	// You may need to change this to 16 or 32 for your system
	if (SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1) != 0)
	{
		logger.Log("ERROR: SDL_GL_SetAttribute SDL_GL_DOUBLEBUFFER failed. " + std::string(SDL_GetError()));
	}

	std::cout << "GL_VERSION: " << glGetString(GL_VERSION) << std::endl;
	//std::cout << "GL_VERSION? " << std::endl;

	// Parameter 0 = no vsync, 1 = vsync
	if (SDL_GL_SetSwapInterval(1) != 0)
	{
		logger.Log("ERROR: SDL_GL_SetSwapInterval failed. " + std::string(SDL_GetError()));
	}

#ifndef EMSCRIPTEN
	glewExperimental = GL_TRUE;
	glewInit();
	glAlphaFunc(GL_GREATER, 0.1);
	glEnable(GL_ALPHA_TEST);
#endif

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glViewport(0, 0, screenWidth, screenHeight);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	SDL_GL_SwapWindow(window);

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

	// Creating and binding a mesh before creating the shaders is necessary
	// for other platforms that use Open GL 4.X like Macs
	Mesh* m = CreateQuadMesh();
	m->BindMesh();

	renderer.CreateShaders(); // we must create the shaders at this point

	m->ClearMesh();

	mainFrameBuffer = new FrameBuffer(renderer, screenWidth, screenHeight);
	prevMainFrameBuffer = new FrameBuffer(renderer, screenWidth, screenHeight);
	cutsceneFrameBuffer = new FrameBuffer(renderer, screenWidth, screenHeight);
	prevCutsceneFrameBuffer = new FrameBuffer(renderer, screenWidth, screenHeight);
}

void Game::InitSDL()
{
	
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER);
	TTF_Init();

	std::cout << "Creating window..." << std::endl;

	window = SDL_CreateWindow(windowTitle.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, screenWidth, screenHeight, SDL_WINDOW_OPENGL);

	if (window == nullptr)
	{
		logger.Log("ERROR: SDL failed to create window!");
	}
	else
	{
		SDL_SetWindowIcon(window, IMG_Load(windowIconFilepath.c_str()));
	}

	CheckController(true);
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

	TTF_Quit();
	SDL_Quit();
	IMG_Quit();
}

void Game::CheckController(bool output)
{
	if (SDL_NumJoysticks() < 1)
	{
		if (output)
		{
			std::cout << "No controller connected." << std::endl;
		}
	}
	else
	{
		for (int i = 0; i < SDL_NumJoysticks(); i++)
		{
			if (SDL_IsGameController(i))
			{
				if (output)
				{
					std::cout << "Controller connected!" << std::endl;
				}

				controller = SDL_GameControllerOpen(i);
				std::cout << SDL_GameControllerMapping(controller) << std::endl;
				break;
			}
		}

		SDL_Joystick* joystick = SDL_JoystickOpen(0);

		if (output)
		{
			std::cout << "Controller Name: " << SDL_JoystickName(joystick) << std::endl;
			std::cout << "Num Axes: " << SDL_JoystickNumAxes(joystick) << std::endl;
			std::cout << "Num Buttons: " << SDL_JoystickNumButtons(joystick) << std::endl;
		}

		SDL_JoystickClose(joystick);
	}
}

glm::vec3 Game::SnapToGrid(glm::vec3 position, int size)
{
	int x = position.x - ((int)(position.x) % (size));
	int y = position.y - ((int)(position.y) % (size));

	if (x % 2 != 0)
		x++;

	if (y % 2 != 0)
		y++;

	return glm::vec3(x, y, position.z);
}

void Game::RefreshAnimator(Entity* newEntity, const std::string& entityName) const
{
	if (newEntity == nullptr)
		return;

	std::unordered_map<std::string, std::string> args;

	std::string filepath = entityName + "/";

	args["0"] = entityName; // std::to_string(subtype);
	args["1"] = "";

	if (entityTypes.count(entityName) > 0 && entityTypes[entityName].size() > newEntity->subtype)
	{
		args["1"] = entityTypes[entityName][newEntity->subtype];
	}

	bool subdirectory = false;
	for (int i = 0; i < dirNames.size(); i++)
	{
		if (entityName == dirNames[i])
		{
			subdirectory = true;
		}
	}

	if (args["1"] != "" && subdirectory)
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
	// so that you can write Animator files that immediately go to other states
	// (such as "notidle: bool true == true")
	// - OR add a simple way to define the starting state in the animator file itself
	// (such as "^*unpressed*") and otherwise just default to the topmost state

	if (animStates.size() > 0)
	{
		Animator* newAnimator = nullptr;

		// TODO: It seems like we need a new animator for each entity
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
			std::string initialState = initialStates.count(newEntity->etype) ? initialStates.at(newEntity->etype) : "idle";
			newAnimator = new Animator(filepath, animStates, initialState);
			//animators[filepath] = newAnimator;
		}

		newEntity->SetAnimator(*newAnimator);
	}
}

Entity* Game::CreateEntity(const std::string& entityName, const glm::vec3& position, int subtype) const
{
	Entity* newEntity = entityFactory->Create(entityName, position);

	if (newEntity != nullptr)
	{
		newEntity->subtype = subtype;
		RefreshAnimator(newEntity, entityName);	
	}

	return newEntity;
}

// TODO: We want to sort the entities every time we spawn one
// as long as the game is running, but not when we are first loading the level
Entity* Game::SpawnEntity(const std::string& entityName, const glm::vec3& position, const int subtype) const
{
	Entity* entity = CreateEntity(entityName, position, subtype); //entityFactory->Create(entityName, position);

	if (entity != nullptr)
	{
		entity->subtype = subtype;
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
glm::vec3 Game::CalculateObjectSpawnPosition(glm::vec2 mousePos, const int GRID_SIZE)
{
	glm::vec3 worldPosition = ConvertFromScreenSpaceToWorldSpace(mousePos);

	int afterModX = ((int)(worldPosition.x) % (GRID_SIZE * (int)Camera::MULTIPLIER));
	int afterModY = ((int)(worldPosition.y) % (GRID_SIZE * (int)Camera::MULTIPLIER));

	glm::vec2 snappedPos = glm::vec2(worldPosition.x - afterModX, worldPosition.y - afterModY);

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

	return glm::vec3(newTileX, newTileY, 0);
}

Tile* Game::CreateTile(const glm::vec2& frame, const int tilesheetIndex,
	const glm::vec3& position, DrawingLayer drawingLayer) const
{
	Tile* tile = new Tile(position, frame, 
		spriteManager.GetImage(editor->GetTileSheetFileName(tilesheetIndex)),
		renderer, editor->SPAWN_TILE_SIZE);

	tile->layer = drawingLayer;
	tile->impassable = drawingLayer == DrawingLayer::COLLISION
		|| drawingLayer == DrawingLayer::COLLISION2;
	tile->tilesheetIndex = tilesheetIndex;

	return tile;
}

Tile* Game::SpawnTile(const glm::vec2& frame, const int tilesheetIndex,
	const glm::vec3& position, DrawingLayer drawingLayer) const
{
	Tile* tile = new Tile(position, frame, 
		spriteManager.GetImage(editor->GetTileSheetFileName(tilesheetIndex)), 
		renderer, editor->SPAWN_TILE_SIZE);

	tile->layer = drawingLayer;
	tile->impassable = drawingLayer == DrawingLayer::COLLISION 
		|| drawingLayer == DrawingLayer::COLLISION2;

	tile->tilesheetIndex = tilesheetIndex;
	tile->CalculateCollider();

	entities.emplace_back(tile);

	return tile;
}

// TODO: Spawning more than one player this way breaks things
Entity* Game::SpawnPlayer(const glm::vec3& position)
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
	ShouldDeleteEntity(entities[index]);
}

void Game::ShouldDeleteEntity(Entity* entity)
{
	// Only add this entity to the list if it is not already there
	std::vector<Entity*>::iterator iter = std::find(entitiesToDelete.begin(), entitiesToDelete.end(), entity);
	if (iter == entitiesToDelete.end())
	{
		entitiesToDelete.emplace_back(entity);
	}
	else
	{
		logger.LogEntity("Failed to mark entity for deletion", *entity);
	}
}

void Game::DeleteEntity(Entity* entity)
{
	std::vector<Entity*>::iterator index = std::find(entities.begin(), entities.end(), entity);
	if (index != entities.end())
	{
		delete_it(*index);
		entities.erase(index);
		entitiesToDelete.erase(entitiesToDelete.begin());
	}	
	else
	{
		logger.LogEntity("Failed to delete entity", *entity);
	}
}

void Game::DeleteEntity(int index)
{
	// TODO: Is there a more efficient way of handling this?
	if (entities[index]->isLightSource)
	{
		for (int i = 0; i < lightSourcesInLevel.size(); i++)
		{
			if (entities[index] == lightSourcesInLevel[i])
			{
				lightSourcesInLevel.erase(lightSourcesInLevel.begin() + i);
				break;
			}
		}
	}

	delete_it(entities[index]);
	entities.erase(entities.begin() + index);
	entitiesToDelete.erase(entitiesToDelete.begin());
}

void Game::StartTextInput(Dialog& dialog, const std::string& reason)
{
	currentDialog = &dialog;
	shouldUpdateDialogInput = true;
	inputReason = reason;
	SDL_StartTextInput();
	inputText = "";
	inputType = "String";
}

void Game::StopTextInput(Dialog& dialog)
{
	shouldUpdateDialogInput = false;
	dialog.visible = false;
	SDL_StopTextInput();

	if (inputReason == "properties")
	{		
		editor->SetPropertyText(inputText);
		editor->propertyIndex = -1;
		editor->DoAction();
	}
	else if (inputReason == "sound_test_dir")
	{
		soundManager.soundTest->AfterDirDialog(inputText);
	}
	else if (inputReason == "sound_test_file")
	{
		soundManager.soundTest->AfterFileDialog(inputText);
	}
	else if (inputReason == "sound_test_jump")
	{
		soundManager.soundTest->AfterJumpDialog(inputText);
	}
	else if (inputReason == "sound_test_loop_time1")
	{
		soundManager.soundTest->AfterLoopDialog1(inputText);
	}
	else if (inputReason == "sound_test_loop_time2")
	{
		soundManager.soundTest->AfterLoopDialog2(inputText);
	}
	else if (inputReason == "sound_test_loop_color")
	{
		soundManager.soundTest->AfterLoopDialog3(inputText);
	}
	else if (inputReason == "start_watch")
	{
		waitingForDebugDialog = false;
		cutsceneManager.inputTimer.Start(cutsceneManager.inputTimeToWait * 2);

		if (inputText != "")
		{
			debugScreen->InsertVariable(inputText);
		}
	}
	else if (inputReason == "end_watch")
	{
		waitingForDebugDialog = false;
		cutsceneManager.inputTimer.Start(cutsceneManager.inputTimeToWait * 2);

		if (inputText != "")
		{
			debugScreen->RemoveVariable(inputText);
		}
	}
	else if (inputReason == "new_level")
	{
		if (inputText != "")
		{
			editor->ClearLevelEntities();
			player = SpawnPlayer(glm::vec3(0, 0, 0));

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
			background->SpawnBackground(inputText, 0, 0, *this);
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
		newEntityName[0] = std::toupper(newEntityName[0]);

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
		std::string directory = fs::current_path().string() + "\\data\\animators\\";
		fs::create_directories(directory + newEntityName + "\\");

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

		fs::path path = fs::current_path();
		std::vector<std::string> classNames;
		for (const auto& entry : fs::directory_iterator(path))
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
			if (key == "cameraBounds" || key == "path" || key == "pathnode" || key == "particle")
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

		// TODO: 6. Modify Loading file


	}
}

void Game::LoadTitleScreen()
{
	renderer.camera.ResetCamera();
	openedMenus.clear();

	// Automatically create a first level if none exists
	std::ifstream fin;
	fin.open("data/levels/title.lvl");

	if (!fin.is_open())
	{
		std::ofstream fout;
		fout.open("data/levels/title.lvl");
		fout << "0 bg 0 0 title" << std::endl;
		fout.close();
	}
	else
	{
		fin.close();
	}

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
				fileManager->AfterLoadLevelFromFile();
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
	//clickedMouse = false;

	bool quit = false;

	// Reset controller buttons each frame
	for (auto& [key, val] : inputManager.buttonsPressed)
	{
		inputManager.buttonsPressed[key] = false;
	}

	for (auto& [key, val] : inputManager.buttonsReleased)
	{
		inputManager.buttonsReleased[key] = false;
	}

	// Check for inputs
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
		case SDL_MOUSEBUTTONUP:
			cutsceneManager.previousMouseState = 0;
			break;
		case SDL_CONTROLLERBUTTONDOWN:
			inputManager.buttonsPressed[event.cbutton.button] = true;

			if (event.cbutton.button == SDL_CONTROLLER_BUTTON_A)
			{
				std::cout << "PRESSED A" << std::endl;
			}
			else if (event.cbutton.button == SDL_CONTROLLER_BUTTON_B)
			{
				std::cout << "PRESSED B" << std::endl;
			}
			break;
		case SDL_CONTROLLERBUTTONUP:
			inputManager.buttonsReleased[event.cbutton.button] = true;
			break;

		case SDL_KEYDOWN:
			switch (event.key.keysym.sym) {
			case SDLK_RETURN:
				std::cout << "HIT RETURN EVENT" << std::endl;
				break;
			default:
				break;
			}
			break;

		case SDL_KEYUP:
			switch (event.key.keysym.sym) {
			case SDLK_RETURN:
				std::cout << "HIT RETURN EVENT UP" << std::endl;
				break;
			default:
				break;
			}
			break;

		default:
			break;
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
	// We use a while loop because we must remove from the list while deleting
	while(entitiesToDelete.size() > 0)
	{
		DeleteEntity(entitiesToDelete[0]);
	}	
}

void Game::HandleEditMode()
{
	if (!shouldUpdateDialogInput)
	{
		const Uint8* input = SDL_GetKeyboardState(NULL);
		renderer.camera.KeyControl(input, dt, screenWidth, screenHeight);
		renderer.guiCamera.KeyControl(input, dt, screenWidth, screenHeight);

		editor->HandleEdit();
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
	fout << "level " << (currentLevel == "" ? "title" : currentLevel ) << std::endl;

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
	fout << "language " << Globals::currentLanguageIndex << std::endl;

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

		bool hasSettingsButton = (allMenus["Settings"] != nullptr);

		// TODO: Refactor this so that we have different menus for different games
		// but still don't have to rewrite a bunch of code to change basic settings.

		if (tokens[0] == "music_volume")
		{
			soundManager.SetVolumeBGMIndex(std::stoi(tokens[1]));

			if (hasSettingsButton)
			{
				SettingsButton* button = dynamic_cast<SettingsButton*>(allMenus["SoundSettings"]->GetButtonByName("Music Volume"));
				if (button != nullptr)
				{
					button->selectedOption = std::stoi(tokens[1]);
				}				
			}

		}
		else if (tokens[0] == "sound_volume")
		{
			soundManager.SetVolumeSoundIndex(std::stoi(tokens[1]));

			if (hasSettingsButton)
			{
				SettingsButton* button = dynamic_cast<SettingsButton*>(allMenus["SoundSettings"]->GetButtonByName("Sound Volume"));
				if (button != nullptr)
				{
					button->selectedOption = std::stoi(tokens[1]);
				}				
			}

		}
		else if (tokens[0] == "fullscreen")
		{
			isFullscreen = std::stoi(tokens[1]);

			if (isFullscreen)
				SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
			else
				SDL_SetWindowFullscreen(window, 0);	

			if (hasSettingsButton)
			{
				SettingsButton* button = dynamic_cast<SettingsButton*>(allMenus["DisplaySettings"]->GetButtonByName("Windowed"));
				if (button != nullptr)
				{
					button->selectedOption = std::stoi(tokens[1]);
				}
			}


		}
		else if (tokens[0] == "screen_resolution")
		{
			indexScreenResolution = std::stoi(tokens[1]);

			if (hasSettingsButton)
			{
				SettingsButton* button = dynamic_cast<SettingsButton*>(allMenus["DisplaySettings"]->GetButtonByName("Screen Resolution"));

				if (button != nullptr)
				{
					button->selectedOption = indexScreenResolution;
					button->ExecuteSelectedOption(*this);
				}
			}
		}
		else if (tokens[0] == "display_fps")
		{
			showFPS = std::stoi(tokens[1]);

			if (hasSettingsButton)
			{
				SettingsButton* button = dynamic_cast<SettingsButton*>(allMenus["DisplaySettings"]->GetButtonByName("Display FPS"));
				if (button != nullptr)
				{
					button->selectedOption = std::stoi(tokens[1]);
				}
			}
		}
		else if (tokens[0] == "display_timer")
		{
			showTimer = std::stoi(tokens[1]);

			if (hasSettingsButton)
			{
				SettingsButton* button = dynamic_cast<SettingsButton*>(allMenus["DisplaySettings"]->GetButtonByName("Display Timer"));
				if (button != nullptr)
				{
					button->selectedOption = std::stoi(tokens[1]);
				}
			}
		}
		else if (tokens[0] == "language")
		{
			Globals::currentLanguageIndex = std::stoi(tokens[1]);

			if (hasSettingsButton)
			{
				SettingsButton* button = dynamic_cast<SettingsButton*>(allMenus["OtherSettings"]->GetButtonByName("Language"));
				if (button != nullptr)
				{
					button->selectedOption = std::stoi(tokens[1]);
				}
			}
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
		if (inputManager.isCheckingForKeyMapping)
		{
			inputManager.pressedKey = event.key.keysym.scancode;
		}
		else
		{
			std::cout << "Handle Menu Event" << std::endl;
			switch (event.key.keysym.sym)
			{
			case SDLK_RETURN:
				std::cout << "Handle Menu Event - hit return key" << std::endl;
				quit = openedMenus[openedMenus.size() - 1]->PressSelectedButton(*this);
				break;
#if _DEBUG
			case SDLK_2:
				guiMode = !guiMode;
				break;
#endif
			default:
				break;
			}
		}
		
	}

	return quit;
}

void Game::ResetLevel()
{
	editor->InitLevelFromFile(currentLevel);
}

void Game::SetFullScreen(bool setFull)
{
	if (setFull)
	{
		//TODO: Should we figure out how to use FULLSCREEN_DESKTOP?
		SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
	}
	else
	{
		SDL_SetWindowFullscreen(window, 0);
	}	

	isFullscreen = setFull;
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
				if (cutsceneManager.watchingCutscene)
				{
					if (!cutsceneManager.waitingForButton)
					{
						cutsceneManager.OpenBacklog();
					}
				}
				else
				{
					renderer.camera.Zoom(-0.1f, screenWidth, screenHeight);
				}

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
				if (cutsceneManager.watchingCutscene)
				{
					if (cutsceneManager.readingBacklog)
					{
						cutsceneManager.CloseBacklog();
					}
					else // advance text
					{

					}					
				}
				else
				{
					renderer.camera.Zoom(0.1f, screenWidth, screenHeight);
				}

			}
		}
	}

	if (event.type == SDL_KEYDOWN)
	{
		if (shouldUpdateDialogInput)
		{
			//Handle backspace
			if (event.key.keysym.sym == SDLK_BACKSPACE && inputText.length() > 0)
			{
				inputText.pop_back();
				editor->dialog->Update(inputText);
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
				editor->dialog->Update(inputText);
			}
			else if (event.key.keysym.sym == SDLK_DOWN)
			{
				std::string optionString = editor->GetCurrentPropertyOptionString(1);
				if (optionString != "")
				{
					inputText = optionString;
					editor->dialog->Update(inputText);
				}
			}
			else if (event.key.keysym.sym == SDLK_UP)
			{
				std::string optionString = editor->GetCurrentPropertyOptionString(-1);
				if (optionString != "")
				{
					inputText = optionString;
					editor->dialog->Update(inputText);
				}
			}
			// Pressed enter, submit the input
			else if (event.key.keysym.sym == SDLK_RETURN)
			{
				if (currentDialog != nullptr)
				{
					StopTextInput(*currentDialog);
				}				
			}
		}
		else
		{

#if _DEBUG
			switch (event.key.keysym.sym)
			{


			// DEVELOPER BUTTONS

			case SDLK_r:
				if (!editMode && !freeCameraMode)
					ResetLevel();
				break;			
			case SDLK_t:
				if (!editMode && !freeCameraMode)
					LoadLevel(nextLevel);
				break;
			case SDLK_0:
#if USE_NETWORKING
				// Save this level data to the database
				if (editMode)
				{
					std::cout << "Saving level to database..." << std::endl;
					nlohmann::json leveldata;
					leveldata["name"] = currentLevel;
					leveldata["data"] = editor->ReadLevelFromFile(currentLevel);
					networkManager->curlPostRequest("http://localhost:3000/api/levels", leveldata.dump().c_str(), "");
				}					
#endif
				break;
			case SDLK_1: // toggle Debug mode
				std::cout << "Handle Menu Event - hit 1 key" << std::endl;
				debugMode = !debugMode;
				break;
			case SDLK_2: // toggle Editor mode

				if (!soundMode && !guiMode)
				{
					if (openedMenus.size() > 0)
					{
						guiMode = !guiMode;
					}
					else
					{
						editMode = !editMode;

						if (editMode)
						{
							editor->StartEdit();
						}
						else
						{
							editor->StopEdit();
						}
					}

				}

				break;
			case SDLK_3: // toggle Editor settings
				if (editMode)
				{
					openedMenus.emplace_back(allMenus["EditorSettings"]);
				}
				else if (!guiMode)
				{
					// if not in edit mode, bring up/down the Sound Test
					soundMode = !soundMode;
				}
				break;
			case SDLK_4: // GUI edit button
				if (editMode)
				{
					// ...?
				}
				else if (!soundMode)
				{
					// if not in edit mode, bring up/down the Gui Editor
					guiMode = !guiMode;
				}
				break;
			case SDLK_MINUS: // Undo Button
				editor->UndoAction();
				//cutsceneManager.renderCutscene = !cutsceneManager.renderCutscene;
				break;
			case SDLK_PLUS: // Redo Button

				// Free camera mode
				//freeCameraMode = !freeCameraMode;

				editor->RedoAction();
				//EndGIF();
				break;
			case SDLK_6: // Screenshot Button
				SaveScreenshot("screenshots/", "", ".png");
				break;
			case SDLK_7:
				//_CrtDumpMemoryLeaks();
				
				if (inputManager.isRecordingInput)
					inputManager.StopRecording();
				else
					inputManager.StartRecording();

				std::cout << "Toggle recording " << inputManager.isRecordingInput << std::endl;

				break;
			case SDLK_8: // save game
				//fileManager->SaveFile(currentSaveFileName);
				
				if (inputManager.isPlayingBackInput)
					inputManager.StopPlayback();
				else
					inputManager.StartPlayback("data/inputs.dat");
								
				std::cout << "Toggle playback " << inputManager.isPlayingBackInput << std::endl;

				break;
			case SDLK_9: 

				if (savingGIF)
				{
					EndGIF();
				}
				else
				{
					StartGIF("screenshots/gif/frames/");
				}

				break;

			default:
				break;
			}

#endif

		}
		
	}
	//Special text input event
	else if (event.type == SDL_TEXTINPUT)
	{
		if (shouldUpdateDialogInput)
		{
			//Not copy or pasting, just entering characters as usual
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
						|| (c == '.' && inputText.size() > 1 && inputText[inputText.size() - 1] != '-' && inputText.find('.') == string::npos);
				}

				if (valid)
				{
					//Append character
					inputText += c;
					editor->dialog->Update(inputText);
				}
			}
		}
		else
		{

		}
	}

	return quit;
}

void Game::StartGIF(const std::string& filepath)
{
	savingGIF = true;
	gifFrameNumber = 0;
	gifFolderPath = filepath;
}

void Game::EndGIF()
{
	savingGIF = false;

	std::string timestamp = CurrentDate() + "-" + CurrentTime();
	for (int i = 0; i < timestamp.size(); i++)
	{
		if (timestamp[i] == ':')
			timestamp[i] = '-';
	}

	std::string command = "data\\editor\\creategif.bat " + timestamp;

	system(command.c_str());
}

// TODO: This really lags the game... do we need multi-threading here?
void Game::SaveGIF()
{
	std::string filename = std::to_string(gifFrameNumber);

	if (gifFrameNumber < 10)
	{
		filename.insert(filename.begin(), '0');
		filename.insert(filename.begin(), '0');
	}
	else if (gifFrameNumber < 100)
	{
		filename.insert(filename.begin(), '0');
	}

	SaveScreenshot(gifFolderPath, filename, ".bmp");
	gifFrameNumber++;

	// Stop if overflow
	if (gifFrameNumber < 0)
	{
		EndGIF();
	}
}

void Game::SaveScreenshot(const std::string& filepath, const std::string& filename, const std::string& extension)
{
#ifndef EMSCRIPTEN
	const unsigned int bytesPerPixel = 3;

	unsigned char* pixels = new unsigned char[screenWidth * screenHeight * bytesPerPixel]; // 4 bytes for RGBA
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

		if (extension == ".png")
			IMG_SavePNG(screenshot, ("screenshots/" + timestamp + extension).c_str());
		else
			SDL_SaveBMP(screenshot, ("screenshots/" + timestamp + extension).c_str());
	}
	else if (filename == "")
	{
		std::string timestamp = CurrentDate() + "-" + CurrentTime();
		for (int i = 0; i < timestamp.size(); i++)
		{
			if (timestamp[i] == ':')
				timestamp[i] = '-';
		}

		if (extension == ".png")
			IMG_SavePNG(screenshot, (filepath + timestamp + extension).c_str());
		else
			SDL_SaveBMP(screenshot, (filepath + timestamp + extension).c_str());
	}
	else
	{
		if (extension == ".png")
			IMG_SavePNG(screenshot, (filepath + filename + extension).c_str());
		else
			SDL_SaveBMP(screenshot, (filepath + filename + extension).c_str());
	}
	
	SDL_FreeSurface(screenshot);

	delete[] pixels;
#endif
}

void Game::GetMenuInput()
{
	const uint8_t* input = SDL_GetKeyboardState(NULL);

	if (cutsceneManager.watchingCutscene && cutsceneManager.GetLabelName(cutsceneManager.currentLabel) == "title")
	{
		cutsceneManager.Update();
	}
	else
	{
		uint32_t ticks = timer.GetTicks();
		if (ticks > lastPressedKeyTicks + 100) //TODO: Check for overflow errors
		{
			// If we have pressed any key on the menu, add a delay between presses
			if (openedMenus[openedMenus.size() - 1]->Update(*this))
				lastPressedKeyTicks = ticks;

			// TODO: Is there a better way to do this?
			// We can't pop_back inside of the Update() function!
			if (menuManager->shouldPopBackThisFrame)
			{
				menuManager->shouldPopBackThisFrame = false;
				if (openedMenus.size() > 0)
					openedMenus.pop_back();
			}
				
		}
	}
}

void Game::UpdateClickAndDrag()
{
	// Get position of mouse and any clicks
	previousMouseState = mouseState;
	mouseState = SDL_GetMouseState(&mouseRect.x, &mouseRect.y);

	glm::vec3 worldPosition = ConvertFromScreenSpaceToWorldSpace(glm::vec2(mouseRect.x, mouseRect.y));

	mouseRect.x = worldPosition.x;
	mouseRect.y = worldPosition.y;
	mouseRect.w = 1;
	mouseRect.h = 1;

	std::vector<Entity*> clickableEntities;
	quadTree.Retrieve(&mouseRect, clickableEntities, &quadTree);

	for (unsigned int i = 0; i < clickableEntities.size(); i++)
	{
		if (clickableEntities[i]->clickable || clickableEntities[i]->draggable)
		{
			clickableEntities[i]->GetSprite()->isHovered = false;

			if (mouseState & SDL_BUTTON(SDL_BUTTON_LEFT))
			{
				if (HasIntersection(mouseRect, ConvertCoordsFromCenterToTopLeft(*clickableEntities[i]->GetBounds())))
				{
					if (!(previousMouseState & SDL_BUTTON(SDL_BUTTON_LEFT)))
						clickableEntities[i]->OnClickPressed(mouseState, *this);
					else
						clickableEntities[i]->OnClick(mouseState, *this);
					break;
				}
			}
			else if (previousMouseState & SDL_BUTTON(SDL_BUTTON_LEFT))
			{
				if (HasIntersection(mouseRect, ConvertCoordsFromCenterToTopLeft(*clickableEntities[i]->GetBounds())))
				{
					clickableEntities[i]->OnClickReleased(mouseState, *this);
				}
			}
			else if (HasIntersection(mouseRect, ConvertCoordsFromCenterToTopLeft(*clickableEntities[i]->GetBounds())))
			{
				clickableEntities[i]->GetSprite()->isHovered = true;
			}

			if (draggedEntity != nullptr)
			{
				draggedEntity->position = worldPosition;
			}

		}
	}
}

void Game::TransitionMenu()
{
	// If the menu that is open now is not the one that was open before...
	// Then we want to transition from the old to the new
	// (exit old, then enter new)
	if (menuLastFrame != nullptr && menuLastFrame->isPlayingExitAnimation)
	{
		// For every keyframe that is within the current time,
		// update the entity's properties for that frame
		bool isFinished = true;
		for (auto& anim : menuLastFrame->exitAnimation)
		{
			for (auto& keyframe : anim->keyframes)
			{
				// We want to update using the latest keyframe 
				// for each entity without going past the current time
				if (keyframe->time > Globals::CurrentTicks
					&& keyframe->previousFrame != nullptr)
				{
					isFinished = false;
					keyframe->Update(anim->entity, Globals::CurrentTicks);
					break;
				}
			}
		}

		if (isFinished)
		{
			menuLastFrame->isPlayingExitAnimation = false;
			openedMenus.back()->isPlayingEnterAnimation = true;

			// Set the time for each keyframe
			for (auto& anim : openedMenus.back()->enterAnimation)
			{
				for (auto& keyframe : anim->keyframes)
				{
					keyframe->CalculateTime();
				}
			}
		}
	}
	else if (openedMenus.back()->isPlayingEnterAnimation)
	{
		// For every keyframe that is within the current time,
		// update the entity's properties for that frame
		bool isFinished = true;
		MenuScreen* nextMenu = openedMenus.back();
		for (auto& anim : nextMenu->enterAnimation)
		{
			for (auto& keyframe : anim->keyframes)
			{
				if (keyframe->time > Globals::CurrentTicks
					&& keyframe->previousFrame != nullptr)
				{
					isFinished = false;
					keyframe->Update(anim->entity, Globals::CurrentTicks);
					break;
				}
			}
		}

		if (isFinished)
		{
			nextMenu->isPlayingEnterAnimation = false;
			menuLastFrame = nextMenu;
			menuLastFrame->HighlightSelectedButton(*this);
		}
	}
	else if (menuLastFrame != openedMenus.back() && menuLastFrame != nullptr)
	{
		menuLastFrame->isPlayingExitAnimation = true;
		menuLastFrame->UnhighlightSelectedButton(*this);

		// Set the time for each keyframe
		for (auto& anim : menuLastFrame->exitAnimation)
		{
			for (auto& keyframe : anim->keyframes)
			{
				keyframe->CalculateTime();
			}
		}
	}
	else
	{
		menuLastFrame = openedMenus.back();
		GetMenuInput();
	}
}

void Game::OpenMenu(const std::string& menuName)
{
	MenuScreen& menu = *allMenus[menuName];

	if (menu.isDynamic)
	{
		menu.ResetMenu();
		menu.CreateMenu(menuName, *this);
	}

	openedMenus.emplace_back(&menu);
}

void Game::Update()
{
	if (networkManager && networkManager->protocol == Protocol::UDP_Server)
	{
		networkManager->UDPServer();
		networkManager->ReadMessage(*this);
	}

	inputManager.StartUpdate();

	shouldQuit = CheckInputs();

	menuManager->Update(*this);

	CheckDeleteEntities();

	if (!updateScreenTexture)
	{
		updateScreenTexture = !cutsceneManager.watchingCutscene || cutsceneManager.printNumber > 0;
	}

	gui->Update();
	renderer.Update();

	if (soundMode)
	{
		soundManager.soundTest->UpdateSoundMode(*this);
		return;
	}

	soundManager.soundTest->Update(*this);

	if (freeCameraMode)
	{
		static bool mouseFirstMoved = true;
		static int lastMouseX = 0;
		static int lastMouseY = 0;

		const Uint8* input = SDL_GetKeyboardState(NULL);

		int mouseX = 0;
		int mouseY = 0;

		SDL_GetMouseState(&mouseX, &mouseY);

		if (mouseFirstMoved)
		{
			lastMouseX = mouseX;
			lastMouseY = mouseY;
			mouseFirstMoved = false;
		}

		float xChange = (mouseX - lastMouseX);
		float yChange = (mouseY - lastMouseY);

		lastMouseX = mouseX;
		lastMouseY = mouseY;

		renderer.camera.MouseControl(xChange, yChange);
		renderer.camera.KeyControl(input, dt, screenWidth, screenHeight);
		renderer.guiCamera.KeyControl(input, dt, screenWidth, screenHeight);
	}


	// NOTE: We always need to call EndUpdate()
	// on the inputManager before the end of this function,
	// so we use a bool here to only do the things we need.
	bool updateNormalStuff = true;

	// VERY IMPORTANT TO DO THIS HERE!
	// Let's just keep mouse input stuff all in one place, then reference it everywhere else.
	int mouseX, mouseY = 0;
	const uint32_t mouseState = SDL_GetMouseState(&mouseX, &mouseY);
	worldPosition = ConvertFromScreenSpaceToWorldSpace(glm::vec2(mouseX, mouseY));

	if (openedMenus.size() > 0)
	{
		if (guiMode)
		{
			editor->HandleGUIMode();
		}

		if (waitForMenuTransitions)
		{
			TransitionMenu();
		}
		else
		{

			if (openedMenus.size() > 0)
			{
				guiMode = !guiMode;
			}

			GetMenuInput();
		}

		
		updateNormalStuff = openedMenus[openedMenus.size() - 1]->shouldUpdate;
	}
	else if (editMode)
	{
		HandleEditMode();
		updateNormalStuff = false;
	}
	else if (guiMode)
	{
		editor->HandleGUIMode();
	}
	else if (transitionState > 0)
	{
		TransitionLevel();
		updateNormalStuff = false;
	}

	if (updateNormalStuff)
	{
		const Uint8* input = SDL_GetKeyboardState(NULL);

		if (debugMode)
		{
			// If we click on a button on the debug screen,
			// don't execute any game or cutscene code
			if (debugScreen->Update())
				waitingForDebugDialog = true;
		}

		if (waitingForDebugDialog)
		{
			updateNormalStuff = false;
		}

		if (updateNormalStuff)
		{
			PopulateQuadTree();

			if (cutsceneManager.watchingCutscene)
			{
				cutsceneManager.Update();
			}
			else
			{
				UpdateClickAndDrag();
			}

			// TODO: Don't hardcode these numbers
			const SDL_Rect cameraBounds = renderer.camera.GetBounds();

			// Update all entities
			updateCalls = 0;
			collisionChecks = 0;
			for (unsigned int i = 0; i < entities.size(); i++)
			{
				const SDL_Rect* theirBounds = entities[i]->GetBounds();
				if (entities[i]->active && HasIntersection(cameraBounds, *theirBounds))
				{
					entities[i]->Update(*this);
				}
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
	}

	inputManager.EndUpdate();
}

glm::vec3 Game::ConvertFromScreenSpaceToWorldSpace(const glm::vec2& pos)
{
	float halfScreenWidth = screenWidth / 2.0f;
	float halfScreenHeight = screenHeight / 2.0f;

	glm::mat4 projection = renderer.camera.projection;
	glm::mat4 view = renderer.camera.CalculateViewMatrix();

	glm::mat4 invMat = glm::inverse(projection * view);

	// Near = -1, Far = 1, but these only work with Perspective cameras.
	// For an orthographic camera, we need to use 0, or the midpoint between Near and Far
	glm::vec4 mid = glm::vec4((pos.x - halfScreenWidth) / halfScreenWidth, -1 * (pos.y - halfScreenHeight) / halfScreenHeight, 0, 1.0);

	glm::vec4 midResult = invMat * mid;
	midResult /= midResult.w;

	return glm::vec3(midResult);
}

void Game::PopulateQuadTree()
{
	quadTree.Reset();

	for (int i = 0; i < entities.size(); i++)
	{
		if (entities[i]->impassable || entities[i]->trigger || entities[i]->clickable
			|| entities[i]->jumpThru || entities[i]->etype == "player")
		{
			quadTree.Insert(entities[i]);
		}
	}
}

void Game::SetScreenResolution(const unsigned int width, const unsigned int height)
{

	screenWidth = width;
	screenHeight = height;

	SDL_SetWindowSize(window, screenWidth, screenHeight);
	renderer.camera.ResetProjection();
	renderer.guiCamera.Zoom(0.0f, screenWidth * Camera::MULTIPLIER, screenHeight * Camera::MULTIPLIER);

	ShaderProgram* shader1 = nullptr;
	ShaderProgram* shader2 = nullptr;
	ShaderProgram* shader3 = nullptr;
	ShaderProgram* shader4 = nullptr;

	if (mainFrameBuffer != nullptr)
	{
		shader1 = mainFrameBuffer->sprite->GetShader();
		delete_it(mainFrameBuffer);
	}

	if (cutsceneFrameBuffer != nullptr)
	{
		shader2 = cutsceneFrameBuffer->sprite->GetShader();
		delete_it(cutsceneFrameBuffer);
	}

	if (prevCutsceneFrameBuffer != nullptr)
	{
		shader3 = prevCutsceneFrameBuffer->sprite->GetShader();
		delete_it(prevCutsceneFrameBuffer);
	}

	if (prevMainFrameBuffer != nullptr)
	{
		shader4 = prevMainFrameBuffer->sprite->GetShader();
		delete_it(prevMainFrameBuffer);
	}

	mainFrameBuffer = new FrameBuffer(renderer, screenWidth, screenHeight);
	cutsceneFrameBuffer = new FrameBuffer(renderer, screenWidth, screenHeight);
	prevCutsceneFrameBuffer = new FrameBuffer(renderer, screenWidth, screenHeight);
	prevMainFrameBuffer = new FrameBuffer(renderer, screenWidth, screenHeight);

	if (shader1 != nullptr)
		mainFrameBuffer->sprite->SetShader(shader1);

	if (shader2 != nullptr)
		cutsceneFrameBuffer->sprite->SetShader(shader2);

	if (shader3 != nullptr)
		prevCutsceneFrameBuffer->sprite->SetShader(shader3);

	if (shader4 != nullptr)
		prevMainFrameBuffer->sprite->SetShader(shader4);

	glViewport(0, 0, screenWidth, screenHeight);
}


void Game::Render()
{	
	// zero pass
	glBindFramebuffer(GL_FRAMEBUFFER, mainFrameBuffer->framebufferObject);

	//glClearColor(0.1f, 0.5f, 1.0f, 1.0f);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 

	RenderNormally();

	// first pass
	glBindFramebuffer(GL_FRAMEBUFFER, cutsceneFrameBuffer->framebufferObject);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	RenderScene();

	// second pass
	bool renderSecondCutsceneBuffer = false;
	//bool renderSecondMainBuffer = false;
	
	// Don't render the scene twice outside of cutscenes
	if (!cutsceneManager.watchingCutscene)
	{
		updateScreenTexture = false;
	}

	//std::cout << "Update? " << updateScreenTexture << " " << std::to_string(prevScreenSprite->color.a) << std::endl;

	if (updateScreenTexture)
	{
		renderSecondCutsceneBuffer = true;

		float timeLeft = cutsceneManager.printTimer.endTime - cutsceneManager.printTimer.startTicks;
		float t = (timeLeft > 0) ? std::min(1.0f, (cutsceneManager.printTimer.GetTicks() / timeLeft)) : 1.0f; // percentage of passed time

		float alpha = prevCutsceneFrameBuffer->sprite->color.a;
		LerpCoord(alpha, 255, 0, t); 

		prevCutsceneFrameBuffer->sprite->color.a = alpha;
		prevMainFrameBuffer->sprite->color.a = alpha;

		// TODO: For alpha mask, use a shader to get the max of (alpha, pixel of black/white texture)
		// so that the black parts render before the white parts

		std::cout << std::to_string(prevMainFrameBuffer->sprite->color.a) << std::endl;

		if (prevCutsceneFrameBuffer->sprite->color.a <= 10) // this can't be exactly 0 in case the timer expires first
		{
			//std::cout << "RENDER PREV SCENE" << std::endl;
			renderSecondCutsceneBuffer = false;
			updateScreenTexture = false;

			glBindFramebuffer(GL_FRAMEBUFFER, prevMainFrameBuffer->framebufferObject);
			glClearColor(0.1f, 0.5f, 1.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			RenderNormally();
			prevMainFrameBuffer->sprite->color.a = 255;
			
			glBindFramebuffer(GL_FRAMEBUFFER, prevCutsceneFrameBuffer->framebufferObject);
			glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			RenderScene();
			prevCutsceneFrameBuffer->sprite->color.a = 255;
		}
	}

	// final pass
	glBindFramebuffer(GL_FRAMEBUFFER, 0); // back to default
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glm::vec3 screenPos = glm::vec3(renderer.camera.startScreenWidth, renderer.camera.startScreenHeight, 0);
	glm::vec2 screenScale = glm::vec2(renderer.camera.startScreenWidth / screenWidth, renderer.camera.startScreenHeight / -screenHeight);

	RenderQuake(screenPos);

	mainFrameBuffer->sprite->Render(screenPos, renderer, screenScale);

	if (renderSecondCutsceneBuffer)
	{
		prevMainFrameBuffer->sprite->Render(screenPos, renderer, screenScale);
	}

	cutsceneFrameBuffer->sprite->Render(screenPos, renderer, screenScale);

	if (renderSecondCutsceneBuffer)
	{
		prevCutsceneFrameBuffer->sprite->Render(screenPos, renderer, screenScale);
	}

	// Render the GUI above everything
	if (!cutsceneManager.watchingCutscene && !editMode)
	{
		gui->Render(renderer);
	}

	// Render all menu screens above the GUI
	if (openedMenus.size() > 0)
	{
		if (menuLastFrame != nullptr && menuLastFrame->isPlayingExitAnimation)
		{
			menuLastFrame->Render(renderer);
		}
		else
		{
			openedMenus[openedMenus.size() - 1]->Render(renderer);
		}
	}

	// Always render the mouse last
	if (cursorSprite != nullptr)
	{
		int mouseX = 0;
		int mouseY = 0;
		SDL_GetMouseState(&mouseX, &mouseY);
		cursorSprite->Render(glm::vec3(mouseX, mouseY, 0), 0, renderer, glm::vec3(1,1,1), glm::vec3(0,0,0));
	}

	glUseProgram(0);
	SDL_GL_SwapWindow(window);

	if (savingGIF)
	{
		SaveGIF();
	}
}

void Game::RenderQuake(glm::vec3& screenPos)
{
	// If the timer is not up, then we should shake the screen
	if (cutsceneManager.commands.isQuakeHorizontal || cutsceneManager.commands.isQuakeVertical)
	{
		// let's say we want to shake the screen for 1000 ms
		// and we want the screen to shake 5 times
		// then each shake should be 200 ms
		// (200 ms * 5 = 1000 ms)
		// then divide that by 4
		// center -> right,
		// right -> center,
		// center -> left,
		// left -> center
		// = 50 ms each

		uint32_t currentTime = Globals::CurrentTicks / (float)cutsceneManager.commands.quakeIntensity;
		uint32_t startTime = cutsceneManager.commands.quakeTimer.startTicks / (float)cutsceneManager.commands.quakeIntensity;
		uint32_t endTime = cutsceneManager.commands.quakeTimer.endTime / (float)cutsceneManager.commands.quakeIntensity;

		const float distPercent = 0.25f;

		static float distX = screenPos.x * distPercent;
		static float distY = screenPos.y * distPercent;

		static glm::vec3 screenCenter = screenPos;

		static glm::vec3 screenLeft = glm::vec3(screenPos.x - distX, screenPos.y, 0);
		static glm::vec3 screenRight = glm::vec3(screenPos.x + distX, screenPos.y, 0);

		static glm::vec3 screenUp = glm::vec3(screenPos.x, screenPos.y - distY, 0);
		static glm::vec3 screenDown = glm::vec3(screenPos.x, screenPos.y + distY, 0);

		static glm::vec3 quakeStartPos = screenCenter;
		static glm::vec3 quakeEndPos = screenCenter;

		static bool randomX = false;
		static bool randomY = false;

		// Initial Step
		if (cutsceneManager.commands.quakeCount == 0)
		{
			// Randomize the first loop here
			randomX = randomManager.RandomInt(100) > 50;
			randomY = randomManager.RandomInt(100) > 50;

			quakeStartPos = screenCenter;

			if (cutsceneManager.commands.isQuakeHorizontal)
			{
				quakeEndPos.x = randomX ? screenRight.x : screenLeft.x;
			}
			if (cutsceneManager.commands.isQuakeVertical)
			{
				quakeEndPos.y = randomY ? screenDown.y : screenUp.y;
			}

			cutsceneManager.commands.quakeCount++;
		}
		else if (cutsceneManager.commands.quakeNumberOfLoops >= cutsceneManager.commands.quakeIntensity)
		{
			cutsceneManager.commands.isQuakeHorizontal = false;
			cutsceneManager.commands.isQuakeVertical = false;
		}
		else
		{
			// 0. Go from center to right
			if (LerpVector3(cutsceneManager.commands.currentQuakePosition,
				quakeStartPos, quakeEndPos, currentTime, startTime, endTime))
			{
				cutsceneManager.commands.quakeCount++;
				cutsceneManager.commands.quakeTimer.Reset();

				switch (cutsceneManager.commands.quakeCount)
				{
				case 2:
					// 1. Go from right/down to center
					quakeStartPos = screenCenter;
					quakeEndPos = screenCenter;

					if (cutsceneManager.commands.isQuakeHorizontal)
					{
						quakeStartPos.x = randomX ? screenRight.x : screenLeft.x;
					}
					if (cutsceneManager.commands.isQuakeVertical)
					{
						quakeStartPos.y = randomY ? screenDown.y : screenUp.y;
					}

					break;
				case 3:
					// 2. Go from center to left/up
					quakeStartPos = screenCenter;
					quakeEndPos = screenCenter;

if (cutsceneManager.commands.isQuakeHorizontal)
{
	quakeEndPos.x = randomX ? screenLeft.x : screenRight.x;
}
if (cutsceneManager.commands.isQuakeVertical)
{
	quakeEndPos.y = randomY ? screenUp.y : screenDown.y;
}

break;
				case 4:
					// 3. Go from left/up to center
					quakeStartPos = screenCenter;
					if (cutsceneManager.commands.isQuakeHorizontal)
					{
						quakeStartPos.x = randomX ? screenLeft.x : screenRight.x;
					}
					if (cutsceneManager.commands.isQuakeVertical)
					{
						quakeStartPos.y = randomY ? screenUp.y : screenDown.y;
					}

					quakeEndPos = screenCenter;
					break;
				case 5: // 4. repeat

					// Randomize the next loop here
					randomX = randomManager.RandomInt(100) > 50;
					randomY = randomManager.RandomInt(100) > 50;

					quakeStartPos = screenCenter;

					quakeEndPos = screenCenter;
					if (cutsceneManager.commands.isQuakeHorizontal)
					{
						quakeEndPos.x = randomX ? screenRight.x : screenLeft.x;
					}
					if (cutsceneManager.commands.isQuakeVertical)
					{
						quakeEndPos.y = randomY ? screenDown.y : screenUp.y;
					}

					cutsceneManager.commands.quakeNumberOfLoops++;
					cutsceneManager.commands.quakeCount = 1;
					break;
				}

			}

			screenPos = cutsceneManager.commands.currentQuakePosition;
		}

	}


}

void Game::RenderNormally()
{
	if (!use2DCamera)
	{
		glEnable(GL_DEPTH_TEST);
	}

	gui->RenderStart();

	// Render all backgrounds and their layers
	
	if (background != nullptr)
	{
		background->Render(renderer);
	}

	quadTree.Render(renderer);

	// Render only entities in frame, or all entities
	if (renderer.camera.useOrthoCamera && !editMode)
	{
		entitiesToRender.clear();
		debugEntities.clear();

		SDL_Rect cameraBounds = renderer.camera.GetBounds();

		bool useQuadTree = false;

		if (useQuadTree)
		{
			// Once all entities are sorted, render the ones on screen
			quadTree.Retrieve(&cameraBounds, entitiesToRender, &quadTree);

			// If one of the entities on screen moved this frame, sort them all by Y position
			for (unsigned int i = 0; i < entitiesToRender.size(); i++)
			{
				Entity* e = entitiesToRender[i];
				if (e->position != e->lastPosition)
				{
					SortEntities(entitiesToRender, sortByPosY);
					break;
				}
			}

			for (unsigned int i = 0; i < entitiesToRender.size(); i++)
			{
				Entity* e = entitiesToRender[i];
				if (e->active)
				{
					e->Render(renderer);
				}

				if (debugMode)
				{
					if (e->etype == "player" || e->impassable || e->trigger || e->jumpThru)
					{
						debugEntities.push_back(entitiesToRender[i]);
					}
				}
			}
		}
		else
		{
			for (unsigned int i = 0; i < entities.size(); i++)
			{
				const SDL_Rect* theirBounds = entities[i]->GetBounds();
				if (entities[i]->active && HasIntersection(cameraBounds, *theirBounds))
				{
					entitiesToRender.push_back(entities[i]);
				}
			}

			// If one of the entities on screen moved this frame, sort them all by Y position
			for (unsigned int i = 0; i < entitiesToRender.size(); i++)
			{
				Entity* e = entitiesToRender[i];
				if (e->position != e->lastPosition)
				{
					SortEntities(entitiesToRender, sortByPosY);
					break;
				}
			}

			for (unsigned int i = 0; i < entitiesToRender.size(); i++)
			{
				entitiesToRender[i]->Render(renderer);

				if (debugMode)
				{
					if (entitiesToRender[i]->etype == "player" || entitiesToRender[i]->impassable
						|| entitiesToRender[i]->trigger || entitiesToRender[i]->jumpThru)
					{
						debugEntities.push_back(entitiesToRender[i]);
					}
				}
			}
		}






		if (debugMode)
		{
			for (unsigned int i = 0; i < debugEntities.size(); i++)
			{
				debugEntities[i]->RenderDebug(renderer);
			}
		}

	}
	else
	{
		for (unsigned int i = 0; i < entities.size(); i++)
		{
			if (entities[i]->active)
			{
				entities[i]->Render(renderer);
			}
			
			if (debugMode)
				entities[i]->RenderDebug(renderer);
		}
	}

	if (!use2DCamera && triangle3D != nullptr)
	{
		triangle3D->Render(glm::vec3(0, 800, 300), 0, renderer, glm::vec3(200, 200, 200), glm::vec3(0,0,0));
	}

}

void Game::RenderScene()
{
	// Draw anything in the cutscenes
	glDisable(GL_DEPTH_TEST);

	cutsceneManager.Render(renderer); // includes the overlay

	// Render editor toolbox
	if (editMode)
	{
		editor->Render(renderer);
	}

	if (soundMode && soundManager.soundTest != nullptr)
	{
		soundManager.soundTest->Render(renderer);
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

Entity* Game::GetEntityFromID(int id)
{
	// TODO: Implement a method in the Game class
	// that allows for fast retrieval of an entity
	// based on its ID (like using a map or something)

	for (int i = 0; i < entities.size(); i++)
	{
		if (entities[i]->id == id)
		{
			return entities[i];
		}
	}

	return nullptr;
}

// Implementation of insertion sort:
// Splits the list into two portions - sorted and unsorted.
// Then steps through the unsorted list, checking where the next one fits.
void Game::SortEntities(std::vector<Entity*>& entityVector, bool sortByPosY)
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
				if (sortByPosY)
				{
					if (entityVector[j - 1]->position.y > entityVector[j]->position.y)
					{
						std::swap(entityVector[j], entityVector[j - 1]);
					}
					else
					{
						if (entityVector[j - 1]->drawOrder > entityVector[j]->drawOrder)
						{
							std::swap(entityVector[j], entityVector[j - 1]);
						}
					}
				}
				else
				{
					if (entityVector[j - 1]->drawOrder > entityVector[j]->drawOrder)
					{
						std::swap(entityVector[j], entityVector[j - 1]);
					}
				}

			}
			j--;
		}
	}
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

	Mesh* mesh = new Mesh();
	mesh->CreateMesh(quadVertices, quadIndices, 20, 12, 5, 3, 0);

	return mesh;
}

Mesh* Game::CreateCubeMesh()
{
	unsigned int cubeIndices[] = {
	3, 0, 4,
	6, 3, 7,
	1, 3, 2,
	6, 2, 3,
	3, 4, 7,
	1, 0, 3,
	4, 0, 1,
	5, 2, 6,
	2, 5, 1,
	5, 6, 7,
	5, 7, 4,
	5, 4, 1
	};

	// Our vertices. Three consecutive floats give a 3D vertex; Three consecutive vertices give a triangle.
	// A cube has 6 faces with 2 triangles each, so this makes 6*2=12 triangles, and 12*3 vertices
	// 6 faces, 4 vertices per face = 24 indices

	GLfloat cubeVertices[] = {
		-1.0f,-1.0f,-1.0f, // triangle 1 : begin
		-1.0f,-1.0f, 1.0f,
		-1.0f, 1.0f, 1.0f, // triangle 1 : end

		1.0f, 1.0f,-1.0f, // triangle 2 : begin
		-1.0f,-1.0f,-1.0f,
		-1.0f, 1.0f,-1.0f, // triangle 2 : end

		1.0f,-1.0f, 1.0f,
		-1.0f,-1.0f,-1.0f,
		1.0f,-1.0f,-1.0f,

		1.0f, 1.0f,-1.0f,
		1.0f,-1.0f,-1.0f,
		-1.0f,-1.0f,-1.0f,

		-1.0f,-1.0f,-1.0f,
		-1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f,-1.0f,

		1.0f,-1.0f, 1.0f,
		-1.0f,-1.0f, 1.0f,
		-1.0f,-1.0f,-1.0f,

		-1.0f, 1.0f, 1.0f,
		-1.0f,-1.0f, 1.0f,
		1.0f,-1.0f, 1.0f,

		1.0f, 1.0f, 1.0f,
		1.0f,-1.0f,-1.0f,
		1.0f, 1.0f,-1.0f,

		1.0f,-1.0f,-1.0f,
		1.0f, 1.0f, 1.0f,
		1.0f,-1.0f, 1.0f,

		1.0f, 1.0f, 1.0f,
		1.0f, 1.0f,-1.0f,
		-1.0f, 1.0f,-1.0f,

		1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f,-1.0f,
		-1.0f, 1.0f, 1.0f,

		1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f, 1.0f,
		1.0f,-1.0f, 1.0f
	};

	Mesh* mesh = new Mesh();
	mesh->CreateMesh(cubeVertices, cubeIndices, 108, 36, 3, 0, 0);

	return mesh;
}
