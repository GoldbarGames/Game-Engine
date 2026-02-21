#include "leak_check.h"
#include "Renderer.h"
#include "Sprite.h"
#include "Game.h"
#include "Texture.h"
#include "Mesh.h"
#include <algorithm>
#include <filesystem>
#include <glm/gtc/type_ptr.hpp>

ShaderProgram* Renderer::textShader;
ShaderProgram* Renderer::tileShader;

ShaderProgram* Renderer::GetTextShader()
{
	return textShader;
}

ShaderProgram* Renderer::GetShader(int key) const
{
	if (shaders.count(key) != 0)
	{
		return shaders[key];
	}

	game->logger.Log("ERROR: Shader not in renderer list: " + std::to_string(key));

	return shaders[1];
}

Renderer::Renderer()
{
	layersVisible[DrawingLayer::BACK] = true;
	layersVisible[DrawingLayer::MIDDLE] = true;
	layersVisible[DrawingLayer::OBJECT] = true;
	layersVisible[DrawingLayer::COLLISION] = true;
	layersVisible[DrawingLayer::COLLISION2] = true;
	layersVisible[DrawingLayer::FRONT] = true;
	layersVisible[DrawingLayer::BG] = true;
	layersVisible[DrawingLayer::INVISIBLE] = false;

	timerOverlayColor.Start(1);
	reloadTimer.Start(1000);
}

void Renderer::Init(Game* g)
{
	game = g;

	// TODO: Why do all these values get overwritten?
	guiCamera.maxZoom = 1000;
	guiCamera.minZoom = 0.0001f;
	guiCamera.isGUI = true;
}

void Renderer::SetDepthTestEnabled(bool enabled) const
{
	if (enabled)
	{
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);
	}
	else
	{
		glDisable(GL_DEPTH_TEST);
	}
}

void Renderer::SetDepthBias(float factor, float units) const
{
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(factor, units);
}

void Renderer::ClearDepthBias() const
{
	glDisable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(0, 0);
}

void Renderer::HotReload()
{
#ifndef EMSCRIPTEN
	if (reloadTimer.HasElapsed())
	{
		reloadTimer.Reset();

		std::string vertexFile = "";
		std::string fragmentFile = "";

		const fs::path dir = std::filesystem::current_path();

		static std::vector<int> missingShaders;

		for (size_t i = 0; i < shaderList.size(); i++)
		{
			size_t index = 0;
			ParseWord(shaderList[i], ' ', index);
			vertexFile = shaderFolder + ParseWord(shaderList[i], ' ', index);
			fragmentFile = shaderFolder + ParseWord(shaderList[i], ' ', index);

			fs::path path1 = dir / vertexFile;
			fs::path path2 = dir / fragmentFile;
			fs::directory_entry entry1 { path1 };
			fs::directory_entry entry2 { path2 };

			try
			{
				if (lastModified.count(vertexFile) == 0
					|| lastModified.count(fragmentFile) == 0
					|| entry1.last_write_time() != lastModified[vertexFile]
					|| entry2.last_write_time() != lastModified[fragmentFile])
				{
					lastModified[vertexFile] = entry1.last_write_time();
					lastModified[fragmentFile] = entry2.last_write_time();

					std::cout << "reloading shader " << shaderList[i] << std::endl;

					shaders[i + 1]->ClearShader();
					shaders[i + 1]->CreateFromFiles(vertexFile.c_str(), fragmentFile.c_str());
				}
			}
			catch (std::exception ex)
			{
				bool errorShown = false;
				for (size_t k = 0; k < missingShaders.size(); k++)
				{
					if (missingShaders[k] == i)
					{
						errorShown = true;
					}
				}

				if (!errorShown)
				{
					missingShaders.emplace_back(i);
					std::cout << ex.what() << shaderList[i] << std::endl;
				}

			}



		}
	}
#endif
}

