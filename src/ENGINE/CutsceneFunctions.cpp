#include "CutsceneFunctions.h"
#include <iostream>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include "CutsceneHelper.h"
#include "CutsceneManager.h"
#include "Game.h"
#include "Timer.h"
#include "Animator.h"
#include "Logger.h"
#include "SoundManager.h"
#include "RandomManager.h"
#include "Renderer.h"
#include "ParticleSystem.h"

namespace CutsceneFunctions
{

	int DoNothing(CutsceneParameters parameters, CutsceneCommands& c)
	{
		// Do nothing in this function!
		return 0;
	}

	int SetNumberVariable(CutsceneParameters parameters, CutsceneCommands& c)
	{
		c.cacheParseNumbers.erase(parameters[1]);

		c.key = c.ParseNumberValue(parameters[1]);
		c.parseNumberValue = c.ParseNumberValue(parameters[2]);

		if (parameters.size() > 3 && parameters[3] == "no_alias")
		{
			c.parseNumberValue = std::stoi(parameters[2]);
		}

		if (c.numberVariables[c.key] != c.parseNumberValue)
		{
			c.numberVariables[c.key] = c.parseNumberValue;

			// If global variable, save change to file
			if (c.key >= c.manager->globalStart)
				c.manager->SaveGlobalVariable(c.key, std::to_string(c.numberVariables[c.key]), true);
		}

		return 0;
	}

	int SetStringVariable(CutsceneParameters parameters, CutsceneCommands& c)
	{
		c.key = c.ParseNumberValue(parameters[1]);

		c.parseStringValue = parameters[2];

		if (c.parseStringValue[0] == '[')
		{
			int varNameIndex = 1;
			c.parseStringValue = ParseWord(c.parseStringValue, ']', varNameIndex);
		}
		else
		{
			c.parseStringValue = c.ParseStringValue(c.parseStringValue);
		}

		if (c.stringVariables[c.key] != c.parseStringValue)
		{
			c.stringVariables[c.key] = c.parseStringValue;
			//c.cacheParseStrings["$" + parameters[1]] = c.parseStringValue;

			// If global variable, save change to file
			if (c.key >= c.manager->globalStart)
				c.manager->SaveGlobalVariable(c.key, c.stringVariables[c.key], false);
		}



		return 0;
	}



	int SetStringAlias(CutsceneParameters parameters, CutsceneCommands& c)
	{
		//c.cacheParseStrings.erase(parameters[1]);
		c.stralias[parameters[1]] = parameters[2];
		c.stralias[parameters[1]].erase(0, c.stralias[parameters[1]].find_first_not_of('['));
		c.stralias[parameters[1]].erase(c.stralias[parameters[1]].find_last_not_of(']') + 1);
		return 0;
	}


	int SetNumAlias(CutsceneParameters parameters, CutsceneCommands& c)
	{
		c.cacheParseNumbers.erase(parameters[1]);
		c.numalias[parameters[1]] = c.ParseNumberValue(parameters[2]);
		SetNumberVariable({ "", parameters[2], "0" }, c);
		return 0;
	}




	int MusicCommand(CutsceneParameters parameters, CutsceneCommands& c)
	{
		//TODO: Deal with custom loop times

		if (parameters[1] == "play")
		{
			c.manager->game->soundManager.PlayBGM(c.pathPrefix + c.ParseStringValue(parameters[2]), true);
		}
		else if (parameters[1] == "once")
		{
			c.manager->game->soundManager.PlayBGM(c.pathPrefix + c.ParseStringValue(parameters[2]), false);
		}
		else if (parameters[1] == "stop")
		{
			c.manager->game->soundManager.StopBGM();
		}
		else if (parameters[1] == "fadein")
		{
			c.manager->game->soundManager.FadeInBGM(c.pathPrefix + c.ParseStringValue(parameters[2]), c.ParseNumberValue(parameters[3]), true);
		}
		else if (parameters[1] == "fadeout")
		{
			if (c.manager->isSkipping)
				c.manager->game->soundManager.StopBGM();
			else
				c.manager->game->soundManager.FadeOutBGM(c.ParseNumberValue(parameters[2]));
		}
		else if (parameters[1] == "fadeinw")
		{
			c.manager->game->soundManager.FadeInBGM(c.pathPrefix + c.ParseStringValue(parameters[2]), c.ParseNumberValue(parameters[3]), true);
			Wait({ "", parameters[2] }, c);
		}
		else if (parameters[1] == "fadeoutw")
		{
			if (c.manager->isSkipping)
			{
				c.manager->game->soundManager.StopBGM();
			}
			else
			{
				c.manager->game->soundManager.FadeOutBGM(c.ParseNumberValue(parameters[2]));
				Wait({ "", parameters[2] }, c);
			}
		}
		else if (parameters[1] == "volume")
		{
			c.manager->game->soundManager.SetVolumeBGM(c.ParseNumberValue(parameters[2]));
		}
		else if (parameters[1] == "volumei")
		{
			c.manager->game->soundManager.SetVolumeBGMIndex(c.ParseNumberValue(parameters[2]));
		}

		return 0;
	}

	int MusicEffectCommand(CutsceneParameters parameters, CutsceneCommands& c)
	{
		if (parameters[1] == "play")
		{
			//TODO: Deal with multiple channels
			if (parameters.size() > 3)
			{
				c.manager->game->soundManager.PlaySound(c.pathPrefix + c.ParseStringValue(parameters[3]), c.ParseNumberValue(parameters[2]), -1);
			}
			else
			{
				c.manager->game->soundManager.PlaySound(c.pathPrefix + c.ParseStringValue(parameters[2]), -1, -1);
			}
		}
		else if (parameters[1] == "volume")
		{
			c.manager->game->soundManager.SetVolumeSound(c.ParseNumberValue(parameters[2]));
		}
		else if (parameters[1] == "volumei")
		{
			c.manager->game->soundManager.SetVolumeSoundIndex(c.ParseNumberValue(parameters[2]));
		}
		else if (parameters[1] == "fadeout")
		{
			if (c.manager->isSkipping)
			{
				c.manager->game->soundManager.FadeOutChannel(1);
			}
			else
			{
				if (parameters.size() > 3)
					c.manager->game->soundManager.FadeOutChannel(c.ParseNumberValue(parameters[2]), c.ParseNumberValue(parameters[3]));
				else
					c.manager->game->soundManager.FadeOutChannel(c.ParseNumberValue(parameters[2]));
			}
		}
		else if (parameters[1] == "fadeoutw")
		{
			if (c.manager->isSkipping)
			{
				c.manager->game->soundManager.FadeOutChannel(1);
			}
			else
			{
				if (parameters.size() > 3)
					c.manager->game->soundManager.FadeOutChannel(c.ParseNumberValue(parameters[2]), c.ParseNumberValue(parameters[3]));
				else
					c.manager->game->soundManager.FadeOutChannel(c.ParseNumberValue(parameters[2]));

				Wait({ "", parameters[2] }, c);
			}
		}
		else if (parameters[1] == "stop")
		{
			if (parameters.size() == 3)
			{
				const int ch = c.ParseNumberValue(parameters[2]);
				if (c.manager->game->soundManager.sounds.count(ch) != 0)
				{
					if (c.manager->game->soundManager.sounds[ch] != nullptr)
					{
						c.manager->game->soundManager.sounds[ch]->Stop();
					}
				}
			}
			else
			{
				for (auto& [key, channel] : c.manager->game->soundManager.sounds)
				{
					if (channel != nullptr && channel->loop != 0)
					{
						channel->Stop();
					}
				}
			}

		}

		return 0;
	}

	int SoundCommand(CutsceneParameters parameters, CutsceneCommands& c)
	{
		if (parameters[1] == "play")
		{
			//TODO: Deal with multiple channels
			if (parameters.size() > 3)
			{
				c.manager->game->soundManager.PlaySound(c.pathPrefix + c.ParseStringValue(parameters[3]), c.ParseNumberValue(parameters[2]));
			}
			else
			{
				c.manager->game->soundManager.PlaySound(c.pathPrefix + c.ParseStringValue(parameters[2]));
			}
		}
		else if (parameters[1] == "loop")
		{
			// TODO: We can probably just eliminate this command altogether
			// because a Music Effect is just a sound effect that loops,
			// and we want to have control over the properties of individual channels anyway
			MusicEffectCommand({ "me", "play", parameters[2], parameters[3] }, c);
		}
		else if (parameters[1] == "volume")
		{
			c.manager->game->soundManager.SetVolumeSoundOnChannel(c.ParseNumberValue(parameters[3]), c.ParseNumberValue(parameters[2]));
		}
		else if (parameters[1] == "volumei")
		{
			c.manager->game->soundManager.SetVolumeSoundIndex(c.ParseNumberValue(parameters[2]));
		}
		else if (parameters[1] == "stop")
		{
			if (parameters.size() == 3)
			{
				const int ch = c.ParseNumberValue(parameters[2]);
				if (c.manager->game->soundManager.sounds.count(ch) != 0)
				{
					if (c.manager->game->soundManager.sounds[ch] != nullptr)
					{
						c.manager->game->soundManager.sounds[ch]->Stop();
					}
				}
			}
			else
			{
				for (auto& [key, channel] : c.manager->game->soundManager.sounds)
				{
					if (channel != nullptr && channel->loop == 0)
					{
						channel->Stop();
					}
				}
			}
		}

		return 0;
	}

