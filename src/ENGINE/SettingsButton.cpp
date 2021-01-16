#include "SettingsButton.h"
#include "Game.h"
#include "SoundManager.h"
#include "Editor.h"
#include "Text.h"

SettingsButton::SettingsButton(const std::string& n, const Vector2& pos, Game& game, bool isKeyMap)
{
	name = n;
	position = pos;

	label = new Text(game.headerFont, name);
	label->SetPosition(position.x - 400, position.y);
	position.x += 400;

	label->GetSprite()->keepPositionRelativeToCamera = true;
	label->GetSprite()->keepScaleRelativeToCamera = true;

	std::vector<std::string> optionNames;

	std::vector<std::string> settingsNames = ReadStringsFromFile("data/settings.names");

	bool foundOptionInList = false;

	for (int i = 0; i < settingsNames.size(); i++)
	{
		int index = 0;
		std::string optName = ParseWord(settingsNames[i], ':', index);

		if (name == optName)
		{
			foundOptionInList = true;
			std::string word = "";
			while (index < settingsNames[i].size())
			{
				if (settingsNames[i][index] == ',')
				{
					optionNames.emplace_back(Trim(word));
					word = "";
				}
				else
				{
					word += settingsNames[i][index];
				}
				index++;
			}
			optionNames.emplace_back(Trim(word));
		}
	}

	if (!foundOptionInList)
	{
		if (isKeyMap)
		{
			isKeyMapButton = isKeyMap;
			optionNames = { game.inputManager.GetMappedKeyAsString(name) };
		}
		else // always make sure there is at least one option
		{
			optionNames = { "" };
		}
	}

	// Actually create all of the text items for each option
	for (int i = 0; i < optionNames.size(); i++)
	{
		Text* text = new Text(game.headerFont, optionNames[i]);
		text->SetPosition(position.x, position.y);
		text->GetSprite()->keepPositionRelativeToCamera = true;
		text->GetSprite()->keepScaleRelativeToCamera = true;
		options.emplace_back(text);
	}
}


SettingsButton::~SettingsButton()
{
	for (int i = 0; i < options.size(); i++)
	{
		if (options[i] != nullptr)
			delete_it(options[i]);
	}

	if (label != nullptr)
		delete_it(label);
}

void SettingsButton::Render(const Renderer& renderer)
{
	label->Render(renderer);
	options[selectedOption]->Render(renderer);
}

void SettingsButton::SetOptionColors(Color color)
{
	for (unsigned int i = 0; i < options.size(); i++)
	{
		options[i]->SetText(options[i]->txt, color);
	}
}

BaseButton* SettingsButton::Update(Game& game, const Uint8* currentKeyStates)
{
	pressedAnyKey = true;

	if (isKeyMapButton && game.inputManager.inputTimer.HasElapsed())
	{
		if (game.inputManager.isCheckingForKeyMapping)
		{
			// If a button is pressed
			if (game.inputManager.pressedKey != SDL_SCANCODE_UNKNOWN)
			{
				// Set the mapping to the pressed button for this action
				game.inputManager.keys[name].mappedKey = game.inputManager.pressedKey;

				// Set the options text to the new key mapping
				options[0]->SetText(game.inputManager.GetMappedKeyAsString(name));

				game.inputManager.pressedKey = SDL_SCANCODE_UNKNOWN;

				game.inputManager.isCheckingForKeyMapping = false;

				game.inputManager.SaveMappingsToFile();

				game.inputManager.inputTimer.Start(500);

				SetOptionColors({ 0, 255, 0, 255 });

				return this;
			}

		}
		else
		{
			if (currentKeyStates[SDL_SCANCODE_SPACE] || currentKeyStates[SDL_SCANCODE_RETURN])
			{
				options[0]->SetText("Press Any Key");
				game.inputManager.isCheckingForKeyMapping = true;
				SetOptionColors({ 0, 255, 0, 255 });
				return this;
			}
		}
	}

	if (currentKeyStates[SDL_SCANCODE_UP] || currentKeyStates[SDL_SCANCODE_W])
	{
		if (buttonPressedUp != nullptr)
		{
			SetOptionColors({ 255, 255, 255, 255 });
			buttonPressedUp->SetOptionColors({ 0, 255, 0, 255 });
			return buttonPressedUp;
		}
	}
	else if (currentKeyStates[SDL_SCANCODE_DOWN] || currentKeyStates[SDL_SCANCODE_S])
	{
		if (buttonPressedDown != nullptr)
		{
			SetOptionColors({ 255, 255, 255, 255 });
			buttonPressedDown->SetOptionColors({ 0, 255, 0, 255 });
			return buttonPressedDown;
		}
	}
	else if (currentKeyStates[SDL_SCANCODE_LEFT] || currentKeyStates[SDL_SCANCODE_A])
	{
		selectedOption--;
		if (selectedOption < 0)
			selectedOption = 0;
		ExecuteSelectedOption(game);
		return this;
	}
	else if (currentKeyStates[SDL_SCANCODE_RIGHT] || currentKeyStates[SDL_SCANCODE_D])
	{
		selectedOption++;
		if (selectedOption > options.size() - 1)
			selectedOption = options.size() - 1;
		ExecuteSelectedOption(game);
		return this;
	}
	

	pressedAnyKey = false;

	return this;
}

void SettingsButton::ExecuteSelectedOption(Game& game)
{
	if (name == "Music Volume")
	{
		game.soundManager.SetVolumeBGMIndex(selectedOption);
	}
	else if (name == "Sound Volume")
	{
		game.soundManager.SetVolumeSoundIndex(selectedOption);
	}
	else if (name == "Fullscreen")
	{
		game.SetFullScreen(selectedOption != 0);		
	}
	else if (name == "Screen Resolution")
	{		
		// TODO: Find a way to customize screen resolutions
		// depending on the player's monitor resolution

		// TODO: How to support resolutions with a 4:3 aspect ratio, black bars?

		game.indexScreenResolution = selectedOption;
		switch (selectedOption)
		{
			case 0:
				game.SetScreenResolution(640, 360);
				break;
			case 1:
				game.SetScreenResolution(1280, 720);
				break;
			case 2:
				game.SetScreenResolution(1600, 900);
				break;
			case 3:
				game.SetScreenResolution(1920, 1080);
				break;
			default:
				break;
		}
	}
	else if (name == "Vsync")
	{ 
		SDL_GL_SetSwapInterval(selectedOption);
	}
	else if (name == "Display FPS")
	{
		game.showFPS = (selectedOption == 1);
	}
	else if (name == "Display Timer")
	{
		game.showTimer = (selectedOption == 1);
	}
	else if (name == "Language")
	{
		//TODO: Deal with this when we implement translations
	}
	else if (name == "UI Size")
	{
		//TODO: Use a static variable or something to scale all menu sprites
	}
	else if (name == "Replacing")
	{
		game.editor->replaceSettingIndex = selectedOption;
		game.SaveEditorSettings();
	}
	else if (name == "Deleting")
	{
		game.editor->deleteSettingIndex = selectedOption;
		game.SaveEditorSettings();
	}
	else if (name == "Button Color")
	{
		game.editor->colorSettingIndex = selectedOption;
		game.SaveEditorSettings();

		//TODO: Maybe use an unordered map to set the color
		//TODO: Maybe use a color palette rather than coloring each button
	}

	game.SaveSettings();
}