void Renderer::CreateShaders()
{
	shaderList = ReadStringsFromFile("data/config/shaders.dat");
	shaderFolder = "data/shaders/";

	std::string vertexFile = "";
	std::string fragmentFile = "";

#ifdef EMSCRIPTEN
	shaderFolder += "webgl/";
#endif

	const std::string defaultVert =
	"#version 300 es\n"
	"precision mediump float;\n"
	"layout (location = 0) in vec3 pos;\n"
	"layout (location = 1) in vec2 tex;\n"
	"out vec4 vertexColor;\n"
	"out vec2 TexCoord;\n"
	"out vec2 MaskCoord;\n"
	"uniform mat4 model;\n"
	"uniform mat4 projection;\n"
	"uniform mat4 view;\n"
	"uniform vec2 texFrame;\n"
	"uniform vec2 texOffset;\n"
	"void main()\n"
	"{\n"
	"    gl_Position = projection * view * model * vec4(pos, 1.0);\n"
	"    vertexColor = vec4(clamp(pos, 0.0, 1.0), 1.0);\n"
	"    TexCoord = texOffset + (texFrame * tex);\n"
	"    vec2 frame = vec2(1.0, 1.0);\n"
	"    MaskCoord = frame * tex;\n"
	"}";


	const std::string defaultFrag =
	"#version 300 es\n"
	"precision mediump float;\n"
	"in vec2 TexCoord;\n"
	"out vec4 color;\n"
	"uniform sampler2D theTexture;\n"
	"uniform vec4 spriteColor;\n"
	"void main()\n"
	"{\n"
	"    vec4 newColor = texture(theTexture, TexCoord) * spriteColor;\n"
	"    color = vec4(newColor.r, newColor.g, newColor.b, newColor.a);\n"
	"}";

	CreateShader(0, defaultVert.c_str(), defaultFrag.c_str(), true);

	for (size_t i = 0; i < shaderList.size(); i++)
	{
		size_t index = 0;
		std::cout << "Parsing shader: " << shaderList[i] << std::endl;
		ParseWord(shaderList[i], ' ', index);
		vertexFile = shaderFolder + ParseWord(shaderList[i], ' ', index);
		fragmentFile = shaderFolder + ParseWord(shaderList[i], ' ', index);
		CreateShader(i + 1, vertexFile.c_str(), fragmentFile.c_str());
	}

	tileShader = shaders[1]; // default
	textShader = shaders[2]; // gui
}

Renderer::~Renderer()
{
	for (auto& [key, val] : shaders)
	{
		if (val != nullptr)
			delete_it(val);
	}

	if (debugSprite != nullptr)
		delete_it(debugSprite);

	if (overlaySprite != nullptr)
		delete_it(overlaySprite);

	if (batchMesh != nullptr)
		delete_it(batchMesh);

	if (instancedShader != nullptr)
		delete_it(instancedShader);

	if (instanceVBO != 0)
		glDeleteBuffers(1, &instanceVBO);
}

