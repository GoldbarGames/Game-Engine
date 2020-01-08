#include "Sprite.h"
#include "globals.h"
#include "Renderer.h"

using std::string;

void Sprite::CreateMesh()
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

	mesh = new Mesh();
	mesh->CreateMesh(quadVertices, quadIndices, 20, 12);
}

Sprite::Sprite(Texture* t, Shader* s)
{
	CreateMesh();

	texture = t;
	shader = s;
	animLow = 0;
	animHigh = 0;
	animFrames = 1;
	currentFrame = 0;
}

// constructor for tiles from tilesheets
Sprite::Sprite(Vector2 frame, Texture * image, Shader * s)
{
	texture = image;
	shader = s;

	CreateMesh();

	// this converts from human coords to pixel coords
	Vector2 currentFrame = Vector2((frame.x - 1) * TILE_SIZE, (frame.y - 1) * TILE_SIZE);

	// Set start position
	windowRect.x = 0;
	windowRect.y = 0;

	frameWidth = TILE_SIZE;
	frameHeight = TILE_SIZE;

	//textureRect.x = currentFrame.x;
	//textureRect.y = currentFrame.y;
	//textureRect.w = TILE_SIZE;
	//textureRect.h = TILE_SIZE;
}

Sprite::Sprite(int numFrames, SpriteManager* manager, std::string filepath, 
	Shader * s, Vector2 newPivot)
{
	texture = manager->GetImage(filepath);
	shader = s;

	CreateMesh();

	pivot = newPivot;

	// Set start position
	windowRect.x = 0;
	windowRect.y = 0;

	//'textureRect' defines the dimensions of the rendering sprite on texture	
	//textureRect.x = 0;
	//textureRect.y = 0;
	//textureRect.w = texture->GetWidth();
	//textureRect.h = texture->GetHeight();

	//SDL_QueryTexture() method gets the width and height of the texture
	//SDL_QueryTexture(texture, NULL, NULL, &textureRect.w, &textureRect.h);

	//Now, textureRect.w and textureRect.h are filled with respective dimensions of the image/texture

	//As there are 8 frames with same width, we simply get the width of a frame by dividing with 8
	numberFrames = numFrames;
	frameWidth = texture->GetWidth() / numberFrames;
	frameHeight = texture->GetHeight();

	windowRect.w = frameWidth * Renderer::GetScale();
	windowRect.h = frameHeight * Renderer::GetScale();

	startFrame = 0;
	endFrame = numberFrames;
	//textureRect.x = startFrame * textureRect.w;
}

Sprite::Sprite(int start, int end, int numFrames, SpriteManager* manager, 
	std::string filepath, Shader* s, Vector2 newPivot, bool loop)
{
	texture = manager->GetImage(filepath);
	shader = s;

	CreateMesh();

	pivot = newPivot;

	// Set start position
	windowRect.x = 0;
	windowRect.y = 0;

	//'textureRect' defines the dimensions of the rendering sprite on texture	
	/*
	textureRect.x = 0;
	textureRect.y = 0;

	textureRect.w = texture->GetWidth();
	textureRect.h = texture->GetHeight();
	*/

	//Now, textureRect.w and textureRect.h are filled with respective dimensions of the image/texture

	//As there are 8 frames with same width, we simply get the width of a frame by dividing with 8
	numberFrames = numFrames;

	frameWidth = texture->GetWidth() / numberFrames;
	frameHeight = texture->GetHeight();
	//textureRect.w /= numberFrames;
	//windowRect.w = frameWidth * Renderer::GetScale();
	//windowRect.h = frameHeight * Renderer::GetScale();

	startFrame = start;
	endFrame = end;
	shouldLoop = loop;
	//textureRect.x = startFrame * textureRect.w;
}

Sprite::~Sprite()
{
}

void Sprite::Animate(int msPerFrame, Uint32 time)
{
	if (msPerFrame != 0 && endFrame != 0 && (startFrame - endFrame) != 0)
	{
		if (time < 0) // change frame based on total time
			time = SDL_GetTicks();
		else if (time > 0) // change frame based on time relative to the animator
		{
			int frame = startFrame + ((time / msPerFrame) % endFrame);
			//textureRect.x = frame * textureRect.w;
		}
		else // if time == 0, show the last frame
		{
			//textureRect.x = endFrame * textureRect.w;
		}
	}
}

void Sprite::Render(Vector2 position, Renderer* renderer, GLuint uniformModel)
{
	Render(position, 0, -1, SDL_FLIP_NONE, renderer, uniformModel, 0);
}

void Sprite::AnimateMesh(GLfloat time)
{
	//if (animFrames <= 1
	//    return;

	if (time < lastAnimTime + animSpeed)
		return;

	lastAnimTime = time;

	mesh->ClearMesh();

	// Set coordinates

	unsigned int* quadIndices = new unsigned int[12]{
	0, 3, 1,
	1, 3, 2,
	2, 3, 0,
	0, 1, 2
	};

	GLfloat* quadVertices = new GLfloat[20]{
		-1.0f, -1.0f, 0.0f,  1.0f, 0.0f,
		1.0f, -1.0f, 0.0f,   0.0f, 0.0f,
		-1.0f, 1.0f, 0.0f,   1.0f, 1.0f,
		1.0f, 1.0f, 0.0f,    0.0f, 1.0f
	};

	animLow = animHigh;
	animHigh += (1.0f) / ((GLfloat)animFrames);

	quadVertices[3] = animHigh;
	quadVertices[8] = animLow;
	quadVertices[13] = animHigh;
	quadVertices[18] = animLow;

	mesh->CreateMesh(quadVertices, quadIndices, 20, 12);

	currentFrame++;

	if (currentFrame >= animFrames)
	{
		animLow = 0.0f;
		animHigh = 0.0f;
		currentFrame = 0;
	}
}

void Sprite::Render(Vector2 position, int speed, Uint32 time, SDL_RendererFlip flip, Renderer * renderer, GLuint uniformModel, float angle)
{

	glm::mat4 model(1.0f);

	// Translate, Rotate, Scale
	model = glm::translate(model, glm::vec3(position.x, position.y, 2.0f));
	//model = glm::rotate(model, currentAngle * toRadians, glm::vec3(0.0f, -1.0f, 0.0f));
	model = glm::scale(model, glm::vec3(texture->GetWidth() / (animFrames), texture->GetHeight(), 1.0f));

	// Set uniform variables
	glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));

	// Use Texture
	texture->UseTexture();

	// Render Mesh
	mesh->RenderMesh();

	/*
	windowRect.x = position.x;
	windowRect.y = position.y;
	windowRect.w = frameWidth * Renderer::GetScale();
	windowRect.h = frameHeight * Renderer::GetScale();

	if (windowRect.x < screenWidth && windowRect.y < screenHeight
		&& windowRect.x > -windowRect.w && windowRect.y > -windowRect.h)
	{
		Animate(speed, time);

		const SDL_Point point = SDL_Point{ (int)pivot.x, (int)pivot.y };

		renderer->RenderCopyEx(texture, &textureRect, &windowRect, angle, &point, flip);
	}
	*/
}

const SDL_Rect* Sprite::GetRect()
{
	return &windowRect;
}