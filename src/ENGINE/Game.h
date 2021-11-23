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
#include "InputManager.h"
#include "Renderer.h"
#include "Logger.h"
#include "FrameBuffer.h"
#include "Material.h"

#include "Model.h"

class FileManager;
class MenuManager;

class FrameBuffer;
class Dialog;

enum class GameState { NORMAL, EDIT_MODE, ON_MENU, RESET_LEVEL, LOAD_NEXT_LEVEL };

class KINJO_API Game
{
private:
	SDL_Surface* screenSurface = nullptr;
	SDL_GLContext mainContext = nullptr;

	float now = 0; // duration from game start to current frame
	bool waitingForDebugDialog = false;

	// Below are reserved for the main loop

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

public:

	bool use2DCamera = true;

	Model modelChopper;

	Mesh* CreateQuadMesh();
	Mesh* CreateCubeMesh();

	std::string currentGame = "";
	bool freeCameraMode = false;

	mutable std::unordered_map<std::string, std::vector<std::string>> entityTypes;
	std::unordered_map<std::string, std::vector<std::string>> spriteMap;

	// Used when creating entities
	std::vector<std::string> dirNames;
	std::unordered_map<std::string, std::string> initialStates;

	std::string windowIconFilepath = "";
	std::string windowTitle = "";

	std::string currentSaveFileName = "";

	Material shinyMaterial;
	Material dullMaterial;

	Logger logger;
	SDL_GameController* controller = nullptr;

	// Keep this a pointer so we can use polymorphism
	const EntityFactory* entityFactory = nullptr;
	const FileManager* fileManager = nullptr;
	const MenuManager* menuManager = nullptr;

	std::vector<Entity*> entitiesToDelete;

	SDL_Rect mouseRect;
	uint32_t mouseState;
	uint32_t previousMouseState;

	int screenWidth = 1280;
	int screenHeight = 720;

	bool debugMode = false;
	bool editMode = false;
	bool soundMode = false;

	GUI* gui;
	Mesh* cubeMesh;
	QuadTree quadTree;

	Entity* draggedEntity = nullptr;

	unsigned int collisionChecks = 0;
	unsigned int updateCalls = 0;

	std::vector<Entity*> quadrantEntities;

	Entity* GetEntityFromID(int id);

	void DeleteEntity(Entity* entity);
	void DeleteEntity(int index);

	void ReadEntityLists();

	// play this cutscene on level start
	std::string levelStartCutscene = "";

	using clock = std::chrono::steady_clock;
	using seconds = std::chrono::seconds;
	using milliseconds = std::chrono::milliseconds;

	clock::time_point startOfGame;
	clock::time_point previousTime;

	std::unordered_map<std::string, MenuScreen*> allMenus;

	FrameBuffer* mainFrameBuffer = nullptr;
	FrameBuffer* prevMainFrameBuffer = nullptr;
	FrameBuffer* cutsceneFrameBuffer = nullptr;
	FrameBuffer* prevCutsceneFrameBuffer = nullptr;

	Sprite* triangle3D = nullptr;

	bool updateScreenTexture = false;

	void InitOpenGL();

	int BeforeMainLoop();
	int MainLoop();

	void CheckController(bool output);

	void Update();
	void UpdateClickAndDrag();
	void Render();
	void RenderScene();
	void RenderNormally();
	void RenderQuake(glm::vec3& screenPos);

	bool HandleEvent(SDL_Event& event);
	bool HandleMenuEvent(SDL_Event& event);
	void HandleEditMode();

	void GetMenuInput();

	void PopulateQuadTree();

	void CalcDt();
	bool CheckInputs();
	void CheckDeleteEntities();

	void SetScreenResolution(const unsigned int width, const unsigned int height);
	Entity* CreateEntity(const std::string& entityName, const glm::vec3& position, int spriteIndex) const;
	Entity* SpawnEntity(const std::string& entityName, const glm::vec3& position, const int spriteIndex) const;

	FontInfo* CreateFont(const std::string& fontName, int size);

	Timer timer;
	Timer fpsLimit;
	bool limitFPS = false;

	bool isPaused = false;

