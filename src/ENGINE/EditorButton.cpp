#include "EditorButton.h"
#include "Game.h"
#include "Renderer.h"
#include "Editor.h"

EditorButton::EditorButton(std::string txt, std::string filename, glm::vec3 pos, Game& game, Vector2 size, Color color)
{
	buttonTextureRect.x = 0;
	buttonTextureRect.y = 0;

	image = new Sprite(1, game.spriteManager, "assets/editor/btn" + filename + ".png", 
		game.renderer.shaders[ShaderName::GUI], Vector2(0, 0));

	image->keepPositionRelativeToCamera = true;
	image->keepScaleRelativeToCamera = true;
	image->color = color;

	position = glm::vec3(pos.x, pos.y, pos.z);
	name = filename;

	buttonTextureRect.w *= 1;
	buttonTextureRect.h *= 1;

	buttonWindowRect.x = 0;
	buttonWindowRect.y = 0;

	text = new Text(game.editor->fontInfo, txt, true, true);

	if (image == nullptr) // if no image, set the size to 50,50
	{
		buttonWindowRect.w = 50;
		buttonWindowRect.h = 50;
	}
	else // set a custom size for the button other than the size of the image
	{
		if (size.x != 0)
			buttonWindowRect.w = (int)size.x;
		else
			buttonWindowRect.w = buttonTextureRect.w;

		if (size.y != 0)
			buttonWindowRect.h = (int)size.y;
		else
			buttonWindowRect.h = buttonTextureRect.h;
	}

	text->SetPosition(pos.x, pos.y + (buttonWindowRect.h / 2) - (text->GetTextHeight() / 2));
}

EditorButton::~EditorButton()
{
	if (text != nullptr)
		delete_it(text);

	if (image != nullptr)
		delete_it(image);
}

void EditorButton::Render(const Renderer& renderer)
{
	// Set positiion of the button
	buttonWindowRect.w = 100;
	buttonWindowRect.h = 100;

	buttonWindowRect.x = (int)position.x - (buttonWindowRect.w/2);
	buttonWindowRect.y = (int)position.y - (buttonWindowRect.h/2);

	// Darken/lighten the button based on status
	if (isClicked)
		image->color = { (uint8_t)(color.r * 0.5f), (uint8_t)(color.g * 0.5f), (uint8_t)(color.b * 0.5f), 255 };
	else if (isHovered)
		image->color = { (uint8_t)(color.r * 0.75f), (uint8_t)(color.g * 0.75f), (uint8_t)(color.b * 0.75f), 255 };
	else
		image->color = color;

	// Render the button's image and text
	image->Render(position, renderer, imageScale);
	text->Render(renderer, glm::vec3(0,0,0));
}

bool EditorButton::IsPointInsideButton(const int& x, const int& y)
{	
	return (x >= buttonWindowRect.x && x <= buttonWindowRect.x + buttonWindowRect.w &&
		y >= buttonWindowRect.y && y <= buttonWindowRect.y + buttonWindowRect.h);
}