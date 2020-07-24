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

	unsigned int previousFrame = 0;
	unsigned int currentFrame = 0;
	unsigned int currentRow = 0;

	bool HasAnimationElapsed();
	
	static Mesh* mesh;
	ShaderProgram* shader;	
	Texture* texture;

	Color color { 255, 255, 255, 255 };
	std::string selectedColor = "clear";

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
	Vector2 pivot = Vector2(0, 0);
	std::string filename = "";
	
	SDL_Rect windowRect; //TODO: Get rid of this?
	const SDL_Rect* GetRect();

	void Render(Vector2 position, Renderer* renderer);
	void Render(Vector2 position, int speed, SDL_RendererFlip flip, Renderer* renderer, glm::vec3 rotation);
	void SetScale(Vector2 s);
	bool ShouldAnimate(float time);
	void CreateMesh();
	
	Sprite(Texture* t, ShaderProgram* s);
	Sprite(Vector2 frame, Texture* image, ShaderProgram* shader);
	Sprite(int numFrames, SpriteManager* manager, const std::string& filepath, ShaderProgram* shader, Vector2 newPivot);

	glm::vec2 CalculateRenderFrame(Renderer* renderer, float animSpeed);
	void CalculateModel(Vector2 position, glm::vec3 rotation, Renderer* renderer, SDL_RendererFlip flip);

	//TODO: What should we do here?
	// start = first frame of animation
	// end = last frame of animation
	// numFrames = the number of frames in the whole sheet, regardless of the animation
	// so the total number is used to derive the width and height of a single frame
	Sprite(int start, int end, int numFrames, SpriteManager* manager, const std::string& filepath, ShaderProgram* s, Vector2 newPivot, bool loop = true);
	Sprite(int start, int end, int width, int height, SpriteManager* manager, const std::string& filepath, ShaderProgram* s, Vector2 newPivot, bool loop = true);
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
	bool expectedValue;    // false

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

	AnimatorInfo(std::string name);
};

#endif