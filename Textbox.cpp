#include "Textbox.h"
#include "globals.h"

Textbox::Textbox(SpriteManager * manager, Renderer * renderer)
{
	//TODO: Replace these with the real fonts
	textFont = TTF_OpenFont("assets/fonts/default.ttf", 24);
	speakerFont = TTF_OpenFont("assets/fonts/default.ttf", 24);

	boxSprite = new Sprite(0, 0, 1, manager, "assets/gui/textbox.png", renderer, Vector2(0, 0));

	text = new Text(renderer, textFont);
	speaker = new Text(renderer, speakerFont);	

	speaker->SetPosition(70, 480);
	text->SetPosition(70, 530);

	//TODO: Should we create one texture for each alphabet letter and show the ones relevant to the string?
	speaker->SetText("GRAMPS");
	text->SetTextWrapped("Hello, world! This is some text. The quick brown fox jumped over the lazy dog. Let's see what happens when we run out of room on the textbox!", 575 * SCALE);
}

Textbox::~Textbox()
{

}

void Textbox::Render(Renderer * renderer)
{
	//TODO: Make sure the position is in the center of the screen
	boxSprite->Render(position, 0, -1, SDL_FLIP_NONE, renderer);
	speaker->Render(renderer);
	text->Render(renderer);
}
