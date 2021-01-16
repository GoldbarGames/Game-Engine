#include "GUI.h"
#include "Game.h"

void GUI::Init(Game* g)
{
	game = g;
	textNames = { "FPS", "timer" };

	for (int i = 0; i < textNames.size(); i++)
	{
		texts[textNames[i]] = new Text(game->theFont);
		texts[textNames[i]]->isRichText = true;
		texts[textNames[i]]->SetText("");
		texts[textNames[i]]->GetSprite()->keepPositionRelativeToCamera = true;
		texts[textNames[i]]->GetSprite()->keepScaleRelativeToCamera = true;;
	}

	ResetText();
}

GUI::~GUI()
{
	for (auto& [key, val] : texts)
	{
		if (val != nullptr)
			delete_it(val);
	}
}

void GUI::RenderStart()
{

}

void GUI::Render(const Renderer& renderer)
{
	// TODO: Refactor these texts into structs with bools in them

	if (game->showFPS)
		texts[textNames[0]]->Render(renderer);

	if (game->showTimer)
		texts[textNames[1]]->Render(renderer);
}

void GUI::ResetText()
{
	// FPS text
	texts[textNames[0]]->SetText("FPS:");
	texts[textNames[0]]->SetPosition((game->screenWidth * 2) - (texts[textNames[0]]->GetTextWidth() * 4), texts[textNames[0]]->GetTextHeight());

	// Timer text
	texts[textNames[1]]->SetText("");
	texts[textNames[1]]->SetPosition((game->screenWidth) - (texts[textNames[1]]->GetTextWidth() * 2), texts[textNames[1]]->GetTextHeight());

	//texts["bug"].SetText("Bugs Remaining: " + std::to_string(bugsRemaining));
	//texts["bug"].SetPosition(texts["bug"].GetTextWidth() * 1.25f, texts["bug"].GetTextHeight() * 1.25f);

	//texts["ether"].SetText("Ether: " + std::to_string(currentEther));
	//texts["ether"].SetPosition(0, 150);
}