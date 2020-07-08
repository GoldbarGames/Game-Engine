#include "Sprite.h"
#include "globals.h"
#include "Renderer.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using std::string;

Mesh* Sprite::mesh = nullptr;

unsigned int Sprite::Size()
{
	unsigned int totalSize = 0;

	if (shader != nullptr)
		totalSize += sizeof(shader);

	if (mesh != nullptr)
		totalSize += sizeof(mesh);

	if (texture != nullptr)
		totalSize += sizeof(texture);

	totalSize += sizeof(color);
	totalSize += sizeof(keepPositionRelativeToCamera);
	totalSize += sizeof(keepScaleRelativeToCamera);
	totalSize += sizeof(lastAnimTime);
	totalSize += sizeof(frameWidth);
	totalSize += sizeof(frameHeight);
	totalSize += sizeof(scale);
	totalSize += sizeof(shouldLoop);
	totalSize += sizeof(startFrame);
	totalSize += sizeof(endFrame);
	totalSize += sizeof(numberFramesInTexture);
	totalSize += sizeof(framesPerRow);
	totalSize += sizeof(numberRows);
	totalSize += sizeof(pivot);
	totalSize += sizeof(filename);
	//totalSize += sizeof(windowRect);

	return totalSize;
}


void Sprite::CreateMesh()
{
	if (mesh == nullptr)
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
}

Sprite::Sprite(Texture* t, ShaderProgram* s)
{
	model = glm::mat4(1.0f);
	texture = t;
	shader = s;

	numberFramesInTexture = 1;
	framesPerRow = numberFramesInTexture;
	startFrame = 0;
	endFrame = numberFramesInTexture;

	CreateMesh();
	currentFrame = 0;

	frameWidth = texture->GetWidth() / numberFramesInTexture;
	frameHeight = texture->GetHeight() / numberFramesInTexture;
}

// constructor for tiles from tilesheets
Sprite::Sprite(Vector2 frame, Texture * image, ShaderProgram * s)
{
	model = glm::mat4(1.0f);
	texture = image;
	shader = s;

	CreateMesh();

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

Sprite::Sprite(int numFrames, SpriteManager* manager, const std::string& filepath,
	ShaderProgram * s, Vector2 newPivot)
{
	model = glm::mat4(1.0f);
	filename = filepath;
	texture = manager->GetImage(filepath);
	shader = s;

	CreateMesh();

	pivot = newPivot;

	numberFramesInTexture = numFrames;
	framesPerRow = numberFramesInTexture;

	frameWidth = texture->GetWidth() / numberFramesInTexture;
	frameHeight = texture->GetHeight() / (numberFramesInTexture/framesPerRow);

	startFrame = 0;
	endFrame = numberFramesInTexture;
}

Sprite::Sprite(int start, int end, int width, int height, SpriteManager* manager,
	const std::string& filepath, ShaderProgram* s, Vector2 newPivot, bool loop)
{
	model = glm::mat4(1.0f);
	filename = filepath;
	texture = manager->GetImage(filepath);
	shader = s;

	CreateMesh();

	pivot = newPivot;

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
	const std::string& filepath, ShaderProgram* s, Vector2 newPivot, bool loop)
{
	model = glm::mat4(1.0f);
	texture = manager->GetImage(filepath);
	shader = s;

	CreateMesh();

	pivot = newPivot;

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
		//delete mesh;
	}

	if (texture != nullptr)
	{
		//TODO: Should we clear the texture here?
		//texture->ClearTexture();
	}		
}

void Sprite::Render(Vector2 position, Renderer* renderer)
{
	Render(position, 0, -1, SDL_FLIP_NONE, renderer, glm::vec3(0,0,0));
}

bool Sprite::ShouldAnimate(float time)
{
	return numberFramesInTexture > 1 && time > lastAnimTime + 1000.0f;
}

void Sprite::AnimateMesh(float time)
{
	
}

