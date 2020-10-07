#ifndef SPRITE_H
#define SPRITE_H
#pragma once

#include <string>

#include "SDL.h"
#include <SDL_image.h>
#include <GL/glew.h>

#include "globals.h"

#include "Vector2.h"
#include "SpriteManager.h"

#include "Texture.h"
#include "Shader.h"
#include "Mesh.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

class Renderer;


class Sprite
{
private:

public:	

	unsigned int Size();
	void SetShader(ShaderProgram* s) { shader = s; }
	ShaderProgram* GetShader() { return shader; }
	void SetTexture(Texture* t) { texture = t; }
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

	ShaderProgram* shader = nullptr;
	Texture* texture = nullptr;

	Color color { 255, 255, 255, 255 };
	static std::string selectedColor;

	bool keepPositionRelativeToCamera = false;
	bool keepScaleRelativeToCamera = false;

	float lastAnimTime = -1;

	int frameWidth = 0;
	int frameHeight = 0;
	Vector2 scale = Vector2(1, 1);

	Vector2 lastPosition = Vector2(0, 0);
	glm::vec3 lastRotation = glm::vec3(0, 0, 0);
	Vector2 lastScale = Vector2(0, 0);

	bool shouldLoop = true;
	int startFrame = 0;
	int endFrame = 0;
	unsigned int numberFramesInTexture = 1;
	unsigned int framesPerRow = numberFramesInTexture;
	unsigned int numberRows = 1;

	glm::mat4 model;

	// The pivot point's origin (0,0) is the center of the sprite.
	// The pivot is added or subtracted based on the direction.
	// Change this if you want the sprite's center to be offset.
	// For example, if a sprite is too far to the left by X pixels,
	// you will want to add X so that it moves to the right.
	Vector2 pivot = Vector2(0, 0);

	std::string filename = "";
	
	SDL_Rect rect; //TODO: Get rid of this?
	const SDL_Rect* GetRect();

	void Render(const Vector2& position, const Renderer& renderer, const glm::vec3& rotation=glm::vec3(0,0,0));
	void Render(const Vector2& position, int speed, const Renderer& renderer, const glm::vec3& rotation);

	void SetScale(Vector2 s);
	bool ShouldAnimate(float time);
	void CreateMesh(MeshType meshType = MeshType::Quad);
	
	Sprite(ShaderProgram* s);
	Sprite(Texture* t, ShaderProgram* s);
	Sprite(const Vector2& frame, Texture* image, ShaderProgram* shader);
	Sprite(int numFrames, const SpriteManager& manager, const std::string& filepath, ShaderProgram* shader, Vector2 newPivot);

	glm::vec2 CalculateRenderFrame(const Renderer& renderer, float animSpeed);
	void CalculateModel(Vector2 position, glm::vec3 rotation, const Renderer& renderer);

	//TODO: What should we do here?
	// start = first frame of animation
	// end = last frame of animation
	// numFrames = the number of frames in the whole sheet, regardless of the animation
	// so the total number is used to derive the width and height of a single frame
	Sprite(int start, int end, int numFrames, const SpriteManager& manager, const std::string& filepath, ShaderProgram* s, const Vector2& newPivot, bool loop = true);
	Sprite(int start, int end, int width, int height, const SpriteManager& manager, const std::string& filepath, ShaderProgram* s, const Vector2& newPivot, bool loop = true);
	~Sprite();
};



#endif