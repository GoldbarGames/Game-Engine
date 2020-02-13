#pragma once
#include <string>
#include <vector>

const int TILE_SIZE = 24;
const int SCREEN_FPS = 60;
const int SCREEN_TICKS_PER_FRAME = 1000 / SCREEN_FPS;

enum DrawingLayer { BACK = 0, FRONT = 40, OBJECT = 20, COLLISION = 30, MIDDLE = 10 };

std::string GetDrawingLayerName(DrawingLayer layer);

struct Color {
	int r = 0;
	int g = 0;
	int b = 0;
	int a = 0;

	bool operator==(const Color& other) const
	{
		return (r == other.r && g == other.g && b == other.b && a == other.a);
	}

	bool operator!=(const Color& other) const
	{
		return !(*this == other);
	}
};