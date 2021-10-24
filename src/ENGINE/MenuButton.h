#ifndef MENUBUTTON_H
#define MENUBUTTON_H
#pragma once

#include <SDL2/SDL.h>
#include <string>
#include "Sprite.h"
#include "BaseButton.h"
#include "leak_check.h"
class Game;

class KINJO_API MenuButton : public BaseButton
{
private:
	int thickness = 2;
	glm::vec3 imagePosition = glm::vec3(0, 0, 0);
public:	

	bool deleteOtherImages = true;
	std::vector<Entity*> otherImages;

	MenuButton(const std::string& txt, const std::string& filepath, 
		const std::string& function, const glm::vec3& pos, Game& game, 
		Color col = { 255, 255, 255, 255 });
	~MenuButton();
	void Render(const Renderer& renderer);
	BaseButton* Update(Game& game, const Uint8* currentKeyStates);
	void SetOptionColors(Color color);

	void SetScale(const Vector2& newScale);

	void Highlight(Game& game);
	void Unhighlight(Game& game);
};

#endif