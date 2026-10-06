#ifndef RENDER_VIEWS_H
#define RENDER_VIEWS_H
#pragma once

// Engine-internal (not exported). Split screen: Game::Render draws the world
// chain (RenderWorldChain) once per view (Renderer::SetViews), each through
// its own camera into its own view-sized framebuffer, and the composite puts
// each in its part of the window.
//
// While a view is drawing:
//   - renderer.camera holds a copy of the view's camera (its projection fitted
//     to the view), and the main camera is restored afterwards;
//   - the modules whose state lasts between frames keep one set per view and
//     switch to it here: TAA's history, the sun's shadow cascades, the light
//     clusters; the GPU-driven path may draw its models again;
//   - code inside the chain asks ViewTargetWidth / Height / FrameBuffer for
//     the target it is drawing, rather than Game's screen size and main
//     framebuffer (which stay the window's, for game code that runs mid-frame).
// Outside split screen nothing is current, view 0's state is used, and the
// getters give the window's - exactly as before views existed.

class Game;
class FrameBuffer;

const int kMaxRenderViews = 4;

// Game::Render, around each view's world chain.
void BeginViewRender(int index, int width, int height, FrameBuffer* target);
void EndViewRender();

int CurrentViewIndex();     // 0 outside split screen
bool RenderingViews();      // a view is drawing now

int ViewTargetWidth(const Game& game);
int ViewTargetHeight(const Game& game);
FrameBuffer* ViewTargetFrameBuffer(const Game& game);

#endif
