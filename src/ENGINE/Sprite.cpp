#include "leak_check.h"
#include "Sprite.h"
#include "globals.h"
#include "Renderer.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using std::string;

//Mesh* Sprite::mesh = nullptr;
Mesh* Sprite::meshQuad = nullptr;
Mesh* Sprite::meshTri = nullptr;
Mesh* Sprite::meshLine = nullptr;
std::string Sprite::selectedColor = "clear";

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
	//totalSize += sizeof(scale);
	totalSize += sizeof(shouldLoop);
	totalSize += sizeof(startFrame);
	totalSize += sizeof(endFrame);
	totalSize += sizeof(numberFramesInTexture);
	totalSize += sizeof(framesPerRow);
	totalSize += sizeof(numberRows);
	totalSize += sizeof(pivot);
	//totalSize += sizeof(filename);
	//totalSize += sizeof(windowRect);

	return totalSize;
}

void Sprite::CreateMesh(MeshType meshType)
{
	if (mesh == nullptr)
	{
		if (meshType == MeshType::Quad)
		{
			if (meshQuad == nullptr)
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

				meshQuad = neww Mesh();
				meshQuad->CreateMesh(quadVertices, quadIndices, 20, 12);
			}

			mesh = meshQuad;

		}
		else if (meshType == MeshType::Triangle)
		{
			if (meshTri == nullptr)
			{
				unsigned int triIndices[] = {
					0, 3, 1,
					1, 3, 2,
					2, 3, 0,
					0, 1, 2
				};

				GLfloat triVertices[] = {
					-1.0f, -1.0f, -1.0f,
					0.0f, -1.0f, 1.0f,
					1.0f, -1.0f, 0.0f,
					0.0f, 1.0f, -1.0f
				};

				meshTri = neww Mesh();
				meshTri->CreateMesh(triVertices, triIndices, 12, 6);
			}

			mesh = meshTri;
		}
		else if (meshType == MeshType::Line)
		{
			if (meshLine == nullptr)
			{
				unsigned int lineIndices[] = {
					0, 3, 1,
					1, 3, 2,
					2, 3, 0,
					0, 1, 2
				};

				GLfloat lineVertices[] = {
					-1.0f, 0.0f, 0.0f,  1.0f, 0.0f,
					1.0f, 0.0f, 0.0f,   0.0f, 0.0f,
					-1.0f, 0.01f, 0.0f,   1.0f, 1.0f,
					1.0f, 0.01f, 0.0f,    0.0f, 1.0f
				};

				meshLine = neww Mesh();
				meshLine->CreateMesh(lineVertices, lineIndices, 20, 12);
			}

			mesh = meshLine;
		}
	}
}

