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

	unsigned int currentFrame = 0;
	unsigned int currentRow = 0;
	
	static Mesh* mesh;
	ShaderProgram* shader;	
	Texture* texture;

	Color color { 255, 255, 255, 255 };

	bool renderRelativeToCamera = false;
	bool keepScaleRelativeToCamera = false;

	float lastAnimTime = -1;
	float animSpeed = 1.0f;

	int frameWidth = 0;
	int frameHeight = 0;
	Vector2 scale = Vector2(1, 1);

	bool shouldLoop = true;
	int startFrame = 0;
	int endFrame = 0;
	unsigned int numberFramesInTexture = 1;
	unsigned int framesPerRow = numberFramesInTexture;
	unsigned int numberRows = 1;

	Vector2 pivot = Vector2(0, 0);
	std::string filename = "";
	SDL_Rect windowRect;

	float angle = 0;

	const SDL_Rect* GetRect();
	void Animate(int msPerFrame, Uint32 time);
	void Render(Vector2 position, Renderer* renderer);
	void Render(Vector2 position, int speed, Uint32 time, SDL_RendererFlip flip, Renderer* renderer, float angle = 0);
	void SetScale(Vector2 s);
	bool ShouldAnimate(float time);
	void CreateMesh();
	Sprite(Texture* t, ShaderProgram* s);
	Sprite(Vector2 frame, Texture* image, ShaderProgram* shader);
	Sprite(int numFrames, SpriteManager* manager, std::string filepath, ShaderProgram* shader, Vector2 newPivot);
	
	//TODO: What should we do here?
	// start = first frame of animation
	// end = last frame of animation
	// numFrames = the number of frames in the whole sheet, regardless of the animation
	// so the total number is used to derive the width and height of a single frame
	Sprite(int start, int end, int numFrames, SpriteManager* manager, std::string filepath, ShaderProgram* s, Vector2 newPivot, bool loop = true);
	Sprite(int start, int end, int width, int height, SpriteManager* manager, std::string filepath, ShaderProgram* s, Vector2 newPivot, bool loop = true);
	~Sprite();
};

