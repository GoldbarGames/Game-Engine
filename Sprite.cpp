#include "Sprite.h"
#include "globals.h"

using std::string;

Sprite::Sprite(int numFrames, SpriteManager& manager, std::string filepath, SDL_Renderer * renderer, Vector2 newPivot)
{
	texture = SDL_CreateTextureFromSurface(renderer, manager.GetImage(filepath));

	pivot = newPivot;

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
	windowRect.w = frameWidth * SCALE;
	windowRect.h = frameHeight * SCALE;

	startFrame = 0;
	endFrame = numberFrames;
}

Sprite::Sprite(int start, int end, int numFrames, SpriteManager& manager, 
	std::string filepath, SDL_Renderer * renderer, Vector2 newPivot, bool loop)
{
	texture = SDL_CreateTextureFromSurface(renderer, manager.GetImage(filepath));

	pivot = newPivot;

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
	windowRect.w = frameWidth * SCALE;
	windowRect.h = frameHeight * SCALE;

	startFrame = start;
	endFrame = end;
	shouldLoop = loop;
}

Sprite::~Sprite()
{
	
}

void Sprite::Destroy()
{
	SDL_DestroyTexture(texture);
}

void Sprite::Animate(int msPerFrame, Uint32 time)
{
	if (msPerFrame != 0)
	{
		if (time < 0) // change frame based on total time
			time = SDL_GetTicks();
		else if (time > 0) // change frame based on time relative to the animator
		{
			int frame = startFrame + ((time / msPerFrame) % endFrame);
			textureRect.x = frame * textureRect.w;
		}
		else // if time == 0, show the last frame
		{
			textureRect.x = endFrame * textureRect.w;
		}
	}
}

void Sprite::Render(Vector2 position, int speed, Uint32 time, SDL_RendererFlip flip, SDL_Renderer * renderer, float angle)
{
	windowRect.x = position.x;
	windowRect.y = position.y;
	windowRect.w = frameWidth * SCALE;
	windowRect.h = frameHeight * SCALE;

	if (windowRect.x < screenWidth && windowRect.y < screenHeight
		&& windowRect.x > -windowRect.w && windowRect.y > -windowRect.h)
	{
		Animate(speed, time);

		const SDL_Point point = SDL_Point{ (int)pivot.x, (int)pivot.y };

		SDL_RenderCopyEx(renderer, texture, &textureRect, &windowRect, angle, &point, flip);
	}
}

const SDL_Rect* Sprite::GetRect()
{
	return &windowRect;
}