#include "EditorButton.h"
#include "Game.h"

EditorButton::EditorButton(std::string filepath, std::string function, Vector2 pos, Game& game)
{
	buttonTexture = SDL_CreateTextureFromSurface(game.renderer, game.spriteManager.GetImage(filepath));
	buttonTextureRect.x = 0;
	buttonTextureRect.y = 0;

	position = pos;
	name = function;

	SDL_QueryTexture(buttonTexture, NULL, NULL, &buttonTextureRect.w, &buttonTextureRect.h);

	buttonTextureRect.w *= 1;
	buttonTextureRect.h *= 1;

	buttonWindowRect.x = 0;
	buttonWindowRect.y = 0;
	buttonWindowRect.w = buttonTextureRect.w;
	buttonWindowRect.h = buttonTextureRect.h;
}

EditorButton::~EditorButton()
{

}

void EditorButton::Render(SDL_Renderer* renderer)
{
	buttonWindowRect.x = position.x;
	buttonWindowRect.y = position.y;

	SDL_RenderCopy(renderer, buttonTexture, &buttonTextureRect, &buttonWindowRect);
}

bool EditorButton::IsClicked(const int& x, const int& y)
{
	return (x >= buttonWindowRect.x && x <= buttonWindowRect.x + buttonWindowRect.w &&
		y >= buttonWindowRect.y && y <= buttonWindowRect.y + buttonWindowRect.h);
}