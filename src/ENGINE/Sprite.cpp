#include "leak_check.h"
#include "Sprite.h"
#include "globals.h"
#include "Renderer.h"
#include "Game.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using std::string;

//Mesh* Sprite::mesh = nullptr;
Mesh* Sprite::meshQuad = nullptr;
Mesh* Sprite::meshTri = nullptr;
Mesh* Sprite::meshLine = nullptr;
Mesh* Sprite::meshPyramid = nullptr;
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
				
				/*
				unsigned int quadIndices[] = {
					0, 2, 1,
					1, 2, 3
				};

				GLfloat quadVertices[] = {
					-1.0f, 0.0f, -1.0f,  0.0f, 0.0f,	0.0f, -1.0f, 0.0f,
					1.0f, 0.0f, -1.0f,   1.0f, 0.0f,	0.0f, -1.0f, 0.0f,
					-1.0f, 0.0f, 1.0f,   0.0f, 1.0f,	0.0f, -1.0f, 0.0f,
					1.0f, 0.0f, 1.0f,    1.0f, 1.0f,	0.0f, -1.0f, 0.0f
				};

				CalcAverageNormals(quadIndices, 6, quadVertices, 32, 8, 5);

				meshQuad = new Mesh();
				meshQuad->CreateMesh(quadVertices, quadIndices, 32, 6, 8, 3, 5);

				*/



				unsigned int quadIndices[] = {
					0, 3, 1,
					1, 3, 2,
					2, 3, 0,
					0, 1, 2
				};

				GLfloat quadVertices[] = {
					-1.0f, -1.0f, 0.0f,  1.0f, 0.0f,	0.0f, 0.0f, 0.0f,
					1.0f, -1.0f, 0.0f,   0.0f, 0.0f,	0.0f, 0.0f, 0.0f,
					-1.0f, 1.0f, 0.0f,   1.0f, 1.0f,	0.0f, 0.0f, 0.0f,
					1.0f, 1.0f, 0.0f,    0.0f, 1.0f,	0.0f, 0.0f, 0.0f
				};

				CalcAverageNormals(quadIndices, 12, quadVertices, 32, 8, 5);

				meshQuad = new Mesh();
				meshQuad->CreateMesh(quadVertices, quadIndices, 32, 12, 8, 3, 5);
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

				meshTri = new Mesh();
				meshTri->CreateMesh(triVertices, triIndices, 12, 6, 5, 3, 0);
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

				meshLine = new Mesh();
				meshLine->CreateMesh(lineVertices, lineIndices, 20, 12, 5, 3, 5);
			}

			mesh = meshLine;
		}
		else if (meshType == MeshType::Pyramid)
		{
			if (meshPyramid == nullptr)
			{
				unsigned int pyramidIndices[] = {
					0, 3, 1,
					1, 3, 2,
					2, 3, 0,
					0, 1, 2
				};

				// x y z u v nx ny nz

				GLfloat pyramidVertices[] = {
					-1.0f, -1.0f, -0.6f,  1.0f, 0.0f,	0.0f, 0.0f, 0.0f,
					0.0f, -1.0f, 1.0f,   0.5f, 0.0f,	0.0f, 0.0f, 0.0f,
					1.0f, -1.0f, -0.6f,   1.0f, 0.0f,	0.0f, 0.0f, 0.0f,
					0.0f, 1.0f, 0.0f,    0.5f, 1.0f,	0.0f, 0.0f, 0.0f
				};

				CalcAverageNormals(pyramidIndices, 12, pyramidVertices, 32, 8, 5);

				meshPyramid = new Mesh();
				meshPyramid->CreateMesh(pyramidVertices, pyramidIndices, 32, 12, 8, 3, 5);
			}

			mesh = meshPyramid;

		}
	}
}

Sprite::Sprite(ShaderProgram* s, MeshType m)
{
	model = glm::mat4(1.0f);
	shader = s;
	texture = nullptr;

	numberFramesInTexture = 1;
	framesPerRow = numberFramesInTexture;
	startFrame = 0;
	endFrame = numberFramesInTexture;

	CreateMesh(m);
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
Sprite::Sprite(const glm::vec2& frame, Texture* image, ShaderProgram* s, const int tileSize, const int tileSize2, const int cf)
{
	model = glm::mat4(1.0f);
	texture = image;
	shader = s;

	CreateMesh();

	frameWidth = tileSize > 0 ? tileSize : texture->GetWidth();
	frameHeight = tileSize2 > 0 ? tileSize2 : (tileSize > 0 ? tileSize : texture->GetHeight());

	framesPerRow = std::max(1, texture->GetWidth() / frameWidth);
	numberRows = std::max(1, texture->GetHeight() / frameHeight);

	numberFramesInTexture = framesPerRow * numberRows;
	startFrame = 0;
	endFrame = numberFramesInTexture;

	// The lowest number input would be (0,0) so we subtract 1 from each
	currentFrame = cf > -1 ? cf : (frame.y * framesPerRow) + (frame.x);

	currentFrame--;
	currentRow = currentFrame / framesPerRow;
	currentRow--;
}

Sprite::Sprite(int numFrames, const SpriteManager& manager, const std::string& filepath,
	ShaderProgram* s, glm::vec2 newPivot)
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
	const std::string& filepath, ShaderProgram* s, const glm::vec2& newPivot, bool loop)
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
	const std::string& filepath, ShaderProgram* s, const glm::vec2& newPivot, bool loop)
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

