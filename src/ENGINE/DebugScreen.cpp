#include "DebugScreen.h"
#include "Renderer.h"
#include "Text.h"
#include "Sprite.h"
#include "Game.h"
#include "Editor.h"
#include "Dialog.h"

DebugScreen::DebugScreen(Game& g)
{
	game = &g;

#ifdef _DEBUG
	onePixelSprite = game->CreateSprite("assets/editor/1pixel.png", ShaderName::GUI);
	camera = &game->renderer.camera;

	insertVariableButton = new EditorButton("Watch", "Btn", glm::vec3(2000, 100, 0), *game);
	removeVariableButton = new EditorButton("Unwatch", "Btn", glm::vec3(2200, 100, 0), *game);

	insertVariableButton->imageScale = Vector2(2, 1);
	removeVariableButton->imageScale = Vector2(2, 1);

	CreateDebugText(DebugText::cursorPositionInScreen, 400, 50);
	CreateDebugText(DebugText::cursorPositionInWorld, 400, 100);
	CreateDebugText(DebugText::currentEditModeLayer, 400, 200);
	CreateDebugText(DebugText::drawCalls, 400, 1100);
	CreateDebugText(DebugText::updateCalls, 400, 1200);
	CreateDebugText(DebugText::collisionChecks, 400, 1300);
	CreateDebugText(DebugText::hoveredEntityID, 1200, 50);
	CreateDebugText(DebugText::cameraPosition, 1400, 100);
	CreateDebugText(DebugText::cameraAngle, 400, 450);
	CreateDebugText(DebugText::cameraYaw, 400, 550);
	CreateDebugText(DebugText::cameraPitch, 400, 650);
	CreateDebugText(DebugText::cameraRoll, 400, 750);

	std::vector<std::string> variables = ReadStringsFromFile("data/debug.vars");
	for (int i = 0; i < variables.size(); i++)
	{
		InsertVariable(variables[i]);
	}
#endif

}

DebugScreen::~DebugScreen()
{
	// Save our debug list
#if _DEBUG

	std::ofstream fout;

	fout.open("data/debug.vars");
	if (fout.is_open())
	{
		for (int i = 0; i < cutsceneVariableNames.size(); i++)
		{
			fout << cutsceneVariableNames[i].c_str() << std::endl;
		}
	}
	fout.close();

#endif

	for (auto& [key, val] : debugText)
	{
		if (val != nullptr)
			delete_it(val);
	}

	if (onePixelSprite != nullptr)
		delete_it(onePixelSprite);

	for (int i = 0; i < variableTextLeft.size(); i++)
	{
		if (variableTextLeft[i] != nullptr)
			delete_it(variableTextLeft[i]);
	}

	for (int i = 0; i < variableTextCenter.size(); i++)
	{
		if (variableTextCenter[i] != nullptr)
			delete_it(variableTextCenter[i]);
	}

	for (int i = 0; i < variableTextRight.size(); i++)
	{
		if (variableTextRight[i] != nullptr)
			delete_it(variableTextRight[i]);
	}

	if (insertVariableButton != nullptr)
		delete_it(insertVariableButton);

	if (removeVariableButton != nullptr)
		delete_it(removeVariableButton);

}

void DebugScreen::CreateDebugText(const DebugText textName, const int x, const int y)
{
	debugText[textName] = new Text(Editor::fontInfo);
	debugText[textName]->SetPosition(x, y);
	debugText[textName]->SetText("");
	debugText[textName]->GetSprite()->keepPositionRelativeToCamera = true;
	debugText[textName]->GetSprite()->keepScaleRelativeToCamera = true;
}

glm::vec3 DebugScreen::ConvertFromScreenSpaceToWorldSpace(const glm::vec2& pos)
{
	float halfScreenWidth = game->screenWidth / 2.0f;
	float halfScreenHeight = game->screenHeight / 2.0f;

	glm::mat4 projection = game->renderer.camera.projection;
	glm::mat4 view = game->renderer.camera.CalculateViewMatrix();

	glm::mat4 invMat = glm::inverse(projection * view);

	// Near = -1, Far = 1, but these only work with Perspective cameras.
	// For an orthographic camera, we need to use 0, or the midpoint between Near and Far
	glm::vec4 mid = glm::vec4((pos.x - halfScreenWidth) / halfScreenWidth, -1 * (pos.y - halfScreenHeight) / halfScreenHeight, 0, 1.0);
	
	glm::vec4 midResult = invMat * mid;
	midResult /= midResult.w;

	return glm::vec3(midResult);
}

