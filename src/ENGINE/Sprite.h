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
	static Mesh* meshCubeTile;
	static Mesh* meshSphere;

	ShaderProgram* shader = nullptr;
	Texture* texture = nullptr;
	Material* material = nullptr;

	Texture* mask = nullptr;

	Color color { 255, 255, 255, 255 };
	static std::string selectedColor;

	// Render fullbright, ignoring scene lighting (skyboxes, glows, suns).
	// Drives the "emissive" uniform in shaders that declare it.
	bool unlit = false;

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

	// 3D world-space render: scale is in world units (a unit-radius sphere
	// mesh scaled by (r, r, r) has world radius r). Converts internally to
	// the 2D path's texture-relative scale convention, so 3D entities don't
	// need the "-radius / textureWidth" gymnastics.
	void RenderWorld(const glm::vec3& position, const glm::vec3& worldScale,
		const glm::vec3& rotation, const Renderer& renderer);

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

	// --- rolling a body about its own length ---------------------------------
	//
	// The rotation triple to pass to RenderWorld/Render for a body that is
	// heading one way, pitched nose-up or nose-down, AND rolled about its own
	// longitudinal (local X) axis.
	//
	// NOTE FIRST, because it is the thing worth knowing: the engine has TWO
	// rotation conventions, and this is the other one.
	//
	//   Scene3DModel::ModelMatrix builds T * Ryaw * Rpitch * Rroll * S, with
	//   roll INNERMOST. That is a proper body rotation and it has worked at any
	//   heading all along. If you are placing a 3-D model, use that.
	//
	//   CalculateModel, below, builds T * Rx * Ry * Rz * S with X OUTERMOST.
	//   That is not an oversight either: Billboard::FaceCamera derives its
	//   angles by inverting exactly this composition
	//   (dir = -sin ry, cos ry sin rx, cos ry cos rx), so the order is load
	//   bearing for every camera-facing sprite in every project.
	//
	// This function exists for things drawn through the SPRITE path that need a
	// body roll anyway - a mesh sprite standing in for a 3-D object. It does not
	// replace the Scene3D convention and should not be used where that one is
	// available.
	//
	// Roll is the one thing this path's rotation parameter cannot be asked for
	// directly. CalculateModel builds
	//
	//     model = T * Rx * Ry * Rz * S
	//
	// with each rotation about the NEGATIVE axis, so `rotation.z` is applied
	// first and pitches a body about its own lateral axis, `rotation.y` then
	// swings it onto its heading - and `rotation.x` is applied LAST, about
	// WORLD X, after the heading. That is not a body axis: it behaves like roll
	// only when the heading happens to be zero.
	//
	// Rather than change that order - which would break Billboard, and silently
	// redefine `rotation.x` for anything else passing one - this inverts it.
	// XYZ Euler angles are COMPLETE: any rotation can be written in that form,
	// so build the rotation actually wanted and solve for the triple that
	// produces it. The renderer is asked for exactly what it could always do.
	//
	// Two things to know if you touch this:
	//
	//   * with zero roll it returns (0, heading, pitch) EXACTLY, which is the
	//     triple callers have always passed. Switching it on cannot move
	//     anything that was already right, and that property is worth keeping;
	//   * verify it against a rebuilt matrix, not by eye. A roll that is
	//     correct at one heading and wrong at another reads as a modelling
	//     mistake and sends you looking in entirely the wrong place.
	//
	// Positive roll tips the body to its RIGHT.
	//
	// Proved out in TrainRails first (a train leaning into a canted curve, and
	// a derailed vehicle lying over in the ditch) before being brought here.
	static glm::vec3 BodyRotation(float headingDegrees, float pitchDegrees,
		float rollDegrees);

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