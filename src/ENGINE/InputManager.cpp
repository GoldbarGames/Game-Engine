#include "InputManager.h"
#include "Camera.h"
#include "Game.h"
#include <iostream>

void InputManager::Init()
{
	std::vector<std::string> keyboardLines;
	std::vector<std::string> controllerLines;

	// Check for new merged input.config format (contains *keyboard* section)
	if (FileExists("data/config/input.config"))
	{
		std::vector<std::string> lines = ReadStringsFromFile("data/config/input.config");
		std::string currentSection = "";

		for (const auto& line : lines)
		{
			if (line == "*keyboard*")
			{
				currentSection = "keyboard";
			}
			else if (line == "*controller*")
			{
				currentSection = "controller";
			}
			else if (!line.empty())
			{
				if (currentSection == "keyboard")
					keyboardLines.emplace_back(line);
				else if (currentSection == "controller")
					controllerLines.emplace_back(line);
			}
		}
	}
	else
	{
		// Fall back to old separate files
		keyboardLines = ReadStringsFromFile("data/config/keyboard.config");
		controllerLines = ReadStringsFromFile("data/config/controller.config");
	}

	// Populate the keyboard keys
	for (size_t i = 0; i < keyboardLines.size(); i++)
	{
		if (keyboardLines[i] != "")
		{
			size_t k = 0;
			int code = std::stoi(ParseWord(keyboardLines[i], ' ', k));
			std::string name = keyboardLines[i].substr(k, keyboardLines[i].size() - k);
			keys[name].mappedKey = (SDL_Scancode)code;
		}
	}

	// Populate the controller buttons
	for (size_t i = 0; i < controllerLines.size(); i++)
	{
		if (controllerLines[i] != "")
		{
			size_t k = 0;
			int code = std::stoi(ParseWord(controllerLines[i], ' ', k));
			std::string name = controllerLines[i].substr(k, controllerLines[i].size() - k);
			buttons[name].mappedButton = (SDL_GameControllerButton)code;
		}
	}

}

void InputManager::StopPlayback()
{
	isPlayingBackInput = false;
	playbackInputs.clear();
	playbackIndex = 0;
}

void InputManager::StartPlayback(const std::string& filepath)
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

void InputManager::StartRecording()
{
	inputsThisFrame.clear();
	inputsLastFrame.clear();
	inputsLastFrame[0] = 1;
	isRecordingInput = true;
	recordedInputs.clear();
	recordedInputs.reserve(10000);
	nCount = 0;
}

void InputManager::StopRecording()
{
	inputsThisFrame.clear();
	inputsLastFrame.clear();
	isRecordingInput = false;
	SaveRecordedInput();
	recordedInputs.clear();
}

void InputManager::RecordInput()
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

void InputManager::SaveRecordedInput()
{
	std::ofstream fout;

	// Try new replays folder first, fall back to old config location
	if (FileExists("data/replays/inputs.dat") || !FileExists("data/config/inputs.dat"))
	{
		fout.open("data/replays/inputs.dat");
	}
	else
	{
		fout.open("data/config/inputs.dat");
	}

	for (const auto& input : recordedInputs)
	{
		fout << input << " ";
	}

	fout.close();
}

void InputManager::SetDefaultKeys(const std::unordered_map<std::string, SDL_Scancode>& defaultKeys) const
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

void InputManager::ResetKeysToDefaults()
{
	for (auto& [key, val] : keys)
	{
		val.mappedKey = val.defaultKey;
	}
}

void InputManager::SaveKeyMappingsToFile()
{
	std::ofstream fout;

	// Check if using new merged input.config format
	if (FileExists("data/config/input.config"))
	{
		// Save both keyboard and controller to merged format
		fout.open("data/config/input.config");
		if (fout.is_open())
		{
			fout << "*keyboard*" << std::endl;
			for (const auto& [key, val] : keys)
			{
				fout << val.mappedKey << " " << key << std::endl;
			}
			fout << std::endl;
			fout << "*controller*" << std::endl;
			for (const auto& [key, val] : buttons)
			{
				fout << (int)val.mappedButton << " " << key << std::endl;
			}
			fout.close();
		}
		else
		{
			std::cout << "ERROR: Could not save key mappings to file" << std::endl;
		}
	}
	else
	{
		// Fall back to old separate file format
		fout.open("data/config/keyboard.config");
		if (fout.is_open())
		{
			for (const auto& [key, val] : keys)
			{
				fout << val.mappedKey << " " << key << std::endl;
			}
			fout.close();
		}
		else
		{
			std::cout << "ERROR: Could not save keyboard mappings to file" << std::endl;
		}
	}
}

void InputManager::SetDefaultButtons(const std::unordered_map<std::string, uint8_t>& defaultButtons) const
{
	// Make sure that if any key mappings are missing from the config file,
	// that we put them in our list of key mappings anyway
	for (const auto& [key, val] : defaultButtons)
	{
		if (buttons.count(key) == 0)
		{
			buttons[key].mappedButton = val;
		}
		buttons[key].defaultButton = val;
	}
}

