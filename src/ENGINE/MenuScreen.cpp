#include "MenuScreen.h"
#include "SettingsButton.h"
#include "Game.h"
#include "globals.h"
#include "Renderer.h"
#include "Editor.h"
#include "FileManager.h"

MenuAnimation::MenuAnimation(Entity* e)
{
	entity = e;

	MenuAnimKeyframe* initialFrame = new MenuAnimKeyframe();
	initialFrame->targetPosition = e->position;
	initialFrame->targetColor = glm::vec4(e->color.r, e->color.g, e->color.b, e->color.a);

	keyframes.emplace_back(initialFrame);
}

MenuAnimKeyframe::MenuAnimKeyframe()
{

}

MenuAnimKeyframe* MenuAnimation::CreateKeyframe(uint32_t duration)
{
	MenuAnimKeyframe* keyframe = new MenuAnimKeyframe(keyframes.back(), duration);
	keyframes.emplace_back(keyframe);
	return keyframe;
}

MenuAnimKeyframe::MenuAnimKeyframe(MenuAnimKeyframe* p, uint32_t d)
{
	previousFrame = p;
	duration = d;

	// Set properties to the same as the previous frame
	targetPosition = p->targetPosition;
	targetColor = p->targetColor;
}

void MenuAnimKeyframe::CalculateTime()
{
	if (previousFrame != nullptr)
		time = previousFrame->time + duration;
	else
		time = duration + Globals::CurrentTicks;
}

void MenuAnimKeyframe::Update(Entity* entity, uint32_t currentTime)
{
	// Can't update an entity that doesn't exist
	if (entity == nullptr)
		return;

	glm::vec3 newPosition = entity->position;

	LerpVector3(newPosition, previousFrame->targetPosition, targetPosition,
		currentTime, previousFrame->time, time);

	entity->SetPosition(newPosition);

	glm::vec4 newColorV4 = glm::vec4(entity->color.r, entity->color.g, entity->color.b, entity->color.a);

	LerpVector4(newColorV4, previousFrame->targetColor, targetColor,
		currentTime, previousFrame->time, time);

	Color newColor = { (uint8_t)newColorV4.r, (uint8_t)newColorV4.g, (uint8_t)newColorV4.b, (uint8_t)newColorV4.a };

	entity->SetColor(newColor);
}

MenuAnimation* MenuScreen::CreateEnterAnimation(Entity* entity)
{
	MenuAnimation* animation = new MenuAnimation(entity);
	enterAnimation.emplace_back(animation);
	return animation;
}

MenuAnimation* MenuScreen::CreateExitAnimation(Entity* entity)
{
	MenuAnimation* animation = new MenuAnimation(entity);
	exitAnimation.emplace_back(animation);
	return animation;
}

MenuScreen::MenuScreen(const std::string& n, Game& game)
{
	name = n;	
}

void MenuScreen::CreateMenu(const std::string& n, Game& game)
{

}

Entity* MenuScreen::AddImage(const std::string& filepath, const glm::vec3& pos, 
	const glm::vec2& scale, const Game& game, const int shader)
{
	Entity* image = new Entity(pos);
	image->GetSprite()->SetTexture(game.spriteManager.GetImage(filepath));

	if (shader > -1)
	{
		image->GetSprite()->SetShader(game.renderer.shaders[shader]);
	}

	image->SetScale(scale);
	image->GetSprite()->keepPositionRelativeToCamera = true;
	image->GetSprite()->keepScaleRelativeToCamera = true;
	images.emplace_back(image);
	return image;
}

MenuButton* MenuScreen::AddButton(const std::string& txt, const std::string& filepath,
	const int btnID, const glm::vec3& pos, Game& game, Color col)
{
	MenuButton* button = new MenuButton(txt, filepath, "", pos, game, col);
	button->btnID = btnID;
	buttons.emplace_back(button);
	return button;
}

// IMPORTANT: When center is true, pass in game.screenWidth as x
Text* MenuScreen::AddText(FontInfo* font, const std::string& message,
	int x, int y, float sx, float sy, bool center)
{
	Text* text = new Text(font, message, true, true);

	if (center)
	{
		int cx = x - (text->GetTextWidth() / 2);
		text->SetPosition(cx, y);
	}
	else
	{
		text->SetPosition(x, y);
	}


	text->SetScale(glm::vec2(sx, sy));
	texts.emplace_back(text);
	
	return text;
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
void MenuScreen::AssignButtons(bool useLeftRight, bool useUpDown)
{
	for (unsigned int i = 0; i < buttons.size(); i++)
	{
		int prevIndex = i - 1;
		int nextIndex = i + 1;

		if (prevIndex < 0)
			prevIndex = buttons.size() - 1;

		if (nextIndex >= buttons.size())
			nextIndex = 0;

		if (useLeftRight && useUpDown)
		{
			buttons[i]->SetButtonsUpDownLeftRight(buttons[prevIndex], buttons[nextIndex],
				buttons[prevIndex], buttons[nextIndex]);
		}
		else if (!useLeftRight && useUpDown)
		{
			buttons[i]->SetButtonsUpDownLeftRight(buttons[prevIndex], buttons[nextIndex],
				nullptr, nullptr);
		}
		else if (useLeftRight && !useUpDown)
		{
			buttons[i]->SetButtonsUpDownLeftRight(nullptr, nullptr,
				buttons[prevIndex], buttons[nextIndex]);
		}
		else
		{
			buttons[i]->SetButtonsUpDownLeftRight(nullptr, nullptr,
				nullptr, nullptr);
		}

	}
}


void MenuScreen::ResetMenu()
{
	for (int i = 0; i < buttons.size(); i++)
	{
		if (rememberLastButton && buttons[i] == selectedButton)
			lastButtonIndex = i;

		if (buttons[i] != nullptr)
			delete_it(buttons[i]);
	}
	buttons.clear();

	for (int i = 0; i < texts.size(); i++)
	{
		if (texts[i] != nullptr)
			delete_it(texts[i]);
	}
	texts.clear();

	for (int i = 0; i < images.size(); i++)
	{
		if (images[i] != nullptr)
			delete_it(images[i]);
	}
	images.clear();

	for (auto& anim : enterAnimation)
	{
		for (auto& keyframe : anim->keyframes)
		{
			if (keyframe != nullptr)
				delete_it(keyframe);
		}
		if (anim != nullptr)
			delete_it(anim);
	}
	enterAnimation.clear();

	for (auto& anim : exitAnimation)
	{
		for (auto& keyframe : anim->keyframes)
		{
			if (keyframe != nullptr)
				delete_it(keyframe);
		}
		if (anim != nullptr)
			delete_it(anim);
	}
	exitAnimation.clear();
}

MenuScreen::~MenuScreen()
{
	ResetMenu();
}

void MenuScreen::HighlightSelectedButton(Game& game)
{
	if (selectedButton != nullptr)
		selectedButton->Highlight(game);
}

void MenuScreen::UnhighlightSelectedButton(Game& game)
{
	if (selectedButton != nullptr)
		selectedButton->Unhighlight(game);
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
	lastButton = selectedButton;

	// Don't crash if there is no button in this menu
	if (selectedButton == nullptr)
		return false;

	selectedButton->isSelected = false;
	selectedButton = selectedButton->Update(game, currentKeyStates);
	selectedButton->isSelected = true;

	if (selectedButton != lastButton)
	{
		selectedButton->Highlight(game);
		lastButton->Unhighlight(game);
	}

	return (lastButton->pressedAnyKey);
}

bool MenuScreen::PressSelectedButton(Game& game)
{
	if (selectedButton == nullptr)
		return false;

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