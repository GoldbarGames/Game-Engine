#include "MenuScreen.h"
#include "SettingsButton.h"
#include "Game.h"
#include "globals.h"

MenuScreen::MenuScreen(std::string n, Game& game)
{
	name = n;

	if (name == "Pause")
	{
		int startWidth = screenWidth / 2;
		int startHeight = 100;
		int distance = 120;

		MenuButton* buttonResume = new MenuButton("Resume", "assets/gui/menu.png",
			"Resume Game", Vector2(startWidth, startHeight + (distance * 0)), game);

		MenuButton* buttonSpellbook = new MenuButton("Spellbook", "assets/gui/menu.png",
			"Spellbook", Vector2(startWidth, startHeight + (distance * 1)), game);

		MenuButton* buttonSettings = new MenuButton("Settings", "assets/gui/menu.png",
			"Settings", Vector2(startWidth, startHeight + (distance * 2)), game);

		MenuButton* buttonExit = new MenuButton("Title Screen", "assets/gui/menu.png",
			"Title Screen", Vector2(startWidth, startHeight + (distance * 3)), game);

		buttonResume->SetButtonsUpDownLeftRight(buttonExit, buttonSpellbook, buttonExit, buttonSpellbook);
		buttonSpellbook->SetButtonsUpDownLeftRight(buttonResume, buttonSettings, buttonResume, buttonSettings);
		buttonSettings->SetButtonsUpDownLeftRight(buttonSpellbook, buttonExit, buttonSpellbook, buttonExit);
		buttonExit->SetButtonsUpDownLeftRight(buttonSettings, buttonResume, buttonSettings, buttonResume);

		buttons.emplace_back(buttonResume);		
		buttons.emplace_back(buttonSpellbook);		
		buttons.emplace_back(buttonSettings);		
		buttons.emplace_back(buttonExit);
	}
	else if (name == "Title")
	{
		int startWidth = screenWidth / 2;
		int startHeight = 200;
		int distance = 120;

		int buttonPosX = 1000;

		//TODO: The only way to center the button is to do startWidth - windowRect.w / 2,
		// but I can't get windowRect.x here because the rect has not been created yet!

		//TODO: Also, the buttons are too tall. Either replace the image, or add an easy way to scale them

		MenuButton* buttonPlay = new MenuButton("Play Game", "assets/gui/menu.png",
			"Play Game", Vector2(buttonPosX, startHeight + (distance * 0)), game);

		MenuButton* buttonSettings = new MenuButton("Settings", "assets/gui/menu.png",
			"Settings", Vector2(buttonPosX, startHeight + (distance * 1)), game);

		MenuButton* buttonExit = new MenuButton("Exit", "assets/gui/menu.png",
			"Exit Game", Vector2(buttonPosX, startHeight + (distance * 2)), game);

		buttonPlay->SetButtonsUpDownLeftRight(buttonExit, buttonSettings, buttonExit, buttonSettings);
		buttonSettings->SetButtonsUpDownLeftRight(buttonPlay, buttonExit, buttonPlay, buttonExit);
		buttonExit->SetButtonsUpDownLeftRight(buttonSettings, buttonPlay, buttonSettings, buttonPlay);

		buttons.emplace_back(buttonPlay);
		buttons.emplace_back(buttonSettings);
		buttons.emplace_back(buttonExit);

		Text* textCopyright = new Text(game.renderer, game.headerFont, "Copyright 2019 Goldbar Games LLC");
		textCopyright->SetPosition(startWidth - (textCopyright->textWindowRect.w / 2), 600);
		texts.emplace_back(textCopyright);

		Entity* titleCharacter = new Entity(Vector2(-200, -200));
		titleCharacter->SetSprite(new Sprite(0, 0, 1, game.spriteManager, "assets/gui/wdk_character.png", game.renderer, Vector2(222, 370), false));
		images.emplace_back(titleCharacter);

		Entity* titleLogo = new Entity(Vector2(startWidth - 320, 100));
		titleLogo->SetSprite(new Sprite(0, 0, 1, game.spriteManager, "assets/gui/wdk_logo.png", game.renderer, Vector2(320, 137), false));
		images.emplace_back(titleLogo);
	}
	else if (name == "Settings")
	{
		int startWidth = screenWidth / 2;
		int startHeight = 100;
		int distance = 60;

		int buttonPosX = (screenWidth / 2);

		Text* header = new Text(game.renderer, game.headerFont, "Settings");
		header->SetPosition(startWidth - (header->textWindowRect.w / 2), startHeight);
		texts.emplace_back(header);

		std::vector<string> buttonNames = { "Music Volume", "Sound Volume", "Screen Resolution",
		"Display FPS", "Display Timer", "Vsync", "Language" };

		for (unsigned int i = 0; i < buttonNames.size(); i++)
		{
			SettingsButton* button = new SettingsButton(buttonNames[i],
				Vector2(buttonPosX, startHeight + (distance * (i+1))), game);

			buttons.emplace_back(button);
		}

		AssignButtons();

		// Highlight the selected option
		//TODO: Is there a better way than hard-coding it?
		buttons[0]->SetOptionColors({ 255, 255, 0, 255 });
	}
	else if (name == "EditorSettings")
	{
		int startWidth = screenWidth / 2;
		int startHeight = 100;
		int distance = 60;

		int buttonPosX = (screenWidth / 2);

		Text* header = new Text(game.renderer, game.headerFont, "Settings");
		header->SetPosition(startWidth - (header->textWindowRect.w / 2), startHeight);
		texts.emplace_back(header);

		std::vector<string> buttonNames = { "Replacing", "Deleting", "Button Color" };

		for (unsigned int i = 0; i < buttonNames.size(); i++)
		{
			SettingsButton* button = new SettingsButton(buttonNames[i],
				Vector2(buttonPosX, startHeight + (distance * (i + 1))), game);

			buttons.emplace_back(button);
		}

		AssignButtons();

		// Highlight the selected option
		//TODO: Is there a better way than hard-coding it?
		buttons[0]->SetOptionColors({ 255, 255, 0, 255 });
	}
	else if (name == "Spellbook")
	{

	}
	else if (name == "File Select")
	{
		int startWidth = screenWidth / 2;
		int startHeight = 200;
		int distance = 120;

		int buttonPosX = (screenWidth / 2);

		//TODO: Replace each text with 'New Game' or 'Load Game' depending on whether there is save data

		std::string file1Function = "Load Game";
		std::string file2Function = "Load Game";
		std::string file3Function = "Load Game";

		MenuButton* buttonFile1 = new MenuButton("File 1", "assets/gui/menu.png",
			file1Function, Vector2(buttonPosX, startHeight + (distance * 0)), game);

		MenuButton* buttonFile2 = new MenuButton("File 2", "assets/gui/menu.png",
			file2Function, Vector2(buttonPosX, startHeight + (distance * 1)), game);

		MenuButton* buttonFile3 = new MenuButton("File 3", "assets/gui/menu.png",
			file3Function, Vector2(buttonPosX, startHeight + (distance * 2)), game);

		buttonFile1->SetButtonsUpDownLeftRight(buttonFile3, buttonFile2, buttonFile3, buttonFile2);
		buttonFile2->SetButtonsUpDownLeftRight(buttonFile1, buttonFile3, buttonFile1, buttonFile3);
		buttonFile3->SetButtonsUpDownLeftRight(buttonFile2, buttonFile1, buttonFile2, buttonFile1);

		buttons.emplace_back(buttonFile1);
		buttons.emplace_back(buttonFile2);
		buttons.emplace_back(buttonFile3);

		Text* textHeader = new Text(game.renderer, game.headerFont, "Select a File");
		textHeader->SetPosition(startWidth - (textHeader->textWindowRect.w / 2), 60);
		texts.emplace_back(textHeader);
	}
	else
	{
		
	}

	// Automatically select the first button in the list
	// TODO: Maybe we want to select a different one?
	if (buttons.size() > 0)
	{
		selectedButton = buttons[0];
		selectedButton->isSelected = true;
	}		
}