void Renderer::InitBatchRendering()
{
	// Create shared quad mesh for batching
	unsigned int quadIndices[] = {
		0, 3, 1,
		1, 3, 2,
		2, 3, 0,
		0, 1, 2
	};

	GLfloat quadVertices[] = {
		// pos              // uv
		-1.0f, -1.0f, 0.0f,  1.0f, 0.0f,
		1.0f, -1.0f, 0.0f,   0.0f, 0.0f,
		-1.0f, 1.0f, 0.0f,   1.0f, 1.0f,
		1.0f, 1.0f, 0.0f,    0.0f, 1.0f
	};

	batchMesh = new Mesh();
	batchMesh->CreateMesh(quadVertices, quadIndices, 20, 12, 5, 3, 0);

	// Create instance VBO for model matrices, tex data, and colors
	glGenBuffers(1, &instanceVBO);
	glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
	// Allocate space for matrices (mat4) + texData (vec4) + colors (vec4) per instance
	size_t instanceDataSize = (sizeof(glm::mat4) + sizeof(glm::vec4) + sizeof(glm::vec4)) * MAX_BATCH_SIZE;
	glBufferData(GL_ARRAY_BUFFER, instanceDataSize, nullptr, GL_DYNAMIC_DRAW);

	// Set up instance attributes on the batch mesh VAO
	glBindVertexArray(batchMesh->GetVAO());
	glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);  // Re-bind to ensure VBO is associated with attributes

	// Model matrix takes 4 attribute slots (3, 4, 5, 6)
	size_t matrixOffset = 0;
	for (int i = 0; i < 4; i++)
	{
		glEnableVertexAttribArray(3 + i);
		glVertexAttribPointer(3 + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(matrixOffset + i * sizeof(glm::vec4)));
		glVertexAttribDivisor(3 + i, 1);
	}

	// Tex data (offset + frame) at attribute 7
	size_t texDataOffset = sizeof(glm::mat4) * MAX_BATCH_SIZE;
	glEnableVertexAttribArray(7);
	glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), (void*)texDataOffset);
	glVertexAttribDivisor(7, 1);

	// Color at attribute 8
	size_t colorOffset = texDataOffset + sizeof(glm::vec4) * MAX_BATCH_SIZE;
	glEnableVertexAttribArray(8);
	glVertexAttribPointer(8, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), (void*)colorOffset);
	glVertexAttribDivisor(8, 1);

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// Reserve space for batch data
	batchMatrices.reserve(MAX_BATCH_SIZE);
	batchTexData.reserve(MAX_BATCH_SIZE);
	batchColors.reserve(MAX_BATCH_SIZE);

	// Create the instanced shader
	std::string instancedVert = shaderFolder + "instanced.vert";
	std::string instancedFrag = shaderFolder + "instanced.frag";
	instancedShader = new ShaderProgram(-1, instancedVert.c_str(), instancedFrag.c_str());

	// Check config file for batching setting
	bool batchingConfigEnabled = true;  // Default to enabled
	auto rendererConfig = GetMapStringsFromFile("data/config/renderer.dat");
	if (rendererConfig.count("batchRendering") > 0)
	{
		batchingConfigEnabled = (rendererConfig["batchRendering"] == "1");
	}

	// Enable batching only if config allows AND everything was created successfully
	if (batchingConfigEnabled && batchMesh != nullptr && instanceVBO != 0 && instancedShader != nullptr)
	{
		batchingEnabled = true;
		std::cout << "Batch rendering enabled" << std::endl;
	}
	else
	{
		batchingEnabled = false;
		if (!batchingConfigEnabled)
		{
			std::cout << "Batch rendering disabled by config" << std::endl;
		}
		else
		{
			std::cout << "Batch rendering initialization failed" << std::endl;
		}
	}
}

void Renderer::BeginBatch(Texture* texture, ShaderProgram* shader) const
{
	// Skip if batching not initialized
	if (!batchingEnabled || batchMesh == nullptr)
		return;

	if (currentBatchTexture != nullptr || currentBatchShader != nullptr)
	{
		FlushBatch();
	}
	currentBatchTexture = texture;
	currentBatchShader = shader;
}

void Renderer::AddToBatch(const glm::mat4& model, const glm::vec2& texOffset, const glm::vec2& texFrame, const Color& color) const
{
	// Skip if batching not initialized
	if (!batchingEnabled || batchMesh == nullptr)
		return;

	if (batchMatrices.size() >= MAX_BATCH_SIZE)
	{
		FlushBatch();
	}

	batchMatrices.push_back(model);
	batchTexData.push_back(glm::vec4(texOffset.x, texOffset.y, texFrame.x, texFrame.y));
	batchColors.push_back(glm::vec4(color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f));
}

