#pragma once
#include <string>

const int TILE_SIZE = 24;
const int SCALE = 2;

// can change resolution, this is base res
const int screenWidth = 640;
const int screenHeight = 480;

enum DrawingLayer { BACKGROUND, FOREGROUND };
static const std::string DrawingLayerNames[] = { "BACKGROUND", "FOREGROUND" };