void InputManager::ResetButtonsToDefaults()
{
	for (auto& [key, val] : buttons)
	{
		val.mappedButton = val.defaultButton;
	}
}

void InputManager::SaveButtonMappingsToFile()
{
	std::ofstream fout;

	// Check if using new merged input.config format
	if (FileExists("data/config/input.config"))
	{
		// Save both keyboard and controller to merged format
		fout.open("data/config/input.config");
		if (fout.is_open())
		{
			fout << "*keyboard*" << std::endl;
			for (const auto& [key, val] : keys)
			{
				fout << val.mappedKey << " " << key << std::endl;
			}
			fout << std::endl;
			fout << "*controller*" << std::endl;
			for (const auto& [key, val] : buttons)
			{
				fout << (int)val.mappedButton << " " << key << std::endl;
			}
			fout.close();
		}
		else
		{
			std::cout << "ERROR: Could not save button mappings to file" << std::endl;
		}
	}
	else
	{
		// Fall back to old separate file format
		fout.open("data/config/controller.config");
		if (fout.is_open())
		{
			for (const auto& [key, val] : buttons)
			{
				fout << (int)val.mappedButton << " " << key << std::endl;
			}
			fout.close();
		}
		else
		{
			std::cout << "ERROR: Could not save button mappings to file" << std::endl;
		}
	}
}

std::string InputManager::GetMappedKeyAsString(const std::string& name)
{
	return GetScancodeAsString(keys[name].mappedKey);
}

std::string InputManager::GetScancodeAsString(SDL_Scancode code)
{
	return SDL_GetScancodeName(code);
}

std::string InputManager::GetMappedButtonAsString(const std::string& name)
{
	return GetButtonEventAsString(buttons[name].mappedButton);
}

std::string InputManager::GetButtonEventAsString(uint8_t code)
{
	return "";
}

const glm::vec3 InputManager::GetMouseWorldPosition() const
{
	int clickedX = mouseX - ((int)mouseX % (Globals::TILE_SIZE));
	int clickedY = mouseY - ((int)mouseY % (Globals::TILE_SIZE));

	return glm::vec3(clickedX, clickedY, 0);
}

void InputManager::StartUpdate()
{
	previousMouseState = mouseState;
	mouseState = SDL_GetMouseState(&mouseX, &mouseY);

	holdingLeft = (mouseState & SDL_BUTTON(SDL_BUTTON_LEFT));
	holdingRight = (mouseState & SDL_BUTTON(SDL_BUTTON_RIGHT));
	holdingMiddle = (mouseState & SDL_BUTTON(SDL_BUTTON_MIDDLE));

	pressedLeft = !(previousMouseState & SDL_BUTTON(SDL_BUTTON_LEFT));
	pressedRight = !(previousMouseState & SDL_BUTTON(SDL_BUTTON_RIGHT));
	pressedMiddle = !(previousMouseState & SDL_BUTTON(SDL_BUTTON_MIDDLE));

	releasedLeft = !holdingLeft && !pressedLeft;
	releasedRight = !holdingRight && !pressedRight;
	releasedMiddle = !holdingMiddle && !pressedMiddle;

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
			for (size_t i = playbackIndex; i < playbackInputs.size(); i++)
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

void InputManager::EndUpdate()
{
	const uint8_t* input = SDL_GetKeyboardState(NULL);
	for (auto& [key, val] : keys)
	{
		val.previousState = input[val.mappedKey];
	}
}

bool InputManager::GetKey(const std::string& keyName)
{
	if (isPlayingBackInput)
	{
		return inputsThisFrame.count(keys[keyName].mappedKey) != 0;
	}
	else
	{
		const uint8_t* input = SDL_GetKeyboardState(NULL);
		bool checkKey = input[keys[keyName].mappedKey];
		bool checkController = false;

		return checkKey || checkController;
	}
}

bool InputManager::GetKeyPressed(const std::string& keyName)
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
		bool checkKeyboard = input[keys[keyName].mappedKey] && !keys[keyName].previousState;
		bool checkController = buttonsPressed[buttons[keyName].mappedButton];

		return checkKeyboard || checkController;
	}
}

bool InputManager::GetKeyReleased(const std::string& keyName)
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
		bool checkKey = !input[keys[keyName].mappedKey] && keys[keyName].previousState;
		bool checkController = buttonsReleased[buttons[keyName].mappedButton];

		return checkKey || checkController;
	}
}

const bool InputManager::GetLeftClicked() const
{
	int mouseX, mouseY;
	const uint32_t mouseState = SDL_GetMouseState(&mouseX, &mouseY);

	return (mouseState & SDL_BUTTON(SDL_BUTTON_LEFT));
}

glm::vec3 InputManager::GetMouseWorldPos(const Game& game) const
{
	return game.ConvertFromScreenSpaceToWorldSpace(glm::vec2(mouseX, mouseY));
}