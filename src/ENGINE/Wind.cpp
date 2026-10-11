#include "Wind.h"
#include <glm/geometric.hpp>
#include <algorithm>
#include <cmath>

namespace
{
	// File-local, so no exported class changes layout.
	glm::vec2 toward = glm::normalize(glm::vec2(1.0f, 0.35f));
	float strength = 0.35f;          // a light breeze: a `wind` material sways out of the box
	float gustLength = 40.0f;
	float gustSpeed = 8.0f;
	float timeScale = 1.0f;

	// The clock in double: the shader is given it wrapped at WRAP seconds, and
	// every rate in shaders/wind.glsl is a whole number of turns in WRAP, so the
	// wrap is never seen. The gusts travel at a speed the game sets, so where
	// they have got to is worked out here, exactly, and handed over as an offset
	// along the wind, wrapped at a gust's length.
	const double WRAP = 4096.0;
	double windClock = 0.0;
	double lastWindClock = 0.0;
	double gustTravel = 0.0;         // world units the gusts have moved
	double lastGustTravel = 0.0;

	float Wrapped(double v, double period)
	{
		return (float)(v - std::floor(v / period) * period);
	}
}

void SetWind(const glm::vec2& towardXZ, float s)
{
	const float len = glm::length(towardXZ);
	if (len > 1e-6f)
		toward = towardXZ / len;
	strength = std::max(0.0f, s);
}

void SetWindGusts(float length, float speed)
{
	gustLength = std::max(0.01f, length);
	gustSpeed = speed;
}

void SetWindTimeScale(float scale)
{
	timeScale = std::max(0.0f, scale);
}

float WindClock() { return (float)windClock; }
float WindTimeScale() { return timeScale; }
glm::vec2 WindToward() { return toward; }
float WindStrength() { return strength; }

void UpdateWind(float realSeconds)
{
	const double dt = (double)std::max(0.0f, realSeconds) * timeScale;
	lastWindClock = windClock;
	lastGustTravel = gustTravel;
	windClock += dt;
	gustTravel += dt * gustSpeed;
}

glm::vec4 WindBlockParams()
{
	return glm::vec4(toward.x, toward.y, strength, Wrapped(windClock, WRAP));
}

glm::vec4 WindBlockParams2()
{
	// Last frame's clock is wrapped with this frame's, so the two stay a frame
	// apart across the wrap (the shader's rates turn whole turns in WRAP).
	const float now = Wrapped(windClock, WRAP);
	const float last = now - (float)(windClock - lastWindClock);
	return glm::vec4(last, gustLength, Wrapped(gustTravel, gustLength),
		Wrapped(gustTravel, gustLength) - (float)(gustTravel - lastGustTravel));
}
