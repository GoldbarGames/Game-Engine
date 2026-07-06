#include "CameraController.h"
#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

void FlyCameraController::Update(Camera& cam, float dtMs, int mouseDX, int mouseDY,
	bool mouseLookHeld, bool wheelUp, bool wheelDown)
{
	float dtSec = dtMs / 1000.0f;
	if (dtSec > 0.1f) dtSec = 0.1f;
	float speed = moveSpeed * 60.0f * dtSec;  // moveSpeed is per-frame at 60fps

	const Uint8* keys = SDL_GetKeyboardState(NULL);

	// The engine looks along -front where
	// front = (cos(yaw)cos(pitch), sin(pitch), sin(yaw)cos(pitch))
	float yawRad = glm::radians(cam.yaw);
	float pitchRad = glm::radians(cam.pitch);
	glm::vec3 forward(-cosf(yawRad) * cosf(pitchRad),
		-sinf(pitchRad),
		-sinf(yawRad) * cosf(pitchRad));
	glm::vec3 rightDir = glm::normalize(glm::cross(forward, glm::vec3(0, 1, 0)));

	if (keys[keyForward]) cam.position += forward * speed;
	if (keys[keyBack]) cam.position -= forward * speed;
	if (keys[keyRight]) cam.position += rightDir * speed;
	if (keys[keyLeft]) cam.position -= rightDir * speed;
	if (keys[keyUp]) cam.position.y -= speed;    // visual up = world -Y
	if (keys[keyDown]) cam.position.y += speed;

	if (mouseLookHeld)
	{
		cam.yaw += mouseDX * mouseSensitivity;
		cam.pitch += mouseDY * mouseSensitivity;  // engine pitch: positive looks down
	}

	if (keys[keyPitchUp]) cam.pitch += rotateKeySpeed;
	if (keys[keyPitchDown]) cam.pitch -= rotateKeySpeed;
	if (keys[keyYawLeft]) cam.yaw -= rotateKeySpeed;
	if (keys[keyYawRight]) cam.yaw += rotateKeySpeed;

	if (cam.pitch > pitchLimit) cam.pitch = pitchLimit;
	if (cam.pitch < -pitchLimit) cam.pitch = -pitchLimit;

	// Mouse wheel dollies along the (possibly just-rotated) view direction
	if (wheelUp || wheelDown)
	{
		yawRad = glm::radians(cam.yaw);
		pitchRad = glm::radians(cam.pitch);
		glm::vec3 viewDir(-cosf(yawRad) * cosf(pitchRad),
			-sinf(pitchRad),
			-sinf(yawRad) * cosf(pitchRad));
		cam.position += viewDir * (moveSpeed * (wheelUp ? wheelDollyMultiplier : -wheelDollyMultiplier));
	}

	cam.Update();
}

void FollowCameraController::StartTransition(const Camera& cam)
{
	blendFromPos = cam.position;
	blendFromPitch = cam.pitch;
	blendFromYaw = fmodf(cam.yaw, 360.0f);
	blend = 0.0f;
}

void FollowCameraController::ZoomIn()
{
	zoom *= zoomInFactor;
	if (zoom < minZoom) zoom = minZoom;
}

void FollowCameraController::ZoomOut()
{
	zoom *= zoomOutFactor;
	if (zoom > maxZoom) zoom = maxZoom;
}

void FollowCameraController::Update(Camera& cam, const glm::vec3& targetPos,
	float targetRadius, float dtMs)
{
	float dist = targetRadius * zoom;
	if (dist < minDistance) dist = minDistance;

	// Visually above = negative world Y (the 3D projection's Y flip)
	glm::vec3 desiredPos = targetPos + glm::vec3(0, -dist, 0);

	if (blend < 1.0f)
	{
		// Glide from the captured pose to the live target pose
		float dtSec = dtMs / 1000.0f;
		if (dtSec > 0.1f) dtSec = 0.1f;
		blend += dtSec / transitionSeconds;
		if (blend > 1.0f) blend = 1.0f;
		float s = blend * blend * (3.0f - 2.0f * blend);  // smoothstep

		// Shortest-path yaw so the view never whips the long way around
		float yawDelta = fmodf(yaw - blendFromYaw + 540.0f, 360.0f) - 180.0f;

		cam.position = glm::mix(blendFromPos, desiredPos, s);
		cam.pitch = blendFromPitch + (pitch - blendFromPitch) * s;
		cam.yaw = blendFromYaw + yawDelta * s;
	}
	else
	{
		// Hard lock: no steady-state lag behind a moving target
		cam.position = desiredPos;
		cam.pitch = pitch;
		cam.yaw = yaw;
	}
	cam.Update();
}

void ChaseCameraController::Update(Camera& cam, const glm::vec3& targetPos, float headingDeg)
{
	float h = glm::radians(headingDeg);
	Update(cam, targetPos, glm::vec3(cosf(h), 0, sinf(h)));
}

void ChaseCameraController::Update(Camera& cam, const glm::vec3& targetPos,
	const glm::vec3& forwardDir)
{
	// Behind the target opposite its travel direction, visually above (-Y)
	cam.position = targetPos - forwardDir * distanceBehind + glm::vec3(0, -heightAbove, 0);

	// Aim at the target: engine looks along -front
	glm::vec3 front = glm::normalize(cam.position - targetPos);
	cam.pitch = glm::degrees(asinf(front.y));
	cam.yaw = glm::degrees(atan2f(front.z, front.x));
	cam.Update();
}
