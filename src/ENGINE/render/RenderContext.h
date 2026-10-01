#ifndef RENDER_CONTEXT_H
#define RENDER_CONTEXT_H
#pragma once

// Engine-internal (not exported). The graphics context behind the game window:
// created once the window exists and before anything uses the RenderDevice,
// destroyed with the window. The GL backend (render/gl/GLContext.cpp) makes an
// SDL GL context, loads the entry points and records the GLSL version; a
// Vulkan backend would create its instance, surface and swapchain here.

#include <cstdint>

struct SDL_Window;
class Logger;

// SDL_CreateWindow flags the backend needs (SDL_WINDOW_OPENGL for GL).
uint32_t RenderWindowFlags();

// Create the context for `window` and make it current. Returns nullptr if no
// context could be made (the reasons go to `logger`).
void* CreateRenderContext(SDL_Window* window, Logger& logger);
void DestroyRenderContext(void* context);

// Show the finished frame.
void PresentFrame(SDL_Window* window);

// Swap interval: 0 = no vsync, 1 = vsync (the default).
void SetVSync(int interval);

#endif
