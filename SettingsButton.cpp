#include "SettingsButton.h"
#include "Game.h"


SettingsButton::SettingsButton(std::string n, Vector2 pos, Game& game)
{
	name = n;
	position = pos;

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
}


SettingsButton::~SettingsButton()
{

}

void SettingsButton::Render(Renderer* renderer)
{
	// Draw the outline yellow if selected, white if not
	if (isSelected)
	{
		// TODO: Make this a color type that we can swap out
		SDL_SetRenderDrawColor(renderer->renderer, 255, 255, 0, 255); //yellow
	}
	else
	{
		SDL_SetRenderDrawColor(renderer->renderer, 255, 255, 255, 255);
	}

	options[selectedOption]->Render(renderer);	
}

BaseButton* SettingsButton::Update(const Uint8* currentKeyStates)
{
	if (currentKeyStates[SDL_SCANCODE_UP] || currentKeyStates[SDL_SCANCODE_W])
	{
		if (buttonPressedUp != nullptr)
		{
			return buttonPressedUp;
		}
	}
	else if (currentKeyStates[SDL_SCANCODE_DOWN] || currentKeyStates[SDL_SCANCODE_S])
	{
		if (buttonPressedDown != nullptr)
		{
			return buttonPressedDown;
		}
	}
	else if (currentKeyStates[SDL_SCANCODE_LEFT] || currentKeyStates[SDL_SCANCODE_A])
	{
		selectedOption--;
		if (selectedOption < 0)
			selectedOption = 0;
	}
	else if (currentKeyStates[SDL_SCANCODE_RIGHT] || currentKeyStates[SDL_SCANCODE_D])
	{
		selectedOption++;
		if (selectedOption > options.size())
			selectedOption = options.size();
	}

	return this;
}