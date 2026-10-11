#ifndef AXIS_ROTATION_H
#define AXIS_ROTATION_H
#pragma once

#include <cmath>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

// Model matrices built straight from their numbers (since 2026-10-10).
//
// glm::rotate takes any axis, normalises it and builds a general axis-angle
// matrix; glm::translate and glm::scale go through vector operators. In an
// unoptimized (Debug) build every one of those is a chain of calls, and the
// engine makes a model matrix for every model in every pass of every frame:
// profiled, glm::rotate alone was 13% of TrainRails' Debug frame. These give
// the same matrices with plain arithmetic.

// The degrees-to-radians factor, as glm::radians uses.
constexpr float kDegToRad = 0.01745329251994329576923690768489f;

// m = glm::rotate(m, radians, axis) for a coordinate axis (`axis` 0 x, 1 y,
// 2 z; `sign` -1 for the negative axis). A zero angle leaves m exactly as
// glm::rotate would: unchanged.
inline void RotateAboutAxis(glm::mat4& m, float radians, int axis, float sign = 1.0f)
{
	if (radians == 0.0f)
		return;
	const float c = std::cos(radians);
	const float s = sign * std::sin(radians);
	// The two columns the turn mixes: about x, y and z; about y, z and x; about z, x and y.
	const int a = (axis == 0) ? 1 : (axis == 1) ? 2 : 0;
	const int b = (axis == 0) ? 2 : (axis == 1) ? 0 : 1;
	float* p = &m[0][0];
	for (int r = 0; r < 4; r++)
	{
		const float va = p[a * 4 + r];
		const float vb = p[b * 4 + r];
		p[a * 4 + r] = va * c + vb * s;
		p[b * 4 + r] = vb * c - va * s;
	}
}

// glm::translate(glm::mat4(1.0f), t).
inline glm::mat4 TranslationMatrix(const glm::vec3& t)
{
	glm::mat4 m(1.0f);
	float* p = &m[0][0];
	p[12] = t.x;
	p[13] = t.y;
	p[14] = t.z;
	return m;
}

// m = glm::scale(m, s).
inline void ScaleColumns(glm::mat4& m, const glm::vec3& s)
{
	float* p = &m[0][0];
	for (int r = 0; r < 4; r++)
	{
		p[r] *= s.x;
		p[4 + r] *= s.y;
		p[8 + r] *= s.z;
	}
}

#endif
