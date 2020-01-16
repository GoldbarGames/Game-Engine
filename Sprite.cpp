#include "Sprite.h"
#include "globals.h"
#include "Renderer.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

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

Sprite::Sprite(Texture* t, ShaderProgram* s)
{
	texture = t;
	shader = s;

	numberFramesInTexture = 1;
	framesPerRow = numberFramesInTexture;
	startFrame = 0;
	endFrame = numberFramesInTexture;

	CreateMesh();

	animLow = 0;
	animHigh = 0;
	currentFrame = 0;

	frameWidth = texture->GetWidth() / numberFramesInTexture;
	frameHeight = texture->GetHeight() / numberFramesInTexture;
}

// constructor for tiles from tilesheets
Sprite::Sprite(Vector2 frame, Texture * image, ShaderProgram * s)
{
	texture = image;
	shader = s;

	CreateMesh();

	// Set start position
	windowRect.x = 0;
	windowRect.y = 0;

	// don't animate the tiles!
	animSpeed = 0.0f;

	frameWidth = TILE_SIZE;
	frameHeight = TILE_SIZE;

	framesPerRow = texture->GetWidth() / frameWidth;
	numberRows = texture->GetHeight() / frameHeight;

	numberFramesInTexture = framesPerRow * numberRows;
	startFrame = 0;
	endFrame = numberFramesInTexture;

	// this converts from human coords to pixel coords
	//Vector2 frameVec2 = Vector2((frame.x - 1) * TILE_SIZE, (frame.y - 1) * TILE_SIZE);

	// The lowest number input would be (0,0) so we subtract 1 from each
	currentFrame = (frame.y * framesPerRow) + (frame.x);

	currentRow = currentFrame / framesPerRow;
	currentRow--;

	currentFrame--;
}

Sprite::Sprite(int numFrames, SpriteManager* manager, std::string filepath, 
	ShaderProgram * s, Vector2 newPivot)
{
	texture = manager->GetImage(filepath);
	shader = s;

	CreateMesh();

	pivot = newPivot;

	windowRect.x = 0;
	windowRect.y = 0;

	numberFramesInTexture = numFrames;
	framesPerRow = numberFramesInTexture;

	frameWidth = texture->GetWidth() / numberFramesInTexture;
	frameHeight = texture->GetHeight() / (numberFramesInTexture/framesPerRow);

	windowRect.w = frameWidth;
	windowRect.h = frameHeight;

	startFrame = 0;
	endFrame = numberFramesInTexture;
	//textureRect.x = startFrame * textureRect.w;
}

Sprite::Sprite(int start, int end, int width, int height, SpriteManager* manager,
	std::string filepath, ShaderProgram* s, Vector2 newPivot, bool loop)
{
	texture = manager->GetImage(filepath);
	shader = s;

	CreateMesh();

	pivot = newPivot;

	windowRect.x = 0;
	windowRect.y = 0;

	startFrame = start;
	endFrame = end;	

	frameWidth = width;
	frameHeight = height;

	//TODO: This only works if there is only one row, but that is okay for now
	numberFramesInTexture = texture->GetWidth() / frameWidth;
	framesPerRow = numberFramesInTexture;

	shouldLoop = loop;
}

Sprite::Sprite(int start, int end, int numframes, SpriteManager* manager, 
	std::string filepath, ShaderProgram* s, Vector2 newPivot, bool loop)
{
	texture = manager->GetImage(filepath);
	shader = s;

	CreateMesh();

	pivot = newPivot;

	windowRect.x = 0;
	windowRect.y = 0;

	startFrame = start;
	endFrame = end;

	numberFramesInTexture = end - start + 1;

	frameWidth = texture->GetWidth() / numberFramesInTexture;
	frameHeight = texture->GetHeight();


	shouldLoop = loop;
}