//TODO: Maybe rename this to a better name
void MenuScreen::AssignButtons()
{
	for (unsigned int i = 0; i < buttons.size(); i++)
	{
		int prevIndex = i - 1;
		int nextIndex = i + 1;

		if (prevIndex < 0)
			prevIndex = buttons.size() - 1;

		if (nextIndex >= buttons.size())
			nextIndex = 0;

		buttons[i]->SetButtonsUpDownLeftRight(buttons[prevIndex], buttons[nextIndex], nullptr, nullptr);
	}
}


MenuScreen::~MenuScreen()
{

}

void MenuScreen::Render(Renderer* renderer)
{
	for (unsigned int i = 0; i < buttons.size(); i++)
	{
		buttons[i]->Render(renderer);
	}

	for (unsigned int i = 0; i < images.size(); i++)
	{
		images[i]->Render(renderer, Vector2(0,0));
	}

	for (unsigned int i = 0; i < texts.size(); i++)
	{
		texts[i]->Render(renderer);
	}
}

bool MenuScreen::Update(Game& game)
{
	const Uint8* currentKeyStates = SDL_GetKeyboardState(NULL);

	BaseButton* lastButton = selectedButton;

	selectedButton->isSelected = false;
	selectedButton = selectedButton->Update(game, currentKeyStates);
	selectedButton->isSelected = true;

	return (lastButton->pressedAnyKey);
}

bool MenuScreen::PressSelectedButton(Game& game)
{
	if (selectedButton == nullptr)
		return false;

	if (selectedButton->name == "Resume Game")
	{
		game.openedMenus.pop_back();
	}
	else if (selectedButton->name == "Exit Game")
	{
		return true;
	}
	else if (selectedButton->name == "Settings")
	{
		game.openedMenus.clear();
		game.openedMenus.emplace_back(game.allMenus["Settings"]);
	}
	else if (selectedButton->name == "Spellbook")
	{

	}
	else if (selectedButton->name == "Load Game")
	{
		game.PlayLevel("test1");
	}
	else if (selectedButton->name == "Play Game")
	{
		game.openedMenus.clear();
		game.openedMenus.emplace_back(game.allMenus["File Select"]);
	}
	else if (selectedButton->name == "Title Screen")
	{
		game.LoadTitleScreen();
	}

	return false;
}


BaseButton* MenuScreen::GetButtonByName(std::string buttonName)
{
	for (int i = 0; i < buttons.size(); i++)
	{
		if (buttons[i]->name == buttonName)
			return buttons[i];
	}

	return nullptr;
}