void Renderer::FlushBatch() const
{
	if (!batchingEnabled || batchMesh == nullptr || batchMatrices.empty() || instancedShader == nullptr || instanceVBO == 0)
	{
		batchMatrices.clear();
		batchTexData.clear();
		batchColors.clear();
		return;
	}

	instancedShader->UseShader();

	// Set up uniforms that exist in the instanced shader
	glUniformMatrix4fv(instancedShader->GetUniformVariable(ShaderVariable::view), 1, GL_FALSE,
		glm::value_ptr(camera.CalculateViewMatrix()));
	glUniformMatrix4fv(instancedShader->GetUniformVariable(ShaderVariable::projection), 1, GL_FALSE,
		glm::value_ptr(camera.projection));
	glUniform1f(instancedShader->GetUniformVariable(ShaderVariable::distanceToLight2D), 1.0f);  // lightRatio

	// Bind texture
	if (currentBatchTexture != nullptr)
	{
		currentBatchTexture->UseTexture();
	}

	// Upload instance data
	glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);

	size_t matrixDataSize = batchMatrices.size() * sizeof(glm::mat4);
	size_t texDataSize = batchTexData.size() * sizeof(glm::vec4);
	size_t colorDataSize = batchColors.size() * sizeof(glm::vec4);

	// Upload matrices
	glBufferSubData(GL_ARRAY_BUFFER, 0, matrixDataSize, batchMatrices.data());

	// Upload tex data
	size_t texDataOffset = sizeof(glm::mat4) * MAX_BATCH_SIZE;
	glBufferSubData(GL_ARRAY_BUFFER, texDataOffset, texDataSize, batchTexData.data());

	// Upload colors
	size_t colorOffset = texDataOffset + sizeof(glm::vec4) * MAX_BATCH_SIZE;
	glBufferSubData(GL_ARRAY_BUFFER, colorOffset, colorDataSize, batchColors.data());

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// Draw instanced
	glBindVertexArray(batchMesh->GetVAO());
	glDrawElementsInstanced(GL_TRIANGLES, 12, GL_UNSIGNED_INT, 0, static_cast<GLsizei>(batchMatrices.size()));
	glBindVertexArray(0);

	drawCallsPerFrame++;

	// Clear batch data
	batchMatrices.clear();
	batchTexData.clear();
	batchColors.clear();
}

void Renderer::EndBatch() const
{
	FlushBatch();
	currentBatchTexture = nullptr;
	currentBatchShader = nullptr;
}

void Renderer::RenderDebugRect(const SDL_Rect& targetRect, const glm::vec2& targetScale, Color color) const
{
	RenderDebugRect(targetRect, targetScale, glm::vec2(0, 0), color);
}

void Renderer::RenderDebugRect(const SDL_Rect& targetRect, const glm::vec2& targetScale, const glm::vec2& targetPivot, Color color) const
{
	debugSprite->color = color;
	debugSprite->pivot = targetPivot;
	debugScale = glm::vec2(CalculateScale(*debugSprite, targetRect.w, targetRect.h, targetScale));
	debugSprite->Render(glm::vec3(targetRect.x, targetRect.y, 0), *this, debugScale);
}

glm::vec2 Renderer::CalculateScale(const Sprite& sourceSprite, int targetWidth, int targetHeight, const glm::vec2& targetScale) const
{
	if (sourceSprite.texture == nullptr)
		return glm::vec2(targetWidth * targetScale.x, targetHeight * targetScale.y);

	float sourceWidth = sourceSprite.texture->GetWidth();
	float sourceHeight = sourceSprite.texture->GetHeight();

	return glm::vec2(targetWidth * targetScale.x / sourceWidth,
		targetHeight * targetScale.y / sourceHeight);
}

void Renderer::LerpColor(float& color, float target, const float& speed)
{
	if (color > target)
	{
		color -= speed * game->dt;
		if (color < target)
			color = target;
	}
	else
	{
		color += speed * game->dt;
		if (color > target)
			color = target;
	}
}

void Renderer::Update()
{
	if (changingOverlayColor) // && timerOverlayColor.HasElapsed())
	{		
		//changingOverlayColor = false;

		static float colorSpeed = -1.0f;
		static float r, g, b, a = 0.0f;

		if (colorSpeed < 0.0f)
		{
			// Calculate the longest amount of time it should take to change all colors
			int maxDiff = std::abs(overlayColor.r - targetColor.r);
			maxDiff = std::max(maxDiff, std::abs(overlayColor.g - targetColor.g));
			maxDiff = std::max(maxDiff, std::abs(overlayColor.b - targetColor.b));
			maxDiff = std::max(maxDiff, std::abs(overlayColor.a - targetColor.a));

			// Calculate the speed to change colors based on the desired time
			colorSpeed = maxDiff / (float)(overlayEndTime - overlayStartTime);

			std::cout << colorSpeed << std::endl;

			r = overlayColor.r;
			g = overlayColor.g;
			b = overlayColor.b;
			a = overlayColor.a;
		}

		//std::cout << currentTime << " / " << difference << " = " << t << std::endl;

		// Only update every millisecond
		LerpColor(r, targetColor.r, colorSpeed);
		LerpColor(g, targetColor.g, colorSpeed);
		LerpColor(b, targetColor.b, colorSpeed);
		LerpColor(a, targetColor.a, colorSpeed);

		// std::cout << a << " / " << (int)targetColor.a << std::endl;

		overlayColor = { (uint8_t)r, (uint8_t)g, (uint8_t)b, (uint8_t)a };

		if (overlayColor.r == targetColor.r
			&& overlayColor.g == targetColor.g
			&& overlayColor.b == targetColor.b
			&& overlayColor.a == targetColor.a)
		{
			changingOverlayColor = false;
			colorSpeed = -1.0f;
			std::cout << colorSpeed << std::endl;
		}
	}
}

