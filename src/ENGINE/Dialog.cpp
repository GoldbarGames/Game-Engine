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

void Dialog::Update(const std::string& newText)
{
	input->SetText(newText);
}

void Dialog::Render(const Renderer& renderer)
{
	sprite->Render(position, renderer, scale);

	text->Render(renderer);

	input->Render(renderer);
}