Sprite::~Sprite()
{
	if (mesh != nullptr)
	{
		mesh->ClearMesh();
	}

	if (texture != nullptr)
	{
		//TODO: Should we clear the texture here?
		//texture->ClearTexture();
	}		
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

void Sprite::Render(Vector2 position, Renderer* renderer)
{
	Render(position, 0, -1, SDL_FLIP_NONE, renderer, 0);
}

bool Sprite::ShouldAnimate(float time)
{
	return numberFramesInTexture > 1 && time > lastAnimTime + 1000.0f;
}

void Sprite::AnimateMesh(float time)
{
	
}

void Sprite::Render(Vector2 position, int speed, Uint32 time, SDL_RendererFlip flip, Renderer * renderer, float angle)
{
	GLfloat multiplyColor[] = { 0.0f, 1.0f, 0.0f, 1.0f };

	ShaderProgram* shader = GetShader();

	shader->UseShader();

	renderer->uniformModel = shader->GetModelLocation();
	renderer->uniformProjection = shader->GetProjectionLocation();
	renderer->uniformView = shader->GetViewLocation();
	renderer->uniformViewTexture = shader->GetViewTextureLocation();
	renderer->uniformOffsetTexture = shader->GetOffsetTextureLocation();
	//uniformMultiplyColor = glGetUniformLocation(shader->GetID(), "multiplyColor");

	glUniformMatrix4fv(renderer->uniformProjection, 1, GL_FALSE,
		glm::value_ptr(renderer->camera.projection));

	glUniformMatrix4fv(renderer->uniformView, 1, GL_FALSE,
		glm::value_ptr(renderer->camera.CalculateViewMatrix()));

	//you have a view matrix
	//and you have a view matrix for the texture
	//then multiply by texture view matrix to get the offset for the desired sprite in the larger texture
	//you'll basically just use glm::translate

	GLfloat totalFrames = (endFrame - startFrame) + 1;

	glm::vec2 texFrame = glm::vec2((1.0f / framesPerRow), frameHeight/(GLfloat)texture->GetHeight());
	glm::vec2 texOffset = glm::vec2(0.5f, 0.0f);

	numberRows = texture->GetHeight() / frameHeight;

	if (numberRows > 1) // this is mainly the code for the tilesheets
	{
		// Only go to the next frame when enough time has passed
		if (numberFramesInTexture > 1 && animSpeed > 0 && renderer->now > lastAnimTime + 100)
		{
			currentFrame++;

			if (currentFrame > ((currentRow + 1) * framesPerRow))
				currentRow++;

			if (currentFrame > numberFramesInTexture)
			{
				currentFrame = 0;
				currentRow = 0;
			}

			lastAnimTime = renderer->now;
			//std::cout << currentFrame << std::endl;
		}

		// Set the texture offset based on the current frame
		unsigned int currentFrameOnRow = (currentFrame % framesPerRow);
		texOffset.x = (1.0f / framesPerRow) * currentFrameOnRow;
		texOffset.y = (frameHeight * (currentRow)) / (GLfloat)texture->GetHeight();
	}
	else
	{
		// 0, 1, 2, 3, 4, 5
		// animation is frames 2, 3 and 4
		// start frame = 2
		// end frame = 4
		// number of frames in animation = 3 (a difference of 2 from the start)

		// Only go to the next frame when enough time has passed
		if (numberFramesInTexture > 1 && animSpeed > 0 && renderer->now > lastAnimTime + 100)
		{
			currentFrame++;

			if (currentFrame > endFrame)
			{
				currentFrame = startFrame;
			}

			lastAnimTime = renderer->now;
			//std::cout << currentFrame << std::endl;
		}

		texOffset.x = (1.0f / framesPerRow) * currentFrame;
		texOffset.y = 0;
	}

	// Send the info to the shader
	glUniform2fv(renderer->uniformViewTexture, 1, glm::value_ptr(texFrame));
	glUniform2fv(renderer->uniformOffsetTexture, 1, glm::value_ptr(texOffset));

	

	glm::mat4 model(1.0f);

	// Translate, Rotate, Scale
	model = glm::translate(model, glm::vec3(position.x, position.y, -2.0f));
	//model = glm::rotate(model, currentAngle * toRadians, glm::vec3(0.0f, -1.0f, 0.0f));
	model = glm::scale(model, glm::vec3(-1 * scale.x * texture->GetWidth() / (GLfloat)(framesPerRow), 
		scale.y * texture->GetHeight() / (GLfloat)numberRows, 1.0f));

	// Set uniform variables
	glUniformMatrix4fv(renderer->uniformModel, 1, GL_FALSE, glm::value_ptr(model));

	// Use Texture
	texture->UseTexture();

	// Render Mesh
	mesh->RenderMesh();
}

const SDL_Rect* Sprite::GetRect()
{
	return &windowRect;
}

void Sprite::SetScale(Vector2 s)
{
	scale = s;
}