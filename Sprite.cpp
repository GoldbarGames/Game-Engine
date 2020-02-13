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
	totalSize += sizeof(renderRelativeToCamera);
	totalSize += sizeof(keepScaleRelativeToCamera);
	totalSize += sizeof(lastAnimTime);
	totalSize += sizeof(animSpeed);
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
	texture = image;
	shader = s;

	CreateMesh();

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
	std::string filepath, ShaderProgram* s, Vector2 newPivot, bool loop)
{
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
	std::string filepath, ShaderProgram* s, Vector2 newPivot, bool loop)
{
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
	renderer->drawCallsPerFrame++;

	GLfloat multiplyColor[] = { 0.0f, 1.0f, 0.0f, 1.0f };

	ShaderProgram* shader = GetShader();

	shader->UseShader();

	if (renderRelativeToCamera)
	{
		glUniformMatrix4fv(shader->GetUniformVariable("view"), 1, GL_FALSE,
			glm::value_ptr(renderer->guiCamera.CalculateViewMatrix()));
	}
	else
	{
		glUniformMatrix4fv(shader->GetUniformVariable("view"), 1, GL_FALSE,
			glm::value_ptr(renderer->camera.CalculateViewMatrix()));
	}



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
	glUniform2fv(shader->GetUniformVariable("texFrame"), 1, glm::value_ptr(texFrame));
	glUniform2fv(shader->GetUniformVariable("texOffset"), 1, glm::value_ptr(texOffset));

	glm::vec4 spriteColor = glm::vec4(color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f);
	glUniform4fv(shader->GetUniformVariable("spriteColor"), 1, glm::value_ptr(spriteColor));

	if (shader->GetName() == "fade-in-out")
	{
		GLfloat fadePoint = abs(sin(renderer->now / 1000));
		glm::vec4 fadeColor = glm::vec4(fadePoint, fadePoint, fadePoint, fadePoint);

		// in order to fade to a color, we want to oscillate all the colors we DON'T want
		// (so in order to fade to clear/transparent, we oscillate EVERY color)

		std::string selectedColor = "clear";

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

		glUniform4fv(shader->GetUniformVariable("fadeColor"), 1, glm::value_ptr(fadeColor));
	}
	else if (shader->GetName() == "glow")
	{      
		GLfloat fadePoint = abs(sin(renderer->now / 1000));
		glm::vec4 fadeColor = glm::vec4(fadePoint, fadePoint, fadePoint, fadePoint);

		glUniform1f(shader->GetUniformVariable("currentTime"), renderer->now);

		// in order to fade to a color, we want to oscillate all the colors we DON'T want
		// (so in order to fade to clear/transparent, we oscillate EVERY color)

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
		}

		glUniform4fv(shader->GetUniformVariable("fadeColor"), 1, glm::value_ptr(fadeColor));
	}

	glm::mat4 model(1.0f);

	// Translate, Rotate, Scale
	//TODO: Translate based on pivot point: (center - pivot)
	// this will make the sprites look more centered
	
	if (renderRelativeToCamera)
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

	if (keepScaleRelativeToCamera)
	{
		glUniformMatrix4fv(shader->GetUniformVariable("projection"), 1, GL_FALSE,
			glm::value_ptr(renderer->camera.guiProjection));
	}
	else
	{
		glUniformMatrix4fv(shader->GetUniformVariable("projection"), 1, GL_FALSE,
			glm::value_ptr(renderer->camera.projection));
	}

	// Rotation
	if (!renderRelativeToCamera)
	{
		const float toRadians = 3.14159265f / 180.0f;
		model = glm::rotate(model, angle * toRadians, glm::vec3(0.0f, -1.0f, 0.0f));
	}

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

	// Set uniform variables
	glUniformMatrix4fv(shader->GetUniformVariable("model"), 1, GL_FALSE, glm::value_ptr(model));

	// Use Texture
	texture->UseTexture();

	// Render Mesh
	mesh->RenderMesh();



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