bool DebugScreen::Update()
{
#ifndef _DEBUG
	return false;
#endif
	const Uint32 mouseState = SDL_GetMouseState(&mouseX, &mouseY);

	worldPosition = ConvertFromScreenSpaceToWorldSpace(glm::vec2(mouseX, mouseY));

	std::string clickedText = std::to_string(mouseX) + " " + std::to_string(mouseY);
	game->debugScreen->debugText[DebugText::cursorPositionInScreen]->SetText("Mouse Screen: " + clickedText);

	std::string clickedText2 = std::to_string((int)worldPosition.x) + " " + std::to_string((int)worldPosition.y);
	game->debugScreen->debugText[DebugText::cursorPositionInWorld]->SetText("Mouse World: " + clickedText2);

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

	// Check for Left Click
	if (mouseState & SDL_BUTTON(SDL_BUTTON_LEFT))
	{
		// We multiply X and Y by 2 because the guiProjection is multiplied by 2
		// TODO: Maybe remove the multiplier
		mouseX *= Camera::MULTIPLIER;
		mouseY *= Camera::MULTIPLIER;

		// Only on click, not hold
		if (!(previousMouseState & SDL_BUTTON(SDL_BUTTON_LEFT)))
		{
			previousMouseState = mouseState;

			// Check which button was clicked
			if (insertVariableButton->IsPointInsideButton(mouseX, mouseY))
			{
				game->editor->CreateDialog("Type the name of the variable to start watching:");
				game->StartTextInput(*game->editor->dialog, "start_watch");
				//clickedButton->isClicked = false;
				return true;
			}
			else if (removeVariableButton->IsPointInsideButton(mouseX, mouseY))
			{
				game->editor->CreateDialog("Type the name of the variable to stop watching:");
				game->StartTextInput(*game->editor->dialog, "end_watch");
				//clickedButton->isClicked = false;
				return true;
			}

		}
	}

	previousMouseState = mouseState;

	return false;
}

// Function for adding variables to the table
void DebugScreen::InsertVariable(const std::string& variableName)
{
	bool found = false;
	for (int i = 0; i < cutsceneVariableNames.size(); i++)
	{
		if (cutsceneVariableNames[i] == variableName)
			found = true;
	}

	// Check to make sure that the variable we are watching
	// actually exists within the cutscene manager
	int numalias = game->cutsceneManager.commands.GetNumAlias(variableName);
	bool exists = (game->cutsceneManager.commands.numalias.find(variableName) 
		!= game->cutsceneManager.commands.numalias.end());

	if (!exists)
	{
		game->logger.Log("ERROR: Cutscene variable " + variableName + " does not exist!");
	}
	else if (found)
	{
		game->logger.Log("ERROR: Cutscene variable " + variableName + " already being tracked!");
	}
	else if (!found && exists)
	{
		// TODO: If the variable is not initialized when we add it,
		// then it won't automatically update over time
		// (otherwise, it updates itself just fine)

		int startY = 200;
		int distance = 50;
		cutsceneVariableNames.push_back(variableName);
		int size = cutsceneVariableNames.size() - 1;

		Text* newTextLeft = new Text(game->theFont);
		newTextLeft->SetPosition(100, startY + (distance * size));
		newTextLeft->isRichText = true;
		newTextLeft->GetSprite()->keepScaleRelativeToCamera = true;
		newTextLeft->GetSprite()->keepPositionRelativeToCamera = true;
		variableTextLeft.push_back(newTextLeft);

		Text* newTextCenter = new Text(game->theFont);
		newTextCenter->SetPosition(400, startY + (distance * size));
		newTextCenter->isRichText = true;
		newTextCenter->GetSprite()->keepScaleRelativeToCamera = true;
		newTextCenter->GetSprite()->keepPositionRelativeToCamera = true;
		variableTextCenter.push_back(newTextCenter);

		Text* newTextRight = new Text(game->theFont);
		newTextRight->SetPosition(700, startY + (distance * size));
		newTextRight->isRichText = true;
		newTextRight->GetSprite()->keepScaleRelativeToCamera = true;
		newTextRight->GetSprite()->keepPositionRelativeToCamera = true;
		variableTextRight.push_back(newTextRight);
	}
}

// Function for removing variables from the table
void DebugScreen::RemoveVariable(const std::string& variableName)
{
	int index = -1;
	for (int i = 0; i < cutsceneVariableNames.size(); i++)
	{
		if (cutsceneVariableNames[i] == variableName)
			index = i;
	}

	if (index > -1)
	{
		cutsceneVariableNames.erase(cutsceneVariableNames.begin() + index);
		variableTextLeft.erase(variableTextLeft.begin() + index);
		variableTextCenter.erase(variableTextCenter.begin() + index);
		variableTextRight.erase(variableTextRight.begin() + index);

		// Reset the positions of each item in the list after removal

		int startY = 200;
		int distance = 50;

		for (int i = 0; i < cutsceneVariableNames.size(); i++)
		{
			variableTextLeft[i]->SetPosition(100, startY + (distance * i));
			variableTextCenter[i]->SetPosition(400, startY + (distance * i));
			variableTextRight[i]->SetPosition(700, startY + (distance * i));
		}

	}
}

