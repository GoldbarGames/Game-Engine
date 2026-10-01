// GL backend side of render/RenderContext.h: the SDL GL context, GLEW, the
// achieved GLSL version and (debug desktop builds) GL debug output. Moved out
// of Game::InitOpenGL unchanged, in the same order.

#include "../RenderContext.h"
#include "../../opengl_includes.h"
#include "../../Logger.h"
#include "../../Shader.h"
#include <SDL2/SDL.h>
#include <iostream>
#include <string>

#ifdef __EMSCRIPTEN__
#include <emscripten/html5.h>
#endif

#if defined(_DEBUG) && !defined(__EMSCRIPTEN__)
// GL 4.3+ debug output: the driver hands us the exact offending call with a
// readable message + severity, replacing the engine's sticky-glGetError chasing.
// Installed after glewInit when the context is >= 4.3 (debug desktop builds only;
// web / release desktop are untouched). See ApplyVersion/[[engine-gl-version]].
static void GLAPIENTRY GLDebugCallback(GLenum source, GLenum type, GLuint id,
	GLenum severity, GLsizei /*length*/, const GLchar* message, const void* /*user*/)
{
	// Mute low-value driver chatter (e.g. NVIDIA "buffer will use VIDEO memory").
	if (severity == GL_DEBUG_SEVERITY_NOTIFICATION) return;
	if (id == 131185 || id == 131218 || id == 131204) return;

	const char* src = "?";
	switch (source)
	{
		case GL_DEBUG_SOURCE_API:             src = "API";      break;
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   src = "WinSys";   break;
		case GL_DEBUG_SOURCE_SHADER_COMPILER: src = "Shader";   break;
		case GL_DEBUG_SOURCE_THIRD_PARTY:     src = "3rdParty"; break;
		case GL_DEBUG_SOURCE_APPLICATION:     src = "App";      break;
		case GL_DEBUG_SOURCE_OTHER:           src = "Other";    break;
	}
	const char* typ = "?";
	switch (type)
	{
		case GL_DEBUG_TYPE_ERROR:               typ = "ERROR";       break;
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: typ = "DEPRECATED";  break;
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  typ = "UNDEFINED";   break;
		case GL_DEBUG_TYPE_PORTABILITY:         typ = "PORTABILITY"; break;
		case GL_DEBUG_TYPE_PERFORMANCE:         typ = "PERF";        break;
		case GL_DEBUG_TYPE_MARKER:              typ = "MARKER";      break;
		case GL_DEBUG_TYPE_OTHER:               typ = "OTHER";       break;
	}
	const char* sev = (severity == GL_DEBUG_SEVERITY_HIGH)   ? "HIGH"
	                : (severity == GL_DEBUG_SEVERITY_MEDIUM) ? "MED" : "LOW";
	std::cout << "[GL " << sev << "/" << src << "/" << typ << " #" << id << "] "
		<< message << std::endl;
}
#endif

uint32_t RenderWindowFlags()
{
	return SDL_WINDOW_OPENGL;
}

void* CreateRenderContext(SDL_Window* window, Logger& logger)
{
	SDL_GLContext mainContext = nullptr;

#if EMSCRIPTEN

	std::cout << "Attempting to create emscripten WebGL context..." << std::endl;

	// 3.2 is part of the modern versions of OpenGL, but most video cards whould be able to run it
	if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3) != 0)
	{
		logger.Log("ERROR: SDL_GL_SetAttribute SDL_GL_CONTEXT_MAJOR_VERSION failed. " + std::string(SDL_GetError()));
	}

	if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2) != 0)
	{
		logger.Log("ERROR: SDL_GL_SetAttribute SDL_GL_CONTEXT_MINOR_VERSION failed. " + std::string(SDL_GetError()));
	}

	// Set our OpenGL version.
	// SDL_GL_CONTEXT_CORE gives us only the newer version, deprecated functions are disabled
	if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES) != 0)
	{
		logger.Log("ERROR: SDL_GL_SetAttribute SDL_GL_CONTEXT_PROFILE_MASK failed. " + std::string(SDL_GetError()));
	}

	std::cout << "Attempting to create emscripten WebGL context 1 ..." << std::endl;

	EmscriptenWebGLContextAttributes attrs;
	std::cout << "Attempting to create emscripten WebGL context 2 ..." << std::endl;

	// The following lines must be done in exact order, or it will break!
	emscripten_webgl_init_context_attributes(&attrs); // you MUST init the attributes before creating the context

	attrs.majorVersion = 2;  // WebGL 2 = OpenGL ES 3.0
	attrs.minorVersion = 0;
	attrs.alpha = true;
	attrs.antialias = true;
	attrs.powerPreference = EM_WEBGL_POWER_PREFERENCE_DEFAULT;

	EMSCRIPTEN_WEBGL_CONTEXT_HANDLE webgl_context = emscripten_webgl_create_context("#canvas", &attrs);

	std::cout << "Attempting to create emscripten WebGL context 3 ..." << std::endl;

	emscripten_webgl_make_context_current(webgl_context);

	std::cout << "Attempting to create emscripten WebGL context 4..." << std::endl;

	mainContext = SDL_GL_CreateContext(window);

