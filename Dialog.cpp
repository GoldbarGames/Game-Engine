#include "Dialog.h"
#include "SpriteManager.h"
#include "Renderer.h"
#include "Shader.h"

Dialog::Dialog(const Vector2& pos, SpriteManager* manager)
{
	position = pos;
	sprite = neww Sprite(manager->renderer->shaders[ShaderName::Default]);
}

Dialog::~Dialog()
{
	if (sprite != nullptr)
		delete sprite;
	
	if (text != nullptr)
		delete text;
	
	if (input != nullptr)
		delete input;
}

void Dialog::Update()
{
	//TODO: Get the input here, if this dialog takes input
	// (move relevant stuff from the Game Update to here)
}

void Dialog::Render(const Renderer& renderer)
{
	sprite->Render(position, renderer);

	text->Render(renderer);

	input->Render(renderer);
}