#include "MyGUI.h"
#include "Spell.h"
#include "../ENGINE/HealthComponent.h"
#include "../ENGINE/Game.h"

void MyGUI::Init(Game* g)
{
	game = g;
	textNames = { "FPS", "timer", "bug", "ether" };

	for (int i = 0; i < textNames.size(); i++)
	{
		texts[textNames[i]] = neww Text(game->theFont);
		texts[textNames[i]]->SetText("");
		texts[textNames[i]]->GetSprite()->keepPositionRelativeToCamera = true;
		texts[textNames[i]]->GetSprite()->keepScaleRelativeToCamera = true;;
	}

	ResetText();
}

void MyGUI::RenderStart()
{
	healthComponents.clear();
}

void MyGUI::Render(const Renderer& renderer)
{
	for (int i = 0; i < healthComponents.size(); i++)
	{
		healthComponents[i]->Render(renderer);
	}

	if (playerSpell != nullptr)
	{
		//playerSpell->Render(renderer);
	}

	// TODO: Refactor these texts into structs with bools in them

	if (game->showFPS)
		texts[textNames[0]]->Render(renderer);

	if (game->showTimer)
		texts[textNames[1]]->Render(renderer);
}

void MyGUI::ResetText()
{
	// FPS text
	texts[textNames[0]]->SetText("FPS:");
	texts[textNames[0]]->SetPosition((game->screenWidth * 2) - (texts[textNames[0]]->GetTextWidth() * 2), texts[textNames[0]]->GetTextHeight());

	// Timer text
	texts[textNames[1]]->SetText("");
	texts[textNames[1]]->SetPosition((game->screenWidth) - (texts[textNames[1]]->GetTextWidth() * 2), texts[textNames[1]]->GetTextHeight());

	// Initialize starting properties
	/*
	game->currentEther = game->startingEther;
	game->bugsRemaining = 0;
	for (unsigned int i = 0; i < game->entities.size(); i++)
	{
		if (game->entities[i]->etype == "bug")
			game->bugsRemaining++;
	}
	*/

	//texts["bug"].SetText("Bugs Remaining: " + std::to_string(bugsRemaining));
	//texts["bug"].SetPosition(texts["bug"].GetTextWidth() * 1.25f, texts["bug"].GetTextHeight() * 1.25f);

	//texts["ether"].SetText("Ether: " + std::to_string(currentEther));
	//texts["ether"].SetPosition(0, 150);
}