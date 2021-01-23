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
// - Figure out how to get these to be compatible with physical controllers
// - Automatically translate text if not in English to the selected language

struct KeyMapData
{
	SDL_Scancode defaultKey = SDL_SCANCODE_UNKNOWN;
	SDL_Scancode mappedKey = SDL_SCANCODE_UNKNOWN;
	bool previousState = false;
};

class KINJO_API InputManager
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
	Timer updateTimer;

	const int GetMouseX() const { return mouseX; }
	const int GetMouseY() const { return mouseY; }

	void Init();
	void StartPlayback(const std::string& filepath);
	void StopPlayback();

	void StartRecording();
	void StopRecording();

	void RecordInput();
	void SaveRecordedInput();

	void SetDefaultKeys(const std::unordered_map<std::string, SDL_Scancode>& defaultKeys) const;

	void ResetKeysToDefaults();

	void SaveMappingsToFile();

	std::string GetMappedKeyAsString(const std::string& name);
	std::string GetScancodeAsString(SDL_Scancode code);

	void StartUpdate();
	void EndUpdate();

	bool GetKey(const std::string& keyName);
	bool GetKeyPressed(const std::string& keyName);
	bool GetKeyReleased(const std::string& keyName);
};

#endif