#pragma once
#include <string>

#include "SDL.h"
#include <SDL_image.h>
#include <GL/glew.h>

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
	//SDL_Rect textureRect;
	
	//Vector2 scale = Vector2(1, 1);
	
public:	
	void SetShader(ShaderProgram* s) { shader = s; }
	ShaderProgram* GetShader() { return shader; }

	void SetTexture(Texture* t) { texture = t; }

	void AnimateMesh(float time);

	unsigned int currentFrame = 0;

	unsigned int animFrames = 1;

	ShaderProgram* shader;
	Mesh* mesh;
	Texture* texture;

	float animLow = 0;
	float animHigh = 0;
	float lastAnimTime = -1;
	//unsigned int * quadIndices;
	//GLfloat* quadVertices;
	float animSpeed = 0.1f;

	int frameWidth = 0;
	int frameHeight = 0;

	Vector2 scale = Vector2(1, 1);

	bool shouldLoop = true;
	int startFrame = 0;
	int endFrame = 0;
	int numberFrames = 1;
	Vector2 pivot = Vector2(0, 0);
	std::string filename = "";
	SDL_Rect windowRect;
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
	Sprite(int start, int end, int numFrames, SpriteManager* manager, std::string filepath, ShaderProgram* s, Vector2 newPivot, bool loop = true);
	~Sprite();
};