	int IfCondition(CutsceneParameters parameters, CutsceneCommands& c)
	{
		int index = 1;
		bool conditionIsTrue = false;

		// If waiting for a button press, evaluate these statements on button press
		if (c.manager->waitingForButton)
		{
			std::string statement = "";
			for (int i = 0; i < parameters.size(); i++)
			{
				statement += parameters[i] + " ";
			}
			c.manager->choiceIfStatements.push_back(statement);
		}


		do
		{
			c.leftHandIsNumber = false;
			c.rightHandIsNumber = false;

			c.leftValueStr = "";
			c.rightValueStr = "";
			c.leftValueNum = 0;
			c.rightValueNum = 0;

			// 1. First, check the left hand side - is it a string, number, strvar, or numvar?
			// 2. Then, check the operator (>, >=, <, <=, ==, !=)
			// 3. Then, check the right hand side - is it a string, number, strvar, or numvar?
			// 4. If left and right sides are the same type, evaluate the condition
			// 5. If the condition is true, execute the command that follows it
			// 6. If there's an &&, repeat for the next condition, and only execute if all true

			// NOTE: We don't ~really~ need OR because we can just do another IF on the next line
			std::string word = parameters[index];
			switch (parameters[index][0])
			{
			case '$': // string variable
				c.leftHandIsNumber = false;
				c.leftValueStr = c.ParseStringValue(word);
				std::replace(c.leftValueStr.begin(), c.leftValueStr.end(), '[', ' ');
				std::replace(c.leftValueStr.begin(), c.leftValueStr.end(), ']', ' ');
				Trim(c.leftValueStr);
				break;
			case '%': // number variable
				c.leftHandIsNumber = true;
				c.leftValueNum = c.ParseNumberValue(word);
				break;
			case '?':
				if (c.GetArray(parameters[index]))
				{
					c.leftHandIsNumber = true;
					c.leftValueNum = c.arrayVariables[c.arrayIndex][c.vectorIndex];
				}
				else
				{
					return -1;
				}
				break;
			default:
				if (parameters[index].find_first_not_of(c.DIGITMASK) == std::string::npos)
				{
					c.leftValueNum = c.ParseNumberValue(parameters[index]);
					c.leftHandIsNumber = true;
				}
				else
				{
					c.leftValueStr = c.ParseStringValue(parameters[index]);
					std::replace(c.leftValueStr.begin(), c.leftValueStr.end(), '[', ' ');
					std::replace(c.leftValueStr.begin(), c.leftValueStr.end(), ']', ' ');
					Trim(c.leftValueStr);
					c.leftHandIsNumber = false;
				}

				break;
			}

			index += 2; // skip the operator

			word = parameters[index];
			switch (parameters[index][0])
			{
			case '$': // string variable
				c.rightHandIsNumber = false;
				c.rightValueStr = c.ParseStringValue(word);
				std::replace(c.rightValueStr.begin(), c.rightValueStr.end(), '[', ' ');
				std::replace(c.rightValueStr.begin(), c.rightValueStr.end(), ']', ' ');
				Trim(c.rightValueStr);
				break;
			case '%': // number variable
				c.rightHandIsNumber = true;
				c.rightValueNum = c.ParseNumberValue(word);
				break;
			case '?':
				if (c.GetArray(parameters[1]))
				{
					c.rightHandIsNumber = true;
					c.rightValueNum = c.arrayVariables[c.arrayIndex][c.vectorIndex];
				}
				else
				{
					return -1;
				}
				break;
			default:
				if (parameters[index].find_first_not_of(c.DIGITMASK) == std::string::npos)
				{
					c.rightValueNum = c.ParseNumberValue(parameters[index]);
					c.rightHandIsNumber = true;
				}
				else
				{
					c.rightValueStr = c.ParseStringValue(parameters[index]);
					std::replace(c.rightValueStr.begin(), c.rightValueStr.end(), '[', ' ');
					std::replace(c.rightValueStr.begin(), c.rightValueStr.end(), ']', ' ');
					Trim(c.rightValueStr);
					c.rightHandIsNumber = false;
				}

				break;
			}

			// Don't do any comparison if they are not the same type
			if (c.leftHandIsNumber != c.rightHandIsNumber)
				return -1;

			index--; // go back to operator

			if (parameters[index] == ">")
			{
				if (c.leftHandIsNumber)
				{
					conditionIsTrue = (c.leftValueNum > c.rightValueNum);
				}
				else
				{
					conditionIsTrue = (c.leftValueStr > c.rightValueStr);
				}
			}
			else if (parameters[index] == ">=")
			{
				if (c.leftHandIsNumber)
				{
					conditionIsTrue = (c.leftValueNum >= c.rightValueNum);
				}
				else
				{
					conditionIsTrue = (c.leftValueStr >= c.rightValueStr);
				}
			}
			else if (parameters[index] == "<")
			{
				if (c.leftHandIsNumber)
				{
					conditionIsTrue = (c.leftValueNum < c.rightValueNum);
				}
				else
				{
					conditionIsTrue = (c.leftValueStr < c.rightValueStr);
				}
			}
			else if (parameters[index] == "<=")
			{
				if (c.leftHandIsNumber)
				{
					conditionIsTrue = (c.leftValueNum <= c.rightValueNum);
				}
				else
				{
					conditionIsTrue = (c.leftValueStr <= c.rightValueStr);
				}
			}
			else if (parameters[index] == "==" || parameters[index] == "=")
			{
				if (c.leftHandIsNumber)
				{
					conditionIsTrue = (c.leftValueNum == c.rightValueNum);
				}
				else
				{
					conditionIsTrue = (c.leftValueStr == c.rightValueStr);
				}
			}
			else if (parameters[index] == "!=")
			{
				if (c.leftHandIsNumber)
				{
					conditionIsTrue = (c.leftValueNum != c.rightValueNum);
				}
				else
				{
					conditionIsTrue = (c.leftValueStr != c.rightValueStr);
				}
			}

			// If the condition is true, check for other conditions
			if (conditionIsTrue)
			{
				index += 2;

				// If all conditions are true, execute the following commands
				if (parameters[index] != "&&")
				{
					c.manager->foundTrueConditionOnBtnWait = true;

					c.nextCommand = "";
					for (int i = index; i < parameters.size(); i++)
						c.nextCommand += (parameters[i] + " ");

					// split the command into multiple commands if necessary
					if (c.nextCommand.find(':') != std::string::npos)
					{
						c.subcommands.clear();
						int cmdLetterIndex = 0;
						int cmdLetterLength = 0;
						while (cmdLetterIndex < c.nextCommand.size())
						{
							cmdLetterIndex++;
							cmdLetterLength++;
							if (c.nextCommand[cmdLetterIndex] == ':' || cmdLetterIndex >= c.nextCommand.size())
							{
								if (c.nextCommand != "" && c.nextCommand != " ")
								{
									c.subcommands.emplace_back(c.nextCommand.substr((cmdLetterIndex - cmdLetterLength), cmdLetterLength));
									cmdLetterIndex++;
								}
								cmdLetterLength = 0;
							}
						}

						// We must break early here if gosub or goto because otherwise it will continue to carry out commands in here
						for (int i = 0; i < c.subcommands.size(); i++)
						{
							c.ExecuteCommand(Trim(c.subcommands[i]));

							// Save commands to be called after gosub
							if (c.subcommands[i].find("gosub") != std::string::npos)
							{
								for (int k = i + 1; k < c.subcommands.size(); k++)
								{
									c.manager->gosubStack.back()->commands.emplace_back(Trim(c.subcommands[k]));
								}
								return 0;
							}

							// No need to call any commands after goto
							//if (c.subcommands[i].find("goto") != std::string::npos)
							//	return 0;
						}
					}
					else
					{
						// If this is from a choice, don't evaluate any more
						c.manager->choiceIfStatements.clear();

						c.ExecuteCommand(Trim(c.nextCommand));
					}

				}
				else
				{
					conditionIsTrue = false;
					index++;
				}
			}
			else // else exit, do nothing
			{
				return -198;
			}

		} while (!conditionIsTrue);


		return 0;
	}

	int DefineUserFunction(CutsceneParameters parameters, CutsceneCommands& c)
	{
		// If function name does not exist, add it to the list
		if (c.userDefinedFunctions.count(parameters[1]) == 0)
		{
			UserDefinedFunction* newFunction = neww UserDefinedFunction;
			newFunction->functionName = parameters[1];

			for (int i = 2; i < parameters.size(); i++)
				newFunction->parameters.push_back(parameters[i]);

			c.userDefinedFunctions[parameters[1]] = newFunction;
		}

		return 0;
	}

	int GoSubroutine(CutsceneParameters parameters, CutsceneCommands& c)
	{
		// Save our current spot in the text file
		c.manager->PushCurrentSceneDataToStack();

		// Jump to the specified label
		GoToLabel(parameters, c);

		return 0;
	}

	int ReturnFromSubroutine(CutsceneParameters parameters, CutsceneCommands& c)
	{
		SceneData data;

		if (!c.manager->PopSceneDataFromStack(data))
		{
			c.manager->game->logger.Log("ERROR: Nowhere to return to!");
			return -99;
		}

		// Check the label name to see if it is a variable
		const std::string labelName = c.ParseStringValue(data.labelName);

		c.manager->currentLabel = c.manager->JumpToLabel(labelName.c_str());
		if (c.manager->currentLabel == nullptr)
		{
			if (data.labelIndex < c.manager->labels.size())
				c.manager->currentLabel = &c.manager->labels[data.labelIndex];
			else
				std::cout << "ERROR: Could not find label " << labelName << std::endl;
		}

		c.manager->FlushCurrentColor();

		// Execute commands on the same line
		for (int i = 0; i < data.commands.size(); i++)
		{
			c.ExecuteCommand(data.commands[i]);
		}

		return 0;
	}

	// How choices work:
	// defchoice 1 [choice command]
	// ^ do this in the definition file
	// Then in the game script:
	// choice show 1
	// will call the corresponding choice command
	// This is done so that the game knows the set of all choices and where they all go
	// so that you can create an in-game graph of which choices the player hasn't explored yet
	// (Or, you can ignore this and just use the normal choice command)
	int DefineChoice(CutsceneParameters parameters, CutsceneCommands& c)
	{
		int index = c.ParseNumberValue(parameters[1]);

		SceneChoice sceneChoice;
		int size = c.ParseNumberValue(parameters[2]);
		sceneChoice.resultVariable = parameters[3];
		sceneChoice.prompt = parameters[4];

		for (int i = 0; i < size * 2; i += 2)
		{
			std::string response = parameters[i + 5];
			std::string label = parameters[i + 6];
			std::replace(response.begin(), response.end(), '[', ' ');
			std::replace(response.begin(), response.end(), ']', ' ');
			std::replace(label.begin(), label.end(), '[', ' ');
			std::replace(label.begin(), label.end(), ']', ' ');
			sceneChoice.responses.emplace_back(response);
			sceneChoice.labels.emplace_back(label);
		}

		sceneChoice.label = c.manager->GetLabelName(c.manager->GetCurrentLabel());

		c.manager->allChoices[index] = sceneChoice;

		return 0;
	}

	//TODO: Change properties of the choices
	// (font type, size, color, position, alignment, etc.)
	int DisplayChoice(CutsceneParameters parameters, CutsceneCommands& c)
	{
		c.manager->currentChoice = -1;

		if (parameters[1] == "bg")
		{
			c.choiceBGFilePath = c.ParseStringValue(parameters[2]);
			return 0;
		}
		else if (parameters[1] == "clear")
		{
			c.manager->selectedChoices.clear();
			return 0;
		}
		else if (parameters[1] == "get")
		{
			// TODO: Get other types of choice info
			if (parameters[2] == "seen")
			{
				if (parameters[3] == "size")
				{
					MoveVariables({ "mov", parameters[4], std::to_string(c.manager->selectedChoices.size()) }, c);
				}
				else if (parameters[3] == "prompt")
				{
					int selectedNum = c.ParseNumberValue(parameters[5]);
					int choiceNumber = c.manager->selectedChoices[selectedNum].choiceNumber;
					MoveVariables({ "mov", parameters[4], c.manager->allChoices[choiceNumber].prompt }, c);
				}
				else if (parameters[3] == "response")
				{
					int selectedNum = c.ParseNumberValue(parameters[5]);
					int choiceNumber = c.manager->selectedChoices[selectedNum].choiceNumber;
					int responseNumber = c.manager->selectedChoices[selectedNum].responseNumber;
					MoveVariables({ "mov", parameters[4],  c.manager->allChoices[choiceNumber].responses[responseNumber] }, c);
				}
			}

			return 0;
		}
		else if (parameters[1] == "show")
		{
			int result = DisplayChoice(c.manager->allChoices[c.ParseNumberValue(parameters[2])].GetCommandString(), c);
			c.manager->currentChoice = c.ParseNumberValue(parameters[2]);
			return result;
		}
		else
		{

			unsigned int numberOfChoices = c.ParseNumberValue(parameters[1]);

			int index = 3; // skip 2

			// TODO: Should be able to customize this number via script
			int spriteNumber = c.manager->choiceSpriteStartNumber;

			std::string choiceQuestion = c.ParseStringValue(parameters[index]);

			LoadSprite({ "ld", std::to_string(spriteNumber), c.choiceBGFilePath,
				std::to_string(c.manager->game->screenWidth), std::to_string(c.manager->game->screenHeight) }, c);

			spriteNumber++;
			int choiceYPos = 280;
			LoadText({ "", std::to_string(spriteNumber), std::to_string(c.manager->game->screenWidth), std::to_string(choiceYPos), choiceQuestion }, c);
			AlignCommand({ "align", "x", "center", std::to_string(spriteNumber) }, c);
			spriteNumber++;

			c.manager->choiceIfStatements.clear();
			//c.manager->activeButtons.clear();

			for (int i = 0; i < numberOfChoices; i++)
			{
				// Get the text and label for the choice
				index++;
				std::string choiceText = c.ParseStringValue(parameters[index]);
				index++;
				std::string choiceLabel = c.ParseStringValue(parameters[index]);

				// Display the choice as a text sprite on the screen
				std::string choiceNumber = std::to_string(spriteNumber + i);
				std::string responseNumber = std::to_string(i);

				//TODO: Don't hard-code these numbers
				choiceYPos = 400 + (120 * i);

				//TODO: Don't hardcode 1280
				LoadText({ "", choiceNumber, "1280",
					std::to_string(choiceYPos), choiceText }, c);

				c.manager->images[c.ParseNumberValue(choiceNumber)]->collider.offset.x = 0;
				c.manager->images[c.ParseNumberValue(choiceNumber)]->CalculateCollider();

				AlignCommand({ "align", "x", "center", choiceNumber }, c);

				// Make the text sprite act as a button
				SetSpriteButton({ "", choiceNumber , responseNumber }, c);
				if (i == 0)
				{
					c.manager->images[std::stoi(choiceNumber)]->SetColor({ 255, 255, 0, 255 });
				}

				// Wait for button input, store result in variable
				WaitForButton({ "choice",  parameters[2] }, c);

				// Construct the if-statement and store it
				//if %42 == 21 goto label_left ;

				c.manager->choiceIfStatements.push_back("if %" + parameters[2] + " == " + responseNumber + " goto " + choiceLabel + " ;");
			}

			c.manager->atChoice = true;
			if (c.manager->autoChoice == 0)
			{
				c.manager->inputTimer.Start(c.manager->inputTimeToWait);
			}
			else
			{
				c.manager->MakeChoice();
			}

			return 0;
		}

	}

	int WaitForClick(CutsceneParameters parameters, CutsceneCommands& c)
	{
		c.manager->waitingForClick = true;
		c.manager->waitingForButton = true;

		return 0;
	}

	int WaitForButton(CutsceneParameters parameters, CutsceneCommands& c)
	{
		// If there are no active buttons, you can't wait for a button
		if (c.manager->activeButtons.size() > 0)
		{
			// Get the variable number to store the result in
			c.manager->buttonResult = c.ParseNumberValue(parameters[1]);

			// Change the state of the game to wait until a button has been pressed
			c.manager->waitingForButton = true;

			// Set the first button as highlighted
			c.manager->buttonIndex = 0;
			//c.manager->images[c.manager->activeButtons[c.manager->buttonIndex]]->
			//	GetSprite()->color = { 255, 255, 0, 255 };

			c.manager->isCarryingOutCommands = true;
			c.manager->isReadingNextLine = true;
			c.manager->textbox->isReading = false;

			c.manager->textbox->text->SetText(c.manager->previousText);
			c.manager->currentText = "";

			// Clear out the list of if-statments for this button press
			if (parameters[0] != "choice")
			{
				c.manager->choiceIfStatements.clear();
			}

		}

		return 0;
	}

