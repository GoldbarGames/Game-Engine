#include "Dialog.h"
#include "SpriteManager.h"
#include "Renderer.h"
#include "Shader.h"

Dialog::Dialog(SpriteManager* manager)
{
	sprite = new Sprite(manager->renderer->shaders[ShaderName::Default]);
}

Dialog::Dialog(const glm::vec3& pos, SpriteManager* manager)
{
	position = pos;
	sprite = new Sprite(manager->renderer->shaders[ShaderName::Default]);
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
	if (visible)
	{
		sprite->Render(position, renderer, scale);

		text->Render(renderer);

		input->Render(renderer);
	}
}