glm::vec2 Sprite::CalculateRenderFrame(Renderer* renderer, float animSpeed)
{
	glm::vec2 texOffset = glm::vec2(0.5f, 0.0f);

	numberRows = texture->GetHeight() / frameHeight;

	if (numberRows > 1) // this is mainly the code for the tilesheets
	{
		// Only go to the next frame when enough time has passed
		if (numberFramesInTexture > 1 && animSpeed > 0 && renderer->now > lastAnimTime + animSpeed)
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

	return texOffset;
}

void Sprite::CalculateModel(Vector2 position, glm::vec3 rotation, Renderer* renderer, SDL_RendererFlip flip)
{
	if (rotation.x >= 89)
		int test = 0;

	// Only recalculate the model if position, rotation, or scale have changed
	if (position != lastPosition || rotation != lastRotation || scale != lastScale || keepPositionRelativeToCamera)
	{
		model = glm::mat4(1.0f);

		lastPosition = position;
		lastRotation = rotation;
		lastScale = scale;

		// Translate, Rotate, Scale
		//TODO: Translate based on pivot point: (center - pivot)
		// this will make the sprites look more centered

		// Position
		if (keepPositionRelativeToCamera)
		{
			if (renderer->guiCamera.useOrthoCamera)
			{
				model = glm::translate(model, glm::vec3(position.x + renderer->guiCamera.position.x,
					position.y + renderer->guiCamera.position.y, -2.0f));
			}
			else
			{
				model = glm::translate(model, glm::vec3(position.x + renderer->guiCamera.position.x,
					position.y + renderer->guiCamera.position.y, renderer->guiCamera.position.z));
			}
		}
		else
		{
			model = glm::translate(model, glm::vec3(position.x, position.y, -2.0f));
		}

		// Rotation
		/*
		if (!keepPositionRelativeToCamera)
		{
			
		}*/

		const float toRadians = 3.14159265f / 180.0f;
		model = glm::rotate(model, rotation.x * toRadians, glm::vec3(-1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, rotation.y * toRadians, glm::vec3(0.0f, -1.0f, 0.0f));
		model = glm::rotate(model, rotation.z * toRadians, glm::vec3(0.0f, 0.0f, -1.0f));

		// Scale
		// TODO: Remove the enum, just change the scale
		if (flip != SDL_FLIP_HORIZONTAL)
		{
			model = glm::scale(model, glm::vec3(-1 * scale.x * texture->GetWidth() / (GLfloat)(framesPerRow),
				scale.y * texture->GetHeight() / (GLfloat)numberRows, 1.0f));
		}
		else
		{
			model = glm::scale(model, glm::vec3(scale.x * texture->GetWidth() / (GLfloat)(framesPerRow),
				scale.y * texture->GetHeight() / (GLfloat)numberRows, 1.0f));
		}
	}	
}

void Sprite::Render(Vector2 position, int speed, Uint32 time, SDL_RendererFlip flip, Renderer * renderer, glm::vec3 rotation)
{
	renderer->drawCallsPerFrame++;

	ShaderProgram* shader = GetShader();
	shader->UseShader();

	if (keepPositionRelativeToCamera)
	{
		glUniformMatrix4fv(shader->GetUniformVariable(ShaderVariable::view), 1, GL_FALSE,
			glm::value_ptr(renderer->guiCamera.CalculateViewMatrix()));
	}
	else
	{
		glUniformMatrix4fv(shader->GetUniformVariable(ShaderVariable::view), 1, GL_FALSE,
			glm::value_ptr(renderer->camera.CalculateViewMatrix()));
	}

	GLfloat totalFrames = (endFrame - startFrame) + 1;
	glm::vec2 texFrame = glm::vec2((1.0f / framesPerRow), frameHeight/(GLfloat)texture->GetHeight());
	glm::vec2 texOffset = CalculateRenderFrame(renderer, speed);
	
	// Send the info to the shader
	glUniform2fv(shader->GetUniformVariable(ShaderVariable::texFrame), 1, glm::value_ptr(texFrame));
	glUniform2fv(shader->GetUniformVariable(ShaderVariable::texOffset), 1, glm::value_ptr(texOffset));

	GLfloat fadePoint;
	glm::vec4 fadeColor;

	switch (shader->GetName())
	{
	case ShaderName::FadeInOut:
		fadePoint = abs(sin(renderer->now / 1000));
		fadeColor = glm::vec4(fadePoint, fadePoint, fadePoint, fadePoint);

		// in order to fade to a color, we want to oscillate all the colors we DON'T want
		// (so in order to fade to clear/transparent, we oscillate EVERY color)		

		if (selectedColor == "red")
		{
			fadeColor = glm::vec4(1, fadePoint, fadePoint, 1);
		}
		else if (selectedColor == "green")
		{
			fadeColor = glm::vec4(fadePoint, 1, fadePoint, 1);
		}
		else if (selectedColor == "blue")
		{
			fadeColor = glm::vec4(fadePoint, fadePoint, 1, 1);
		}
		else if (selectedColor == "black")
		{
			fadeColor = glm::vec4(fadePoint, fadePoint, fadePoint, 1);
		}
		else if (selectedColor == "white")
		{
			fadeColor = glm::vec4(1, 1, 1, 1);
		}
		else if (selectedColor == "clear")
		{
			fadeColor = glm::vec4(1, 1, 1, fadePoint);
		}

		glUniform4fv(shader->GetUniformVariable(ShaderVariable::fadeColor), 1, glm::value_ptr(fadeColor));
		break;
	case ShaderName::Glow:
		fadePoint = abs(sin(renderer->now / 1000));
		fadeColor = glm::vec4(fadePoint, fadePoint, fadePoint, fadePoint);

		glUniform1f(shader->GetUniformVariable(ShaderVariable::currentTime), renderer->now);

		// in order to fade to a color, we want to oscillate all the colors we DON'T want
		// (so in order to fade to clear/transparent, we oscillate EVERY color)

		/*
		std::string selectedColor = "green";

		if (selectedColor == "red")
		{
			fadeColor = glm::vec4(1, 0, 0, 1);
		}
		else if (selectedColor == "green")
		{
			fadeColor = glm::vec4(0, 1, 0, 1);
		}
		else if (selectedColor == "blue")
		{
			fadeColor = glm::vec4(0, 0, 1, 1);
		}
		else if (selectedColor == "black")
		{
			fadeColor = glm::vec4(0, 0, 0, 1);
		}
		else if (selectedColor == "white")
		{
			fadeColor = glm::vec4(1, 1, 1, 1);
		}*/

		glUniform4fv(shader->GetUniformVariable(ShaderVariable::fadeColor), 1, glm::value_ptr(fadeColor));
		break;
	default:
		glm::vec4 spriteColor = glm::vec4(color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f);
		glUniform4fv(shader->GetUniformVariable(ShaderVariable::fadeColor), 1, glm::value_ptr(spriteColor));
		break;
	}

	CalculateModel(position, rotation, renderer, flip);

	// Projection
	if (keepScaleRelativeToCamera)
	{
		glUniformMatrix4fv(shader->GetUniformVariable(ShaderVariable::projection), 1, GL_FALSE,
			glm::value_ptr(renderer->camera.guiProjection));
	}
	else
	{
		glUniformMatrix4fv(shader->GetUniformVariable(ShaderVariable::projection), 1, GL_FALSE,
			glm::value_ptr(renderer->camera.projection));
	}

	// Set uniform variables
	glUniformMatrix4fv(shader->GetUniformVariable(ShaderVariable::model), 1, GL_FALSE, glm::value_ptr(model));

	// Use Texture
	texture->UseTexture();

	// Render Mesh
	mesh->RenderMesh();

	// TODO: Only draw these rectangles for game entities and not menu images
	/*
	if (GetModeDebug())
	{
		spriteColor = glm::vec4(1, 0, 0, 1); //TODO: Maybe make this color a parameter?
		glUniform4fv(shader->GetUniformVariable("spriteColor"), 1, glm::value_ptr(spriteColor));

		// Use Texture
		renderer->debugSprite->texture->UseTexture();

		// Render Mesh
		mesh->RenderMesh();
	}
	*/


	// Update this rectangle for calculating physics
	//windowRect.x = position.x;
	//windowRect.y = position.y;
	//windowRect.w = frameWidth;
	//windowRect.h = frameHeight;

	//TODO: Draw a rectangle around the sprite's bounds



	/*


	glLineWidth(10);

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	glBegin(GL_POLYGON);
	glVertex2i(windowRect.x, windowRect.y);
	glVertex2i(windowRect.x + windowRect.w, windowRect.y);
	glVertex2i(windowRect.x + windowRect.w, windowRect.y + windowRect.h);
	glVertex2i(windowRect.x, windowRect.y + windowRect.h);
	glEnd();

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	//glFlush();
	*/
}

const SDL_Rect* Sprite::GetRect()
{
	return &windowRect;
}

void Sprite::SetScale(Vector2 s)
{
	scale = s;
}