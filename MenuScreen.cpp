#include "MenuScreen.h"
#include "SettingsButton.h"
#include "Game.h"
#include "globals.h"
#include "Renderer.h"
#include "Editor.h"

MenuScreen::MenuScreen(const std::string& n, Game& game)
{
	name = n;

	if (name == "Pause")
	{
		int distance = 120;
		int startPosX = game.screenWidth;
		int startPosY = game.screenHeight - distance;		

		int endPosY = startPosY + (distance * 5);
		int newStartPosY = startPosY - ((endPosY - startPosY) / 2);

		Text* header = neww Text(game.headerFont, "Game Paused", true, true);
		header->SetPosition(startPosX, newStartPosY);
		header->GetSprite()->SetScale(Vector2(2.0f, 2.0f));
		texts.emplace_back(header);

		MenuButton* buttonResume = neww MenuButton("Resume", "assets/gui/menu.png",
			"Resume Game", Vector2(startPosX, newStartPosY + (distance * 2)), game);

		MenuButton* buttonSpellbook = neww MenuButton("Spellbook", "assets/gui/menu.png",
			"Spellbook", Vector2(startPosX, newStartPosY + (distance * 2)), game);

		MenuButton* buttonSettings = neww MenuButton("Settings", "assets/gui/menu.png",
			"Settings", Vector2(startPosX, newStartPosY + (distance * 3)), game);

		MenuButton* buttonExit = neww MenuButton("Title Screen", "assets/gui/menu.png",
			"Title Screen", Vector2(startPosX, newStartPosY + (distance * 4)), game);

		buttons.emplace_back(buttonResume);		
		//buttons.emplace_back(buttonSpellbook); // COMMENTED OUT FOR DEMO
		buttons.emplace_back(buttonSettings);
		buttons.emplace_back(buttonExit);

		AssignButtons(true);
	}
	else if (name == "Credits")
	{
		int startWidth = game.screenWidth / 2;
		int startHeight = 300;
		int distance = 60;

		MenuButton* buttonBack = neww MenuButton("Back", "assets/gui/menu.png",
			"Go To Title", Vector2(game.screenWidth, 1200), game);

		buttons.emplace_back(buttonBack);

		AssignButtons(true);

		Text* creditsHeader = neww Text(game.headerFont, "CREDITS", true, true);
		creditsHeader->SetPosition(game.screenWidth - (creditsHeader->GetTextWidth() / 2), 100);
		creditsHeader->SetScale(Vector2(2.0f, 2.0f));
		texts.emplace_back(creditsHeader);

		std::vector<string> textLines = { 
			"Programming: Kinjo",
			"Concept Art: Osato", 
			"Pixel Art: TahYllis",
			"Music: Solo Acapello", 
			" ", 
			"Special thanks to our Patreon and Twitch supporters:", 
			" ",
			"Van Kadix, NovelistEzhno, M.E. Hatch, JimmyJDogg,", 
			"Sajo8, Zephilinox, MrJontel, EminentVirtue" };

		for (unsigned int i = 0; i < textLines.size(); i++)
		{
			int offsetX = 400;

			if (i < 4)
				offsetX = 0;

			Text* textLine = neww Text(game.headerFont, textLines[i], true, true);
			textLine->SetPosition(game.screenWidth + offsetX - (textLine->GetTextWidth() / 2),
				startHeight + (distance * (i + 1)));

			textLine->GetSprite()->SetScale(Vector2(1.0f, 1.0f));
			texts.emplace_back(textLine);
		}


	}
	else if (name == "Title")
	{
		int startWidth = game.screenWidth / 2;
		int distance = 120;
		int startPosX = 1600;
		int startPosy = 700;

		MenuButton* buttonPlay = neww MenuButton("Play Game", "assets/gui/menu.png",
			"Play Game", Vector2(startPosX, startPosy + (distance * 0)), game);

		MenuButton* buttonSettings = neww MenuButton("Settings", "assets/gui/menu.png",
			"Settings", Vector2(startPosX, startPosy + (distance * 1)), game);

		MenuButton* buttonCredits = neww MenuButton("Credits", "assets/gui/menu.png",
			"Credits", Vector2(startPosX, startPosy + (distance * 2)), game);

		MenuButton* buttonExit = neww MenuButton("Exit", "assets/gui/menu.png",
			"Exit Game", Vector2(startPosX, startPosy + (distance * 3)), game);

		buttons.emplace_back(buttonPlay);
		buttons.emplace_back(buttonSettings);
		buttons.emplace_back(buttonCredits);
		buttons.emplace_back(buttonExit);

		AssignButtons(true);

		Text* textCopyright = neww Text(game.headerFont, "Copyright 2020 Goldbar Games LLC", true, true);
		//textCopyright->SetPosition(game.screenWidth - (textCopyright->GetTextWidth() / 2), 700);
		textCopyright->SetPosition(startPosX - 400, 1200);
		textCopyright->GetSprite()->SetScale(Vector2(1.0f, 1.0f));
		texts.emplace_back(textCopyright);

		Text* textVersion = neww Text(game.headerFont, "Demo Version 2020-10-31", true, true);
		//textVersion->SetPosition(game.screenWidth - (textCopyright->GetTextWidth() / 2), 700);
		textVersion->SetPosition(startPosX - 100, 1300);
		textVersion->GetSprite()->SetScale(Vector2(1.0f, 1.0f));
		texts.emplace_back(textVersion);

		Entity* titleCharacter = neww Entity(Vector2(600, 350));

		Sprite* newSprite = neww Sprite(0, 0, 1, *game.spriteManager,
			"assets/gui/wdk_character.png", game.renderer->shaders[ShaderName::FadeInOut],
			Vector2(0, 0), false);

		titleCharacter->SetSprite(*newSprite);

		titleCharacter->GetSprite()->SetScale(Vector2(0.5f, 0.5f));
		titleCharacter->GetSprite()->keepPositionRelativeToCamera = true;
		titleCharacter->GetSprite()->keepScaleRelativeToCamera = true;
		images.emplace_back(titleCharacter);

		Entity* titleLogo = neww Entity(Vector2(1600, 350));
		newSprite = neww Sprite(0, 0, 1, *game.spriteManager, "assets/gui/wdk_logo.png",
			game.renderer->shaders[ShaderName::Default], Vector2(0, 0), false);
		titleLogo->SetSprite(*newSprite);
		titleLogo->GetSprite()->SetScale(Vector2(0.25f, 0.25f));
		titleLogo->GetSprite()->keepPositionRelativeToCamera = true;
		titleLogo->GetSprite()->keepScaleRelativeToCamera = true;
		images.emplace_back(titleLogo);
	}
	else if (name == "Settings")
	{
		std::vector<string> buttonNames = { "Music Volume", "Sound Volume", "Fullscreen",
		"Screen Resolution", "Display FPS", "Display Timer", "Vsync", "Language" };

		int distance = 120;
		int startPosX = game.screenWidth;
		int startPosY = game.screenHeight - distance;	

		int endPosY = startPosY + (distance * buttonNames.size());
		int newStartPosY = startPosY - ((endPosY - startPosY) / 2);

		Text* header = neww Text(game.headerFont, "Settings", true, true);
		header->SetPosition(startPosX, newStartPosY);
		header->SetScale(Vector2(2.0f, 2.0f));
		texts.emplace_back(header);

		for (unsigned int i = 0; i < buttonNames.size(); i++)
		{
			SettingsButton* button = neww SettingsButton(buttonNames[i],
				Vector2(startPosX, newStartPosY + (distance * (i+2))), game);

			buttons.emplace_back(button);
		}

		AssignButtons(false);

		// Highlight the selected option
		//TODO: Is there a better way than hard-coding it?
		buttons[0]->SetOptionColors({ 0, 255, 0, 255 });
	}
	else if (name == "EditorSettings")
	{
		int startWidth = game.screenWidth / 2;
		int startHeight = 100;
		int distance = 60;

		int buttonPosX = (game.screenWidth / 2);

		Text* header = neww Text(game.headerFont, "Editor Settings", true, true);
		header->SetPosition(startWidth - (header->GetTextWidth() / 2), startHeight);
		texts.emplace_back(header);

		std::vector<string> buttonNames = { "Replacing", "Deleting", "Button Color" };

		for (unsigned int i = 0; i < buttonNames.size(); i++)
		{
			SettingsButton* button = neww SettingsButton(buttonNames[i],
				Vector2(buttonPosX, startHeight + (distance * (i + 1))), game);

			buttons.emplace_back(button);
		}

		AssignButtons(false);

		// Highlight the selected option
		//TODO: Is there a better way than hard-coding it?
		buttons[0]->SetOptionColors({ 255, 255, 0, 255 });
	}
	else if (name == "Spellbook")
	{

	}
	else if (name == "File Select")
	{
		int startWidth = game.screenWidth / 2;
		int startHeight = 200;
		int distance = 120;

		int buttonPosX = (game.screenWidth / 2);

		// Replace each text with 'New Game' or 'Load Game' 
		// depending on whether there is save data

		const int NUM_FILES = 3;
		std::string filepath, fileFunction;

		for (int i = 1; i <= 3; i++)
		{
			filepath = "saves/wdk" + std::to_string(i) + ".sav";
			fileFunction = FileExists(filepath) ? "File " + std::to_string(i) : "New Game";

			MenuButton* buttonFile = neww MenuButton(fileFunction, "assets/gui/menu.png",
				fileFunction, Vector2(buttonPosX, startHeight + (distance * (i-1))), game);

			buttons.emplace_back(buttonFile);
		}

		AssignButtons(true);

		Text* textHeader = neww Text(game.headerFont, "Select a File");
		textHeader->SetPosition(startWidth - (textHeader->GetTextWidth() / 2), 60);
		textHeader->GetSprite()->keepPositionRelativeToCamera = true;
		textHeader->GetSprite()->keepScaleRelativeToCamera = true;
		texts.emplace_back(textHeader);
	}

	// Automatically select the first button in the list
	// TODO: Maybe we want to select a different one?
	if (buttons.size() > 0)
	{
		selectedButton = buttons[0];
		selectedButton->isSelected = true;
		if (selectedButton->image != nullptr)
			selectedButton->image->SetShader(game.renderer->shaders[ShaderName::Glow]);
	}		
}

