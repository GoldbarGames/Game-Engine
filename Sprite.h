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
	void SetShader(Shader* s) { shader = s; }
	Shader* GetShader() { return shader; }

	void SetTexture(Texture* t) { texture = t; }

	void AnimateMesh(GLfloat time);

	GLuint currentFrame = 0;

	GLuint animFrames = 1;
	Shader* shader;
	Mesh* mesh;
	Texture* texture;
	GLfloat animLow = 0;
	GLfloat animHigh = 0;
	GLfloat lastAnimTime = -1;
	//unsigned int * quadIndices;
	//GLfloat* quadVertices;
	float animSpeed = 0.1f;

	int frameWidth = 0;
	int frameHeight = 0;

	bool shouldLoop = true;
	int startFrame = 0;
	int endFrame = 0;
	int numberFrames = 1;
	Vector2 pivot = Vector2(0, 0);
	std::string filename = "";
	SDL_Rect windowRect;
	const SDL_Rect* GetRect();
	void Animate(int msPerFrame, Uint32 time);
	void Render(Vector2 position, Renderer* renderer, GLuint uniformModel);
	void Render(Vector2 position, int speed, Uint32 time, SDL_RendererFlip flip, Renderer* renderer, GLuint uniformModel, float angle = 0);
	
	void CreateMesh();
	Sprite(Texture* t, Shader* s);
	Sprite(Vector2 frame, Texture* image, Shader * shader);
	Sprite(int numFrames, SpriteManager* manager, std::string filepath, Shader* shader, Vector2 newPivot);
	Sprite(int start, int end, int numFrames, SpriteManager* manager, std::string filepath, Shader* s, Vector2 newPivot, bool loop = true);
	~Sprite();
};