Sprite::Sprite(ShaderProgram* s)
{
	model = glm::mat4(1.0f);
	shader = s;
	texture = nullptr;

	numberFramesInTexture = 1;
	framesPerRow = numberFramesInTexture;
	startFrame = 0;
	endFrame = numberFramesInTexture;

	CreateMesh(MeshType::Quad);
	currentFrame = 0;
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
Sprite::Sprite(const Vector2& frame, Texture* image, ShaderProgram* s, const int tileSize)
{
	model = glm::mat4(1.0f);
	texture = image;
	shader = s;

	CreateMesh();

	frameWidth = tileSize;
	frameHeight = tileSize;

	framesPerRow = std::max(1, texture->GetWidth() / frameWidth);
	numberRows = std::max(1, texture->GetHeight() / frameHeight);

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

Sprite::Sprite(int numFrames, const SpriteManager& manager, const std::string& filepath,
	ShaderProgram* s, Vector2 newPivot)
{
	model = glm::mat4(1.0f);
	texture = manager.GetImage(filepath);
	shader = s;

	CreateMesh();

	pivot = newPivot;

	numberFramesInTexture = numFrames;
	framesPerRow = numberFramesInTexture;

	frameWidth = texture->GetWidth() / numberFramesInTexture;
	frameHeight = texture->GetHeight() / (numberFramesInTexture / framesPerRow);

	startFrame = 0;
	endFrame = numberFramesInTexture;
}

Sprite::Sprite(int start, int end, int width, int height, const SpriteManager& manager,
	const std::string& filepath, ShaderProgram* s, const Vector2& newPivot, bool loop)
{
	model = glm::mat4(1.0f);
	texture = manager.GetImage(filepath);
	shader = s;

	CreateMesh();

	pivot = newPivot;

	startFrame = start;
	endFrame = end;
	currentFrame = startFrame;

	if (width > texture->GetWidth())
		width = texture->GetWidth();
	if (height > texture->GetHeight())
		height = texture->GetHeight();

	frameWidth = width;
	frameHeight = height;

	//TODO: This only works if there is only one row, but that is okay for now
	numberFramesInTexture = texture->GetWidth() / frameWidth;
	framesPerRow = numberFramesInTexture;

	shouldLoop = loop;
}

Sprite::Sprite(int start, int end, int numframes, const SpriteManager& manager,
	const std::string& filepath, ShaderProgram* s, const Vector2& newPivot, bool loop)
{
	model = glm::mat4(1.0f);
	texture = manager.GetImage(filepath);
	shader = s;

	CreateMesh();

	pivot = newPivot;

	startFrame = start;
	endFrame = end;
	currentFrame = startFrame;

	numberFramesInTexture = end - start + 1;

	frameWidth = texture->GetWidth() / numberFramesInTexture;
	frameHeight = texture->GetHeight();

	shouldLoop = loop;
}

Sprite::Sprite()
{
	model = glm::mat4(1.0f);
	CreateMesh(MeshType::Quad);
}

Sprite::~Sprite()
{
	// We don't actually want to delete any shaders or textures here
	// because they are managed by other objects and will potentially
	// be used again many more times by other sprites.
}

void Sprite::Render(const glm::vec3& position, const Renderer& renderer, const Vector2& scale, const glm::vec3& rotation)
{
	Render(position, 0, renderer, scale, rotation);
}

bool Sprite::ShouldAnimate(float time)
{
	return numberFramesInTexture > 1 && time > lastAnimTime + 1000.0f;
}

void Sprite::AnimateMesh(float time)
{
	
}

void Sprite::SetTexture(Texture* t)
{
	texture = t;
	startFrame = 0;
	currentFrame = 0;
	endFrame = 0;
	frameWidth = texture->GetWidth();
	frameHeight = texture->GetHeight();
	pivot = Vector2(0, 0);

	//TODO: This only works if there is only one row, but that is okay for now
	numberFramesInTexture = 1;
	framesPerRow = 1;
}

const std::string& Sprite::GetFileName()
{
	if (texture == nullptr)
		return Globals::NONE_STRING;

	return texture->GetFilePath();
}

glm::vec2 Sprite::CalculateRenderFrame(const Renderer& renderer, float animSpeed)
{
	glm::vec2 texOffset = glm::vec2(0.5f, 0.0f);

	numberRows = texture->GetHeight() / frameHeight;

	if (numberRows > 1) // this is mainly the code for the tilesheets
	{
		// Only go to the next frame when enough time has passed
		if (numberFramesInTexture > 1 && animSpeed > 0 && renderer.now > lastAnimTime + animSpeed)
		{
			previousFrame = currentFrame;
			currentFrame++;

			if (currentFrame > ((currentRow + 1) * framesPerRow))
				currentRow++;

			if (currentFrame > numberFramesInTexture)
			{
				currentFrame = 0;
				currentRow = 0;
			}

			lastAnimTime = renderer.now;
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
		if (numberFramesInTexture > 1 && animSpeed > 0 && renderer.now > lastAnimTime + animSpeed)
		{
			previousFrame = currentFrame;
			currentFrame++;

			if (currentFrame > endFrame)
			{
				playedOnce = true;
				if (shouldLoop)
				{
					currentFrame = startFrame;
				}
				else
				{
					currentFrame = endFrame;
				}				
			}

			lastAnimTime = renderer.now;
			//std::cout << currentFrame << std::endl;
		}

		texOffset.x = (1.0f / framesPerRow) * currentFrame;
		texOffset.y = 0;
	}

	return texOffset;
}

void Sprite::CalculateModel(glm::vec3 position, const glm::vec3& rotation, const Vector2& scale, const Renderer& renderer)
{
	if (rotation.x >= 89)
		int test = 0;

	// TODO: Maybe do a clever multiplication trick instead
	if (scale.x > 0) // flip the pivot x based on direction
		position += glm::vec3(Camera::MULTIPLIER * pivot.x, Camera::MULTIPLIER * pivot.y, 0);
	else
		position += glm::vec3(-Camera::MULTIPLIER * pivot.x, Camera::MULTIPLIER * pivot.y, 0);

	// Only recalculate the model if position, rotation, or scale have changed
	if (position != lastPosition || rotation != lastRotation || scale != lastScale || keepPositionRelativeToCamera)
	{
		model = glm::mat4(1.0f);

		lastPosition = position;
		lastRotation = rotation;
		lastScale = scale;

		// Translate, Rotate, Scale

		// Position
		if (keepPositionRelativeToCamera)
		{
			if (renderer.guiCamera.useOrthoCamera)
			{
				model = glm::translate(model, glm::vec3(position.x + renderer.guiCamera.position.x,
					position.y + renderer.guiCamera.position.y, -2.0f));
			}
			else
			{
				model = glm::translate(model, glm::vec3(position.x + renderer.guiCamera.position.x,
					position.y + renderer.guiCamera.position.y, renderer.guiCamera.position.z + position.z));
			}
		}
		else
		{
			if (renderer.camera.useOrthoCamera)
			{
				model = glm::translate(model, glm::vec3(position.x, position.y, -2.0f));
			}
			else
			{
				model = glm::translate(model, glm::vec3(position.x, position.y, position.z));
			}
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
		int width = 1;
		if (texture != nullptr)
			width = texture->GetWidth();

		int height = 1;
		if (texture != nullptr)
			height = texture->GetHeight();

		model = glm::scale(model, glm::vec3(-1 * scale.x * width / (GLfloat)(framesPerRow),
			scale.y * height / (GLfloat)numberRows, 1.0f));
	}	
}

// NOTE: This function expects a center-coordinate rectangle to be rendered,
// so if you pass in a top-left rectangle, you'll see something wrong
void Sprite::Render(const glm::vec3& position, int speed, const Renderer& renderer, const Vector2& scale, const glm::vec3& rotation)
{
	renderer.drawCallsPerFrame++;

	ShaderProgram* shader = GetShader();

	if (shader == nullptr)
	{
		shader = renderer.shaders[ShaderName::Default];
	}

	shader->UseShader();

	if (keepPositionRelativeToCamera)
	{
		glUniformMatrix4fv(shader->GetUniformVariable(ShaderVariable::view), 1, GL_FALSE,
			glm::value_ptr(renderer.guiCamera.CalculateViewMatrix()));
	}
	else
	{
		glUniformMatrix4fv(shader->GetUniformVariable(ShaderVariable::view), 1, GL_FALSE,
			glm::value_ptr(renderer.camera.CalculateViewMatrix()));
	}

	float height = frameHeight;
	if (texture != nullptr)
		height = texture->GetHeight();

	GLfloat totalFrames = (endFrame - startFrame) + 1;
	glm::vec2 texFrame = glm::vec2((1.0f / framesPerRow), frameHeight/height);
	glm::vec2 texOffset = glm::vec2(0, 0);
	
	if (texture != nullptr)
		texOffset = CalculateRenderFrame(renderer, speed);

	// Send the info to the shader
	glUniform2fv(shader->GetUniformVariable(ShaderVariable::texFrame), 1, glm::value_ptr(texFrame));
	glUniform2fv(shader->GetUniformVariable(ShaderVariable::texOffset), 1, glm::value_ptr(texOffset));

	float fadePoint, fadeR, fadeG, fadeB, fadeA, freq;
	glm::vec4 fadeColor;

	switch (shader->GetName())
	{
	case ShaderName::FadeInOut:
		// in order to fade to a color, we want to oscillate all the colors we DON'T want
		// (so in order to fade to clear/transparent, we oscillate EVERY color)		
		// To fade to red, oscillate blue and green, but not red or alpha
		fadePoint = abs(sin(renderer.now / 1000));

		// TODO: Change the /255.0f to a custom max amount < 1 (255/255)
		fadeR = color.r > 0 ? (color.r/255.0f) : fadePoint;
		fadeG = color.g > 0 ? (color.g/255.0f) : fadePoint;
		fadeB = color.b > 0 ? (color.b/255.0f) : fadePoint;
		fadeA = color.a > 0 ? (color.a/255.0f) : fadePoint;

		fadeColor = glm::vec4(fadeR, fadeG, fadeB, fadeA);

		glUniform4fv(shader->GetUniformVariable(ShaderVariable::fadeColor), 1, glm::value_ptr(fadeColor));
		break;
	case ShaderName::Glow:
		freq = 0.002f;
		fadePoint = abs(sin(renderer.now * freq));
		fadeR = color.r > fadePoint ? (color.r / 255.0f) : fadePoint;
		fadeG = color.g > fadePoint ? (color.g / 255.0f) : fadePoint;
		fadeB = color.b > fadePoint ? (color.b / 255.0f) : fadePoint;
		fadeA = color.a > fadePoint ? (color.a / 255.0f) : fadePoint;

		fadeR = std::max(fadeR, 0.5f);
		fadeG = std::max(fadeG, 0.5f);
		fadeB = std::max(fadeB, 0.5f);
		fadeA = std::max(fadeA, 0.5f);

		fadeColor = glm::vec4(fadeR, fadeG, fadeB, fadeA);

		glUniform1f(shader->GetUniformVariable(ShaderVariable::currentTime), renderer.now);
		glUniform1f(shader->GetUniformVariable(ShaderVariable::frequency), freq);

		// in order to fade to a color, we want to oscillate all the colors we DON'T want
		// (so in order to fade to clear/transparent, we oscillate EVERY color)

		glUniform4fv(shader->GetUniformVariable(ShaderVariable::fadeColor), 1, glm::value_ptr(fadeColor));
		break;
	default:
		glm::vec4 spriteColor = glm::vec4(color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f);
		glUniform4fv(shader->GetUniformVariable(ShaderVariable::fadeColor), 1, glm::value_ptr(spriteColor));
		glUniform1f(shader->GetUniformVariable(ShaderVariable::currentTime), renderer.now);
		break;
	}

	CalculateModel(position, rotation, scale, renderer);

	// Projection
	if (keepScaleRelativeToCamera)
	{
		glUniformMatrix4fv(shader->GetUniformVariable(ShaderVariable::projection), 1, GL_FALSE,
			glm::value_ptr(renderer.camera.guiProjection));
	}
	else
	{
		glUniformMatrix4fv(shader->GetUniformVariable(ShaderVariable::projection), 1, GL_FALSE,
			glm::value_ptr(renderer.camera.projection));
	}

	// Set uniform variables
	glUniformMatrix4fv(shader->GetUniformVariable(ShaderVariable::model), 1, GL_FALSE, glm::value_ptr(model));

	// Use Texture
	if (texture != nullptr)
		texture->UseTexture();

	// Render Mesh
	mesh->RenderMesh();
}

bool Sprite::HasAnimationElapsed()
{
	return (previousFrame > currentFrame && playedOnce) || (endFrame - startFrame == 0) || (!shouldLoop && currentFrame == endFrame);
}

void Sprite::ResetFrame()
{
	currentFrame = 0;
	previousFrame = 0;
}