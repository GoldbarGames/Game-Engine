#include "EditorButton.h"
#include "Game.h"

EditorButton::EditorButton(std::string txt, std::string filename, Vector2 pos, Game& game, Vector2 size, Color color)
{
	buttonTextureRect.x = 0;
	buttonTextureRect.y = 0;

	buttonColor = color;

	image = new Sprite(1, game.spriteManager, "assets/editor/btn" + filename + ".png", 
		game.renderer->shaders["gui"], Vector2(0, 0));

	image->renderRelativeToCamera = true;
	image->keepScaleRelativeToCamera = true;

	position = Vector2(pos.x * 2, pos.y * 2);
	name = filename;

	buttonTextureRect.w *= 1;
	buttonTextureRect.h *= 1;

	buttonWindowRect.x = 0;
	buttonWindowRect.y = 0;

	text = new Text(game.renderer, game.theFont);
	text->SetText(txt);
	text->GetSprite()->renderRelativeToCamera = true;
	text->GetSprite()->keepScaleRelativeToCamera = true;

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
	// Set positiion of the button

	buttonWindowRect.w = 100;
	buttonWindowRect.h = 100;

	buttonWindowRect.x = (int)position.x - (buttonWindowRect.w/2);
	buttonWindowRect.y = (int)position.y - (buttonWindowRect.h/2);

	// Darken/lighten the button based on status
	if (isClicked)
		image->color = { 128, 128, 128, 255 };
	else if (isHovered)
		image->color = { 384, 384, 384, 255 };
	else
		image->color = { 255, 255, 255, 255 };

	// Render the button's image and text
	Vector2 cameraPosition = Vector2(renderer->camera.position.x, renderer->camera.position.y);
	image->Render(position + cameraPosition, renderer);
	text->Render(renderer, cameraPosition);
}

bool EditorButton::IsPointInsideButton(const int& x, const int& y)
{	
	return (x >= buttonWindowRect.x && x <= buttonWindowRect.x + buttonWindowRect.w &&
		y >= buttonWindowRect.y && y <= buttonWindowRect.y + buttonWindowRect.h);
}

void EditorButton::SetColors(Color c1, Color c2)
{
	colorOn = c1;
	colorOff = c2;
}