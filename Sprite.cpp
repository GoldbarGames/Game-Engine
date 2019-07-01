#include "Sprite.h"

using std::string;

Sprite::Sprite(int numFrames, SDL_Surface * image, SDL_Renderer * renderer)
{
	texture = SDL_CreateTextureFromSurface(renderer, image);

	// Set start position
	windowRect.x = 0;
	windowRect.y = 0;

	//'textureRect' defines the dimensions of the rendering sprite on texture	
	textureRect.x = 0;
	textureRect.y = 0;

	//SDL_QueryTexture() method gets the width and height of the texture
	SDL_QueryTexture(texture, NULL, NULL, &textureRect.w, &textureRect.h);
	//Now, textureRect.w and textureRect.h are filled with respective dimensions of the image/texture

	//As there are 8 frames with same width, we simply get the width of a frame by dividing with 8
	numberFrames = numFrames;
	frameWidth = textureRect.w / numberFrames;
	frameHeight = textureRect.h;
	textureRect.w /= numberFrames;
}

Sprite::~Sprite()
{
	
}

void Sprite::Destroy()
{
	SDL_DestroyTexture(texture);
}

void Sprite::Animate(int msPerFrame)
{
	if (msPerFrame != 0)
	{
		int frame = (SDL_GetTicks() / msPerFrame) % numberFrames;
		textureRect.x = frame * textureRect.w;
	}
}

void Sprite::Render(Vector2 position, int speed, SDL_Renderer * renderer)
{
	windowRect.x = position.x;
	windowRect.y = position.y;
	windowRect.w = frameWidth * scale.x;
	windowRect.h = frameHeight * scale.y;

	Animate(speed);
	SDL_RenderCopy(renderer, texture, &textureRect, &windowRect);
	SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
	SDL_RenderDrawRect(renderer, &windowRect);
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
}

const SDL_Rect* Sprite::GetRect()
{
	return &windowRect;
}