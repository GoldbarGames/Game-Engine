#include "EditorButton.h"
#include "Game.h"

EditorButton::EditorButton(std::string txt, std::string filename, Vector2 pos, Game& game, Vector2 size, Color color)
{
	buttonTexture = game.spriteManager->GetImage(game.renderer, "assets/editor/btn" + filename + ".png");
	buttonTextureRect.x = 0;
	buttonTextureRect.y = 0;

	buttonColor = color;

	position = pos;
	name = filename;

	SDL_QueryTexture(buttonTexture, NULL, NULL, &buttonTextureRect.w, &buttonTextureRect.h);

	buttonTextureRect.w *= 1;
	buttonTextureRect.h *= 1;

	buttonWindowRect.x = 0;
	buttonWindowRect.y = 0;

	text = new Text(game.renderer, game.theFont);
	text->SetText(txt);

	if (size.x != 0)
	{
		buttonWindowRect.w = size.x;
	}
	else if (buttonTexture == nullptr)
	{
		buttonWindowRect.w = 50;
	}
	else
	{
		buttonWindowRect.w = buttonTextureRect.w;
	}

	if (size.y != 0)
	{
		buttonWindowRect.h = size.y;
	}
	else if (buttonTexture == nullptr)
	{
		buttonWindowRect.h = 50;
	}
	else
	{
		buttonWindowRect.h = buttonTextureRect.h;
	}

	text->SetPosition(pos.x, pos.y + (buttonWindowRect.h / 2) - (text->textWindowRect.h / 2));
}

EditorButton::~EditorButton()
{

}

void EditorButton::Render(Renderer* renderer)
{
	buttonWindowRect.x = position.x;
	buttonWindowRect.y = position.y;

	if (buttonTexture != nullptr)
	{
		renderer->RenderCopy(buttonTexture, &buttonTextureRect, &buttonWindowRect);
	}		
	else
	{
		SDL_SetRenderDrawColor(renderer->renderer, buttonColor.r, buttonColor.g, buttonColor.b, buttonColor.a);
		SDL_RenderFillRect(renderer->renderer, &buttonWindowRect);
		SDL_SetRenderDrawColor(renderer->renderer, 0, 0, 0, 255);
	}

	text->Render(renderer);
}

bool EditorButton::IsClicked(const int& x, const int& y)
{
	return (x >= buttonWindowRect.x && x <= buttonWindowRect.x + buttonWindowRect.w &&
		y >= buttonWindowRect.y && y <= buttonWindowRect.y + buttonWindowRect.h);
}

void EditorButton::SetColors(Color c1, Color c2)
{
	colorOn = c1;
	colorOff = c2;
}