#else

	// Desktop: prefer a modern 4.6 core context (unlocks GL 4.x features), and
	// fall back to 3.3 core for older GPUs/drivers. 3.3 is the floor - our shaders
	// need GLSL 330+. The version we actually get drives the "#version" line the
	// shader loader injects (see ShaderProgram::ApplyVersion).
	// SDL_GL_CONTEXT_CORE gives us only the newer version; deprecated functions off.
	if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE) != 0)
	{
		logger.Log("ERROR: SDL_GL_SetAttribute SDL_GL_CONTEXT_PROFILE_MASK failed. " + std::string(SDL_GetError()));
	}

	// Debug builds ask for a debug-capable context so GL 4.3+ debug output
	// (glDebugMessageCallback, installed after glewInit) fires reliably.
#ifdef _DEBUG
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
#endif

	const int tryMajor[] = { 4, 3 };
	const int tryMinor[] = { 6, 3 };
	for (int t = 0; t < 2 && mainContext == nullptr; t++)
	{
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, tryMajor[t]);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, tryMinor[t]);
		std::cout << "Creating GL " << tryMajor[t] << "." << tryMinor[t] << " core context..." << std::endl;
		mainContext = SDL_GL_CreateContext(window);
		if (mainContext == nullptr)
			logger.Log("GL " + std::to_string(tryMajor[t]) + "." + std::to_string(tryMinor[t])
				+ " context unavailable: " + std::string(SDL_GetError()));
	}
	if (mainContext == nullptr)
		logger.Log("ERROR: could not create any desktop GL context. " + std::string(SDL_GetError()));

	if (SDL_GL_MakeCurrent(window, mainContext) != 0)
	{
		std::cout << "Context failed..." << std::endl;
		logger.Log("ERROR: SDL_GL_MakeCurrent failed. " + std::string(SDL_GetError()));
	}

	// Record the achieved GLSL version (GL X.Y -> GLSL X*100+Y*10, floored at
	// 330) so file shaders get their "#version" rewritten to match this context.
	{
		int glMaj = 3, glMin = 3;
		glGetIntegerv(GL_MAJOR_VERSION, &glMaj);
		glGetIntegerv(GL_MINOR_VERSION, &glMin);
		int glsl = glMaj * 100 + glMin * 10;
		if (glsl < 330) glsl = 330;
		ShaderProgram::SetGLSLVersion(glsl);
		std::cout << "Desktop GL " << glMaj << "." << glMin << " -> GLSL " << glsl << std::endl;
	}

	// Turn on double buffering with a 24bit Z buffer.
	// You may need to change this to 16 or 32 for your system
	if (SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1) != 0)
	{
		logger.Log("ERROR: SDL_GL_SetAttribute SDL_GL_DOUBLEBUFFER failed. " + std::string(SDL_GetError()));
	}

	if (SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1) != 0)
	{
		logger.Log("ERROR: SDL_GL_SetAttribute SDL_GL_MULTISAMPLEBUFFERS failed. " + std::string(SDL_GetError()));
	}

	if (SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 2) != 0)
	{
		logger.Log("ERROR: SDL_GL_SetAttribute SDL_GL_MULTISAMPLESAMPLES failed. " + std::string(SDL_GetError()));
	}

	glEnable(GL_MULTISAMPLE);
#endif

	std::cout << "GL_VERSION: " << glGetString(GL_VERSION) << std::endl;

	// Parameter 0 = no vsync, 1 = vsync
	if (SDL_GL_SetSwapInterval(1) != 0)
	{
		logger.Log("ERROR: SDL_GL_SetSwapInterval failed. " + std::string(SDL_GetError()));
	}

#ifndef EMSCRIPTEN
	glewExperimental = GL_TRUE;
	glewInit();
	// (No glAlphaFunc / GL_ALPHA_TEST: fixed-function alpha test does not exist
	// in the core profile the engine runs on; shaders discard transparent
	// pixels themselves.)

#if defined(_DEBUG) && !defined(__EMSCRIPTEN__)
	// GL 4.3+ debug output (desktop debug builds): route driver diagnostics to
	// stdout via GLDebugCallback. No-op on the 3.3 fallback (< 4.3).
	{
		int dMaj = 0, dMin = 0;
		glGetIntegerv(GL_MAJOR_VERSION, &dMaj);
		glGetIntegerv(GL_MINOR_VERSION, &dMin);
		if ((dMaj * 10 + dMin) >= 43 && glDebugMessageCallback != nullptr)
		{
			glEnable(GL_DEBUG_OUTPUT);
			glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);   // report on the offending call's stack
			glDebugMessageCallback(GLDebugCallback, nullptr);
			glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
			glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION,
				0, nullptr, GL_FALSE);
			std::cout << "GL debug output enabled (" << dMaj << "." << dMin << ")" << std::endl;
		}
	}
#endif
#endif

	return mainContext;
}

void DestroyRenderContext(void* context)
{
	SDL_GL_DeleteContext((SDL_GLContext)context);
}

void PresentFrame(SDL_Window* window)
{
	SDL_GL_SwapWindow(window);
}

void SetVSync(int interval)
{
	SDL_GL_SetSwapInterval(interval);
}
