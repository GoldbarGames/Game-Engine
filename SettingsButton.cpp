#include "SettingsButton.h"
#include "Game.h"
#include "SoundManager.h"
#include "Editor.h"
#include "Text.h"

SettingsButton::SettingsButton(const std::string& n, const Vector2& pos, Game& game)
{
	name = n;
	position = pos;

	label = neww Text(game.headerFont, name);
	label->SetPosition(position.x - 400, position.y);
	position.x += 400;

	label->GetSprite()->keepPositionRelativeToCamera = true;
	label->GetSprite()->keepScaleRelativeToCamera = true;

	std::vector<std::string> optionNames;

	// Create the options based on the name of the setting
	if (name == "Music Volume")
	{
		optionNames = { "Silent", "Quiet", "Medium", "Loud", "Max Volume" };
	}
	else if (name == "Fullscreen")
	{
		optionNames = { "Windowed", "Fullscreen" };
	}
	else if (name == "Screen Resolution")
	{
		//TODO: How to support resolutions with a 4:3 aspect ratio, black bars?
		optionNames = { "640 x 360", "1280 x 720", "1600 x 900", "1920 x 1080" };
	}
	else if (name == "Vsync")
	{
		optionNames = { "None", "Synced" };
		//optionNames = { "None", "Synced", "Adaptive" };
	}
	else if (name == "Sound Volume")
	{
		optionNames = { "Silent", "Quiet", "Medium", "Loud", "Max Volume" };
	}
	else if (name == "Display FPS")
	{
		optionNames = { "Off", "On" };
	}
	else if (name == "Display Timer")
	{
		optionNames = { "Off", "On" };
	}
	else if (name == "Language")
	{
		optionNames = { "English" }; //, "Japanese"	
	}
	else if (name == "UI Size")
	{
		optionNames = { "Tiny", "Small", "Medium", "Big", "Large" };
	}
	else if (name == "Replacing") // When placing a tile, should it overwrite the old one?
	{
		optionNames = { "Don't Overwrite", "Overwrite" };
	}
	else if (name == "Deleting") // When deleting, delete only on that layer, or from the front?
	{
		optionNames = { "Same Layer & Mode", "Only Same Layer", "Only Same Mode", "Anything" };
	}
	else if (name == "Button Color") // What is the default color for buttons?
	{
		optionNames = { "Gray", "Red", "Green", "Blue" };
	}

	// Actually create all of the text items for each option
	for (int i = 0; i < optionNames.size(); i++)
	{
		Text* text = neww Text(game.headerFont, optionNames[i]);
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
		//TODO: There might be a better way to do this?
		game.soundManager.SetVolumeBGM(selectedOption);
	}
	else if (name == "Fullscreen")
	{
		if (selectedOption == 0)
			SDL_SetWindowFullscreen(game.window, 0);
		else //TODO: Should we figure out how to use FULLSCREEN_DESKTOP?
			SDL_SetWindowFullscreen(game.window, SDL_WINDOW_FULLSCREEN);

		game.isFullscreen = selectedOption;
	}
	else if (name == "Screen Resolution")
	{		
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
	else if (name == "Sound Volume")
	{
		game.soundManager.SetVolumeSound(selectedOption);
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

		//TODO: Make sure this works properly with other languages
		//TODO: Maybe use an unordered map to set the color
		//TODO: Maybe use a color palette rather than coloring each button

	}

	game.SaveSettings();
}