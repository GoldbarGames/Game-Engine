#include "leak_check.h"
#include "render/RenderDevice.h"
#include "render/ColorPipeline.h"
#include "render/Environment.h"
#include "SkyImage.h"
#include "render/AmbientOcclusion.h"
#include "render/TemporalAA.h"
#include "render/ClusteredLights.h"
#include "render/ColorGrading.h"
#include "render/DepthOfField.h"
#include "render/VolumetricFog.h"
#include "render/Reflections.h"
#include "render/Multisample.h"
#include "RenderState.h"
#include "TransientBuffer.h"
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

namespace
{
	void PointBatchInstances(unsigned int vao, unsigned int buffer, size_t matrixOffset, size_t texOffset, size_t colorOffset);
}

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

	// Initialize point and spot light arrays to nullptr
	for (int i = 0; i < MAX_POINT_LIGHTS; i++)
	{
		pointLights[i] = nullptr;
	}
	for (int i = 0; i < MAX_SPOT_LIGHTS; i++)
	{
		spotLights[i] = nullptr;
	}

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
	RenderState s = CurrentRenderState();
	s.depthTest = enabled;
	if (enabled)
		s.depthCompare = CompareOp::Less;
	ApplyRenderState(s);
}

void Renderer::SetDepthBias(float factor, float units) const
{
	RenderState s = CurrentRenderState();
	s.depthBiasFactor = factor;
	s.depthBiasUnits = units;
	ApplyRenderState(s);
}