	int SetSpriteButton(CutsceneParameters parameters, CutsceneCommands& c)
	{
		unsigned int spriteNumber = c.ParseNumberValue(parameters[1]);
		unsigned int buttonNumber = c.ParseNumberValue(parameters[2]);

		c.manager->spriteButtons[spriteNumber] = buttonNumber;

		c.manager->activeButtons.push_back(spriteNumber);

		return 0;
	}

	int GoToLabel(CutsceneParameters parameters, CutsceneCommands& c)
	{
		if (parameters[1][0] == '*') // remove leading * if there is one
		{
			c.manager->PlayCutscene(c.ParseStringValue(parameters[1].substr(1, parameters[1].size() - 1)).c_str());
		}
		else
		{
			// Check the label name to see if it is a variable
			c.manager->PlayCutscene(c.ParseStringValue(parameters[1]).c_str());
		}

		return 0;
	}

	int JumpBack(CutsceneParameters parameters, CutsceneCommands& c)
	{
		// Search all game states going backwards until we find a ~ in the command list
		// If not found, do nothing and carry on as normal
		// Else, jump to that label and set command index to after the ~
		c.manager->JumpBack();
		return 0;
	}

	int JumpForward(CutsceneParameters parameters, CutsceneCommands& c)
	{
		// Search all game states going forwards until we find a ~ in the command list
		// If not found, do nothing and carry on as normal
		// Else, jump to that label and set command index to after the ~
		c.manager->JumpForward();
		return 0;
	}

	int EndGame(CutsceneParameters parameters, CutsceneCommands& c)
	{
		c.manager->game->shouldQuit = true;
		return 0;
	}

	int ResetGame(CutsceneParameters parameters, CutsceneCommands& c)
	{
		c.manager->ClearAllSprites();
		// TODO: Clear BG
		c.manager->game->soundManager.StopBGM();
		// TODO: Stop all sound channels
		c.manager->PlayCutscene("start");
		return 0;
	}

	int SaveGame(CutsceneParameters parameters, CutsceneCommands& c)
	{
		// savegame saves/ file1.sav
		// savegame file1.sav
		// savegame

		try
		{
			if (parameters.size() > 2)
				c.manager->SaveGame(c.ParseStringValue(parameters[2]).c_str(), c.ParseStringValue(parameters[1]).c_str());
			else if (parameters.size() > 1)
				c.manager->SaveGame(c.ParseStringValue(parameters[1]).c_str());
			else
				c.manager->SaveGame("file1.sav");
		}
		catch (const std::exception& ex)
		{
			c.manager->game->logger.Log("ERROR: Saving game: ");
			c.manager->game->logger.Log(ex.what());
			return -1;
		}

		return 0;
	}

	int LoadGame(CutsceneParameters parameters, CutsceneCommands& c)
	{
		try
		{
			if (parameters.size() > 2)
				c.manager->LoadGame(c.ParseStringValue(parameters[2]).c_str(), c.ParseStringValue(parameters[1]).c_str());
			else if (parameters.size() > 1)
				c.manager->LoadGame(c.ParseStringValue(parameters[1]).c_str());
			else
				c.manager->LoadGame("file1.sav");
		}
		catch (const std::exception& ex)
		{
			c.manager->game->logger.Log("ERROR: Loading game: ");
			c.manager->game->logger.Log(ex.what());
			return -1;
		}

		return 0;
	}

	// If the parameter starts with a % sign 
	// - if it is followed by a string, get the number associated with the string,
	// and get the value of the variable of that number
	// - if it is followed by a number, just get the value of the variable of that number
	// otherwise, if it is a string, get the number associated with the string,
	// or if it is a number, just use the number

	// Adds two strings together
	int ConcatenateStringVariables(CutsceneParameters parameters, CutsceneCommands& c)
	{
		if (parameters[1][0] == '$')
			c.key = c.GetNumAlias(parameters[1].substr(1, parameters[1].size() - 1));
		else
			c.key = c.GetNumAlias(parameters[1]);

		/*
		if (c.cacheParseStrings.count(parameters[1]) != 0)
		{
			c.word1 = c.cacheParseStrings[parameters[1]];
		}
		else
		{

		}

		if (c.cacheParseStrings.count(parameters[2]) != 0)
		{
			c.word2 = c.cacheParseStrings[parameters[2]];
		}
		else
		{

		}
		*/

		c.word1 = c.ParseStringValue(parameters[1]);
		c.word2 = c.ParseStringValue(parameters[2]);

		c.stringVariables[c.key] = c.word1 + c.word2;
		c.cacheParseStrings[parameters[1]] = c.stringVariables[c.key];

		// If global variable, save change to file
		if (c.key >= c.manager->globalStart)
			c.manager->SaveGlobalVariable(c.key, c.stringVariables[c.key], false);

		return 0;
	}

	void CacheNumberVariables(CutsceneParameters parameters, CutsceneCommands& c)
	{
		if (c.cacheParseNumbers.count(parameters[1]) != 0)
		{
			c.number1 = c.cacheParseNumbers[parameters[1]];
		}
		else
		{
			c.number1 = c.ParseNumberValue(parameters[1]);
		}

		if (c.cacheParseNumbers.count(parameters[2]) != 0)
		{
			c.number2 = c.cacheParseNumbers[parameters[2]];
		}
		else
		{
			c.number2 = c.ParseNumberValue(parameters[2]);
		}
	}

	//TODO: Check and see if both parameters are strings.
	// If so, then call the function that adds strings together
	int AddNumberVariables(CutsceneParameters parameters, CutsceneCommands& c)
	{
		if (parameters[1][0] == '$')
		{
			ConcatenateStringVariables({ "concat", parameters[1], parameters[2] }, c);
			return 0;
		}

		if (parameters[1][0] == '%')
			c.key = c.GetNumAlias(parameters[1].substr(1, parameters[1].size() - 1));
		else
			c.key = c.GetNumAlias(parameters[1]);

		CacheNumberVariables(parameters, c);

		c.numberVariables[c.key] = c.number1 + c.number2;
		c.cacheParseNumbers[parameters[1]] = c.numberVariables[c.key];

		// If global variable, save change to file
		if (c.key >= c.manager->globalStart)
			c.manager->SaveGlobalVariable(c.key, std::to_string(c.numberVariables[c.key]), true);

		return 0;
	}

	int SubtractNumberVariables(CutsceneParameters parameters, CutsceneCommands& c)
	{
		if (parameters[1][0] == '%')
			c.key = c.GetNumAlias(parameters[1].substr(1, parameters[1].size() - 1));
		else
			c.key = c.GetNumAlias(parameters[1]);

		CacheNumberVariables(parameters, c);

		c.numberVariables[c.key] = c.number1 - c.number2;
		c.cacheParseNumbers[parameters[1]] = c.numberVariables[c.key];

		// If global variable, save change to file
		if (c.key >= c.manager->globalStart)
			c.manager->SaveGlobalVariable(c.key, std::to_string(c.numberVariables[c.key]), true);

		return 0;
	}

	int MultiplyNumberVariables(CutsceneParameters parameters, CutsceneCommands& c)
	{
		if (parameters[1][0] == '%')
			c.key = c.GetNumAlias(parameters[1].substr(1, parameters[1].size() - 1));
		else
			c.key = c.GetNumAlias(parameters[1]);

		CacheNumberVariables(parameters, c);

		c.numberVariables[c.key] = c.number1 * c.number2;
		c.cacheParseNumbers[parameters[1]] = c.numberVariables[c.key];

		// If global variable, save change to file
		if (c.key >= c.manager->globalStart)
			c.manager->SaveGlobalVariable(c.key, std::to_string(c.numberVariables[c.key]), true);

		return 0;
	}

	int DivideNumberVariables(CutsceneParameters parameters, CutsceneCommands& c)
	{
		if (parameters[1][0] == '%')
			c.key = c.GetNumAlias(parameters[1].substr(1, parameters[1].size() - 1));
		else
			c.key = c.GetNumAlias(parameters[1]);

		CacheNumberVariables(parameters, c);

		c.numberVariables[c.key] = c.number1 / c.number2;
		c.cacheParseNumbers[parameters[1]] = c.numberVariables[c.key];

		// If global variable, save change to file
		if (c.key >= c.manager->globalStart)
			c.manager->SaveGlobalVariable(c.key, std::to_string(c.numberVariables[c.key]), true);

		return 0;
	}

	int ModNumberVariables(CutsceneParameters parameters, CutsceneCommands& c)
	{
		if (parameters[1][0] == '%')
			c.key = c.GetNumAlias(parameters[1].substr(1, parameters[1].size() - 1));
		else
			c.key = c.GetNumAlias(parameters[1]);

		CacheNumberVariables(parameters, c);

		c.numberVariables[c.key] = c.number1 % c.number2;
		c.cacheParseNumbers[parameters[1]] = c.numberVariables[c.key];

		// If global variable, save change to file
		if (c.key >= c.manager->globalStart)
			c.manager->SaveGlobalVariable(c.key, std::to_string(c.numberVariables[c.key]), true);

		return 0;
	}

	int RandomNumberVariable(CutsceneParameters parameters, CutsceneCommands& c)
	{
		if (parameters[1] == "seed")
		{
			if (parameters[2] == "time")
			{
				c.manager->game->randomManager.Seed();
			}
			else
			{
				c.manager->game->randomManager.Seed(c.ParseNumberValue(parameters[2]));
			}
		}
		else if (parameters[1] == "range")
		{
			c.key = c.ParseNumberValue(parameters[2]);
			unsigned int minNumber = c.ParseNumberValue(parameters[3]);
			unsigned int maxNumber = c.ParseNumberValue(parameters[4]);

			c.numberVariables[c.key] = c.manager->game->randomManager.RandomRange(minNumber, maxNumber);

			// If global variable, save change to file
			if (c.key >= c.manager->globalStart)
				c.manager->SaveGlobalVariable(c.key, std::to_string(c.numberVariables[c.key]), true);
		}
		else // no offset
		{
			c.key = c.ParseNumberValue(parameters[1]);
			unsigned int maxNumber = c.ParseNumberValue(parameters[2]);

			int value = c.manager->game->randomManager.RandomInt(maxNumber);
			c.numberVariables[c.key] = value;

			// If global variable, save change to file
			if (c.key >= c.manager->globalStart)
				c.manager->SaveGlobalVariable(c.key, std::to_string(c.numberVariables[c.key]), true);
		}

		return 0;
	}

	int SubstringVariables(CutsceneParameters parameters, CutsceneCommands& c)
	{
		if (parameters[1][0] == '$')
		{
			c.key = c.numalias[parameters[1].substr(1, parameters[1].size() - 1)];
		}
		else
		{
			c.key = c.numalias[parameters[1]];
		}

		c.parseStringValue = c.ParseStringValue(parameters[2]);

		c.stringVariables[c.key] = c.parseStringValue.substr(std::stoi(parameters[3]), std::stoi(parameters[4]));

		return 0;
	}

	// TODO: mov x,y -> comma does not work when inside of an "if" condition
	int MoveVariables(CutsceneParameters parameters, CutsceneCommands& c)
	{
		if (parameters[1][0] == '$')
		{
			SetStringVariable({ "mov", parameters[1].substr(1, parameters[1].size() - 1), parameters[2] }, c);
		}
		else if (parameters[1][0] == '%')
		{
			c.cacheParseNumbers.erase(parameters[1]); // remove %var (with % sign) from cache
			SetNumberVariable({ "mov", parameters[1].substr(1, parameters[1].size() - 1), parameters[2] }, c);
		}
		else if (parameters[1][0] == '?')
		{
			// mov ?j_up[3][1] %var1

			if (c.GetArray(parameters[1]))
			{
				c.arrayVariables[c.arrayIndex][c.vectorIndex] = c.ParseNumberValue(parameters[2]);

				// If global variable, save change to file
				if (c.arrayIndex >= c.manager->globalStart)
					c.manager->SaveGlobalVariable(c.arrayIndex, "?", true);
			}
			else
			{
				return -1;
			}
		}
		else
		{
			return -1;
		}

		return 0;
	}

	int LoadBackground(CutsceneParameters parameters, CutsceneCommands& c)
	{
		LoadSprite({ "", parameters[0], parameters[1], parameters[2] }, c);

		return 0;
	}

	int ClearSprite(CutsceneParameters parameters, CutsceneCommands& c)
	{
		if (parameters[1] == "a")
		{
			c.manager->ClearAllSprites();
		}
		else
		{
			unsigned int imageNumber = c.ParseNumberValue(parameters[1]);

			if (c.manager->images[imageNumber] != nullptr)
				delete_it(c.manager->images[imageNumber]);

			if (parameters.size() > 2)
			{
				PrintCommand({ "print", parameters[2] }, c);
			}
		}

		return 0;
	}

