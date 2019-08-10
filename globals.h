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

enum DrawingLayer { BACKGROUND, FOREGROUND, OBJECT };
static const std::vector<std::string> DrawingLayerNames = { "BACKGROUND", "FOREGROUND", "OBJECT" };

//TODO
//enum CollisionLayer { PLAYER, FOREGROUND };
//static const std::string DrawingLayerNames[] = { "PLAYER", "FOREGROUND" };