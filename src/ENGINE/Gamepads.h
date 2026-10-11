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

	bool AnyPressed(int sdlButton);         // on any pad this update (game keys: GetKeyPressed)

	// A menu owned by one input (InputManager::SetMenuOwner). kMenuOwnerAny,
	// the default, lets every input use the menus; kMenuOwnerKeyboard only the
	// keyboard and mouse; 0-3 only that pad. Ignored with multi-pad off. If the
	// owning pad is unplugged, the keyboard and mouse take the menu over.
	constexpr int kMenuOwnerAny = -1;
	constexpr int kMenuOwnerKeyboard = -2;
	void SetMenuOwner(int owner);
	int MenuOwner();
	bool MenuKeyboardAllowed();             // the keyboard and mouse may use the open menu
	bool MenuPadAllowed(int pad);           // this pad may

	// The menus' reads: only the pads that may use the menu count
	bool MenuPressed(int sdlButton);        // this update
	int AnyButtonPressed();                 // the first button pressed (a remap), or -1
	MenuDirection AnyMenuDirection();       // d-pad, or left stick past half way
	// The arrows or WASD (when the keyboard may use the menu), or a pad's d-pad
	// or left stick, tested in that order: what MenuButton and SettingsButton read
	MenuDirection HeldMenuDirection(const Uint8* keyStates);
}

#endif
