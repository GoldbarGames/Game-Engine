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
	
	
	std::string inputFile = "";

	mutable std::unordered_map<std::string, KeyMapData> keys;

	std::vector<int> recordedInputs;
	bool isRecordingInput = false;
	bool isPlayingBackInput = false;

	std::vector<int> playbackInputs;
	int playbackIndex = 0;

	std::unordered_map<int, int> inputsThisFrame;
	std::unordered_map<int, int> inputsLastFrame;

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
		isPlayingBackInput = false;
		playbackInputs.clear();
		playbackIndex = 0;
	}

	void StartPlayback(const std::string& filepath)
	{
		isPlayingBackInput = true;
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
		inputsThisFrame.clear();
		inputsLastFrame.clear();
		inputsLastFrame[0] = 1;
		isRecordingInput = true;
		recordedInputs.clear();
		recordedInputs.reserve(10000);
		nCount = 0;
	}

	void StopRecording()
	{
		inputsThisFrame.clear();
		inputsLastFrame.clear();
		isRecordingInput = false;
		SaveRecordedInput();
		recordedInputs.clear();
	}

	void RecordInput()
	{
		const uint8_t* input = SDL_GetKeyboardState(NULL);

		// Save the inputs from last frame
		inputsLastFrame.clear();
		for (auto& [key, val] : inputsThisFrame)
		{
			inputsLastFrame[key] = val;
		}

		// Get all inputs for the current frame
		inputsThisFrame.clear();
		for (auto& [key, val] : keys)
		{
			if (input[keys[key].mappedKey])
			{
				inputsThisFrame[keys[key].mappedKey] = 1;
			}
		}

		// If we didn't get any key presses this frame,
		// record this frame as a zero
		if (inputsThisFrame.size() == 0)
		{
			inputsThisFrame[0] = 1;
		}

		// We want to check that the two lists are equal

		bool areListsTheSame = true;

		for (auto& [key, val] : inputsThisFrame)
		{
			// Check whether all keys pressed this frame
			// were also pressed the frame before
			if (inputsLastFrame.count(key) == 0)
			{
				// If we get here, then we pressed a key now
				// that had not been pressed before.
				areListsTheSame = false;
			}
		}

		for (auto& [lastkey, lastval] : inputsLastFrame)
		{
			// Check whether all keys pressed this frame
			// were also pressed the frame before
			if (inputsThisFrame.count(lastkey) == 0)
			{
				// If we get here, then we had pressed a key before
				// that we have stopped pressing now.
				areListsTheSame = false;
			}
		}
		
		// If there is any difference between the two lists,
		// then we record the previous list and the number of frames
		// that it had been true that there was no difference.

		// In other words, if there is no difference between the two lists
		// then we increment the counter. If there is a difference,
		// then we record the previous list and reset the counter.

		if (areListsTheSame)
		{
			nCount++;
		}
		else
		{
			for (auto& [key, val] : inputsLastFrame)
			{
				recordedInputs.emplace_back(key);
			}

			recordedInputs.emplace_back(nCount * -1);
			nCount = 0;
		}

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
		else if (isPlayingBackInput)
		{
			pCount++; // the number of frames the button has been pressed

			static int pTargetCount = -1;
			static int endIndex = 0;

			// We don't yet know the target count for this button press
			if (pTargetCount < 0)
			{
				endIndex = playbackIndex;
				for (int i = playbackIndex; i < playbackInputs.size(); i++)
				{
					if (playbackInputs[i] < 0) // found the count
					{
						pTargetCount = -1 * playbackInputs[i];
						endIndex = i;
						break;
					}
				}

				// Save the inputs that are being held last frame
				inputsLastFrame.clear();
				for (auto& [key, val] : inputsThisFrame)
				{
					inputsLastFrame[key] = val;
				}
				
				// Save the inputs that are being held this frame
				inputsThisFrame.clear();
				for (int k = playbackIndex; k < endIndex; k++)
				{
					inputsThisFrame[playbackInputs[k]] = 1;
				}
			}

			if (pCount > pTargetCount)
			{				
				pCount = 0;
				pTargetCount = -1;
				playbackIndex = endIndex + 1; // always the number after the count

				if (playbackIndex >= playbackInputs.size())
				{
					StopPlayback();
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
		if (isPlayingBackInput)
		{
			return inputsThisFrame.count(keys[keyName].mappedKey) != 0;
		}
		else
		{
			const uint8_t* input = SDL_GetKeyboardState(NULL);
			return input[keys[keyName].mappedKey];
		}
	}

	bool GetKeyPressed(const std::string& keyName)
	{
		if (isPlayingBackInput && playbackIndex > 0)
		{
			return inputsThisFrame.count(keys[keyName].mappedKey) != 0 
				&& inputsLastFrame.count(keys[keyName].mappedKey) == 0;
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
		if (isPlayingBackInput)
		{
			return inputsThisFrame.count(keys[keyName].mappedKey) == 0
				&& inputsLastFrame.count(keys[keyName].mappedKey) != 0;
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