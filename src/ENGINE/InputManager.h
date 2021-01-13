#ifndef INPUTMANAGER_H
#define INPUTMANAGER_H
#pragma once

#include <SDL2/SDL.h>
#include <unordered_map>
#include <string>
#include "globals.h"

// TODO: For next time:
// - Actually being able to change these mappings in-game from a menu
// - Then saving these settings to the file
// - Hard-code the default values into each game
// - Add a button to reset the controller mappings to their default values
// - Figure out how to get these to be compatible with physical controllers
// - Maybe figure out how to record our button inputs and play them back
// - Going through the actual process of mapping all keys in our games
// - Maybe refactor the mouse-related stuff to be in this class also


struct KeyMapData
{
	SDL_Scancode defaultKey = SDL_SCANCODE_UNKNOWN;
	SDL_Scancode mappedKey = SDL_SCANCODE_UNKNOWN;
	bool previousState = false;
};

class InputManager
{
private:
	
public:
	
	// TODO: This will be used for automated level testing
	bool readKeyPressesFromFile = false;
	std::string inputFile = "";

	int mouseX = 0;
	int mouseY = 0;
	std::unordered_map<std::string, KeyMapData> keys;

	void Init()
	{
		std::vector<std::string> initialValues = ReadStringsFromFile("data/controller.config");

		for (int i = 0; i < initialValues.size(); i++)
		{
			int k = 0;
			int code = std::stoi(ParseWord(initialValues[i], ' ', k));
			std::string name = initialValues[i].substr(k, initialValues.size() - k);
			keys[name].mappedKey = (SDL_Scancode)code;
		}
	}

	void StartUpdate()
	{
		SDL_GetMouseState(&mouseX, &mouseY);

		// Check every key at the beginning of the Update loop
		// in order to see if it had been pressed or released last frame.
	}

	void EndUpdate()
	{
		const uint8_t* input = SDL_GetKeyboardState(NULL);
		for (auto& [key, val] : keys)
		{
			val.previousState = input[val.mappedKey];
		}
	}

	bool GetKey(const std::string& keyName)
	{
		if (readKeyPressesFromFile)
		{
			return false;
		}
		else
		{
			const uint8_t* input = SDL_GetKeyboardState(NULL);
			return input[keys[keyName].mappedKey];
		}
	}

	bool GetKeyPressed(const std::string& keyName)
	{
		if (readKeyPressesFromFile)
		{
			return false;
		}
		else
		{
			// If it is held down now but not before, it was pressed
			const uint8_t* input = SDL_GetKeyboardState(NULL);
			return input[keys[keyName].mappedKey] && !keys[keyName].previousState;
		}
	}

	bool GetKeyReleased(const std::string& keyName)
	{
		if (readKeyPressesFromFile)
		{
			return false;
		}
		else
		{
			// If it is not held down now but was before, it was released
			const uint8_t* input = SDL_GetKeyboardState(NULL);
			return !input[keys[keyName].mappedKey] && keys[keyName].previousState;
		}
	}
};

#endif