	int LoadSprite(CutsceneParameters parameters, CutsceneCommands& c)
	{
		//if (c.manager->isTravelling)
		//	return 0;

		// std::cout << "Loading sprite: " << parameters[1] << std::endl;

		Vector2 pos = Vector2(0, 0);

		bool isStandingImage = parameters[1] == "l" || parameters[1] == "c" || parameters[1] == "r";

		if (!isStandingImage && parameters[1] != "bg")
		{
			const unsigned int x = c.ParseNumberValue(parameters[3]);
			const unsigned int y = c.ParseNumberValue(parameters[4]);
			pos = Vector2(x, y);

			if (parameters.size() > 5)
				PrintCommand({ "print", parameters[5] }, c);
			else if (parameters.size() == 3)
				PrintCommand({ "print", parameters[2] }, c);
			else
				PrintCommand({ "print", "0" }, c);
		}

		std::string filepath = c.pathPrefix + c.ParseStringValue(parameters[2]);
		unsigned int imageNumber = c.ParseNumberValue(parameters[1]);

		//TODO: Don't delete/new, just grab from entity pool and reset
		if (c.manager->images[imageNumber] != nullptr)
			delete c.manager->images[imageNumber];

		c.manager->images[imageNumber] = neww Entity(pos);

		Entity& newImage = *c.manager->images[imageNumber];

		newImage.GetSprite()->SetTexture(c.manager->game->spriteManager.GetImage(filepath));

		// TODO: Instead of always using the default shader, we should be able to customize this
		// so that we can load sprites under filters (such as grayscale, sepia, or other shaders)

		if (c.shaderFilter == "")
		{
			newImage.GetSprite()->SetShader(c.manager->game->renderer.shaders[ShaderName::Default]);
		}
		else if (imageNumber >= c.filterMin && imageNumber <= c.filterMax)
		{
			newImage.GetSprite()->SetShader(c.customShaders[c.shaderFilter]);
		}

		newImage.CreateCollider(0, 0, newImage.GetSprite()->frameWidth, newImage.GetSprite()->frameHeight);

		if (isStandingImage)
		{
			int halfScreenWidth = ((c.manager->game->screenWidth * 2) / 2);
			int spriteX = 0; // (c.manager->game->screenWidth / 5) * 3;
			int spriteY = c.manager->game->screenHeight;

			switch (parameters[1][0])
			{
			case 'l':
				spriteX = halfScreenWidth - (halfScreenWidth / 2);
				spriteY = (c.manager->game->screenHeight * 2) -
					(c.manager->images[imageNumber]->GetSprite()->frameHeight);

				pos = Vector2(spriteX + c.manager->game->renderer.guiCamera.position.x,
					spriteY + c.manager->game->renderer.guiCamera.position.y);
				break;
			case 'c':
				spriteX = halfScreenWidth; // +(sprites['c']->frameWidth / 2);
				spriteY = (c.manager->game->screenHeight * 2) -
					(c.manager->images[imageNumber]->GetSprite()->frameHeight);

				pos = Vector2(spriteX + c.manager->game->renderer.guiCamera.position.x,
					spriteY + c.manager->game->renderer.guiCamera.position.y);

				break;
			case 'r':
				spriteX = halfScreenWidth + (halfScreenWidth / 2);
				spriteY = (c.manager->game->screenHeight * 2) -
					(c.manager->images[imageNumber]->GetSprite()->frameHeight);

				pos = Vector2(spriteX + c.manager->game->renderer.guiCamera.position.x,
					spriteY + c.manager->game->renderer.guiCamera.position.y);

				break;
			default:
				break;
			}
			newImage.SetPosition(pos);

			if (parameters.size() > 3)
				PrintCommand({ "print", parameters[3] }, c);
			else
				PrintCommand({ "print", "1" }, c);
		}
		else if (parameters[1] == "bg")
		{
			c.manager->ClearAllSprites();

			int halfScreenWidth = ((c.manager->game->screenWidth * 2) / 2);
			int spriteY = c.manager->game->screenHeight;

			int spriteX = halfScreenWidth; // +(sprites['c']->frameWidth / 2);
			spriteY = (c.manager->game->screenHeight * 2) -
				(newImage.GetSprite()->frameHeight);

			pos = Vector2(spriteX + c.manager->game->renderer.guiCamera.position.x,
				spriteY + c.manager->game->renderer.guiCamera.position.y);

			newImage.SetPosition(pos);

			PrintCommand({ "print", parameters[3] }, c);
		}

		newImage.drawOrder = imageNumber;
		newImage.GetSprite()->keepPositionRelativeToCamera = true;
		newImage.GetSprite()->keepScaleRelativeToCamera = true;

		return 0;
	}

	int LoadTextFromSaveFile(CutsceneParameters parameters, CutsceneCommands& c)
	{
		unsigned int imageNumber = c.ParseNumberValue(parameters[0]);
		const unsigned int x = c.ParseNumberValue(parameters[2]);
		const unsigned int y = c.ParseNumberValue(parameters[3]);
		Vector2 pos = Vector2(x, y);

		glm::vec3 rotation = glm::vec3(
			std::stoi(parameters[4]),
			std::stoi(parameters[5]),
			std::stoi(parameters[6]));

		Vector2 scale = Vector2(std::stoi(parameters[7]), std::stoi(parameters[8]));

		std::string text = "";

		int paramIndex = 9;
		int subIndex = 0;

		bool foundStart = false;
		bool foundEnd = false;

		while (!foundStart)
		{
			foundStart = (parameters[paramIndex][subIndex] == '[');
			subIndex++;
			if (subIndex > parameters[paramIndex].size())
			{
				paramIndex++;
				subIndex = 0;
			}
		}

		while (!foundEnd)
		{
			if (parameters[paramIndex][subIndex] != '\0'
				&& parameters[paramIndex][subIndex] != ']')
			{
				text += parameters[paramIndex][subIndex];
			}

			foundEnd = (parameters[paramIndex][subIndex] == ']');
			subIndex++;
			if (subIndex > parameters[paramIndex].size())
			{
				paramIndex++;
				subIndex = 0;
			}
		}

		paramIndex++;

		Color textColor = {
			(uint8_t)std::stoi(parameters[paramIndex]),
			(uint8_t)std::stoi(parameters[paramIndex + 1]),
			(uint8_t)std::stoi(parameters[paramIndex + 2]),
			(uint8_t)std::stoi(parameters[paramIndex + 3]),
		};

		if (c.manager->images[imageNumber] != nullptr)
			delete_it(c.manager->images[imageNumber]);

		//TODO: Also save/load in the font type/size/style for this text object
		Text* newText = neww Text(c.manager->game->theFont, text, textColor);

		newText->isRichText = false;

		c.manager->images[imageNumber] = newText;

		c.manager->images[imageNumber]->SetPosition(pos);
		c.manager->images[imageNumber]->rotation = rotation;
		c.manager->images[imageNumber]->scale = scale;
		c.manager->images[imageNumber]->drawOrder = imageNumber;
		c.manager->images[imageNumber]->GetSprite()->keepPositionRelativeToCamera = true;
		c.manager->images[imageNumber]->GetSprite()->keepScaleRelativeToCamera = true;

		return 0;
	}

	int LoadText(CutsceneParameters parameters, CutsceneCommands& c)
	{
		Vector2 pos = Vector2(0, 0);

		if (parameters[1] == "font")
		{
			c.manager->game->CreateFont(c.ParseStringValue(parameters[2]), c.ParseNumberValue(parameters[3]));
			c.textFontKey = c.ParseStringValue(parameters[2]) + std::to_string(c.ParseNumberValue(parameters[3]));
			return 0;
		}

		// text 9 [Hello, world!] 0 0
		// text 9 #ff0000[Hello, world!]#ffffff 0 0
		//TODO: Make sure variables work (%, $)

		unsigned int imageNumber = c.ParseNumberValue(parameters[1]);
		const unsigned int x = c.ParseNumberValue(parameters[2]);
		const unsigned int y = c.ParseNumberValue(parameters[3]);
		pos = Vector2(x, y);

		std::string text = parameters[4];

		// TODO: So now, the entire text is stored within parameters[4]

		for (int i = 0; i < text.size(); i++)
		{
			if (text[i] == '\\' && text[i + 1] == 'n')
			{
				text[i] = '\n';
				text[i + 1] = ' ';
			}
		}

		//TODO: Don't delete/new, just grab from entity pool and reset
		if (c.manager->images[imageNumber] != nullptr)
			delete_it(c.manager->images[imageNumber]);

		Text* newText = nullptr;
		Color textColor = { 255, 255, 255, 255 };

		int letterIndex = 0;
		std::string finalText = "";

		FontInfo* fontInfo = c.manager->textbox->currentFontInfo;

		if (c.textFontKey != "")
		{
			fontInfo = c.manager->game->fonts[c.textFontKey];
		}

		newText = neww Text(fontInfo, "", textColor);
		newText->SetPosition(pos.x, pos.y);
		newText->isRichText = true;

		while (letterIndex < text.size())
		{
			finalText = c.manager->ParseText(text, letterIndex, textColor, newText);
			for (int i = 0; i < finalText.size(); i++)
			{
				newText->AddText(finalText[i], textColor);
				newText->SetPosition(pos.x, pos.y);
			}
		}

		c.manager->images[imageNumber] = newText;
		c.manager->images[imageNumber]->drawOrder = imageNumber;
		c.manager->images[imageNumber]->GetSprite()->keepPositionRelativeToCamera = true;
		c.manager->images[imageNumber]->GetSprite()->keepScaleRelativeToCamera = true;
		c.manager->images[imageNumber]->GetSprite()->texture->SetFilePath("text");
		c.manager->images[imageNumber]->CreateCollider(newText->GetTextWidth(), 0, newText->GetTextWidth(), newText->GetTextHeight());

		// Color the text yellow when we hover the mouse over it or select with keyboard
		//c.manager->images[imageNumber]->GetSprite()->color = { 255, 255, 0, 255 } ;

		return 0;
	}


	// Assign color of text to a speaking character
	int TextColor(CutsceneParameters parameters, CutsceneCommands& c)
	{
		std::string characterName = parameters[1];

		//TODO: Error checking?
		Color color = c.ParseColorFromParameters(parameters, 2);

		if (characterName == "default")
			c.manager->namesToColors[""] = color;
		else
			c.manager->namesToColors[characterName] = color;

		c.manager->FlushCurrentColor();

		return 0;
	}

	int SetSpriteProperty(CutsceneParameters parameters, CutsceneCommands& c)
	{
		unsigned int imageNumber = c.ParseNumberValue(parameters[1]);

		//TODO: Maybe make a c.manager->GetImage(imageNumber) function for error handling
		Entity* entity = c.manager->images[imageNumber];
		if (entity == nullptr)
			return 0; //TODO: Error log

		//Sprite* sprite = c.manager->images[imageNumber]->GetSprite();
		if (entity->GetSprite() == nullptr)
			return 0; //TODO: Error log

		const std::string spriteProperty = c.ParseStringValue(parameters[2]);

		if (spriteProperty == "color")
		{
			Color color = { (uint8_t)c.ParseNumberValue(parameters[3]),
				(uint8_t)c.ParseNumberValue(parameters[4]),
				(uint8_t)c.ParseNumberValue(parameters[5]),
				(uint8_t)c.ParseNumberValue(parameters[6]) };

			entity->GetSprite()->color = color;
		}
		else if (spriteProperty == "scale")
		{
			// Because animators have different sprites for each animation state,
			// we want to change the scale of the entity and then apply that scale
			// to whatever sprite is currently being animated
			entity->scale = Vector2(c.ParseNumberValue(parameters[3]), c.ParseNumberValue(parameters[4]));
			entity->SetSprite(*entity->GetSprite());
		}
		else if (spriteProperty == "rotate")
		{
			entity->rotation = glm::vec3(c.ParseNumberValue(parameters[3]),
				c.ParseNumberValue(parameters[4]), c.ParseNumberValue(parameters[5]));
			entity->SetSprite(*entity->GetSprite());
		}
		else if (spriteProperty == "shader")
		{
			//TODO: Fix this so that it works with enums

			std::string shaderName = c.ParseStringValue(parameters[3]);

			if (c.customShaders.count(shaderName) > 0)
			{
				entity->GetSprite()->SetShader(c.customShaders[shaderName]);
			}
			else
			{
				c.manager->game->logger.Log("ERROR: Shader " + shaderName + " not defined");
			}
		}
		else if (spriteProperty == "animator")
		{
			const std::string animAction = c.ParseStringValue(parameters[3]);

			if (animAction == "=") // set the sprite's animator equal to this one
			{
				if (entity->GetAnimator() != nullptr)
					delete entity->GetAnimator();

				std::vector<AnimState*> animStates = c.manager->game->spriteManager.ReadAnimData(c.ParseStringValue(parameters[4]));

				for (int i = 0; i < animStates.size(); i++)
				{
					//animStates[i]->sprite->keepPositionRelativeToCamera = true;
					//animStates[i]->sprite->keepScaleRelativeToCamera = true;
				}

				Animator* newAnimator = neww Animator("player", animStates, c.ParseStringValue(parameters[5]));
				entity->SetAnimator(*newAnimator);
			}
			else if (entity->GetAnimator() == nullptr)
			{
				std::cout << "Error, sprite " << imageNumber << " does not have animator" << std::endl;
			}
			else if (animAction == "state") // change the animator's state
			{
				entity->GetAnimator()->SetState(parameters[4].c_str());
			}
			else if (animAction == "bool") // change the animator's bool var
			{
				int num = c.ParseNumberValue(parameters[5]);

				//TODO: Don't just hardcode these values, make it work with cutscene vars too
				if (parameters[5] == "true" || parameters[5] == "True" || num > 0)
					entity->GetAnimator()->SetBool(parameters[4].c_str(), true);
				else
					entity->GetAnimator()->SetBool(parameters[4].c_str(), false);
			}
			else if (animAction == "int") // change the animator's int var
			{
				entity->GetAnimator()->SetInt(parameters[4].c_str(), c.ParseNumberValue(parameters[5]));
			}
			else if (animAction == "float") // change the animator's float var
			{
				//TODO: This will not work because the parse function doesn't get floats
				entity->GetAnimator()->SetInt(parameters[4].c_str(), c.ParseNumberValue(parameters[5]));
			}
		}

		return 0;
	}