void Renderer::FadeOverlay(const int screenWidth, const int screenHeight) const
{
	// Draw the screen overlay above everything else
	float rWidth = overlaySprite->texture->GetWidth();
	float rHeight = overlaySprite->texture->GetHeight();
	overlaySprite->color = overlayColor;
	overlaySprite->pivot = glm::vec2(0, 0);
	overlayScale = glm::vec2(screenWidth / rWidth, screenHeight / rHeight);
}

void Renderer::CreateShader(const int shaderName, const char* vertexFilePath, const char* fragmentFilePath, bool fromString)
{
	if (shaders[shaderName] != nullptr)
		delete_it(shaders[shaderName]);

	shaders[shaderName] = new ShaderProgram(shaderName, vertexFilePath, fragmentFilePath, fromString);
}

bool Renderer::IsVisible(DrawingLayer layer) const
{
	return layersVisible[layer];
}

void Renderer::ToggleVisibility(DrawingLayer layer)
{
	layersVisible[layer] = !layersVisible[layer];
}

void Renderer::UseLight(const ShaderProgram& shader) const
{
	if (light != nullptr)
	{
		light->UseLight(shader);
	}

	// For point lights
	if (pointLights != nullptr)
	{
		if (pointLightCount > MAX_POINT_LIGHTS)
			pointLightCount = MAX_POINT_LIGHTS;

		glUniform1i(shader.GetUniformVariable(ShaderVariable::pointLightCount), pointLightCount);

		for (size_t i = 0; i < pointLightCount; i++)
		{
			pointLights[i]->UseLight(shader);
		}
	}

	// For spot lights
	if (spotLights != nullptr)
	{
		if (pointLightCount > MAX_SPOT_LIGHTS)
			pointLightCount = MAX_SPOT_LIGHTS;

		glUniform1i(shader.GetUniformVariable(ShaderVariable::spotLightCount), spotLightCount);

		for (size_t i = 0; i < spotLightCount; i++)
		{
			spotLights[i]->UseLight(shader);
		}
	}
}


void Renderer::ConfigureInstanceArray(unsigned int amount)
{
	instanceAmount = amount;
	modelMatrices = new glm::mat4[amount];

	unsigned int buffer;
	glGenBuffers(1, &buffer);
	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	glBufferData(GL_ARRAY_BUFFER, instanceAmount * sizeof(glm::mat4), &modelMatrices[0], GL_STATIC_DRAW);

	for (unsigned int i = 0; i < game->entities.size(); i++)
	{
		glBindVertexArray(game->entities[i]->GetSprite()->mesh->GetVAO());

		// set attribute pointers for matrix (4 times vec4)
		glEnableVertexAttribArray(3);
		glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)0);
		glEnableVertexAttribArray(4);
		glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(sizeof(glm::vec4)));
		glEnableVertexAttribArray(5);
		glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(2 * sizeof(glm::vec4)));
		glEnableVertexAttribArray(6);
		glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(3 * sizeof(glm::vec4)));

		glVertexAttribDivisor(3, 1);
		glVertexAttribDivisor(4, 1);
		glVertexAttribDivisor(5, 1);
		glVertexAttribDivisor(6, 1);

		glBindVertexArray(0);
	}


}