	GameState state;
	//GameState previousState;

	std::string nextLevel = "";
	std::string nextBGM = "";

	uint32_t lastPressedKeyTicks = 0;

	Background* background = nullptr;

	Renderer renderer;
	CutsceneManager cutsceneManager;
	SpriteManager spriteManager;
	SoundManager soundManager;
	RandomManager randomManager;
	InputManager inputManager;

	FontInfo* theFont = nullptr;
	FontInfo* headerFont = nullptr;

	std::unordered_map<std::string, FontInfo*> fonts;

	std::vector<Entity*> entitiesToRender;
	std::vector<MenuScreen*> openedMenus;

	bool shouldUpdateDialogInput = false;
	bool shouldQuit = false;
	bool savingGIF = false;

	std::string inputText = "";
	std::string inputReason = "";
	std::string inputType = "";

	std::string currentLevel = "";
	int levelNumber = 1;
	float dt = 0;
	int transitionExit = -1;
	int transitionEnter = -1;
	int transitionState = 0;

	float timeScale = 1.0f;
	float dtUnscaled = 0;

	// Automatically take a screenshot every X seconds
	int autoScreenshots = 0;
	Timer screenshotTimer;

	// Automatically record GIFs every X seconds for Y seconds
	int autoGIFsDelay = 0;
	Timer autoGifDelayTimer;
	int autoGIFsDuration = 0;
	Timer autoGifDurationTimer;

	DebugScreen* debugScreen = nullptr;
	Editor* editor = nullptr;
	SDL_Window* window = nullptr;
	Entity* player = nullptr;

	MenuScreen* menuLastFrame = nullptr;

	bool waitForMenuTransitions = true;
	bool loadingFromSaveFile = false;

	bool isFullscreen = false;
	bool showFPS = false;
	bool showTimer = false;
	int indexScreenResolution = 0;

	mutable std::vector<Entity*> entities;

	std::vector<Entity*> lightSourcesInLevel;

	// Keep these in their own vector for efficiency
	std::vector<Entity*> cameraBoundsEntities;

	std::string gifFolderPath = "screenshots/gif/";
	int gifFrameNumber = 0;

	glm::vec3 ConvertFromScreenSpaceToWorldSpace(const glm::vec2& pos);

	void OpenMenu(const std::string& menuName);

	void ShouldDeleteEntity(int index);
	void ShouldDeleteEntity(Entity* entity);

	Game(const std::string& n, const std::string& title, const std::string& icon, bool is2D,
		const EntityFactory& e, const FileManager& f, GUI& g, MenuManager& m);
	~Game();

	void InitSDL();
	void EndSDL();
	void SortEntities(std::vector<Entity*>& entityVector);

	// Spawn functions
	Tile* CreateTile(const glm::vec2& frame, const int tilesheetIndex,
		const glm::vec3& position, DrawingLayer drawingLayer) const;
	Tile* SpawnTile(const glm::vec2& frame, const int tilesheetIndex,
		const glm::vec3& position, DrawingLayer drawingLayer) const;

	Entity* SpawnPlayer(const glm::vec3& position);

	Sprite* cursorSprite = nullptr;

	void TransitionMenu();
	void TransitionLevel();

	void LoadTitleScreen();
	void LoadLevel(const std::string& level, int onExit = 0, int onEnter = 0);

	glm::vec3 CalculateObjectSpawnPosition(glm::vec2 mousePos, const int GRID_SIZE);

	glm::vec3 SnapToGrid(glm::vec3 position, int size);

	void SetFullScreen(bool setFull);

	Dialog* currentDialog = nullptr;

	void StartTextInput(Dialog& dialog, const std::string& reason);
	void StopTextInput(Dialog& dialog);

	void ResetLevel();

	void SaveSettings();
	void LoadSettings();

	void SaveEditorSettings();
	void LoadEditorSettings();

	void StartGIF(const std::string& filepath = "");
	void SaveGIF();
	void EndGIF();

	void SaveScreenshot(const std::string& filepath, const std::string& filename, const std::string& extension);

	Sprite* CreateSprite(const std::string& filepath, const int shaderName = 1);

};

#endif