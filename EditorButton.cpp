#include "EditorButton.h"
#include "Game.h"

EditorButton::EditorButton(std::string txt, std::string filename, Vector2 pos, Game& game, Vector2 size, Color color)
{
	buttonTextureRect.x = 0;
	buttonTextureRect.y = 0;

	buttonColor = color;

	image = new Sprite(1, game.spriteManager, "assets/editor/btn" + filename + ".png", 
		game.renderer->shaders["default"], Vector2(0, 0));

	position = Vector2(pos.x * 2, pos.y * 2);
	name = filename;

	//SDL_QueryTexture(buttonTexture, NULL, NULL, &buttonTextureRect.w, &buttonTextureRect.h);

	buttonTextureRect.w *= 1;
	buttonTextureRect.h *= 1;

	buttonWindowRect.x = 0;
	buttonWindowRect.y = 0;

	text = new Text(game.renderer, game.theFont);
	text->SetText(txt);

	if (size.x != 0)
	{
		buttonWindowRect.w = (int)size.x;
	}
	else if (image == nullptr)
	{
		buttonWindowRect.w = 50;
	}
	else
	{
		buttonWindowRect.w = buttonTextureRect.w;
	}

	if (size.y != 0)
	{
		buttonWindowRect.h = (int)size.y;
	}
	else if (image == nullptr)
	{
		buttonWindowRect.h = 50;
	}
	else
	{
		buttonWindowRect.h = buttonTextureRect.h;
	}

	text->SetPosition(pos.x, pos.y + (buttonWindowRect.h / 2) - (text->GetTextHeight() / 2));
}

EditorButton::~EditorButton()
{

}

void EditorButton::Render(Renderer* renderer)
{
	buttonWindowRect.x = (int)position.x;
	buttonWindowRect.y = (int)position.y;
	buttonWindowRect.w = 50;
	buttonWindowRect.h = 50;

	Vector2 pos = Vector2(renderer->camera.position.x, renderer->camera.position.y);

	image->Render(position + pos, renderer);
	text->Render(renderer, pos);
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