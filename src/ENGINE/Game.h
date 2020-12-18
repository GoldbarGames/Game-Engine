#ifndef GAME_H
#define GAME_H
#pragma once
#include "leak_check.h"
#include "filesystem_types.h"
#include "GUI.h"
#include <chrono>
#include "globals.h"
#include "Timer.h"
#include "QuadTree.h"
#include "CutsceneManager.h"
#include "RandomManager.h"
#include "SoundManager.h"
#include "SpriteManager.h"
#include "Renderer.h"
#include "Logger.h"

#include "gifanim.h"

class FileManager;
class MenuManager;

enum class GameState { NORMAL, EDIT_MODE, ON_MENU, RESET_LEVEL, LOAD_NEXT_LEVEL };

class KINJO_API Game
{
private:
	SDL_Surface* screenSurface = nullptr;
	
	SDL_GLContext mainContext = nullptr;

	Uint64 timeNow = SDL_GetPerformanceCounter();
	Uint64 timePrev = 0;

	float currentAngle = 0.0f;
	const float toRadians = 3.14159265f / 180.0f;

	float now = 0;
	
	std::unordered_map<std::string, Animator*> animators;
	std::unordered_map<std::string, Mesh*> meshes;

	Sprite* screenSprite = nullptr;
	Sprite* prevScreenSprite = nullptr;

	bool waitingForDebugDialog = false;


public:

	Mesh* CreateQuadMesh();
	Mesh* CreateCubeMesh();

	std::string currentGame = "";

	mutable std::unordered_map<std::string, std::vector<std::string>> entityTypes;

	std::string windowIconFilepath = "";
	std::string windowTitle = "";

	std::string currentSaveFileName = "";

	Logger logger;
	SDL_GameController* controller = nullptr;

	// Keep this a pointer so we can use polymorphism
	const EntityFactory* entityFactory = nullptr;
	const FileManager* fileManager = nullptr;
	const MenuManager* menuManager = nullptr;

	SDL_Rect mouseRect;
	Uint32 mouseState;
	Uint32 previousMouseState;

	int screenWidth = 1280;
	int screenHeight = 720;

	bool debugMode = false;
	bool editMode = false;

	// TODO: Make this more like a bunch of colliders than a solid line
	// TODO: Also have a way to create these barriers via the editor
	int deathBarrierY = 500;

	Mesh* cubeMesh;
	QuadTree quadTree;
	GUI* gui;

	unsigned int collisionChecks = 0;
	unsigned int updateCalls = 0;

	std::vector<Entity*> quadrantEntities;
	
	std::unordered_map<std::string, std::vector<std::string>> spriteMap;

	void DeleteEntity(Entity* entity);
	void DeleteEntity(int index);

	void ReadEntityLists();

	// Player / Level Info
	int startingEther = 40;
	int currentEther = 40;
	int bugsDefeated = 0;
	int bugsRemaining = 0;

	// play this cutscene on level start
	std::string levelStartCutscene = "";

	using clock = std::chrono::steady_clock;
	using seconds = std::chrono::seconds;
	using milliseconds = std::chrono::milliseconds;

	clock::time_point startOfGame;
	clock::time_point start_time;

	std::unordered_map<std::string, MenuScreen*> allMenus;
	std::vector<SDL_Rect*> debugRectangles;


	unsigned int framebuffer;
	unsigned int renderBufferObject;
	unsigned int textureColorBuffer;

	unsigned int prevFramebuffer;
	unsigned int prevRenderBufferObject;
	unsigned int prevTextureColorBuffer;

	bool updateScreenTexture = false;

	Sprite* InitFramebuffer(unsigned int& fbo, unsigned int& tex, unsigned int& rbo );

	void InitOpenGL();

	int MainLoop();

	void CheckController(bool output);

	void Update();
	void Render();
	void RenderScene();
	bool SetOpenGLAttributes();

	bool HandleEvent(SDL_Event& event);
	bool HandleMenuEvent(SDL_Event& event);
	void HandleEditMode();

	void GetMenuInput();

	void CalcDt();
	bool CheckInputs();
	void CheckDeleteEntities();

