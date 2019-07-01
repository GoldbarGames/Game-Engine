#include "Game.h"
#include "Sprite.h"
#include "Player.h"

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
	window = NULL;

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

Player* Game::SpawnPlayer(Vector2 position)
{
	Player* player = new Player();
	Animator* anim1 = new Animator("blink");

	Sprite* sprite1 = new Sprite(6, spriteManager.GetImage("assets/sprites/wdk_walk.png"), renderer);
	Sprite* sprite2 = new Sprite(5, spriteManager.GetImage("assets/sprites/wdk_blink.png"), renderer);

	anim1->MapStateToSprite("walk", sprite1);
	anim1->MapStateToSprite("blink", sprite2);

	player->SetAnimator(anim1);
	player->SetSprite(sprite2);
	player->SetPosition(position);
	player->startPosition = position;

	entities.emplace_back(player);

	return player;
}

void Game::Play(string gameName)
{
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

	SpawnPerson(Vector2(400, 0));
	SpawnPerson(Vector2(0, 0));

	entities.emplace_back(floor);

	//entities.emplace_back(bg);

	mainContext = SDL_GL_CreateContext(window);

	SetOpenGLAttributes();

	SDL_GL_SetSwapInterval(1);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	SDL_GL_SwapWindow(window);

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

		Update();

		Render();
	}

}

void Game::Update()
{
	CalcDt();

	for (int i = entities.size() - 1; i >= 0; i--)
	{
		entities[i]->Update(*this);
	}
}

void Game::Render()
{
	SDL_RenderClear(renderer);

	for (int i = entities.size()-1; i >= 0; i--)
	{
		entities[i]->Render(renderer);
	}
	
	SDL_RenderPresent(renderer);
}