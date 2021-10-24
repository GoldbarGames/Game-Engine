#ifndef OPENGL_INCLUDES_H
#define OPENGL_INCLUDES_H
#pragma once

#ifdef EMSCRIPTEN
	#include <emscripten.h>
	//#include <GL/Regal.h>
	#include <GLES3/gl3.h>
#else
	#include <GL/glew.h>
#endif

#endif