	void SetScreenResolution(const unsigned int width, const unsigned int height);
	Entity* CreateEntity(const std::string& entityName, const Vector2& position, int spriteIndex) const;
	Entity* SpawnEntity(const std::string& entityName, const Vector2& position, const int spriteIndex) const;

	FontInfo* CreateFont(const std::string& fontName, int size);

	Timer timer;
	Timer fpsLimit;
	bool limitFPS = false;
	
	GameState state;
	//GameState previousState;

	int nextDoorID = -1;
	bool useNextPlayerStartPosition = false;
	Vector2 nextPlayerStartPosition = Vector2(0, 0);
	std::string nextLevel = "";
	std::string nextBGM = "";

	Uint32 lastPressedKeyTicks = 0;

	Background* background = nullptr;	

	Renderer renderer;
	CutsceneManager cutsceneManager;
	SpriteManager spriteManager;
	SoundManager soundManager;
	RandomManager randomManager;

	FontInfo* theFont = nullptr;
	FontInfo* headerFont = nullptr;

	std::unordered_map<std::string, FontInfo*> fonts;

	std::vector<Entity*> entitiesToRender;
	std::vector<MenuScreen*> openedMenus;

	bool getKeyboardInput = false;
	bool shouldQuit = false;
	bool savingGIF = false;
	GifWriter gifWriter;
	GifAnim gifAnim;
	std::vector<uint8_t> gifData;

	std::string inputText = "";
	std::string inputReason = "";
	std::string inputType = "";

	std::string currentLevel = "";
	int levelNumber = 1;
	float dt = 0;
	int transitionExit = -1;
	int transitionEnter = -1;
	int transitionState = 0;

	//std::string collisionLayerNames[8] = { "default", "player", "layer3", "layer4", "layer5", "layer6", "layer7", "layer8" };
	//uint8_t collisionLayers[8] = { 0, 1, 2, 4, 8, 16, 64, 128 };

	DebugScreen* debugScreen = nullptr;
	Editor* editor = nullptr;
	SDL_Window* window = nullptr;
	Entity* player = nullptr;

	bool loadingFromSaveFile = false;

	bool isFullscreen = false;
	bool showFPS = false;
	bool showTimer = false;
	int indexScreenResolution = 0;
	
	//TODO: Make an input class maybe
	bool pressedDebugButton = false;
	bool pressedSpellButton = false;
	bool pressedLeftTrigger = false;
	bool pressedRightTrigger = false;

	mutable std::vector<Entity*> entities;
	std::vector<Entity*> bgEntities;

	std::vector<Entity*> cameraBoundsEntities;

	void ShouldDeleteEntity(int index);
	void ShouldDeleteEntity(Entity* entity);

	Game(const std::string& n, const std::string& title, const std::string& icon, 
		const EntityFactory& e, const FileManager& f, GUI& g, MenuManager& m);
	~Game();

	void InitSDL();
	void EndSDL();
	void SortEntities(std::vector<Entity*>& entityVector);
	
	// Spawn functions
	Tile* CreateTile(const Vector2& frame, const std::string& tilesheet,
		const Vector2& position, DrawingLayer drawingLayer) const;
	Tile* SpawnTile(const Vector2& frame, const std::string& tilesheet,
		const Vector2& position, DrawingLayer drawingLayer) const;

	Entity* SpawnPlayer(const Vector2& position);

	void TransitionLevel();

	void LoadTitleScreen();
	void LoadLevel(const std::string& level, int onExit=0, int onEnter=0);

	Vector2 CalculateObjectSpawnPosition(Vector2 mousePos, const int GRID_SIZE);

	Vector2 SnapToGrid(Vector2 position);

	std::vector<std::string> ReadStringsFromFile(const std::string& filepath);

	void SetFullScreen(bool setFull);

	void UpdateTextInput();
	void StartTextInput(const std::string& reason);
	void StopTextInput();

	void EscapeMenu();

	void ResetLevel();

	void SaveSettings();
	void LoadSettings();

	void SaveEditorSettings();
	void LoadEditorSettings();

	void StartGIF(const std::string& filepath = "");
	void SaveGIF();
	void EndGIF();

	void SaveScreenshot(const std::string& filepath="");

	Sprite* CreateSprite(const std::string& filepath, const ShaderName shaderName = ShaderName::Default);
};

#endif