void Renderer::ClearDepthBias() const
{
	RenderState s = CurrentRenderState();
	s.depthBiasFactor = 0.0f;
	s.depthBiasUnits = 0.0f;
	ApplyRenderState(s);
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

			// Watch the file actually loaded: the game's copy, else the engine's
			fs::path path1 = dir / ShaderProgram::ResolvePath(vertexFile);
			fs::path path2 = dir / ShaderProgram::ResolvePath(fragmentFile);
			fs::directory_entry entry1 { path1 };
			fs::directory_entry entry2 { path2 };

			try
			{
				// The first look at a file only records its time. Treating "not
				// seen yet" as "changed" recompiled every shader on the first
				// frame - the whole set twice at every start-up, two seconds of
				// DB2's (docs/STARTUP.md).
				if (lastModified.count(vertexFile) == 0 || lastModified.count(fragmentFile) == 0)
				{
					lastModified[vertexFile] = entry1.last_write_time();
					lastModified[fragmentFile] = entry2.last_write_time();
					continue;
				}
				if (entry1.last_write_time() != lastModified[vertexFile]
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

	// The fallback sprite shader (slot 0). Self-contained - it must work even if
	// no shader files are found - so the Camera block (shaders/camera.glsl) and
	// the sprite DrawData (shaders/sprite_draw.glsl) are spelled out here; keep
	// them identical to those files.
	const std::string spriteInterface =
	"layout(std140) uniform Camera { mat4 view; mat4 projection; };\n"
	"struct DrawData { mat4 model; vec2 texFrame; vec2 texOffset; vec4 spriteColor;\n"
	"                  float lightRatio; float time; float freq; float emissive; };\n"
	"uniform DrawData draw;\n";

	const std::string defaultVert =
	"#version 300 es\n"
	"precision mediump float;\n"
	"layout (location = 0) in vec3 pos;\n"
	"layout (location = 1) in vec2 tex;\n"
	"out vec4 vertexColor;\n"
	"out vec2 TexCoord;\n"
	"out vec2 MaskCoord;\n"
	+ spriteInterface +
	"void main()\n"
	"{\n"
	"    gl_Position = projection * view * draw.model * vec4(pos, 1.0);\n"
	"    vertexColor = vec4(clamp(pos, 0.0, 1.0), 1.0);\n"
	"    TexCoord = draw.texOffset + (draw.texFrame * tex);\n"
	"    vec2 frame = vec2(1.0, 1.0);\n"
	"    MaskCoord = frame * tex;\n"
	"}";


	const std::string defaultFrag =
	"#version 300 es\n"
	"precision mediump float;\n"
	"in vec2 TexCoord;\n"
	"out vec4 color;\n"
	"uniform sampler2D theTexture;\n"
	+ spriteInterface +
	"void main()\n"
	"{\n"
	"    vec4 newColor = texture(theTexture, TexCoord) * draw.spriteColor;\n"
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
	{
		BufferHandle vbo(instanceVBO);
		Device().DestroyBuffer(vbo);
		instanceVBO = 0;
	}

	ReleaseOverlayResources();
	ReleaseColorPipeline();
	ReleaseEnvironment();
	ReleaseHdrSky();
	ReleaseAmbientOcclusion();
	ReleaseTemporalAA();
	ReleaseClusteredLights();
	ReleaseColorGrading();
	ReleaseDepthOfField();
	ReleaseVolumetricFog();
	ReleaseReflections();
	ReleaseMultisample();
	ReleaseUniformBlocks();
	TransientRelease();
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

	float quadVertices[] = {
		// pos              // uv
		-1.0f, -1.0f, 0.0f,  1.0f, 0.0f,
		1.0f, -1.0f, 0.0f,   0.0f, 0.0f,
		-1.0f, 1.0f, 0.0f,   1.0f, 1.0f,
		1.0f, 1.0f, 0.0f,    0.0f, 1.0f
	};

	batchMesh = new Mesh();
	batchMesh->CreateMesh(quadVertices, quadIndices, 20, 12, 5, 3, 0);

	// Create instance VBO for model matrices, tex data, and colors
	// Allocate space for matrices (mat4) + texData (vec4) + colors (vec4) per instance
	size_t instanceDataSize = (sizeof(glm::mat4) + sizeof(glm::vec4) + sizeof(glm::vec4)) * MAX_BATCH_SIZE;
	instanceVBO = Device().CreateBuffer(instanceDataSize, nullptr, BufferUsage::Dynamic).id;

	// Instance attributes on the batch mesh VAO: model matrix in slots 3-6,
	// tex data (offset + frame) at 7, color at 8 - one block of each
	size_t texDataOffset = sizeof(glm::mat4) * MAX_BATCH_SIZE;
	size_t colorOffset = texDataOffset + sizeof(glm::vec4) * MAX_BATCH_SIZE;
	PointBatchInstances(batchMesh->GetVAO(), instanceVBO, 0, texDataOffset, colorOffset);

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
	auto rendererConfig = ReadRendererConfig();
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

namespace
{
	// Point the batch quad's instance attributes (3-6 model matrix, 7 texData,
	// 8 color) at three arrays living in `buffer`.
	void PointBatchInstances(unsigned int vao, unsigned int buffer, size_t matrixOffset, size_t texOffset, size_t colorOffset)
	{
		RenderDevice& device = Device();
		const VertexArrayHandle v(vao);
		const BufferHandle b(buffer);
		for (unsigned int i = 0; i < 4; i++)
			device.SetVertexAttribute(v, 3 + i, b, 4, sizeof(glm::mat4), matrixOffset + i * sizeof(glm::vec4), 1);
		device.SetVertexAttribute(v, 7, b, 4, sizeof(glm::vec4), texOffset, 1);
		device.SetVertexAttribute(v, 8, b, 4, sizeof(glm::vec4), colorOffset, 1);
	}

	// One batch's instance data for this frame: streamed through the transient
	// ring (no per-draw upload into a buffer the GPU may still be reading), or,
	// if the ring is unavailable, written into the renderer's own instanceVBO
	// at its fixed block offsets as before.
	void StreamBatchInstances(unsigned int vao, unsigned int instanceVBO, const glm::mat4* models,
		const glm::vec4* texData, const glm::vec4* colors, size_t count, size_t capacity)
	{
		const TransientAlloc m = TransientUpload(models, count * sizeof(glm::mat4));
		const TransientAlloc t = TransientUpload(texData, count * sizeof(glm::vec4));
		const TransientAlloc c = TransientUpload(colors, count * sizeof(glm::vec4));
		if (m.Valid() && t.Valid() && c.Valid() && m.buffer == t.buffer && t.buffer == c.buffer)
		{
			PointBatchInstances(vao, m.buffer, m.offset, t.offset, c.offset);
			return;
		}

		const size_t texOffset = sizeof(glm::mat4) * capacity;
		const size_t colorOffset = texOffset + sizeof(glm::vec4) * capacity;
		RenderDevice& device = Device();
		const BufferHandle vbo(instanceVBO);
		device.UpdateBuffer(vbo, 0, count * sizeof(glm::mat4), models);
		device.UpdateBuffer(vbo, texOffset, count * sizeof(glm::vec4), texData);
		device.UpdateBuffer(vbo, colorOffset, count * sizeof(glm::vec4), colors);
		PointBatchInstances(vao, instanceVBO, 0, texOffset, colorOffset);
	}
}

void Renderer::BeginBatch(Texture* texture, ShaderProgram* shader) const{
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

	// Camera block for the engine shader; the loose view/projection uniforms
	// are still set for game overrides that predate the block (no-ops otherwise).
	const glm::mat4 view = camera.CalculateViewMatrix();
	BindCameraBlock(view, camera.projection);
	Device().SetUniform((int)(instancedShader->GetUniformVariable(ShaderVariable::view)), view);
	Device().SetUniform((int)(instancedShader->GetUniformVariable(ShaderVariable::projection)), camera.projection);
	Device().SetUniform((int)(instancedShader->GetUniformVariable(ShaderVariable::distanceToLight2D)), (float)(1.0f));  // lightRatio

	// Bind texture
	if (currentBatchTexture != nullptr)
	{
		currentBatchTexture->UseTexture();
	}

	// Instance data (matrices | texData | colors) streamed for this draw
	StreamBatchInstances(batchMesh->GetVAO(), instanceVBO, batchMatrices.data(),
		batchTexData.data(), batchColors.data(), batchMatrices.size(), MAX_BATCH_SIZE);

	// Draw instanced
	Device().DrawIndexed(VertexArrayHandle(batchMesh->GetVAO()), Primitive::Triangles, 12, (int)batchMatrices.size());

	drawCallsPerFrame++;

	// Clear batch data
	batchMatrices.clear();
	batchTexData.clear();
	batchColors.clear();
}

bool Renderer::GlyphBatchReady() const
{
	return instancedShader != nullptr && instancedShader->GetID() != 0
		&& batchMesh != nullptr && instanceVBO != 0;
}

void Renderer::DrawGlyphBatch(Texture* atlas, const std::vector<glm::mat4>& models,
	const std::vector<glm::vec4>& texData, const std::vector<glm::vec4>& colors) const
{
	if (!GlyphBatchReady() || atlas == nullptr || models.empty())
		return;

	size_t count = models.size();
	if (count > (size_t)MAX_BATCH_SIZE) count = MAX_BATCH_SIZE;

	instancedShader->UseShader();
	// Match the per-glyph immediate path exactly (Sprite::Render): camera-relative
	// text uses the GUI camera's VIEW but the main camera's guiProjection.
	const glm::mat4 guiView = guiCamera.CalculateViewMatrix();
	BindCameraBlock(guiView, camera.guiProjection);
	Device().SetUniform((int)(instancedShader->GetUniformVariable(ShaderVariable::view)), guiView);
	Device().SetUniform((int)(instancedShader->GetUniformVariable(ShaderVariable::projection)), camera.guiProjection);
	Device().SetUniform((int)(instancedShader->GetUniformVariable(ShaderVariable::distanceToLight2D)), (float)(1.0f));

	atlas->UseTexture();

	// Same instance streams FlushBatch uses (matrices | texData | colors).
	StreamBatchInstances(batchMesh->GetVAO(), instanceVBO, models.data(),
		texData.data(), colors.data(), count, MAX_BATCH_SIZE);

	Device().DrawIndexed(VertexArrayHandle(batchMesh->GetVAO()), Primitive::Triangles, 12, (int)count);

	drawCallsPerFrame++;
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
	// Engine shaders read the point lights from the SpriteLights block.
	if (BindSpriteLightsBlock(shader))
		return;

	// Legacy: per-member loose uniforms for game shaders that predate the block.
	if (light != nullptr)
	{
		light->UseLight(shader);
	}

	// Slot filler for empty entries inside the active count: black light,
	// constant attenuation 1 (never left as all-zero uniforms — a zero
	// attenuation constant divides by zero in the shader and goes NaN/fullbright)
	static PointLight disabledPointLight(glm::vec3(0.0f), 0.0f, 0.0f, glm::vec3(0.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	static SpotLight disabledSpotLight(glm::vec3(0.0f), 0.0f, 0.0f, glm::vec3(0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), 0.0f);

	// For point lights
	if (pointLightCount > MAX_POINT_LIGHTS)
		pointLightCount = MAX_POINT_LIGHTS;

	Device().SetUniform((int)(shader.GetUniformVariable(ShaderVariable::pointLightCount)), (int)(pointLightCount));

	for (size_t i = 0; i < pointLightCount; i++)
	{
		PointLight* p = (pointLights[i] != nullptr) ? pointLights[i] : &disabledPointLight;
		p->UseLight(shader, (int)i);
	}

	// For spot lights
	if (spotLightCount > MAX_SPOT_LIGHTS)
		spotLightCount = MAX_SPOT_LIGHTS;

	Device().SetUniform((int)(shader.GetUniformVariable(ShaderVariable::spotLightCount)), (int)(spotLightCount));

	for (size_t i = 0; i < spotLightCount; i++)
	{
		SpotLight* s = (spotLights[i] != nullptr) ? spotLights[i] : &disabledSpotLight;
		s->UseLight(shader, (int)i);
	}
}


void Renderer::ConfigureInstanceArray(unsigned int amount)
{
	instanceAmount = amount;
	modelMatrices = new glm::mat4[amount];

	const BufferHandle buffer = Device().CreateBuffer(instanceAmount * sizeof(glm::mat4), &modelMatrices[0], BufferUsage::Static);

	for (unsigned int i = 0; i < game->entities.size(); i++)
	{
		// attribute pointers for the matrix (4 times vec4), one per instance
		const VertexArrayHandle vao(game->entities[i]->GetSprite()->mesh->GetVAO());
		for (unsigned int c = 0; c < 4; c++)
			Device().SetVertexAttribute(vao, 3 + c, buffer, 4, sizeof(glm::mat4), c * sizeof(glm::vec4), 1);
	}

}