	//TODO: Maybe put this code somewhere so it can be used
	// both by the cutscene system and the level editor properties?
	int SetVelocity(CutsceneParameters parameters, CutsceneCommands& c)
	{
		/*
		PhysicsComponent* physics = nullptr;

		for (unsigned int i = 0; i < c.manager->game->entities.size(); i++)
		{
			if (c.manager->game->entities[i]->name == parameters[1])
			{
				physics = c.manager->game->entities[i]->physics;

				if (physics != nullptr)
				{
					unsigned int x = c.ParseNumberValue(parameters[2]);
					unsigned int y = c.ParseNumberValue(parameters[3]);
					Vector2 velocity = Vector2(x * 0.001f, y * 0.001f);
					physics->SetVelocity(velocity);
				}
				break;
			}
		}
		*/

		return 0;
	}

	int Wait(CutsceneParameters parameters, CutsceneCommands& c)
	{
		if (!c.manager->isTravelling)
		{
			c.manager->msGlyphTime -= c.ParseNumberValue(parameters[1]); // wait for a certain amount of time (milliseconds)
			c.manager->textbox->isReading = false; // don't render the textbox while waiting
		}

		return 0;
	}

	// namedef but Butler
	// namedef bu2 Butler
	// TODO: How to handle multiple textboxes? Add a function for setting the speaker's text?
	int NameDefineCommand(CutsceneParameters parameters, CutsceneCommands& c)
	{
		c.manager->namesToNames[c.ParseStringValue(parameters[1])] = c.ParseStringValue(parameters[2]);
		return 0;
	}

	// name NAME
	int NameCommand(CutsceneParameters parameters, CutsceneCommands& c)
	{
		c.manager->overwriteName = false;
		c.manager->SetSpeakerText(c.ParseStringValue(parameters[1]));

		return 0;
	}

	int Namebox(CutsceneParameters parameters, CutsceneCommands& c)
	{
		if (parameters[1] == "on")
		{
			c.manager->textbox->shouldRender = true;
		}
		else if (parameters[1] == "off")
		{
			c.manager->textbox->shouldRender = false;
		}
		else if (parameters[1] == "text")
		{
			if (parameters[2] == "color")
			{
				c.manager->textbox->speaker->GetSprite()->color = c.ParseColorFromParameters(parameters, 2);
			}
			else if (parameters[2] == "contents")
			{
				//TODO: This probably won't work well because the text will be overwritten
				// once all the commands are finished executing.
				c.manager->textbox->speaker->SetText(c.ParseStringValue(parameters[2]));
			}
			else if (parameters[2] == "font")
			{
				c.manager->textbox->ChangeNameFont(c.ParseStringValue(parameters[3]), c.ParseNumberValue(parameters[4]));
			}
		}
		else if (parameters[1] == "color")
		{
			c.manager->textbox->nameObject->GetSprite()->color = c.ParseColorFromParameters(parameters, 2);
		}
		else if (parameters[1] == "position")
		{
			Vector2 newPos = Vector2(c.ParseNumberValue(parameters[2]), c.ParseNumberValue(parameters[3]));
			c.manager->textbox->speaker->SetPosition(newPos.x, newPos.y);
			c.manager->textbox->nameObject->SetPosition(newPos);
		}
		else if (parameters[1] == "sprite")
		{
			c.manager->textbox->ChangeNameSprite(c.pathPrefix + c.ParseStringValue(parameters[2]));
		}

		return 0;
	}

	int Textbox(CutsceneParameters parameters, CutsceneCommands& c)
	{
		if (parameters[1] == "on")
		{
			c.manager->textbox->shouldRender = true;
		}
		else if (parameters[1] == "off")
		{
			c.manager->textbox->shouldRender = false;
		}
		else if (parameters[1] == "text")
		{
			if (parameters[2] == "color")
			{
				c.manager->textbox->text->GetSprite()->color = c.ParseColorFromParameters(parameters, 2);
			}
			else if (parameters[2] == "contents")
			{
				//TODO: This probably won't work well because the text will be overwritten
				// once all the commands are finished executing.
				c.manager->textbox->text->SetText(c.ParseStringValue(parameters[2]));
			}
			else if (parameters[2] == "font")
			{
				c.manager->textbox->ChangeBoxFont(c.ParseStringValue(parameters[3]), c.ParseNumberValue(parameters[4]));
			}
		}
		else if (parameters[1] == "color")
		{
			c.manager->textbox->boxObject->GetSprite()->color = c.ParseColorFromParameters(parameters, 2);
		}
		else if (parameters[1] == "position")
		{
			Vector2 newPos = Vector2(c.ParseNumberValue(parameters[2]), c.ParseNumberValue(parameters[3]));
			c.manager->textbox->text->SetPosition(newPos.x, newPos.y);
			c.manager->textbox->boxObject->SetPosition(newPos);
		}
		else if (parameters[1] == "sprite")
		{
			c.manager->textbox->ChangeBoxSprite(c.pathPrefix + c.ParseStringValue(parameters[2]));
		}
		else if (parameters[1] == "wrapWidth")
		{
			c.manager->textbox->boxWidth = c.ParseNumberValue(parameters[2]);
		}
		else if (parameters[1] == "clear")
		{
			c.manager->ClearPage();
		}

		return 0;
	}

	int Fade(CutsceneParameters parameters, CutsceneCommands& c)
	{
		c.manager->game->renderer.changingOverlayColor = true;

		if (parameters.size() == 3)
		{
			c.manager->game->renderer.overlayStartTime = c.manager->game->timer.GetTicks();
			c.manager->game->renderer.overlayEndTime = c.manager->game->renderer.overlayStartTime + std::stoi(parameters[2]);
		}

		c.manager->game->renderer.startColor = c.manager->game->renderer.overlayColor;

		std::cout << "change target color: " << parameters[1] << std::endl;

		if (parameters[1] == "clear")
		{
			c.manager->game->renderer.targetColor = Color{ 0, 0, 0, 0 };
		}
		else if (parameters[1] == "white")
		{
			c.manager->game->renderer.targetColor = Color{ 255, 255, 255, 255 };
		}
		else if (parameters[1] == "black")
		{
			c.manager->game->renderer.targetColor = Color{ 0, 0, 0, 255 };
		}
		else
		{
			if (parameters.size() > 5)
			{
				c.manager->game->renderer.targetColor = {
					(uint8_t)c.ParseNumberValue(parameters[2]),
					(uint8_t)c.ParseNumberValue(parameters[3]),
					(uint8_t)c.ParseNumberValue(parameters[4]),
					(uint8_t)c.ParseNumberValue(parameters[5]) };
			}
			else
			{
				c.manager->game->renderer.targetColor = {
					(uint8_t)c.ParseNumberValue(parameters[2]),
					(uint8_t)c.ParseNumberValue(parameters[3]),
					(uint8_t)c.ParseNumberValue(parameters[4]),
					255 };
			}

		}

		return 0;
	}

	int SetResolution(CutsceneParameters parameters, CutsceneCommands& c)
	{
		//TODO: Maybe place this command in a config file to start the window in a certain resolution?
		const int width = c.ParseNumberValue(parameters[1]);
		const int height = c.ParseNumberValue(parameters[2]);

		c.manager->game->SetScreenResolution(width, height);

		return 0;
	}

	// This number indicates the first global variable slot
	// All variable slots before this number are local
	// Local variables are only saved within the current save file
	// Global variables are saved across all save files
	int SetGlobalNumber(CutsceneParameters parameters, CutsceneCommands& c)
	{
		c.manager->globalStart = c.ParseNumberValue(parameters[1]);
		return 0;
	}

	int OpenBacklog(CutsceneParameters parameters, CutsceneCommands& c)
	{
		if (parameters.size() > 1)
		{
			if (parameters[1] == "size")
			{
				c.manager->backlogMaxSize = c.ParseNumberValue(parameters[2]);
				//TODO: Save this to a settings file
			}
			else if (parameters[1] == "pages")
			{
				c.manager->backlogMaxSize = c.ParseNumberValue(parameters[2]);
			}
			else if (parameters[1] == "open")
			{
				if (c.manager->backlog.size() > 0)
				{
					c.manager->OpenBacklog();
				}
			}
			else if (parameters[1] == "enable")
			{
				c.manager->backlogEnabled = true;
			}
			else if (parameters[1] == "disable")
			{
				c.manager->backlogEnabled = false;
			}
			else if (parameters[1] == "btnUp")
			{
				c.manager->backlogBtnUp = c.ParseStringValue(parameters[2]);
				c.manager->backlogBtnUpX = c.ParseNumberValue(parameters[3]);
				c.manager->backlogBtnUpY = c.ParseNumberValue(parameters[4]);
			}
			else if (parameters[1] == "btnDown")
			{
				c.manager->backlogBtnDown = c.ParseStringValue(parameters[2]);
				c.manager->backlogBtnDownX = c.ParseNumberValue(parameters[3]);
				c.manager->backlogBtnDownY = c.ParseNumberValue(parameters[4]);
			}
			else if (parameters[1] == "color") //TODO: Should this be the same for both name and text?
			{
				if (parameters[2][0] == '#')
				{
					c.manager->backlogColor = ParseColorHexadecimal(parameters[2].c_str());
				}
				else if (parameters.size() == 5)
				{
					c.manager->backlogColor = {
						(uint8_t)c.ParseNumberValue(parameters[4]),
						(uint8_t)c.ParseNumberValue(parameters[3]),
						(uint8_t)c.ParseNumberValue(parameters[2]),
						255 };
				}
				else if (parameters.size() == 6)
				{
					c.manager->backlogColor = {
						(uint8_t)c.ParseNumberValue(parameters[4]),
						(uint8_t)c.ParseNumberValue(parameters[3]),
						(uint8_t)c.ParseNumberValue(parameters[2]),
						(uint8_t)c.ParseNumberValue(parameters[5]) };
				}
			}
		}

		return 0;
	}

	int FlipSprite(CutsceneParameters parameters, CutsceneCommands& c)
	{
		if (parameters.size() > 2)
		{
			int spriteNum = c.ParseNumberValue(parameters[1]);
			Vector2 scale = c.manager->images[spriteNum]->scale;

			std::string direction = c.ParseStringValue(parameters[2]);

			if (direction == "h" || direction == "horizontal")
			{
				SetSpriteProperty({ "", parameters[1], "scale", std::to_string(-scale.x), "1" }, c);
			}
			else if (direction == "v" || direction == "vertical")
			{
				SetSpriteProperty({ "", parameters[1], "scale", "1", std::to_string(-scale.y) }, c);
			}
			else if (direction == "b" || direction == "both")
			{
				SetSpriteProperty({ "", parameters[1], "scale", std::to_string(-scale.x), std::to_string(-scale.y) }, c);
			}
		}

		return 0;
	}

