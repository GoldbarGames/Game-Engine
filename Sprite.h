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

	ShaderProgram* shader;	
	Texture* texture;

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

	void Render(const Vector2& position, const Renderer& renderer);
	void Render(const Vector2& position, int speed, const Renderer& renderer, glm::vec3 rotation);

	void SetScale(Vector2 s);
	bool ShouldAnimate(float time);
	void CreateMesh(MeshType meshType = MeshType::Quad);
	
	Sprite(ShaderProgram* s);
	Sprite(Texture* t, ShaderProgram* s);
	Sprite(const Vector2& frame, Texture* image, ShaderProgram* shader);
	Sprite(int numFrames, SpriteManager* manager, const std::string& filepath, ShaderProgram* shader, Vector2 newPivot);

	glm::vec2 CalculateRenderFrame(const Renderer& renderer, float animSpeed);
	void CalculateModel(Vector2 position, glm::vec3 rotation, const Renderer& renderer);

	//TODO: What should we do here?
	// start = first frame of animation
	// end = last frame of animation
	// numFrames = the number of frames in the whole sheet, regardless of the animation
	// so the total number is used to derive the width and height of a single frame
	Sprite(int start, int end, int numFrames, SpriteManager* manager, const std::string& filepath, ShaderProgram* s, const Vector2& newPivot, bool loop = true);
	Sprite(int start, int end, int width, int height, SpriteManager* manager, const std::string& filepath, ShaderProgram* s, const Vector2& newPivot, bool loop = true);
	~Sprite();
};

// For the sprite to know what to draw
struct AnimState {
	std::string name = "";
	int speed = 100;
	Sprite* sprite = nullptr;

	AnimState() { }

	AnimState(std::string n, int s, Sprite* p) : name(n), speed(s), sprite(p) { }
};

struct AnimCondition
{
	std::string nextState;
	std::string variable;         // name
	std::string check;            // ==
	bool expectedValue;           // false

	AnimCondition(const std::string n, const std::string& v, const std::string& c, bool e)
		: nextState(n), variable(v), check(c), expectedValue(e) { }
};

class AnimStateMachine
{
public:
	// state we are going to, conditions to get there
	std::unordered_map <std::string, std::vector<AnimCondition*>> conditions;
	//TODO: Currently because this is a map, we can't OR together any conditions
	// (which we could do by having them on a separate line)
	// so we need to either use a vector, or a different way
};

// For the animator parser to know how the states interact with each other
class AnimatorInfo
{
public:
	//TODO: Store a way to represent the conditions for moving between states
	std::unordered_map<std::string, AnimStateMachine*> states;
	std::unordered_map<std::string, unsigned int> mapStateNamesToNumbers;
	std::unordered_map<std::string, unsigned int> mapKeysBool;
	std::unordered_map<std::string, unsigned int> mapKeysFloat;
	std::unordered_map<std::string, unsigned int> mapKeysInt;

	AnimatorInfo(std::string filePath);
};

#endif