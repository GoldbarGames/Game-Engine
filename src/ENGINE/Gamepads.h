#ifndef GAMEPADS_H
#define GAMEPADS_H
#pragma once

#include <SDL2/SDL.h>

class Game;

// Several gamepads at once (opt-in: InputManager::SetMultiPad). Engine-internal
// and not exported: the pad table lives in Gamepads.cpp, so no exported class
// changes layout. Games use InputManager's GetPad* methods.
//
// With multi-pad off (the default) every function here reports nothing and the
// engine keeps its old single-pad behaviour.
namespace Gamepads
{
	constexpr int kMaxPads = 4;

	enum MenuDirection { None = 0, Up, Down, Left, Right };

	bool MultiPad();
	void SetMultiPad(bool on);

	// Opens every connected pad into the lowest free slot (and adopts the one
	// the single-pad code already opened). Runs at the first CheckInputs after
	// multi-pad is turned on.
	bool RescanPending();
	void OpenAll(Game& game);
	void CloseAll(Game& game);

	// Hot-plug: SDL_CONTROLLERDEVICEADDED / REMOVED
	void HandleEvent(Game& game, const SDL_Event& event);

	// Once per update: reads every pad's buttons and axes, keeping last
	// update's buttons for the pressed / released edges
	void Poll();

	// Native builds: data/config/gamecontrollerdb.txt, if present, teaches
	// SDL 2.0.9 about pads it doesn't know
	void LoadMappings();

	int Count();
	bool Connected(int pad);
	const char* Name(int pad);
	bool Held(int pad, int sdlButton);
	bool Pressed(int pad, int sdlButton);
	bool Released(int pad, int sdlButton);
	int Axis(int pad, int sdlAxis);        // raw, -32768..32767

	bool AnyPressed(int sdlButton);         // on any pad this update
	int AnyButtonPressed();                 // the first button pressed on any pad, or -1
	MenuDirection AnyMenuDirection();       // d-pad, or left stick past half way, on any pad
}

#endif
