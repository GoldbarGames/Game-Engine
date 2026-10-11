#include "Gamepads.h"
#include "Game.h"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace Gamepads
{
	namespace
	{
		struct Slot
		{
			SDL_GameController* pad = nullptr;
			SDL_JoystickID id = -1;
			bool scripted = false;   // a made-up pad from KINJO_PAD_SCRIPT
			uint32_t held = 0;       // one bit per SDL_GameControllerButton
			uint32_t prevHeld = 0;
			int16_t axes[SDL_CONTROLLER_AXIS_MAX] = {};

			bool Connected() const { return pad != nullptr || scripted; }
		};

		Slot slots[kMaxPads];
		bool multiPad = false;
		bool rescanPending = false;
		int menuOwner = kMenuOwnerAny;

		// KINJO_PAD_SCRIPT: made-up pad input for test runs with no pads plugged
		// in. Entries separated by ';', each "<pad> <control>[=<value>] <from>[-<to>]",
		// frames counted in updates from when multi-pad is turned on. A control
		// is an SDL button name (a, b, x, y, back, guide, start, leftstick,
		// rightstick, leftshoulder, rightshoulder, dpup, dpdown, dpleft, dpright)
		// or an axis name with a value from -1 to 1 (leftx=0.8, lefty=-1,
		// righttrigger=1, ...). Every pad the script names counts as connected.
		struct ScriptEntry
		{
			int pad = 0;
			int button = -1;
			int axis = -1;
			float value = 0.0f;
			int from = 0;
			int to = 0;
		};
		std::vector<ScriptEntry> script;
		int pollCount = 0;

		bool Valid(int pad)
		{
			return pad >= 0 && pad < kMaxPads && slots[pad].Connected();
		}

		bool ValidButton(int b)
		{
			return b >= 0 && b < SDL_CONTROLLER_BUTTON_MAX;
		}

		// Game::controller stays "the first pad", for the code that uses it
		void SyncGameController(Game& game)
		{
			game.controller = nullptr;
			for (const Slot& s : slots)
			{
				if (s.pad != nullptr)
				{
					game.controller = s.pad;
					break;
				}
			}
		}

		int FreeSlot()
		{
			for (int i = 0; i < kMaxPads; i++)
			{
				if (!slots[i].Connected())
					return i;
			}
			return -1;
		}

		int SlotOf(SDL_JoystickID id)
		{
			for (int i = 0; i < kMaxPads; i++)
			{
				if (slots[i].pad != nullptr && slots[i].id == id)
					return i;
			}
			return -1;
		}

		void Adopt(Game& game, SDL_GameController* pad)
		{
			const SDL_JoystickID id = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(pad));
			if (SlotOf(id) >= 0)
				return;
			const int slot = FreeSlot();
			if (slot < 0)
				return;
			slots[slot] = Slot();
			slots[slot].pad = pad;
			slots[slot].id = id;
			std::cout << "Gamepad " << slot << " connected: " << SDL_GameControllerName(pad) << std::endl;
			SyncGameController(game);
		}

		void Open(Game& game, int deviceIndex)
		{
			if (!SDL_IsGameController(deviceIndex))
				return;
			const SDL_JoystickID id = SDL_JoystickGetDeviceInstanceID(deviceIndex);
			if (id < 0 || SlotOf(id) >= 0)
				return;   // already open (SDL also announces the pads present at startup)
			if (FreeSlot() < 0)
			{
				std::cout << "Gamepad ignored: all " << kMaxPads << " slots are in use" << std::endl;
				return;
			}
			SDL_GameController* pad = SDL_GameControllerOpen(deviceIndex);
			if (pad == nullptr)
			{
				std::cout << "Gamepad failed to open: " << SDL_GetError() << std::endl;
				return;
			}
			Adopt(game, pad);
		}

		void ParseScript()
		{
			script.clear();
			const char* text = std::getenv("KINJO_PAD_SCRIPT");
			if (text == nullptr || *text == '\0')
				return;

			std::stringstream all(text);
			std::string entry;
			while (std::getline(all, entry, ';'))
			{
				std::istringstream words(entry);
				std::string control, frames;
				ScriptEntry e;
				if (!(words >> e.pad >> control >> frames) || e.pad < 0 || e.pad >= kMaxPads)
				{
					if (entry.find_first_not_of(" \t") != std::string::npos)
						std::cout << "KINJO_PAD_SCRIPT: can't read '" << entry << "'" << std::endl;
					continue;
				}

				const size_t eq = control.find('=');
				if (eq != std::string::npos)
				{
					e.axis = SDL_GameControllerGetAxisFromString(control.substr(0, eq).c_str());
					e.value = std::strtof(control.substr(eq + 1).c_str(), nullptr);
				}
				else
				{
					e.button = SDL_GameControllerGetButtonFromString(control.c_str());
				}
				if (e.axis < 0 && e.button < 0)
				{
					std::cout << "KINJO_PAD_SCRIPT: unknown control '" << control << "'" << std::endl;
					continue;
				}

				const size_t dash = frames.find('-');
				e.from = std::atoi(frames.substr(0, dash).c_str());
				e.to = (dash == std::string::npos) ? e.from : std::atoi(frames.substr(dash + 1).c_str());
				script.push_back(e);
				slots[e.pad].scripted = true;
			}
			std::cout << "KINJO_PAD_SCRIPT: " << script.size() << " entries" << std::endl;
		}
	}

	bool MultiPad()
	{
		return multiPad;
	}

	void SetMultiPad(bool on)
	{
		multiPad = on;
		rescanPending = on;
		pollCount = 0;
		if (on)
			ParseScript();
	}

	bool RescanPending()
	{
		return multiPad && rescanPending;
	}

	void OpenAll(Game& game)
	{
		rescanPending = false;
		if (!multiPad)
			return;

		// The pad the single-pad code already opened keeps its place as pad 0.
		// (One already in a slot is left alone: if it's been unplugged, its
		// removal event closes it.)
		if (game.controller != nullptr)
		{
			const SDL_JoystickID id = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(game.controller));
			if (SlotOf(id) < 0)
			{
				if (SDL_GameControllerGetAttached(game.controller))
				{
					Adopt(game, game.controller);
				}
				else
				{
					SDL_GameControllerClose(game.controller);
					game.controller = nullptr;
				}
			}
		}

		for (int i = 0; i < SDL_NumJoysticks(); i++)
			Open(game, i);
		SyncGameController(game);
	}

	void CloseAll(Game& game)
	{
		for (Slot& s : slots)
		{
			if (s.pad != nullptr)
				SDL_GameControllerClose(s.pad);
			s = Slot();
		}
		game.controller = nullptr;
	}

	void HandleEvent(Game& game, const SDL_Event& event)
	{
		if (!multiPad)
			return;

		if (event.type == SDL_CONTROLLERDEVICEADDED)
		{
			Open(game, event.cdevice.which);   // a device index
		}
		else if (event.type == SDL_CONTROLLERDEVICEREMOVED)
		{
			const int slot = SlotOf(event.cdevice.which);   // an instance id
			if (slot >= 0)
			{
				std::cout << "Gamepad " << slot << " disconnected" << std::endl;
				SDL_GameControllerClose(slots[slot].pad);
				slots[slot] = Slot();
				SyncGameController(game);
				// A menu it owned would be left with no input that can reach it
				if (menuOwner == slot)
				{
					menuOwner = kMenuOwnerKeyboard;
					std::cout << "Gamepad " << slot << " owned the open menu: the keyboard and mouse have it now" << std::endl;
				}
			}
		}
	}

	void Poll()
	{
		if (!multiPad)
			return;
		pollCount++;

		for (int i = 0; i < kMaxPads; i++)
		{
			Slot& s = slots[i];
			if (!s.Connected())
				continue;

			s.prevHeld = s.held;
			s.held = 0;
			for (int16_t& a : s.axes)
				a = 0;

			if (s.scripted)
			{
				for (const ScriptEntry& e : script)
				{
					if (e.pad != i || pollCount < e.from || pollCount > e.to)
						continue;
					if (e.button >= 0)
						s.held |= 1u << e.button;
					else
						s.axes[e.axis] = static_cast<int16_t>(std::max(-1.0f, std::min(1.0f, e.value)) * 32767.0f);
				}
				continue;
			}

			for (int b = 0; b < SDL_CONTROLLER_BUTTON_MAX; b++)
			{
				if (SDL_GameControllerGetButton(s.pad, static_cast<SDL_GameControllerButton>(b)))
					s.held |= 1u << b;
			}
			for (int a = 0; a < SDL_CONTROLLER_AXIS_MAX; a++)
				s.axes[a] = SDL_GameControllerGetAxis(s.pad, static_cast<SDL_GameControllerAxis>(a));
		}
	}

	void LoadMappings()
	{
#ifndef __EMSCRIPTEN__
		// (On the web, browsers name pads differently, so a mapping file can't match.)
		const char* path = "data/config/gamecontrollerdb.txt";
		std::ifstream file(path);
		if (!file)
			return;
		file.close();
		const int added = SDL_GameControllerAddMappingsFromFile(path);
		std::cout << "Gamepad mappings: " << added << " from " << path << std::endl;
#endif
	}

	int Count()
	{
		int n = 0;
		for (const Slot& s : slots)
			n += s.Connected() ? 1 : 0;
		return multiPad ? n : 0;
	}

	bool Connected(int pad)
	{
		return multiPad && Valid(pad);
	}

	const char* Name(int pad)
	{
		if (!Connected(pad))
			return "";
		if (slots[pad].scripted)
			return "scripted pad (KINJO_PAD_SCRIPT)";
		const char* name = SDL_GameControllerName(slots[pad].pad);
		return name ? name : "";
	}

	bool Held(int pad, int sdlButton)
	{
		return Connected(pad) && ValidButton(sdlButton) && (slots[pad].held >> sdlButton & 1u) != 0;
	}

	bool Pressed(int pad, int sdlButton)
	{
		return Held(pad, sdlButton) && (slots[pad].prevHeld >> sdlButton & 1u) == 0;
	}

	bool Released(int pad, int sdlButton)
	{
		return Connected(pad) && ValidButton(sdlButton)
			&& (slots[pad].held >> sdlButton & 1u) == 0 && (slots[pad].prevHeld >> sdlButton & 1u) != 0;
	}

	int Axis(int pad, int sdlAxis)
	{
		if (!Connected(pad) || sdlAxis < 0 || sdlAxis >= SDL_CONTROLLER_AXIS_MAX)
			return 0;
		return slots[pad].axes[sdlAxis];
	}

	bool AnyPressed(int sdlButton)
	{
		for (int i = 0; i < kMaxPads; i++)
		{
			if (Pressed(i, sdlButton))
				return true;
		}
		return false;
	}

	void SetMenuOwner(int owner)
	{
		if (owner != kMenuOwnerAny && owner != kMenuOwnerKeyboard && (owner < 0 || owner >= kMaxPads))
			owner = kMenuOwnerAny;
		menuOwner = owner;
	}

	int MenuOwner()
	{
		return menuOwner;
	}

	bool MenuKeyboardAllowed()
	{
		return !multiPad || menuOwner == kMenuOwnerAny || menuOwner == kMenuOwnerKeyboard;
	}

	bool MenuPadAllowed(int pad)
	{
		return !multiPad || menuOwner == kMenuOwnerAny || menuOwner == pad;
	}

	bool MenuPressed(int sdlButton)
	{
		for (int i = 0; i < kMaxPads; i++)
		{
			if (MenuPadAllowed(i) && Pressed(i, sdlButton))
				return true;
		}
		return false;
	}

	int AnyButtonPressed()
	{
		for (int i = 0; i < kMaxPads; i++)
		{
			if (!MenuPadAllowed(i))
				continue;
			for (int b = 0; b < SDL_CONTROLLER_BUTTON_MAX; b++)
			{
				if (Pressed(i, b))
					return b;
			}
		}
		return -1;
	}

	MenuDirection HeldMenuDirection(const Uint8* keys)
	{
		const MenuDirection pad = AnyMenuDirection();
		const bool k = keys != nullptr && MenuKeyboardAllowed();
		if ((k && (keys[SDL_SCANCODE_UP] || keys[SDL_SCANCODE_W])) || pad == Up)
			return Up;
		if ((k && (keys[SDL_SCANCODE_DOWN] || keys[SDL_SCANCODE_S])) || pad == Down)
			return Down;
		if ((k && (keys[SDL_SCANCODE_LEFT] || keys[SDL_SCANCODE_A])) || pad == Left)
			return Left;
		if ((k && (keys[SDL_SCANCODE_RIGHT] || keys[SDL_SCANCODE_D])) || pad == Right)
			return Right;
		return None;
	}

	MenuDirection AnyMenuDirection()
	{
		const int half = 16384;
		for (int i = 0; i < kMaxPads; i++)
		{
			if (!Connected(i) || !MenuPadAllowed(i))
				continue;
			if (Held(i, SDL_CONTROLLER_BUTTON_DPAD_UP) || Axis(i, SDL_CONTROLLER_AXIS_LEFTY) <= -half)
				return Up;
			if (Held(i, SDL_CONTROLLER_BUTTON_DPAD_DOWN) || Axis(i, SDL_CONTROLLER_AXIS_LEFTY) >= half)
				return Down;
			if (Held(i, SDL_CONTROLLER_BUTTON_DPAD_LEFT) || Axis(i, SDL_CONTROLLER_AXIS_LEFTX) <= -half)
				return Left;
			if (Held(i, SDL_CONTROLLER_BUTTON_DPAD_RIGHT) || Axis(i, SDL_CONTROLLER_AXIS_LEFTX) >= half)
				return Right;
		}
		return None;
	}
}
