#pragma once
#include <string>
#include <vector>

const int TILE_SIZE = 24;
const int SCALE = 2;
const int SCREEN_FPS = 60;
const int SCREEN_TICKS_PER_FRAME = 1000 / SCREEN_FPS;

// can change resolution, this is base res
// TODO: Get this dynamically when the screen res is changed
const int screenWidth = 1280;
const int screenHeight = 720;

enum DrawingLayer { BACK = 0, FRONT = 40, OBJECT = 20, COLLISION = 30, MIDDLE = 10 };

std::string GetDrawingLayerName(DrawingLayer layer);

struct Color {
	int r = 0;
	int g = 0;
	int b = 0;
	int a = 0;
};