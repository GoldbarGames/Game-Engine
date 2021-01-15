#ifndef INPUTMANAGER_H
#define INPUTMANAGER_H
#pragma once

#include <SDL2/SDL.h>
#include <unordered_map>
#include <string>
#include <fstream>
#include "globals.h"
#include "Timer.h"

// TODO: For next time:

// - Maybe refactor the mouse-related stuff to be in this class also
// - Maybe figure out how to record our button inputs and play them back
// - Figure out how to get these to be compatible with physical controllers
// - Automatically translate text if not in English to the selected language

struct KeyMapData
{
	SDL_Scancode defaultKey = SDL_SCANCODE_UNKNOWN;
	SDL_Scancode mappedKey = SDL_SCANCODE_UNKNOWN;
	bool previousState = false;
};

class InputManager
{
private:
	int mouseX = 0;
	int mouseY = 0;
public:
	
	bool readKeyPressesFromFile = false;
	std::string inputFile = "";

	mutable std::unordered_map<std::string, KeyMapData> keys;

	std::vector<int> recordedInputs;
	bool isRecordingInput = false;
	
	std::vector<int> playbackInputs;
	int playbackIndex = 0;

	int playbackValue = 0;
	int prevPlaybackValue = 0;

	int nCount = 0;
	int pCount = 0;

	bool isCheckingForKeyMapping = false;
	SDL_Scancode pressedKey = SDL_SCANCODE_UNKNOWN;

	Timer inputTimer;

	const int GetMouseX() const { return mouseX; }
	const int GetMouseY() const { return mouseY; }

	void Init()
	{
		std::vector<std::string> initialValues = ReadStringsFromFile("data/controller.config");

		for (int i = 0; i < initialValues.size(); i++)
		{
			if (initialValues[i] != "")
			{
				int k = 0;
				int code = std::stoi(ParseWord(initialValues[i], ' ', k));
				std::string name = initialValues[i].substr(k, initialValues[i].size() - k);
				keys[name].mappedKey = (SDL_Scancode)code;
			}
		}
	}

	void StopPlayback()
	{
		readKeyPressesFromFile = false;
		playbackInputs.clear();
		playbackIndex = 0;
	}

	void StartPlayback(const std::string& filepath)
	{
		readKeyPressesFromFile = true;
		playbackInputs.clear();
		playbackInputs.reserve(10000);
		playbackIndex = 0;
		pCount = 0;

		std::ifstream fin;
		std::string s = "";
		char c;

		fin.open(filepath);
		if (fin.is_open())
		{
			while (!fin.eof())
			{
				fin.get(c);

				if (c == '\n')
					break;

				if (c == ' ' && s.size() > 0)
				{
					playbackInputs.emplace_back(std::stoi(s));
					s = "";
				}
				else
				{
					s += c;
				}
			}
		}

		fin.close();
	}


	
	void StartRecording()
	{
		isRecordingInput = true;
		recordedInputs.clear();
		recordedInputs.reserve(10000);
		nCount = 0;
	}

	void StopRecording()
	{
		isRecordingInput = false;
		SaveRecordedInput();
		recordedInputs.clear();
	}

	void RecordInput()
	{
		static int previousNumber = -1;

		const uint8_t* input = SDL_GetKeyboardState(NULL);
		for (auto& [key, val] : keys)
		{
			if (input[keys[key].mappedKey])
			{
				if (keys[key].mappedKey != previousNumber)
				{
					recordedInputs.emplace_back(keys[key].defaultKey);
					recordedInputs.emplace_back(nCount);
					previousNumber = keys[key].mappedKey;
					nCount = -1;
				}

				nCount++;
				
				return; // Only records the first button-press each frame
			}
		}

		if (0 != previousNumber)
		{
			recordedInputs.emplace_back(0);
			recordedInputs.emplace_back(nCount);
			previousNumber = 0;
			nCount = -1;
		}
		nCount++;
	}

	void SaveRecordedInput()
	{
		std::ofstream fout;

		fout.open("data/inputs.dat");

		for (const auto& input : recordedInputs)
		{
			fout << input << " ";
		}

		fout.close();
	}

	void SetDefaultKeys(const std::unordered_map<std::string, SDL_Scancode>& defaultKeys) const
	{
		// Make sure that if any key mappings are missing from the config file,
		// that we put them in our list of key mappings anyway
		for (const auto& [key, val] : defaultKeys)
		{			
			if (keys.count(key) == 0)
			{
				keys[key].mappedKey = val;
			}
			keys[key].defaultKey = val;
		}
	}

	void ResetKeysToDefaults()
	{
		for (auto& [key, val] : keys)
		{
			val.mappedKey = val.defaultKey;
		}
	}

	void SaveMappingsToFile()
	{
		std::ofstream fout;

		fout.open("data/controller.config");

		for (const auto& [key, val] : keys)
		{
			fout << val.mappedKey << " " << key << std::endl;
		}

		fout.close();
	}

	std::string GetMappedKeyAsString(const std::string& name)
	{
		return GetScancodeAsString(keys[name].mappedKey);
	}

	std::string GetScancodeAsString(SDL_Scancode code)
	{
		return SDL_GetScancodeName(code);
	}

	void StartUpdate()
	{
		SDL_GetMouseState(&mouseX, &mouseY);

		// Check every key at the beginning of the Update loop
		// in order to see if it had been pressed or released last frame.
		if (isRecordingInput)
		{
			RecordInput();
		}
		else if (readKeyPressesFromFile)
		{
			pCount++;
			if (pCount > playbackInputs[playbackIndex + 1])
			{
				playbackIndex += 2;
				pCount = 0;

				if (playbackIndex >= playbackInputs.size())
				{
					StopPlayback();
				}
				else
				{
					playbackValue = playbackInputs[playbackIndex];
				}
			}

		}
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
			return playbackValue == keys[keyName].mappedKey;
		}
		else
		{
			const uint8_t* input = SDL_GetKeyboardState(NULL);
			return input[keys[keyName].mappedKey];
		}
	}

	bool GetKeyPressed(const std::string& keyName)
	{
		if (readKeyPressesFromFile && playbackIndex > 0)
		{
			return playbackValue == keys[keyName].mappedKey && prevPlaybackValue != keys[keyName].mappedKey;
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
			return playbackValue != keys[keyName].mappedKey && prevPlaybackValue == keys[keyName].mappedKey;
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