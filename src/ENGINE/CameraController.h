#ifndef CAMERACONTROLLER_H
#define CAMERACONTROLLER_H
#pragma once

#include <glm/glm.hpp>
#include <SDL2/SDL.h>
#include "leak_check.h"

class Camera;

// Reusable 3D camera behaviors. Each controller drives an existing Camera's
// position/pitch/yaw when its Update is called; call at most one controller
// per frame depending on the game's camera mode.
//
// Axis reminder: the 3D projection's Y flip means "visually up" is world -Y,
// and engine pitch is positive looking down. All controllers encapsulate this.

// Free-flight camera: dt-scaled camera-relative movement (forward/back,
// strafe, vertical), optional mouse look, rotation keys, and mouse-wheel
// dolly along the view direction. Polls the keyboard itself; the game passes
// mouse deltas (poll SDL_GetRelativeMouseState once per frame, every frame,
// so the first look after a mode switch doesn't jump) and wheel state.
class KINJO_API FlyCameraController
{
public:
	float moveSpeed = 5.0f;           // world units per frame at 60fps (dt-scaled)
	float mouseSensitivity = 0.15f;   // degrees per mouse count
	float rotateKeySpeed = 1.0f;      // degrees per frame for the rotation keys
	float wheelDollyMultiplier = 10.0f;  // dolly step = moveSpeed * this
	float pitchLimit = 89.0f;         // exactly 90 degenerates lookAt's up vector

	// Bindings (SDL scancodes); set to SDL_SCANCODE_UNKNOWN to disable a key
	int keyForward = SDL_SCANCODE_W;
	int keyBack = SDL_SCANCODE_S;
	int keyLeft = SDL_SCANCODE_A;
	int keyRight = SDL_SCANCODE_D;
	int keyUp = SDL_SCANCODE_E;       // visual up (world -Y)
	int keyDown = SDL_SCANCODE_Q;
	int keyPitchUp = SDL_SCANCODE_I;
	int keyPitchDown = SDL_SCANCODE_K;
	int keyYawLeft = SDL_SCANCODE_J;
	int keyYawRight = SDL_SCANCODE_L;

	// mouseLookHeld: whether the game's mouse-look condition is active
	// (typically right button held); deltas are ignored otherwise
	void Update(Camera& cam, float dtMs, int mouseDX = 0, int mouseDY = 0,
		bool mouseLookHeld = false, bool wheelUp = false, bool wheelDown = false);
};

// Follow camera: hovers directly above a target looking straight down with a
// FIXED compass heading - the camera only translates as the target moves, the
// view never rotates (fast-orbiting targets stay watchable). Switching
// targets glides through a smoothstep transition with shortest-path yaw.
class KINJO_API FollowCameraController
{
public:
	float zoom = 6.0f;                // follow distance in target radii
	float minZoom = 3.0f;
	float maxZoom = 60.0f;
	float minDistance = 30.0f;        // absolute floor in world units
	float pitch = -89.0f;             // top-down (visually above = world -Y)
	float yaw = 90.0f;                // the fixed heading
	float transitionSeconds = 0.9f;
	float zoomInFactor = 0.8f;        // per ZoomIn/ZoomOut step
	float zoomOutFactor = 1.25f;

	// Alternative framing: place the camera at a fixed world-space offset
	// from the target instead of the zoom/radius model above. Lets 2.5D
	// games keep their exact FollowTarget framing while gaining the glide.
	bool useWorldOffset = false;
	glm::vec3 worldOffset = glm::vec3(0, 0, 0);

	// Capture the camera's current pose as the start of a glide (call when
	// entering follow mode or switching targets)
	void StartTransition(const Camera& cam);

	void ZoomIn();
	void ZoomOut();

	void Update(Camera& cam, const glm::vec3& targetPos, float targetRadius, float dtMs);

private:
	float blend = 1.0f;               // 1.0 = locked on target
	glm::vec3 blendFromPos = glm::vec3(0, 0, 0);
	float blendFromPitch = 0.0f;
	float blendFromYaw = 0.0f;
};

// Chase camera: sits behind and visually above a moving target, aimed at it.
// The heading overload assumes the target travels in the X-Z plane
// (heading 0 = +X, degrees toward +Z); use the direction overload otherwise.
class KINJO_API ChaseCameraController
{
public:
	float distanceBehind = 30.0f;
	float heightAbove = 14.0f;        // visual up (world -Y)

	void Update(Camera& cam, const glm::vec3& targetPos, float headingDeg);
	void Update(Camera& cam, const glm::vec3& targetPos, const glm::vec3& forwardDir);
};

#endif
