#include "EditorButton.h"
#include "Game.h"

EditorButton::EditorButton(std::string txt, ::string filename, Vector2 pos, Game& game, Vector2 size)
{
	buttonTexture = game.spriteManager->GetImage("assets/editor/btn" + filename + ".png");
	buttonTextureRect.x = 0;
	buttonTextureRect.y = 0;

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
	else
	{
		buttonWindowRect.w = buttonTextureRect.w;
	}

	if (size.y != 0)
	{
		buttonWindowRect.h = size.y;
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

	renderer->RenderCopy(buttonTexture, &buttonTextureRect, &buttonWindowRect);

	text->Render(renderer);
}

bool EditorButton::IsClicked(const int& x, const int& y)
{
	return (x >= buttonWindowRect.x && x <= buttonWindowRect.x + buttonWindowRect.w &&
		y >= buttonWindowRect.y && y <= buttonWindowRect.y + buttonWindowRect.h);
}