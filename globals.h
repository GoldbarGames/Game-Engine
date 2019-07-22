#pragma once
#include <string>

const int TILE_SIZE = 24;
const int SCALE = 2;
const int SCREEN_FPS = 60;
const int SCREEN_TICKS_PER_FRAME = 1000 / SCREEN_FPS;

// can change resolution, this is base res
const int screenWidth = 1280;
const int screenHeight = 720;

enum DrawingLayer { BACKGROUND, FOREGROUND };
static const std::string DrawingLayerNames[] = { "BACKGROUND", "FOREGROUND" };

//TODO
//enum CollisionLayer { PLAYER, FOREGROUND };
//static const std::string DrawingLayerNames[] = { "PLAYER", "FOREGROUND" };