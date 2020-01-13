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

	numberFrames = 1;

	CreateMesh();

	animLow = 0;
	animHigh = 0;
	currentFrame = 0;
}

// constructor for tiles from tilesheets
Sprite::Sprite(Vector2 frame, Texture * image, ShaderProgram * s)
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

	numberFrames = numFrames;
	frameWidth = texture->GetWidth() / numberFrames;
	frameHeight = texture->GetHeight();

	windowRect.w = frameWidth;
	windowRect.h = frameHeight;

	startFrame = 0;
	endFrame = numberFrames;
	//textureRect.x = startFrame * textureRect.w;
}

Sprite::Sprite(int start, int end, int numFrames, SpriteManager* manager, 
	std::string filepath, ShaderProgram* s, Vector2 newPivot, bool loop)
{
	texture = manager->GetImage(filepath);
	shader = s;

	CreateMesh();

	pivot = newPivot;

	windowRect.x = 0;
	windowRect.y = 0;

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
	return numberFrames > 1 && time > lastAnimTime + 1000.0f;
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

	GLfloat totalFrames = numberFrames;

	// Texture scaling
	glm::mat4 textureScaleMatrix(1.0f);
	textureScaleMatrix = glm::scale(textureScaleMatrix,
		glm::vec3(1.0f / totalFrames, 1.0f, 1.0f));

	// Texture translation
	glm::mat4 textureTranslateMatrix(1.0f);

	glm::vec2 texFrame = glm::vec2((1.0f / totalFrames), 1.0f);
	glm::vec2 texOffset = glm::vec2(0.5f, 0.0f);

	// Only go to the next frame when enough time has passed
	if (numberFrames > 1 && renderer->now > lastAnimTime + 100)
	{
		currentFrame++;
		if (currentFrame > numberFrames)
			currentFrame = 0;

		lastAnimTime = renderer->now;
		//std::cout << currentFrame << std::endl;
	}

	// Set the texture offset based on the current frame
	texOffset.x = (1.0f / totalFrames) * currentFrame;

	// Send the info to the shader
	glUniform2fv(renderer->uniformViewTexture, 1, glm::value_ptr(texFrame));
	glUniform2fv(renderer->uniformOffsetTexture, 1, glm::value_ptr(texOffset));

	glm::mat4 model(1.0f);

	// Translate, Rotate, Scale
	model = glm::translate(model, glm::vec3(position.x, position.y, 2.0f));
	//model = glm::rotate(model, currentAngle * toRadians, glm::vec3(0.0f, -1.0f, 0.0f));
	model = glm::scale(model, glm::vec3(scale.x * texture->GetWidth() / (GLfloat)(numberFrames), scale.y * texture->GetHeight(), 1.0f));

	// Set uniform variables
	glUniformMatrix4fv(renderer->uniformModel, 1, GL_FALSE, glm::value_ptr(model));
	//TODO: Set uniform variable for the current frame of sprite animation

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