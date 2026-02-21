#ifndef SPRITE_H
#define SPRITE_H
#pragma once

#include <string>

#include <SDL2/SDL.h>

#include "globals.h"

#include "SpriteManager.h"

#include "Texture.h"
#include "Shader.h"
#include "Mesh.h"
#include "Material.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/vec2.hpp>

#include "leak_check.h"

class Renderer;


class KINJO_API Sprite
{
private:

	glm::vec3 lastPosition = glm::vec3(0, 0, 0);
	glm::vec3 lastRotation = glm::vec3(0, 0, 0);
	glm::vec3 lastScale = glm::vec3(0, 0, 0);

public:	

	unsigned int Size();
	void SetShader(ShaderProgram* s) { shader = s; }
	ShaderProgram* GetShader() { return shader; }
	void SetTexture(Texture* t);
	void AnimateMesh(float time);
	bool HasAnimationElapsed();
	void ResetFrame();

	unsigned int previousFrame = 0;
	unsigned int currentFrame = 0;
	unsigned int currentRow = 0;

	//TODO: Maybe a MeshManager?
	Mesh* mesh = nullptr;
	static Mesh* meshQuad;
	static Mesh* meshTri;
	static Mesh* meshLine;
	static Mesh* meshPyramid;
	static Mesh* meshCube;

	ShaderProgram* shader = nullptr;
	Texture* texture = nullptr;
	Material* material = nullptr;

	Texture* mask = nullptr;

	Color color { 255, 255, 255, 255 };
	static std::string selectedColor;

	bool isHovered = false;
	ShaderProgram* hoverShader = nullptr;

	bool keepPositionRelativeToCamera = false;
	bool keepScaleRelativeToCamera = false;

	float lastAnimTime = -1;

	int frameWidth = 1;
	int frameHeight = 1;

	bool playedOnce = false;

	const glm::vec3& GetLastPosition() const { return lastPosition; }
	void SetLastPosition(const glm::vec3& pos) { lastPosition = pos; }

	const glm::vec3& GetLastRotation() const { return lastRotation; }
	void SetLastRotation(const glm::vec3& pos) { lastRotation = pos; }

	const glm::vec3& GetLastScale() const { return lastScale; }
	void SetLastScale(const glm::vec3& pos) { lastScale = pos; }

	bool shouldLoop = true;
	int startFrame = 0;
	int endFrame = 0;
	unsigned int numberFramesInTexture = 1;
	unsigned int framesPerRow = numberFramesInTexture;
	unsigned int numberRows = 1;

	glm::mat4 model;

	bool useCustomTexFrame = false;
	glm::vec2 customTexFrame = glm::vec2(0, 0);

	bool useCustomTexOffset = false;
	glm::vec2 customTexOffset = glm::vec2(0, 0);

	// The pivot point's origin (0,0) is the center of the sprite.
	// The pivot is added or subtracted based on the direction.
	// Change this if you want the sprite's center to be offset.
	// For example, if a sprite is too far to the left by X pixels,
	// you will want to add X so that it moves to the right.
	glm::vec2 pivot = glm::vec2(0, 0);

	const std::string& GetFileName();

	void Render(const glm::vec3& position, const Renderer& renderer, const glm::vec2& scale, const glm::vec3& rotation=glm::vec3(0,0,0));
	void Render(const glm::vec3& position, int speed, const Renderer& renderer, const glm::vec2& scale, const glm::vec3& rotation);
	void Render(const glm::vec3& position, int speed, const Renderer& renderer, const glm::vec3& scale, const glm::vec3& rotation);

	// Check if this sprite can use batched rendering
	bool CanBatch() const;

	bool ShouldAnimate(float time);
	void CreateMesh(MeshType meshType = MeshType::Quad);
	
	Sprite();
	Sprite(ShaderProgram* s, MeshType m=MeshType::Quad);
	Sprite(Texture* t, ShaderProgram* s);
	Sprite(const glm::vec2& frame, Texture* image, ShaderProgram* shader, 
		const int tileSize, const int tileSize2 = 0, const int cf = -1);
	Sprite(int numFrames, const SpriteManager& manager, const std::string& filepath, ShaderProgram* shader, glm::vec2 newPivot);

	glm::vec2 CalculateRenderFrame(const Renderer& renderer, float animSpeed);
	void CalculateModel(glm::vec3 position, const glm::vec3& rotation, const glm::vec3& scale, const Renderer& renderer);

	//TODO: What should we do here?
	// start = first frame of animation
	// end = last frame of animation
	// numFrames = the number of frames in the whole sheet, regardless of the animation
	// so the total number is used to derive the width and height of a single frame
	Sprite(int start, int end, int numFrames, const SpriteManager& manager, const std::string& filepath, ShaderProgram* s, const glm::vec2& newPivot, bool loop = true);
	Sprite(int start, int end, int width, int height, const SpriteManager& manager, const std::string& filepath, ShaderProgram* s, const glm::vec2& newPivot, bool loop = true);
	~Sprite();
};



#endif