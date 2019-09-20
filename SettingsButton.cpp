#include "SettingsButton.h"
#include "Game.h"


SettingsButton::SettingsButton(std::string n, Vector2 pos, Game& game)
{
	name = n;
	position = pos;

	label = new Text(game.renderer, game.headerFont, name);
	label->SetPosition(position.x - 400, position.y);

	if (name == "Music Volume")
	{
		std::vector<std::string> volumeOptions = { "Silent", "Quiet", "Medium", "Loud", "Max Volume" };

		for (int i = 0; i < volumeOptions.size(); i++)
		{
			Text* text = new Text(game.renderer, game.headerFont, volumeOptions[i]);
			text->SetPosition(position.x, position.y);
			options.emplace_back(text);
		}
	}
	else if (name == "Screen Resolution")
	{
		std::vector<std::string> screenOptions = { "Windowed", "Fullscreen" };

		for (int i = 0; i < screenOptions.size(); i++)
		{
			Text* text = new Text(game.renderer, game.headerFont, screenOptions[i]);
			text->SetPosition(position.x, position.y);
			options.emplace_back(text);
		}
	}
}


SettingsButton::~SettingsButton()
{

}

void SettingsButton::Render(Renderer* renderer)
{
	label->Render(renderer);
	options[selectedOption]->Render(renderer);	
}

void SettingsButton::SetOptionColors(Color color)
{
	for (int i = 0; i < options.size(); i++)
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
			buttonPressedUp->SetOptionColors({ 255, 255, 0, 255 });
			return buttonPressedUp;
		}
	}
	else if (currentKeyStates[SDL_SCANCODE_DOWN] || currentKeyStates[SDL_SCANCODE_S])
	{
		if (buttonPressedDown != nullptr)
		{
			SetOptionColors({ 255, 255, 255, 255 });
			buttonPressedDown->SetOptionColors({ 255, 255, 0, 255 });
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
		game.soundManager->SetVolumeBGM(selectedOption);
	}
	else if (name == "Screen Resolution")
	{
		if (selectedOption == 0)
			SDL_SetWindowFullscreen(game.window, 0);
		else
			SDL_SetWindowFullscreen(game.window, SDL_WINDOW_FULLSCREEN_DESKTOP);
	}

	game.SaveSettings();
}