void DebugScreen::Render(const Renderer& renderer)
{
#ifndef _DEBUG
	return;
#endif

	if (renderer.game->debugMode)
	{
		// If we're watching a cutscene,
		// draw a table showing a list of variables and their current values
		// (the user can add/remove variables that they are interested in seeing
		if (renderer.game->cutsceneManager.watchingCutscene)
		{
			if (onePixelSprite != nullptr)
			{
				onePixelSprite->keepPositionRelativeToCamera = true;
				onePixelSprite->keepScaleRelativeToCamera = true;
				onePixelSprite->color = { 0, 0, 0, 64 };
				onePixelSprite->Render(glm::vec3(800, 800, 0), renderer, Vector2(800, 800));
			}

			CutsceneCommands& cmds = renderer.game->cutsceneManager.commands;
			for (int i = 0; i < cutsceneVariableNames.size(); i++)
			{
				// 1. Left cell is the name of the variable
				if (variableTextLeft[i]->txt != cutsceneVariableNames[i])
				{
					variableTextLeft[i]->SetText(cutsceneVariableNames[i]);
					variableTextLeft[i]->SetColor({ 0, 255, 255, 255 });
				}
				else if (!updatedLine)
				{
					variableTextLeft[i]->SetColor({ 255, 255, 255, 255 });
				}
				
				variableTextLeft[i]->Render(renderer);

				// 2. Right cell is the string value
				int numIndex = cmds.GetNumAlias(cutsceneVariableNames[i]);
				
				if (variableTextRight[i]->txt != cmds.GetStringVariable(numIndex))
				{
					variableTextRight[i]->SetText(cmds.GetStringVariable(numIndex));
					variableTextRight[i]->SetColor({ 0, 255, 255, 255 });
				}
				else if (!updatedLine)
				{
					variableTextRight[i]->SetColor({ 255, 255, 255, 255 });
				}

				variableTextRight[i]->Render(renderer);

				// 3. Center cell is the number value
				if (variableTextCenter[i]->txt != std::to_string(cmds.numberVariables[numIndex]))
				{					
					variableTextCenter[i]->SetText(std::to_string(cmds.numberVariables[numIndex]));
					variableTextCenter[i]->SetColor({ 0, 255, 255, 255 });
				}
				else if (!updatedLine)
				{
					variableTextCenter[i]->SetColor({ 255, 255, 255, 255 });
				}

				variableTextCenter[i]->Render(renderer);

				
			}

			insertVariableButton->Render(renderer);
			removeVariableButton->Render(renderer);

			updatedLine = true;
		}

		debugText[DebugText::drawCalls]->SetText("Draw Calls: " + std::to_string(renderer.drawCallsPerFrame));
		debugText[DebugText::drawCalls]->Render(renderer);

		debugText[DebugText::updateCalls]->SetText("Update Calls: " + std::to_string(game->updateCalls));
		debugText[DebugText::updateCalls]->Render(renderer);

		debugText[DebugText::collisionChecks]->SetText("Collision Checks: " + std::to_string(game->collisionChecks));
		debugText[DebugText::collisionChecks]->Render(renderer);

		debugText[DebugText::hoveredEntityID]->SetText("Hovered ID: " + std::to_string(game->editor->hoveredEntityID));
		debugText[DebugText::hoveredEntityID]->Render(renderer);

		debugText[DebugText::cursorPositionInScreen]->Render(renderer);
		debugText[DebugText::cursorPositionInWorld]->Render(renderer);

		if (onePixelSprite != nullptr)
		{
			onePixelSprite->keepPositionRelativeToCamera = false;
			onePixelSprite->keepScaleRelativeToCamera = false;
			onePixelSprite->Render(worldPosition, renderer, scale);
		}

		if (camera != nullptr && !camera->useOrthoCamera)
		{
			debugText[DebugText::cameraPosition]->SetText("Cam Pos: (x: " + 
				std::to_string((int)camera->position.x) + ", y: " + 
				std::to_string((int)camera->position.y) + ", z: " +
				std::to_string((int)camera->position.z) + ")");
			debugText[DebugText::cameraPosition]->Render(renderer);

			debugText[DebugText::cameraAngle]->SetText("Angle: " + std::to_string(camera->angle));
			debugText[DebugText::cameraAngle]->Render(renderer);

			debugText[DebugText::cameraYaw]->SetText("Yaw: " + std::to_string(camera->yaw));
			debugText[DebugText::cameraYaw]->Render(renderer);

			debugText[DebugText::cameraPitch]->SetText("Pitch: " + std::to_string(camera->pitch));
			debugText[DebugText::cameraPitch]->Render(renderer);

			debugText[DebugText::cameraRoll]->SetText("Roll: " + std::to_string(camera->roll));
			debugText[DebugText::cameraRoll]->Render(renderer);
		}

		if (game->editor->dialog != nullptr && game->editor->dialog->visible)
		{
			game->editor->dialog->Render(renderer);
		}
	}
}