bool MenuScreen::FileExists(const std::string& filepath)
{
	std::fstream fin;
	fin.open(filepath);
	if (!fin.good())
		return false;
	else
		fin.close();
	return true;
}

//TODO: Maybe rename this to a better name
void MenuScreen::AssignButtons(bool useLeftRight)
{
	for (unsigned int i = 0; i < buttons.size(); i++)
	{
		int prevIndex = i - 1;
		int nextIndex = i + 1;

		if (prevIndex < 0)
			prevIndex = buttons.size() - 1;

		if (nextIndex >= buttons.size())
			nextIndex = 0;

		if (useLeftRight)
			buttons[i]->SetButtonsUpDownLeftRight(buttons[prevIndex], buttons[nextIndex], 
				buttons[prevIndex], buttons[nextIndex]);
		else
			buttons[i]->SetButtonsUpDownLeftRight(buttons[prevIndex], buttons[nextIndex], 
				nullptr, nullptr);
	}
}


MenuScreen::~MenuScreen()
{

}

void MenuScreen::Render(const Renderer& renderer)
{
	for (unsigned int i = 0; i < images.size(); i++)
	{
		images[i]->Render(renderer);
	}

	for (unsigned int i = 0; i < buttons.size(); i++)
	{
		buttons[i]->Render(renderer);
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

	if (selectedButton != lastButton)
	{
		if (selectedButton->image != nullptr)
			selectedButton->image->SetShader(game.renderer->shaders[ShaderName::Glow]);
		if (lastButton->image != nullptr)
			lastButton->image->SetShader(game.renderer->shaders[ShaderName::Default]);
	}

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
	else if (selectedButton->name == "Credits")
	{
		game.openedMenus.clear();
		game.openedMenus.emplace_back(game.allMenus["Credits"]);
	}
	else if (selectedButton->name == "Spellbook")
	{

	}
	else if (selectedButton->name == "Load Game")
	{		
		game.currentSaveFileName = "wdk1.sav";
		game.LoadFile(game.currentSaveFileName);
	}
	else if (selectedButton->name == "File 1")
	{
		game.currentSaveFileName = "wdk1.sav";
		game.LoadFile(game.currentSaveFileName);
	}
	else if (selectedButton->name == "File 2")
	{
		game.currentSaveFileName = "wdk2.sav";
		game.LoadFile(game.currentSaveFileName);
	}
	else if (selectedButton->name == "File 3")
	{
		game.currentSaveFileName = "wdk3.sav";
		game.LoadFile(game.currentSaveFileName);
	}
	else if (selectedButton->name == "New Game")
	{
		game.openedMenus.clear();

		//game.cutscene->commands.ExecuteCommand("fade black 1000");
		//game.cutscene->commands.ExecuteCommand("wait 1000");

		std::string currentGame = "DB1";

		if (currentGame == "WDK")
		{
			//game.LoadLevel("demo", 1, 1);
#if _DEBUG
			if (game.editor->startEditorLevel != "")
				game.LoadLevel(game.editor->startEditorLevel);
			else
				game.LoadLevel("demo");
#else
			game.LoadLevel("demo");
#endif
		}
		else if (currentGame == "DB1")
		{
			game.LoadLevel("test-vn");
		}
		else if (currentGame == "DB2")
		{
			game.LoadLevel("test-vn");
		}
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
	else if (selectedButton->name == "Go To Title")
	{
		game.openedMenus.clear();
		game.openedMenus.emplace_back(game.allMenus["Title"]);
	}
	else
	{
		std::cout << "ERROR: Selected button name " << selectedButton->name << "not found!" << std::endl;
	}

	return false;
}


BaseButton* MenuScreen::GetButtonByName(const std::string& buttonName)
{
	for (int i = 0; i < buttons.size(); i++)
	{
		if (buttons[i]->name == buttonName)
			return buttons[i];
	}

	return nullptr;
}