void Sprite::Render(const glm::vec3& position, const Renderer& renderer, const glm::vec2& scale, const glm::vec3& rotation)
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
	pivot = glm::vec2(0, 0);

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

	bool shouldUpdateAnimation = (numberFramesInTexture > 1 && animSpeed > 0
		&& renderer.now > lastAnimTime + animSpeed
		&& !renderer.game->isPaused);

	if (numberRows > 1) // this is mainly the code for the tilesheets
	{
		// Only go to the next frame when enough time has passed
		if (shouldUpdateAnimation)
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

		if (useCustomTexOffset)
		{
			return customTexOffset;
		}
		else
		{
			unsigned int currentFrameOnRow = (currentFrame % framesPerRow);
			texOffset.x = (1.0f / framesPerRow) * currentFrameOnRow; // - (1.0f / framesPerRow);
			texOffset.y = (frameHeight * (currentRow)) / (GLfloat)texture->GetHeight();
		}

	}
	else
	{
		// Only go to the next frame when enough time has passed
		if (shouldUpdateAnimation)
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

void Sprite::CalculateModel(glm::vec3 position, const glm::vec3& rotation, const glm::vec3& scale, const Renderer& renderer)
{
	if (rotation.x >= 89)
		int test = 0;

	// flip the pivot x based on direction
	position += glm::vec3( (scale.x > 0 ? 1 : -1) * Camera::MULTIPLIER * pivot.x, Camera::MULTIPLIER * pivot.y, 0);

	// Only recalculate the model if position, rotation, or scale have changed
	if (position != lastPosition || rotation != lastRotation || scale != lastScale)
	{
		model = glm::mat4(1.0f);

		lastPosition = position;
		lastRotation = rotation;
		lastScale = scale;

		// Translate, Rotate, Scale

		//const float z = renderer.guiCamera.useOrthoCamera ? -2.0f : renderer.guiCamera.position.z + position.z;
		const float optsRelative[2] = { -2.0f, renderer.guiCamera.position.z + position.z };
		const float optsAbsolute[2] = { -2.0f, position.z };

		// Position
		if (keepPositionRelativeToCamera)
		{
			model = glm::translate(model, glm::vec3(position.x + renderer.guiCamera.position.x,
				position.y + renderer.guiCamera.position.y, optsRelative[renderer.guiCamera.useOrthoCamera]));
		}
		else
		{
			model = glm::translate(model, glm::vec3(position.x, position.y, optsAbsolute[renderer.camera.useOrthoCamera]));
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
		const int width = (texture != nullptr) ? texture->GetWidth() : 1;
		const int height = (texture != nullptr) ? texture->GetHeight() : 1;

		model = glm::scale(model, glm::vec3(-1 * scale.x * width / (GLfloat)(framesPerRow),
			scale.y * height / (GLfloat)numberRows, scale.z));
	}	
}


void Sprite::Render(const glm::vec3& position, int speed, const Renderer& renderer, const glm::vec2& scale, const glm::vec3& rotation)
{
	// TODO: Refactor this to be more efficient for 2D draw calls
	Render(position, speed, renderer, glm::vec3(scale.x, scale.y, 1.0f), rotation);
}

// NOTE: This function expects a center-coordinate rectangle to be rendered,
// so if you pass in a top-left rectangle, you'll see something wrong
void Sprite::Render(const glm::vec3& position, int speed, const Renderer& renderer, const glm::vec3& scale, const glm::vec3& rotation)
{
	if (hoverShader == nullptr)
	{
		hoverShader = (renderer.shaders.size() > 8) ? renderer.shaders[8] : renderer.shaders[0];
	}

	const ShaderProgram* shaderToUse = isHovered ? hoverShader : ((shader == nullptr) ? renderer.shaders[0] : shader);

	shaderToUse->UseShader();

	const Camera* camera = keepPositionRelativeToCamera ? &renderer.guiCamera : &renderer.camera;
	glUniformMatrix4fv(shaderToUse->GetUniformVariable(ShaderVariable::view), 1, GL_FALSE,
		glm::value_ptr(camera->CalculateViewMatrix()));

	if (!camera->useOrthoCamera)
	{
		renderer.UseLight(*shaderToUse);
	}

	const float height = (texture != nullptr) ? texture->GetHeight() : frameHeight;
	const glm::vec2 texOffset = (texture == nullptr) ? glm::vec2(0, 0) : CalculateRenderFrame(renderer, speed);
	const glm::vec2 texFrame = useCustomTexFrame ? customTexFrame : glm::vec2((1.0f / framesPerRow), frameHeight/height);

	// Send the info to the shader
	glUniform2fv(shaderToUse->GetUniformVariable(ShaderVariable::texFrame), 1, glm::value_ptr(texFrame));
	glUniform2fv(shaderToUse->GetUniformVariable(ShaderVariable::texOffset), 1, glm::value_ptr(texOffset));

	// Calculate 2D lighting
	const float maxDistanceToLight = 10 * Globals::TILE_SIZE;
	float distanceToLightSource = 0;
	float lightRatio = 1.0f;

	// TODO: Only iterate over light sources near the screen (within render distance)
	for (const auto& lightSource : renderer.game->lightSourcesInLevel)
	{
		distanceToLightSource = glm::distance(position, lightSource->position);
		lightRatio += 1.0f - std::min(1.0f, (distanceToLightSource / maxDistanceToLight));
	}

	if (distanceToLightSource == 0 && renderer.game->player != nullptr)
	{
		//distanceToLightSource = glm::distance(position, renderer.game->player->position);
		//lightRatio = 1.0f - std::min(1.0f, (distanceToLightSource / maxDistanceToLight));
	}

	glUniform1f(shaderToUse->GetUniformVariable(ShaderVariable::distanceToLight2D), lightRatio);

	glm::vec4 spriteColor = glm::vec4(color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f);
	glUniform4fv(shaderToUse->GetUniformVariable(ShaderVariable::fadeColor), 1, glm::value_ptr(spriteColor));
	glUniform1f(shaderToUse->GetUniformVariable(ShaderVariable::currentTime), renderer.now);

	renderer.game->gui->SetShaderVariables(*this, shaderToUse);

	CalculateModel(position, rotation, scale, renderer);

	// Projection
	const glm::mat4* cameraProjection = keepScaleRelativeToCamera ? &renderer.camera.guiProjection : &renderer.camera.projection;
	glUniformMatrix4fv(shaderToUse->GetUniformVariable(ShaderVariable::projection), 1, GL_FALSE, glm::value_ptr(*cameraProjection));

	// For lights
	glUniform3f(shaderToUse->GetUniformVariable(ShaderVariable::eyePosition), renderer.camera.position.x,
		renderer.camera.position.y, renderer.camera.position.z);

	// Set uniform variables
	glUniformMatrix4fv(shaderToUse->GetUniformVariable(ShaderVariable::model), 1, GL_FALSE, glm::value_ptr(model));

	// Use Texture
	if (texture != nullptr)
		texture->UseTexture();

	if (mask != nullptr)
	{
		glUniform1i(glGetUniformLocation(shaderToUse->GetID(), "theTexture"), 0);
		glUniform1i(glGetUniformLocation(shaderToUse->GetID(), "maskTexture"), 1);
		mask->UseTexture(GL_TEXTURE1);
	}

	// Use Material
	if (material != nullptr)
	{
		material->UseMaterial(shaderToUse->GetUniformVariable(ShaderVariable::specularIntensity),
			shaderToUse->GetUniformVariable(ShaderVariable::specularShine));
	}

	// Render Mesh
	mesh->RenderMesh(renderer.instanceAmount);
}

bool Sprite::CanBatch() const
{
	// Can batch if: simple quad mesh, has texture, no mask, not hovered, camera-relative
	return mesh == meshQuad && texture != nullptr && mask == nullptr && !isHovered
		&& !keepPositionRelativeToCamera && !keepScaleRelativeToCamera;
}

void Sprite::RenderBatched(const glm::vec3& position, Renderer& renderer, const glm::vec2& scale, const glm::vec3& rotation)
{
	if (!CanBatch())
	{
		// Fall back to regular rendering
		Render(position, renderer, scale, rotation);
		return;
	}

	ShaderProgram* shaderToUse = (shader == nullptr) ? renderer.shaders[0] : shader;

	// Check if we need to start a new batch
	if (texture != renderer.currentBatchTexture || shaderToUse != renderer.currentBatchShader)
	{
		renderer.BeginBatch(texture, shaderToUse);
	}

	// Calculate model matrix
	CalculateModel(position, rotation, glm::vec3(scale.x, scale.y, 1.0f), renderer);

	// Calculate texture frame
	const float height = (texture != nullptr) ? texture->GetHeight() : frameHeight;
	const glm::vec2 texOffset = useCustomTexOffset ? customTexOffset : CalculateRenderFrame(renderer, 0);
	const glm::vec2 texFrame = useCustomTexFrame ? customTexFrame : glm::vec2((1.0f / framesPerRow), frameHeight / height);

	// Add to batch
	renderer.AddToBatch(model, texOffset, texFrame, color);
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