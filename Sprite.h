#pragma once
#include <string>

#include "SDL.h"
#include <SDL_image.h>
#include <GL/glew.h>

#include "Vector2.h"
#include "SpriteManager.h"

class Renderer;

class Sprite
{
private:
	SDL_Texture * texture = nullptr;
	SDL_Rect textureRect;
	
	Vector2 scale = Vector2(1, 1);
	
public:	
	int frameWidth = 0;
	int frameHeight = 0;

	bool shouldLoop = true;
	int startFrame = 0;
	int endFrame = 0;
	int numberFrames = 1;
	Vector2 pivot = Vector2(0, 0);
	std::string filename = "";
	SDL_Rect windowRect;
	const SDL_Rect* GetRect();
	void Animate(int msPerFrame, Uint32 time);
	void Render(Vector2 position, int speed, Uint32 time, SDL_RendererFlip flip, Renderer* renderer, float angle=0);

	Sprite(Vector2 frame, SDL_Texture * image, Renderer * renderer);
	Sprite(int numFrames, SpriteManager* manager, std::string filepath, Renderer * renderer, Vector2 newPivot);
	Sprite(int start, int end, int numFrames, SpriteManager* manager, std::string filepath, Renderer * renderer, Vector2 newPivot, bool loop=true);
	~Sprite();
};