	int TimerFunction(CutsceneParameters parameters, CutsceneCommands& c)
	{
		//TODO: Display errors for syntax errors rather than crashing

		unsigned int timerNumber = c.ParseNumberValue(parameters[2]);

		if (parameters[1] == "start")
		{
			if (c.manager->timers.count(timerNumber) != 1)
				c.manager->timers[timerNumber] = neww Timer();

			unsigned int timerDuration = c.ParseNumberValue(parameters[3]);
			c.manager->timers[timerNumber]->Start(timerDuration);
		}
		else if (parameters[1] == "pause")
		{
			if (c.manager->timers.count(timerNumber) == 1)
			{
				c.manager->timers[timerNumber]->Pause();
			}
		}
		else if (parameters[1] == "unpause")
		{
			if (c.manager->timers.count(timerNumber) == 1)
			{
				c.manager->timers[timerNumber]->Unpause();
			}
		}
		else if (parameters[1] == "reset")
		{
			if (c.manager->timers.count(timerNumber) == 1)
			{
				c.manager->timers[timerNumber]->Reset();
			}
		}
		else if (parameters[1] == "elapsed") //TODO: Change this word?
		{
			if (c.manager->timers.count(timerNumber) == 1)
			{
				// Get the amount of time that has elapsed since the timer started
				SetNumberVariable({ "", parameters[3], std::to_string(c.manager->timers[timerNumber]->GetTicks()) }, c);
			}
		}
		else if (parameters[1] == "delete")
		{
			if (c.manager->timers.count(timerNumber) == 1)
			{
				delete c.manager->timers[timerNumber];
				c.manager->timers.erase(timerNumber);
			}
		}
		else if (parameters[1] == "addcommand")
		{
			std::string cmd = "";

			for (int i = 3; i < parameters.size(); i++)
			{
				cmd += parameters[i] + " ";
			}

			c.manager->timerCommands[timerNumber].push_back(cmd);
		}

		return 0;
	}

	int CameraFunction(CutsceneParameters parameters, CutsceneCommands& c)
	{
		if (parameters[1] == "target")
		{
			//TODO
		}
		else if (parameters[1] == "zoom")
		{
			//TODO: Refactor the camera.Zoom function to make orthoZoom private
			if (parameters[2] == "set")
			{
				c.manager->game->renderer.camera.orthoZoom = c.ParseNumberValue(parameters[3]);
				c.manager->game->renderer.camera.Zoom(0, c.manager->game->screenWidth, c.manager->game->screenHeight);
			}
			else if (parameters[2] == "add")
			{
				c.manager->game->renderer.camera.orthoZoom += c.ParseNumberValue(parameters[3]);
				c.manager->game->renderer.camera.Zoom(0, c.manager->game->screenWidth, c.manager->game->screenHeight);
			}
			else if (parameters[2] == "sub")
			{
				c.manager->game->renderer.camera.orthoZoom -= c.ParseNumberValue(parameters[3]);
				c.manager->game->renderer.camera.Zoom(0, c.manager->game->screenWidth, c.manager->game->screenHeight);
			}
			else if (parameters[2] == "lerp")
			{
				//TODO
			}
		}
		else if (parameters[1] == "position")
		{
			if (parameters[2] == "set")
			{
				c.manager->game->renderer.camera.position = glm::vec3(c.ParseNumberValue(parameters[3]),
					c.ParseNumberValue(parameters[4]), c.ParseNumberValue(parameters[5]));
			}
			else if (parameters[2] == "moveto") //TODO: Last parameter is time or speed?
			{
				const float SPEED = std::stof(parameters[6]);

				if (c.manager->game->renderer.camera.position.x != c.ParseNumberValue(parameters[3]))
				{
					if (c.ParseNumberValue(parameters[3]) > c.manager->game->renderer.camera.position.x)
						c.manager->game->renderer.camera.position.x += SPEED;
					else
						c.manager->game->renderer.camera.position.x -= SPEED;
				}

				if (c.manager->game->renderer.camera.position.y != c.ParseNumberValue(parameters[4]))
				{
					if (c.ParseNumberValue(parameters[4]) > c.manager->game->renderer.camera.position.y)
						c.manager->game->renderer.camera.position.y += SPEED;
					else
						c.manager->game->renderer.camera.position.y -= SPEED;
				}

				if (c.manager->game->renderer.camera.position.z != c.ParseNumberValue(parameters[5]))
				{
					if (c.ParseNumberValue(parameters[5]) > c.manager->game->renderer.camera.position.z)
						c.manager->game->renderer.camera.position.z += SPEED;
					else
						c.manager->game->renderer.camera.position.z -= SPEED;
				}

				//TODO: Round these to ints if they are not even
				glm::vec3 intPos = c.manager->game->renderer.camera.position;

				intPos.x = (int)intPos.x;
				intPos.y = (int)intPos.y;
				intPos.z = (int)intPos.z;

				if (intPos != glm::vec3((int)c.ParseNumberValue(parameters[3]),
					(int)c.ParseNumberValue(parameters[4]), (int)c.ParseNumberValue(parameters[5])))
				{
					return -199;
				}
				else
				{
					c.manager->game->renderer.camera.position = intPos;
				}
			}
			else if (parameters[2] == "add")
			{
				c.manager->game->renderer.camera.position += glm::vec3(c.ParseNumberValue(parameters[3]),
					c.ParseNumberValue(parameters[4]), c.ParseNumberValue(parameters[5]));
			}
			else if (parameters[2] == "sub")
			{
				c.manager->game->renderer.camera.position -= glm::vec3(c.ParseNumberValue(parameters[3]),
					c.ParseNumberValue(parameters[4]), c.ParseNumberValue(parameters[5]));
			}
			else if (parameters[2] == "lerp")
			{
				//TODO
			}
		}
		else if (parameters[1] == "rotation")
		{
			//TODO: Refactor the camera.Zoom function to make angle private
			if (parameters[2] == "set")
			{
				c.manager->game->renderer.camera.angle = c.ParseNumberValue(parameters[3]);
				c.manager->game->renderer.camera.Zoom(0, c.manager->game->screenWidth, c.manager->game->screenHeight);
			}
			else if (parameters[2] == "add")
			{
				c.manager->game->renderer.camera.angle += c.ParseNumberValue(parameters[3]);
				c.manager->game->renderer.camera.Zoom(0, c.manager->game->screenWidth, c.manager->game->screenHeight);
			}
			else if (parameters[2] == "sub")
			{
				c.manager->game->renderer.camera.angle -= c.ParseNumberValue(parameters[3]);
				c.manager->game->renderer.camera.Zoom(0, c.manager->game->screenWidth, c.manager->game->screenHeight);
			}
			else if (parameters[2] == "lerp")
			{
				//TODO
			}
		}
		else if (parameters[1] == "pitch")
		{
			if (parameters[2] == "set")
			{
				c.manager->game->renderer.camera.pitch = c.ParseNumberValue(parameters[3]);
			}
			else if (parameters[2] == "add")
			{
				c.manager->game->renderer.camera.pitch += c.ParseNumberValue(parameters[3]);
			}
			else if (parameters[2] == "sub")
			{
				c.manager->game->renderer.camera.pitch -= c.ParseNumberValue(parameters[3]);
			}
			else if (parameters[2] == "lerp")
			{
				//TODO
			}
		}
		else if (parameters[1] == "yaw")
		{
			if (parameters[2] == "set")
			{
				c.manager->game->renderer.camera.yaw = c.ParseNumberValue(parameters[3]);
			}
			else if (parameters[2] == "add")
			{
				c.manager->game->renderer.camera.yaw += c.ParseNumberValue(parameters[3]);
			}
			else if (parameters[2] == "sub")
			{
				c.manager->game->renderer.camera.yaw -= c.ParseNumberValue(parameters[3]);
			}
			else if (parameters[2] == "lerp")
			{
				//TODO
			}
		}
		else if (parameters[1] == "roll")
		{
			if (parameters[2] == "set")
			{
				c.manager->game->renderer.camera.roll = c.ParseNumberValue(parameters[3]);
			}
			else if (parameters[2] == "add")
			{
				c.manager->game->renderer.camera.roll += c.ParseNumberValue(parameters[3]);
			}
			else if (parameters[2] == "sub")
			{
				c.manager->game->renderer.camera.roll -= c.ParseNumberValue(parameters[3]);
			}
			else if (parameters[2] == "lerp")
			{
				//TODO
			}
		}
		else if (parameters[1] == "projection")
		{
			if (parameters[2] == "orthographic")
			{
				c.manager->game->renderer.camera.useOrthoCamera = true;
				c.manager->game->renderer.camera.Zoom(0, c.manager->game->screenWidth, c.manager->game->screenHeight);
			}
			else if (parameters[2] == "perspective")
			{
				c.manager->game->renderer.camera.useOrthoCamera = false;
				c.manager->game->renderer.camera.Zoom(0, c.manager->game->screenWidth, c.manager->game->screenHeight);
			}
		}

		return 0;
	}

	int WindowFunction(CutsceneParameters parameters, CutsceneCommands& c)
	{
		if (parameters.size() > 1)
		{
			if (parameters[1] == "icon") // NOTE: Icon must be at most 256x256
			{
				c.manager->game->windowIconFilepath = c.ParseStringValue(parameters[2]);
				SDL_SetWindowIcon(c.manager->game->window, IMG_Load(c.manager->game->windowIconFilepath.c_str()));
			}
			else if (parameters[1] == "title")
			{
				c.manager->game->windowTitle = c.ParseStringValue(parameters[2]);
				std::replace(c.manager->game->windowTitle.begin(), c.manager->game->windowTitle.end(), '[', ' ');
				std::replace(c.manager->game->windowTitle.begin(), c.manager->game->windowTitle.end(), ']', ' ');
				Trim(c.manager->game->windowTitle);
				SDL_SetWindowTitle(c.manager->game->window, c.manager->game->windowTitle.c_str());
			}
			else if (parameters[1] == "isfull")
			{
				MoveVariables({ "mov", parameters[2], std::to_string(c.manager->game->isFullscreen) }, c);
			}
			else if (parameters[1] == "setfull")
			{
				c.manager->game->SetFullScreen(std::stoi(parameters[2]));
			}
		}

		return 0;
	}

	int ControlBindings(CutsceneParameters parameters, CutsceneCommands& c)
	{
		if (parameters.size() > 1)
		{
			//TODO: Toggle function?
			if (parameters[1] == "mouse")
			{
				c.manager->useMouseControls = (parameters[2] == "on");
			}
			else if (parameters[1] == "keyboard")
			{
				c.manager->useKeyboardControls = (parameters[2] == "on");
			}
		}

		return 0;
	}

	int BindKeyToLabel(CutsceneParameters parameters, CutsceneCommands& c)
	{
		if (parameters.size() > 2)
		{
			std::string labelName = c.ParseStringValue(parameters[2]);
			unsigned int letter = 999999;

			if (parameters[1].size() > 1)
			{
				// Handle special cases / full words here (spacebar, enter, etc.)
				if (parameters[1] == "spacebar")
				{
					letter = (unsigned int)SDL_SCANCODE_SPACE;
				}
				else if (parameters[1] == "enter" || parameters[1] == "return")
				{
					letter = (unsigned int)SDL_SCANCODE_RETURN;
				}
			}
			else
			{
				//TODO: Handle other single character keys

				// Get the ASCII value of the character
				letter = parameters[1][0];

				// ASCII A starts at 65, SDL starts at 4, so 65-4 = 61 = offset
				// ASCII a starts at 97, SDL starts at 4, so 97-4 = 93 = offset
				// ASCII 1 starts at 49, SDL starts at 30, so 49 - 30 = 19 = offset

				if (letter > 64 && letter < 91) // A
				{
					letter = letter - 61;
				}
				else if (letter > 96 && letter < 123) // a
				{
					letter = letter - 93;
				}
				else if (letter > 48 && letter < 58) // 1
				{
					letter = letter - 19;
				}
				else if (letter == 48) // 0
				{
					letter = (unsigned int)SDL_SCANCODE_0;
				}
			}

			if (letter != 999999)
			{
				c.buttonLabels[letter] = labelName;
				c.buttonLabelsActive[letter] = true;
			}
		}

		return 0;
	}

	int SetClickToContinue(CutsceneParameters parameters, CutsceneCommands& c)
	{
		if (parameters[1] == "state")
		{
			AnimState* state = c.manager->textbox->clickToContinue->GetAnimator()->GetState(parameters[2]);

			int index = 2;
			std::string stateName = parameters[index++];
			int stateSpeed = std::stoi(parameters[index++]);
			int spriteStartFrame = std::stoi(parameters[index++]);
			int spriteEndFrame = std::stoi(parameters[index++]);
			int spriteFrameWidth = std::stoi(parameters[index++]);
			int spriteFrameHeight = std::stoi(parameters[index++]);

			std::string spriteFilePath = parameters[index++];
			int spritePivotX = std::stoi(parameters[index++]);
			int spritePivotY = std::stoi(parameters[index++]);

			state->name = stateName;
			state->filename = spriteFilePath;
			state->speed = stateSpeed;
			state->startFrame = spriteStartFrame;
			state->endFrame = spriteEndFrame;
			state->frameWidth = spriteFrameWidth;
			state->frameHeight = spriteFrameHeight;
			state->pivotX = spritePivotX;
			state->pivotY = spritePivotY;
		}
		else if (parameters[1] == "animator")
		{
			//TODO: Swap out the animator
		}

		return 0;
	}

