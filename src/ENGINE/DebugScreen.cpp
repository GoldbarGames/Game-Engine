#include "DebugScreen.h"
#include "Renderer.h"
#include "Text.h"
#include "Sprite.h"
#include "Game.h"
#include "Editor.h"

DebugScreen::DebugScreen(Game& g)
{
	game = &g;
	sprite = game->CreateSprite("assets/editor/1pixel.png");
	camera = &game->renderer.camera;

	CreateDebugText(DebugText::cursorPositionInScreen, 400, 50);
	CreateDebugText(DebugText::cursorPositionInWorld, 400, 100);
	CreateDebugText(DebugText::currentEditModeLayer, 400, 200);
	CreateDebugText(DebugText::drawCalls, 400, 1100);
	CreateDebugText(DebugText::updateCalls, 400, 1200);
	CreateDebugText(DebugText::collisionChecks, 400, 1300);
	CreateDebugText(DebugText::hoveredEntityID, 1200, 50);
	CreateDebugText(DebugText::cameraPosition, 1400, 100);
	CreateDebugText(DebugText::cameraAngle, 2000, 450);
	CreateDebugText(DebugText::cameraYaw, 2000, 550);
	CreateDebugText(DebugText::cameraPitch, 2000, 650);
	CreateDebugText(DebugText::cameraRoll, 2000, 750);
}

DebugScreen::~DebugScreen()
{
	for (auto& [key, val] : debugText)
	{
		if (val != nullptr)
			delete_it(val);
	}

	if (sprite != nullptr)
		delete_it(sprite);
}

void DebugScreen::CreateDebugText(const DebugText textName, const int x, const int y)
{
	debugText[textName] = neww Text(Editor::fontInfo);
	debugText[textName]->SetPosition(x, y);
	debugText[textName]->SetText("");
	debugText[textName]->GetSprite()->keepPositionRelativeToCamera = true;
}

void DebugScreen::Update()
{
	const Uint32 mouseState = SDL_GetMouseState(&mouseX, &mouseY);

	worldPosition = Vector2(mouseX + game->renderer.camera.position.x, mouseY + game->renderer.camera.position.y);

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

void DebugScreen::Render(const Renderer& renderer)
{
	if (renderer.game->debugMode)
	{
		debugText[DebugText::drawCalls]->SetText("Draw Calls: " + std::to_string(renderer.drawCallsPerFrame));
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

		debugText[DebugText::cursorPositionInScreen]->Render(renderer);
		debugText[DebugText::cursorPositionInWorld]->Render(renderer);

		if (sprite != nullptr)
		{
			sprite->Render(worldPosition, renderer, scale);
		}

		if (camera != nullptr && !camera->useOrthoCamera)
		{
			debugText[DebugText::cameraPosition]->SetText("Cam Pos: (x: " + 
				std::to_string((int)camera->position.x) + ", y: " + 
				std::to_string((int)camera->position.y) + ", z: " +
				std::to_string((int)camera->position.z) + ")");
			debugText[DebugText::cameraPosition]->GetSprite()->keepScaleRelativeToCamera = true;
			debugText[DebugText::cameraPosition]->Render(renderer);

			debugText[DebugText::cameraAngle]->SetText("Angle: " + std::to_string(camera->angle));
			debugText[DebugText::cameraAngle]->GetSprite()->keepScaleRelativeToCamera = true;
			debugText[DebugText::cameraAngle]->Render(renderer);

			debugText[DebugText::cameraYaw]->SetText("Yaw: " + std::to_string(camera->yaw));
			debugText[DebugText::cameraYaw]->GetSprite()->keepScaleRelativeToCamera = true;
			debugText[DebugText::cameraYaw]->Render(renderer);

			debugText[DebugText::cameraPitch]->SetText("Pitch: " + std::to_string(camera->pitch));
			debugText[DebugText::cameraPitch]->GetSprite()->keepScaleRelativeToCamera = true;
			debugText[DebugText::cameraPitch]->Render(renderer);

			debugText[DebugText::cameraRoll]->SetText("Roll: " + std::to_string(camera->roll));
			debugText[DebugText::cameraRoll]->GetSprite()->keepScaleRelativeToCamera = true;
			debugText[DebugText::cameraRoll]->Render(renderer);
		}
	}
}