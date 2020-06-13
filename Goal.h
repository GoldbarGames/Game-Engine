#pragma once
#include "Entity.h"

class Goal : public Entity
{
public:
	int spriteIndex = 0;
	bool isOpen = false;
	std::string nextLevelName = "";

	Goal(Vector2 pos);
	~Goal();

	void Update(Game& game);

	void OnTriggerStay(Entity* other, Game& game);
	void OnTriggerEnter(Entity* other, Game& game);
	void OnTriggerExit(Entity* other, Game& game);
	void GetProperties(Renderer * renderer, FontInfo* font, std::vector<Property*>& properties);
	void SetProperty(std::string prop, std::string newValue);

	void Save(std::ostringstream& level);
};