	int ScreenshotCommand(CutsceneParameters parameters, CutsceneCommands& c)
	{
		if (parameters.size() > 1)
		{
			c.manager->game->SaveScreenshot(parameters[1]);
		}
		else
		{
			c.manager->game->SaveScreenshot();
		}

		return 0;
	}

	int LuaCommand(CutsceneParameters parameters, CutsceneCommands& c)
	{
		return 0;
	}

	int ErrorLog(CutsceneParameters parameters, CutsceneCommands& c)
	{
		if (parameters.size() > 1)
		{
			c.manager->game->logger.Log(parameters[1]);
		}

		return 0;
	}

	// font name path
	int FontCommand(CutsceneParameters parameters, CutsceneCommands& c)
	{
		// Load fonts from files
		c.manager->game->CreateFont(parameters[1], c.ParseNumberValue(parameters[2]));

		return 0;
	}

	int GetData(CutsceneParameters parameters, CutsceneCommands& c)
	{
		// getfilename $myvar bgm

		const std::string& whatToGet = parameters[1];

		if (whatToGet == "name")
		{
			unsigned int varNum = c.ParseNumberValue(parameters[2]);

			if (parameters[3] == "bgm")
			{
				c.stringVariables[varNum] = c.manager->game->soundManager.bgmFilepath;
			}
			else if (parameters[3] == "sound")
			{
				c.stringVariables[varNum] = c.manager->game->soundManager.sounds[c.ParseNumberValue(parameters[4])]->sound->filepath;
			}
			else if (parameters[3] == "sprite")
			{
				c.stringVariables[varNum] = c.manager->images[c.ParseNumberValue(parameters[4])]->GetSprite()->texture->GetFilePath();
			}
			else if (parameters[3] == "script")
			{
				c.stringVariables[varNum] = c.manager->currentScript;
			}
			else if (parameters[3] == "level")
			{
				c.stringVariables[varNum] = c.manager->game->currentLevel;
			}
			else if (parameters[3] == "label")
			{
				c.stringVariables[varNum] = c.manager->GetLabelName(c.manager->currentLabel);
			}
			else if (parameters[3] == "text")
			{
				c.stringVariables[varNum] = c.manager->textbox->text->txt;
			}
		}
		else if (whatToGet == "datetime")
		{
			// getdatetime $var1

			time_t now = time(0);
			std::string dt = ctime(&now);
			dt = dt.substr(0, dt.size() - 1);

			MoveVariables({ "mov", parameters[2], dt }, c);
		}
		else if (whatToGet == "text")
		{
			std::string theText = c.manager->textbox->fullTextString;

			// TODO: Should we remove \n or not?
			// Not removing it would be bad when calling SaveGame() (global.sav)
			std::replace(theText.begin(), theText.end(), '\n', ' ');

			MoveVariables({ "mov", parameters[2], theText }, c);
		}

		return 0;
	}

	int TagCommand(CutsceneParameters parameters, CutsceneCommands& c)
	{
		if (parameters[1] == "define")
		{
			if (c.manager->tags.count(parameters[2]) != 1)
			{
				c.manager->tags[parameters[2]] = neww TextTag();
			}
		}

		return 0;
	}

	int IntToString(CutsceneParameters parameters, CutsceneCommands& c)
	{
		int stringVariableIndex = c.ParseNumberValue(parameters[1]);
		c.stringVariables[stringVariableIndex] = std::to_string(c.ParseNumberValue(parameters[2]));
		return 0;
	}

	int IncrementVariable(CutsceneParameters parameters, CutsceneCommands& c)
	{
		//int index = c.ParseNumberValue(parameters[1]);
		//c.numberVariables[index]++;

		if (parameters[1][0] == '%')
		{
			// Get rid of the % sign to get the actual index
			// TODO: This only works for constants, not variables
			int val = c.numberVariables[std::stoi(parameters[1].substr(1, parameters[1].size() - 1))]++;
			c.cacheParseNumbers[parameters[1]] = val;
		}
		else
		{
			std::cout << "ERROR INC: Missing % sign" << std::endl;
		}

		return 0;
	}

	int DecrementVariable(CutsceneParameters parameters, CutsceneCommands& c)
	{
		//c.numberVariables[c.ParseNumberValue(parameters[1])]--;

		if (parameters[1][0] == '%')
		{
			// Get rid of the % sign to get the actual index
			int val = c.numberVariables[std::stoi(parameters[1].substr(1, parameters[1].size() - 1))]--;
			c.cacheParseNumbers[parameters[1]] = val;
		}
		else
		{
			std::cout << "ERROR DEC: Missing % sign" << std::endl;
		}

		return 0;
	}

	int Output(CutsceneParameters parameters, CutsceneCommands& c)
	{
#if _DEBUG
		c.shouldOutput = true;
#endif

		bool shouldLog = true;

		if (parameters[1] == "on")
		{
			c.outputCommands = true;
		}
		else if (parameters[1] == "off")
		{
			c.outputCommands = false;
		}
		else if (c.shouldOutput)
		{
			if (parameters[1] == "str")
			{
				if (shouldLog)
					c.manager->game->logger.Log(parameters[2] + ": " + c.ParseStringValue(parameters[2]));
				else
					std::cout << parameters[2] << ": " << c.ParseStringValue(parameters[2]) << std::endl;
			}
			else if (parameters[1] == "num")
			{
				if (shouldLog)
					c.manager->game->logger.Log(parameters[2] + ": " + std::to_string(c.ParseNumberValue(parameters[2])));
				else
					std::cout << parameters[2] << ": " << c.ParseNumberValue(parameters[2]) << std::endl;
			}
			else if (parameters[1] == "arr")
			{
				if (c.GetArray(parameters[2]))
				{
					if (shouldLog)
						c.manager->game->logger.Log(parameters[2] + ": " + std::to_string(c.arrayVariables[c.arrayIndex][c.vectorIndex]));
					else
						std::cout << parameters[2] << ": " << c.arrayVariables[c.arrayIndex][c.vectorIndex] << std::endl;
				}
				else
				{
					return -1;
				}
			}
			else
			{
				if (shouldLog)
					c.manager->game->logger.Log("ERROR: Failed to define output type (str/num); cannot log output.");
				else
					std::cout << "ERROR: Failed to define output type (str/num); cannot log output." << std::endl;
			}
		}

		return 0;
	}

	int FileExist(CutsceneParameters parameters, CutsceneCommands& c)
	{
		std::string filename = c.ParseStringValue(parameters[2]);
		MoveVariables({ "mov", parameters[1], std::to_string(fs::exists(filename)) }, c);

		return 0;
	}

	int LineBreakCommand(CutsceneParameters parameters, CutsceneCommands& c)
	{
		if (parameters.size() < 2)
		{
			c.lineBreaks += 2;
		}
		else
		{
			c.lineBreaks += c.ParseNumberValue(parameters[1]);
		}

		return 0;
	}

	// This number is the time in milliseconds to wait,
	// so a larger number actually makes the speed slower
	int TextSpeed(CutsceneParameters parameters, CutsceneCommands& c)
	{
		c.manager->msInitialDelayBetweenGlyphs = c.ParseNumberValue(parameters[1]);

		return 0;
	}

	int AutoMode(CutsceneParameters parameters, CutsceneCommands& c)
	{
		if (parameters.size() > 1)
		{
			if (parameters[1] == "on")
			{
				c.manager->automaticallyRead = true;
			}
			else if (parameters[1] == "off")
			{
				c.manager->automaticallyRead = false;
			}
			else if (parameters[1] == "speed")
			{
				c.manager->autoTimeToWaitPerGlyph = c.ParseNumberValue(parameters[2]);
			}
		}

		return 0;
	}

	// align x center textbox
	// align x left 1
	int AlignCommand(CutsceneParameters parameters, CutsceneCommands& c)
	{
		Text* text = c.manager->textbox->text;

		if (parameters[3] != "textbox")
		{
			if (parameters[3] == "namebox")
			{
				text = c.manager->textbox->speaker;
			}
			else
			{
				text = static_cast<Text*>(c.manager->images[c.ParseNumberValue(parameters[3])]);
			}
		}

		if (text == nullptr)
			return -1;

		if (parameters[1] == "x")
		{
			if (parameters[2] == "left")
			{
				text->alignX = AlignmentX::LEFT;
			}
			else if (parameters[2] == "center")
			{
				text->alignX = AlignmentX::CENTER;
			}
			else if (parameters[2] == "right")
			{
				text->alignX = AlignmentX::RIGHT;
			}
		}
		else if (parameters[1] == "y")
		{
			if (parameters[2] == "top")
			{
				text->alignY = AlignmentY::TOP;
			}
			else if (parameters[2] == "center")
			{
				text->alignY = AlignmentY::CENTER;
			}
			else if (parameters[2] == "bottom")
			{
				text->alignY = AlignmentY::BOTTOM;
			}
		}

		text->SetPosition(text->GetPosition().x, text->GetPosition().y);

		return 0;
	}

	int InputCommand(CutsceneParameters parameters, CutsceneCommands& c)
	{
		return 0;
	}

	int AutoSkip(CutsceneParameters parameters, CutsceneCommands& c)
	{
		c.manager->autoskip = (parameters[1] == "on");

		return 0;
	}

	int AutoSave(CutsceneParameters parameters, CutsceneCommands& c)
	{
		//TODO: Save this setting and remember it when you load the game
		// (Should probably just create a save function for the c.manager
		// and look through the c.manager's variables and save the important ones)

		if (parameters[1] == "on")
		{
			c.manager->autosave = true;
		}
		else if (parameters[1] == "off")
		{
			c.manager->autosave = false;
		}

		return 0;
	}

	int AutoReturn(CutsceneParameters parameters, CutsceneCommands& c)
	{
		//TODO: Save this setting and remember it when you load the game

		if (parameters[1] == "on")
		{
			c.manager->autoreturn = true;
		}
		else if (parameters[1] == "off")
		{
			c.manager->autoreturn = false;
		}

		return 0;
	}

	// Whenever a choice prompt appears, automatically choose choice #x
	// (disabled if x == 0)
	int AutoChoice(CutsceneParameters parameters, CutsceneCommands& c)
	{
		c.manager->autoChoice = std::stoi(parameters[1]);

		return 0;
	}

	// Travel from one label to another, executing all commands but without printing them
	int TravelCommand(CutsceneParameters parameters, CutsceneCommands& c)
	{
		// Interrupt travel prematurely
		if (parameters[1] == "off" || parameters[1] == "stop" || parameters[1] == "end")
		{
			c.manager->isTravelling = false;
		}
		else
		{
			c.manager->isTravelling = true;
			c.manager->endTravelLabel = c.ParseStringValue(parameters[2]);
			std::string startLabel = c.ParseStringValue(parameters[1]);
			GoToLabel({ "", startLabel }, c);
		}

		return 0;
	}

	int AnimationCommand(CutsceneParameters parameters, CutsceneCommands& c)
	{
		static std::unordered_map<std::string, std::string> args;

		if (parameters[1] == "args")
		{
			if (parameters[2] == "clear")
			{
				args.clear();
			}
			else // animation args character but
			{
				args[c.ParseStringValue(parameters[2])] = c.ParseStringValue(parameters[3]);
			}

			return 0;
		}

		const int entityIndex = c.ParseNumberValue(parameters[1]);

		if (c.manager->images[entityIndex] == nullptr)
			return 0;

		// TODO: Parse all these parameters for variables
		if (parameters[2] == "state")
		{
			int index = 3;
			std::string stateName = parameters[index++];
			AnimState* state = c.manager->images[entityIndex]->GetAnimator()->GetState(stateName);

			int stateSpeed = std::stoi(parameters[index++]);
			int spriteStartFrame = std::stoi(parameters[index++]);
			int spriteEndFrame = std::stoi(parameters[index++]);
			int spriteFrameWidth = std::stoi(parameters[index++]);
			int spriteFrameHeight = std::stoi(parameters[index++]);

			std::string spriteFilePath = c.ParseStringValue(parameters[index++]);
			int spritePivotX = std::stoi(parameters[index++]);
			int spritePivotY = std::stoi(parameters[index++]);

			state->name = stateName;
			state->filename = spriteFilePath;
			state->speed = stateSpeed;
			state->startFrame = spriteStartFrame;
			state->endFrame = spriteEndFrame;
			state->frameWidth = spriteFrameWidth;
			state->frameHeight = spriteFrameHeight;
			state->pivotX = spritePivotX;
			state->pivotY = spritePivotY;
		}
		else if (parameters[2] == "disable")
		{
			if (c.manager->images[entityIndex]->GetAnimator() != nullptr)
			{
				c.manager->images[entityIndex]->GetAnimator()->shouldUpdate = false;
			}
		}
		else if (parameters[2] == "enable")
		{
			if (c.manager->images[entityIndex]->GetAnimator() != nullptr)
			{
				c.manager->images[entityIndex]->GetAnimator()->shouldUpdate = true;
			}
		}
		else if (parameters[2] == "set")
		{
			//TODO: This will never work as long as the state machine is active
			// because unless you also set the conditions to get to this state,
			// it will just flow back into whatever state it was in before (or can get to)
			if (parameters[3] == "state")
			{
				c.manager->images[entityIndex]->GetAnimator()->SetState(parameters[4].c_str());
				c.manager->images[entityIndex]->GetAnimator()->Update(*c.manager->images[entityIndex]);
			}
			else if (parameters[3] == "bool")
			{
				c.manager->images[entityIndex]->GetAnimator()->SetBool(parameters[4].c_str(), parameters[5] == "true");
				c.manager->images[entityIndex]->GetAnimator()->Update(*c.manager->images[entityIndex]);
			}
			else if (parameters[3] == "data")
			{
				// TODO: We don't want to constantly make new animators, this will cause memory leaks!

				std::vector<AnimState*> animStates = c.manager->game->spriteManager.ReadAnimData(parameters[5], args);
				Animator* anim1 = neww Animator(parameters[4] + "/" + parameters[4], animStates, parameters[6]);

				c.manager->images[entityIndex]->SetAnimator(*anim1);
				c.manager->images[entityIndex]->GetAnimator()->Update(*c.manager->images[entityIndex]);
				c.manager->images[entityIndex]->GetSprite()->keepPositionRelativeToCamera = true;
				c.manager->images[entityIndex]->GetSprite()->keepScaleRelativeToCamera = true;
			}
		}

		return 0;
	}

