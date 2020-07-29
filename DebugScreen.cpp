#include "DebugScreen.h"
#include "Renderer.h"
#include "Text.h"
#include "Sprite.h"
#include "Game.h"


DebugScreen::DebugScreen(Game& g)
{
	game = &g;
	sprite = new Sprite(0, 0, 1, game->spriteManager, "assets/editor/1pixel.png", game->renderer->shaders[ShaderName::Default], Vector2(0, 0));
	CreateDebugText(DebugText::cursorPositionInScreen, 400, 50);
	CreateDebugText(DebugText::cursorPositionInWorld, 400, 100);
	CreateDebugText(DebugText::currentEditModeLayer, 400, 200);
	CreateDebugText(DebugText::drawCalls, 400, 1100);
	CreateDebugText(DebugText::updateCalls, 400, 1200);
	CreateDebugText(DebugText::collisionChecks, 400, 1300);
	CreateDebugText(DebugText::hoveredEntityID, 1200, 50);
}

void DebugScreen::CreateDebugText(const DebugText textName, const int x, const int y)
{
	debugText[textName] = new Text(game->renderer, game->theFont);
	debugText[textName]->SetPosition(x, y);
	debugText[textName]->SetText("");
	debugText[textName]->GetSprite()->keepPositionRelativeToCamera = true;
}

void DebugScreen::Update()
{
	const Uint32 mouseState = SDL_GetMouseState(&mouseX, &mouseY);

	worldPosition = Vector2(mouseX + game->renderer->camera.position.x, mouseY + game->renderer->camera.position.y);

	std::string clickedText = std::to_string(mouseX) + " " + std::to_string(mouseY);
	game->debugScreen->debugText[DebugText::cursorPositionInScreen]->SetText("Mouse Screen: " + clickedText);
	game->debugScreen->debugText[DebugText::cursorPositionInScreen]->GetSprite()->keepScaleRelativeToCamera = true;

	std::string clickedText2 = std::to_string((int)worldPosition.x) + " " + std::to_string((int)worldPosition.y);
	game->debugScreen->debugText[DebugText::cursorPositionInWorld]->SetText("Mouse World: " + clickedText2);
	game->debugScreen->debugText[DebugText::cursorPositionInWorld]->GetSprite()->keepScaleRelativeToCamera = true;

	// Find the hovered entity ID
	SDL_Point point;
	point.x = worldPosition.x;
	point.y = worldPosition.y;
	for (unsigned int i = 0; i < game->entities.size(); i++)
	{
		if (SDL_PointInRect(&point, game->entities[i]->GetBounds()))
		{
			game->editor->hoveredEntityID = game->entities[i]->id;
			break;
		}
	}
}

void DebugScreen::Render(Renderer* renderer)
{
	if (renderer->game->debugMode)
	{
		debugText[DebugText::drawCalls]->SetText("Draw Calls: " + std::to_string(renderer->drawCallsPerFrame));
		debugText[DebugText::drawCalls]->GetSprite()->keepScaleRelativeToCamera = true;
		debugText[DebugText::drawCalls]->Render(renderer);

		debugText[DebugText::updateCalls]->SetText("Update Calls: " + std::to_string(game->updateCalls));
		debugText[DebugText::updateCalls]->GetSprite()->keepScaleRelativeToCamera = true;
		debugText[DebugText::updateCalls]->Render(renderer);

		debugText[DebugText::collisionChecks]->SetText("Collision Checks: " + std::to_string(game->collisionChecks));
		debugText[DebugText::collisionChecks]->GetSprite()->keepScaleRelativeToCamera = true;
		debugText[DebugText::collisionChecks]->Render(renderer);

		debugText[DebugText::hoveredEntityID]->SetText("Hovered ID: " + std::to_string(game->editor->hoveredEntityID));
		debugText[DebugText::hoveredEntityID]->GetSprite()->keepScaleRelativeToCamera = true;
		debugText[DebugText::hoveredEntityID]->Render(renderer);

		// Draw text
		debugText[DebugText::cursorPositionInScreen]->Render(renderer);
		debugText[DebugText::cursorPositionInWorld]->Render(renderer);

		if (sprite != nullptr)
		{
			sprite->Render(worldPosition, renderer);
		}
	}
}