	// quake 6 600
	// quake x 4 400
	// quake y 4 400
	int Quake(CutsceneParameters parameters, CutsceneCommands& c)
	{
		int quakeDelay = 0;

		// direction to shake
		if (parameters[1] == "x")
		{
			c.isQuakeHorizontal = true;
			c.quakeIntensity = c.ParseNumberValue(parameters[2]);
			quakeDelay = c.ParseNumberValue(parameters[3]);
		}
		else if (parameters[1] == "y")
		{
			c.isQuakeVertical = true;
			c.quakeIntensity = c.ParseNumberValue(parameters[2]);
			quakeDelay = c.ParseNumberValue(parameters[3]);
		}
		else // both
		{
			c.isQuakeHorizontal = true;
			c.isQuakeVertical = true;
			c.quakeIntensity = c.ParseNumberValue(parameters[1]);
			quakeDelay = c.ParseNumberValue(parameters[2]);
		}

		c.quakeCount = 0;
		c.quakeNumberOfLoops = 0;
		c.quakeTimer.Start(quakeDelay * 0.25f / c.quakeIntensity);
		c.currentQuakePosition = Vector2(c.manager->game->renderer.camera.startScreenWidth,
			c.manager->game->renderer.camera.startScreenHeight);

		Wait({ "", std::to_string(quakeDelay) }, c);

		return 0;
	}


	int RightClickSettings(CutsceneParameters parameters, CutsceneCommands& c)
	{
		if (parameters[1] == "off")
		{
			c.manager->rclickEnabled = false;
		}
		else if (parameters[1] == "on")
		{
			c.manager->rclickEnabled = true;
		}

		return 0;
	}

	int PrintCommand(CutsceneParameters parameters, CutsceneCommands& c)
	{
		if (parameters[1] == "auto")
		{
			c.manager->autoprint = (parameters[2] == "on");
		}
		else
		{
			c.manager->printNumber = c.ParseNumberValue(parameters[1]);
		}

		return 0;
	}

	// Define effects for the Print Command
	int EffectCommand(CutsceneParameters parameters, CutsceneCommands& c)
	{
		int num = c.ParseNumberValue(parameters[1]);

		PrintEffect effect;

		effect.delay = c.ParseNumberValue(parameters[2]);
		effect.mask = c.ParseStringValue(parameters[3]);

		c.manager->printEffects[num] = effect;

		return 0;
	}

	int IsSkipping(CutsceneParameters parameters, CutsceneCommands& c)
	{
		MoveVariables({ "mov", parameters[1], std::to_string(c.manager->isSkipping) }, c);
		return 0;
	}

	int ToggleSkipping(CutsceneParameters parameters, CutsceneCommands& c)
	{
		if (parameters[1] == "on" || parameters[1] == "ON")
		{
			c.manager->disableSkip = false;
		}
		else
		{
			c.manager->disableSkip = true;
		}

		return 0;
	}

	int RepeatCommand(CutsceneParameters parameters, CutsceneCommands& c)
	{
		if (parameters[1] == "end")
		{
			if (c.manager->repeatStack.size() > 0)
			{
				SceneRepeatData& rdata = c.manager->repeatStack.back();

				if ((++rdata.count) > rdata.end)
				{
					c.manager->repeatStack.pop_back();
				}
				else
				{
					c.manager->labelIndex = rdata.label;
					c.manager->lineIndex = rdata.line;
					c.manager->commandIndex = rdata.command;
				}
			}
			else
			{
				c.manager->game->logger.Log("ERROR: Trying to end Repeat outside of scope");
			}
		}
		else
		{
			SceneRepeatData rdata;
			rdata.label = c.manager->labelIndex;
			rdata.line = c.manager->lineIndex;
			rdata.command = c.manager->commandIndex;
			rdata.count = 1;
			rdata.end = c.ParseNumberValue(parameters[1]);
			c.manager->repeatStack.push_back(rdata);
		}

		return 0;
	}

	int CreateArrayVariable(CutsceneParameters parameters, CutsceneCommands& c)
	{
		// numalias j_up,2500
		// dim j_up 35 2

		unsigned int index = c.ParseNumberValue(parameters[1]);

		// Is this a 2D array?
		if (parameters.size() > 3)
		{
			int sizeSlots = c.ParseNumberValue(parameters[2]);
			int numbersPerSlot = c.ParseNumberValue(parameters[3]);

			std::vector<int> numbers = std::vector<int>(sizeSlots * numbersPerSlot, 0);
			c.arrayVariables[index] = numbers;
			c.arrayNumbersPerSlot[index] = numbersPerSlot;
		}
		else // just 1D
		{
			int size = c.ParseNumberValue(parameters[2]);
			std::vector<int> numbers = std::vector<int>(size, 0);
			c.arrayVariables[index] = numbers;
			c.arrayNumbersPerSlot[index] = 0;
		}

		return 0;
	}

	// Create a shader for use within the cutscene system
	int CreateShader(CutsceneParameters parameters, CutsceneCommands& c)
	{
		std::string shaderName = c.ParseStringValue(parameters[1]);
		std::string vertexFile = c.ParseStringValue(parameters[2]);
		std::string fragmentFile = c.ParseStringValue(parameters[3]);

		Mesh* m = c.manager->game->CreateQuadMesh();

		m->BindMesh();

		c.customShaders[shaderName] = neww ShaderProgram(ShaderName::Custom, vertexFile.c_str(), fragmentFile.c_str());
		c.customShaders[shaderName]->SetNameString(shaderName);

		m->ClearMesh();

		return 0;
	}

	int SetShaderFilter(CutsceneParameters parameters, CutsceneCommands& c)
	{
		if (parameters[1] == "off")
		{
			c.shaderFilter = "";
		}
		else if (parameters[1] == "min")
		{
			c.filterMin = c.ParseNumberValue(parameters[2]);
		}
		else if (parameters[1] == "max")
		{
			c.filterMax = c.ParseNumberValue(parameters[2]);
		}
		else
		{
			c.shaderFilter = c.ParseStringValue(parameters[1]);
		}

		return 0;
	}

	int ParticleCommand(CutsceneParameters parameters, CutsceneCommands& c)
	{
		if (parameters[1] == "system")
		{
			if (parameters[2] == "create")
			{
				// Create new particle system here
				unsigned int imageNumber = c.ParseNumberValue(parameters[3]);
				const unsigned int x = c.ParseNumberValue(parameters[4]);
				const unsigned int y = c.ParseNumberValue(parameters[5]);

				//TODO: Don't delete/new, just grab from entity pool and reset
				if (c.manager->images[imageNumber] != nullptr)
					delete_it(c.manager->images[imageNumber]);

				ParticleSystem* newParticleSystem = nullptr;
				newParticleSystem = neww ParticleSystem(Vector2(x, y));

				c.manager->images[imageNumber] = newParticleSystem;
				c.manager->images[imageNumber]->drawOrder = imageNumber;
				c.manager->images[imageNumber]->GetSprite()->keepPositionRelativeToCamera = true;
				c.manager->images[imageNumber]->GetSprite()->keepScaleRelativeToCamera = true;
				//c.manager->images[imageNumber]->GetSprite()->texture->SetFilePath("particlesystem");
			}
			else
			{
				// Get index of particle system we want to modify
				unsigned int systemIndex = c.ParseNumberValue(parameters[2]);

				ParticleSystem* particleSystem = dynamic_cast<ParticleSystem*>(c.manager->images[systemIndex]);
				if (particleSystem != nullptr)
				{
					if (parameters[3] == "sprite") // set the sprite that the next particle will use
					{
						// TODO: How to remove from or clear the list?
						particleSystem->nextParticleSpriteFilename.emplace_back(c.pathPrefix + c.ParseStringValue(parameters[4]));
					}
					else if (parameters[3] == "bounds") // set the bounds that next particle spawned within
					{

					}
					else if (parameters[3] == "collider")
					{
						if (parameters[4] == "width")
						{
							particleSystem->nextParticleColliderWidth = c.ParseNumberValue(parameters[5]);
						}
						else if (parameters[4] == "height")
						{
							particleSystem->nextParticleColliderHeight = c.ParseNumberValue(parameters[5]);
						}
					}
					else if (parameters[3] == "velocity") // set the velocity of next particle
					{
						float vx = c.ParseNumberValue(parameters[4]) * 0.001f;
						float vy = c.ParseNumberValue(parameters[5]) * 0.001f;
						particleSystem->nextParticleVelocity = Vector2(vx, vy);
					}
					else if (parameters[3] == "timeToSpawn") // set time between particle spawns
					{
						particleSystem->spawnTimer.Start(c.ParseNumberValue(parameters[4]));
					}
					else if (parameters[3] == "maxNumber") // set the max number of particles
					{
						particleSystem->Resize(c.ParseNumberValue(parameters[4]));
					}
					else if (parameters[3] == "timeToLive") // set time until a particle destroys itself
					{
						particleSystem->nextParticleTimeToLive = c.ParseNumberValue(parameters[4]);
					}
				}

			}

		}
		else
		{
			// Get index of particle system containing particles
			unsigned int systemIndex = c.ParseNumberValue(parameters[2]);

			// Get index of specific particle to modify
			unsigned int particleIndex = c.ParseNumberValue(parameters[3]);

			ParticleSystem* particleSystem = dynamic_cast<ParticleSystem*>(c.manager->images[systemIndex]);
			if (particleSystem != nullptr)
			{
				Entity& particle = particleSystem->particles[particleIndex];

				// TODO: Modify the individual particle here
			}
		}

		return 0;
	}

	int DrawRectCommand(CutsceneParameters parameters, CutsceneCommands& c)
	{
		// image_number x, y, width, height, _current_Value max_value, color

		LoadSprite({ "ld", parameters[1], "", parameters[2], parameters[3], "1" }, c);

		Entity* entity = c.manager->images[c.ParseNumberValue(parameters[1])];

		// currentValue / maxValue
		float ratio = c.ParseNumberValue(parameters[6]) / (float)c.ParseNumberValue(parameters[7]);

		Vector2 newScale = Vector2(c.ParseNumberValue(parameters[4]) * ratio,
			(float)c.ParseNumberValue(parameters[5]));

		// TODO: Don't hardcode this
		newScale.x /= (32.0f * Camera::MULTIPLIER);
		newScale.y /= (32.0f * Camera::MULTIPLIER);

		entity->SetScale(newScale);

		// TODO: Read in the color here

		return 0;
	}

	// TODO: Including windows.h breaks the engine due to macro conflicts
	// TODO: Also, need a cross-platform solution (not just Windows)
	int ShellCommand(CutsceneParameters parameters, CutsceneCommands& c)
	{
		return 0;
	}

	int SteamCommand(CutsceneParameters parameters, CutsceneCommands& c)
	{

		return 0;
	}

	// This function adds a filepath to a list of text files to include.
	// When the main cutscene file is parsed, also parse all files in the include list.
	int IncludeCommand(CutsceneParameters parameters, CutsceneCommands& c)
	{
		c.includeFilepaths.emplace_back(c.ParseStringValue(parameters[1]));

		return 0;
	}

	int AssetPathCommand(CutsceneParameters parameters, CutsceneCommands& c)
	{
		if (parameters[1] == "default")
		{
			c.pathPrefix = "";
		}
		else
		{
			c.pathPrefix = c.ParseStringValue(parameters[1]);
		}

		return 0;
	}

}
