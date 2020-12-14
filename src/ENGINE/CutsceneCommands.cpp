

#include "CutsceneFunctions.h"


CutsceneCommands::CutsceneCommands()
{
	//TODO: Add a command to define this via scripting
	buttonLabels[(unsigned int)SDL_SCANCODE_ESCAPE] = "pause_menu";
	buttonLabelsActive[(unsigned int)SDL_SCANCODE_ESCAPE] = true;

	numalias["bg"] = 0;
	numalias["l"] = 1;
	numalias["c"] = 2;
	numalias["r"] = 3;

	numalias["param1"] = 101;
	numalias["param2"] = 102;
	numalias["param3"] = 103;
	numalias["param4"] = 104;
	numalias["param5"] = 105;
	numalias["param6"] = 106;
	numalias["param7"] = 107;
	numalias["param8"] = 108;
	numalias["param9"] = 109;

	cmd_lut = {
	{"~", &CutsceneFunctions::DoNothing},
	{"add", &CutsceneFunctions::AddNumberVariables},
	{"align", &CutsceneFunctions::AlignCommand},
	{"animation", &CutsceneFunctions::AnimationCommand},
	{"array", &CutsceneFunctions::CreateArrayVariable },
	{"autochoice", &CutsceneFunctions::AutoChoice },
	{"automode", &CutsceneFunctions::AutoMode},
	{"autoreturn", &CutsceneFunctions::AutoReturn},
	{"autosave", &CutsceneFunctions::AutoSave},
	{"backlog", &CutsceneFunctions::OpenBacklog},
	{"bg", &CutsceneFunctions::LoadBackground },
	{"bgm", &CutsceneFunctions::MusicCommand },
	{"br", &CutsceneFunctions::LineBreakCommand },
	{"btnwait", &CutsceneFunctions::WaitForButton },
	{"camera", &CutsceneFunctions::CameraFunction},
	{"controls", &CutsceneFunctions::ControlBindings},
	{"choice", &CutsceneFunctions::DisplayChoice },
	{"cl", &CutsceneFunctions::ClearSprite },
	{"click", &CutsceneFunctions::WaitForClick },
	{"concat", &CutsceneFunctions::ConcatenateStringVariables},
	{"ctc", &CutsceneFunctions::SetClickToContinue},
	{"dec", &CutsceneFunctions::DecrementVariable},
	{"defsub", &CutsceneFunctions::DefineUserFunction},
	{"defchoice", &CutsceneFunctions::DefineChoice},
	{"div", &CutsceneFunctions::DivideNumberVariables},
	{"end", &CutsceneFunctions::EndGame },
	{"effect", &CutsceneFunctions::EffectCommand },
	{"errorlog", &CutsceneFunctions::ErrorLog },
	{"fade", &CutsceneFunctions::Fade },
	{"filter", &CutsceneFunctions::SetShaderFilter },
	{"fileexist", &CutsceneFunctions::FileExist},
	{"flip", &CutsceneFunctions::FlipSprite },
	{"font", &CutsceneFunctions::FontCommand},
	{"get", &CutsceneFunctions::GetData},
	{"global", &CutsceneFunctions::SetGlobalNumber},
	{"gosub", &CutsceneFunctions::GoSubroutine },
	{"goto", &CutsceneFunctions::GoToLabel },
	{"if", &CutsceneFunctions::IfCondition },
	{"inc", &CutsceneFunctions::IncrementVariable},
	{"input", &CutsceneFunctions::InputCommand},
	{"itoa", &CutsceneFunctions::IntToString },
	{"isskip", &CutsceneFunctions::IsSkipping },
	{"jumpb", &CutsceneFunctions::JumpBack },
	{"jumpf", &CutsceneFunctions::JumpForward },
	{"keybind", &CutsceneFunctions::BindKeyToLabel },
	{"ld", &CutsceneFunctions::LoadSprite },
	{"loadgame",&CutsceneFunctions::LoadGame },
	{"lua", &CutsceneFunctions::LuaCommand},
	{"me", &CutsceneFunctions::MusicEffectCommand},
	{"mod", &CutsceneFunctions::ModNumberVariables},
	{"mov", &CutsceneFunctions::MoveVariables},
	{"mul", &CutsceneFunctions::MultiplyNumberVariables},
	{"name", &CutsceneFunctions::NameCommand},
	{"namedef", &CutsceneFunctions::NameDefineCommand},
	{"namebox", &CutsceneFunctions::Namebox},
	{"numalias", &CutsceneFunctions::SetNumAlias },
	{"particle", &CutsceneFunctions::ParticleCommand },
	{"print", &CutsceneFunctions::PrintCommand },
	{"quake", &CutsceneFunctions::Quake },
	{"random", &CutsceneFunctions::RandomNumberVariable },
	{"rect", &CutsceneFunctions::DrawRectCommand },
	{"reset", &CutsceneFunctions::ResetGame },
	{"resolution", &CutsceneFunctions::SetResolution },
	{"repeat", &CutsceneFunctions::RepeatCommand },
	{"return", &CutsceneFunctions::ReturnFromSubroutine },
	{"rclick", &CutsceneFunctions::RightClickSettings },
	{"savegame",&CutsceneFunctions::SaveGame },
	{"screenshot",&CutsceneFunctions::ScreenshotCommand },
	{"se", &CutsceneFunctions::SoundCommand },
	{"set_velocity", &CutsceneFunctions::SetVelocity },
	{"setnumvar", &CutsceneFunctions::SetNumberVariable },
	{"setstrvar", &CutsceneFunctions::SetStringVariable },
	{"shader", &CutsceneFunctions::CreateShader },
	{"shell", &CutsceneFunctions::ShellCommand },
	{"skip", &CutsceneFunctions::ToggleSkipping },
	{"spbtn", &CutsceneFunctions::SetSpriteButton},
	{"sprite", &CutsceneFunctions::SetSpriteProperty },
	{"steam", &CutsceneFunctions::SteamCommand },
	{"stdout", &CutsceneFunctions::Output },
	{"stralias", &CutsceneFunctions::SetStringAlias },
	{"sub", &CutsceneFunctions::SubtractNumberVariables},
	{"substr", &CutsceneFunctions::SubstringVariables},
	{"tag", &CutsceneFunctions::TagCommand},
	{"text", &CutsceneFunctions::LoadText },
	{"textbox", &CutsceneFunctions::Textbox },
	{"textcolor", &CutsceneFunctions::TextColor },
	{"textspeed", &CutsceneFunctions::TextSpeed },
	{"timer", &CutsceneFunctions::TimerFunction},
	{"travel", &CutsceneFunctions::TravelCommand},
	{"wait",&CutsceneFunctions::Wait },
	{"window", &CutsceneFunctions::WindowFunction }
	};

}

CutsceneCommands::~CutsceneCommands()
{
	for (auto& [key, func] : userDefinedFunctions)
	{
		if (func != nullptr)
			delete_it(func);
	}

	for (auto& [key, val] : customShaders)
	{
		if (val != nullptr)
			delete_it(val);
	}
}

int CutsceneCommands::ExecuteCommand(std::string command)
{
	if (manager->newCommands != nullptr && manager->newCommands != this)
	{
		return manager->newCommands->ExecuteCommand(command);
	}

	Timer cTimer;
	cTimer.Start(1);

	int finished = 1;
	// Replace all the bracketed spaces with underscores
	bool shouldReplace = false;
	for (int i = 0; i < command.size(); i++)
	{
		if (shouldReplace && command[i] == ' ')
		{
			command[i] = '`';
		}
		else if (command[i] == '[')
		{
			shouldReplace = true;
		}
		else if (command[i] == ']')
		{
			shouldReplace = false;
		}
	}

	std::istringstream ss(command);
	std::string token;
	std::vector<std::string> parameters;

	char delimit = ' ';
	bool ignoreComma = false;

	// TODO: When an if-condition leads to a text command,
	// because it removed the brackets [ ] it is interpreted
	// as comma-delimited... what can we do to fix this?
	for (int i = 0; i < command.size(); i++)
	{
		if (command[i] == ',' && !ignoreComma)
		{
			delimit = ',';
			break;
		}
		else if (command[i] == '[')
		{
			ignoreComma = true;
		}
		else if (command[i] == ']')
		{
			ignoreComma = false;
		}
	}

	//TODO: Maybe instead of using getline, just check for COMMA or SPACE and accept either one
	if (delimit == ',')
	{
		std::getline(ss, token, ' ');
		parameters.push_back(token);

		//TODO: if commands -- you can't mix space separated and comma separated,
		// so just force space separated (it breaks if it uses commas)
		if (token != "if" && token != "choice")
		{
			while (std::getline(ss, token, delimit))
			{
				if (token != "")
				{
					parameters.push_back(token);
				}
			}
		}
		else
		{
			while (std::getline(ss, token, ' '))
			{
				if (token != "")
				{
					parameters.push_back(token);
				}
			}
		}
	}
	else
	{
		while (std::getline(ss, token, delimit))
		{
			if (token != "")
			{
				parameters.push_back(token);
			}
		}
	}

	if (parameters.size() > 0)
	{
		// Replace all the bracketed underscores with spaces again
		shouldReplace = false;
		for (int i = 0; i < parameters.size(); i++)
		{
			bool replaced = false;
			shouldReplace = false;
			for (int k = 0; k < parameters[i].size(); k++)
			{
				if (shouldReplace && parameters[i][k] == '`')
				{
					parameters[i][k] = ' ';
				}
				else if (parameters[i][k] == '[')
				{
					replaced = true;
					shouldReplace = true;
					//parameters[i][k] = ' ';
				}
				else if (parameters[i][k] == ']')
				{
					replaced = true;
					shouldReplace = false;
					//parameters[i][k] = ' ';
				}
			}

			Trim(parameters[i]);

		}

		// If travelling, ignore some commands
		if (manager->isTravelling)
		{
			static std::vector<std::string> ignoreCommands = manager->game->ReadStringsFromFile("data/commands.ignore");

			for (const auto& cmd : ignoreCommands)
			{
				if (cmd == parameters[0])
				{
					return true;
				}
			}
		}

		if (outputCommands)
		{
			std::cout << "Command: " << command << std::endl;
		}

		bool commandFound = false;

		// Check for functions built into the engine
		if (!commandFound && cmd_lut.count(parameters[0]) != 0)
		{
			commandFound = true;

			try
			{
				int errorCode = (cmd_lut[parameters[0]])(parameters, *this);

				if (errorCode != 0)
				{
					if (errorCode == -99)
					{
						manager->EndCutscene();
						return true;
					}
					else if (errorCode == -199) //TODO: Use enums
					{
						finished = 0;
					}
					else if (errorCode == -198)
					{
						finished = 0;
					}
					else
					{
						std::cout << "ERROR " << errorCode << ": ";
						for (int i = 0; i < parameters.size(); i++)
							std::cout << parameters[i] << " ";
						std::cout << std::endl;
					}
				}
			}
			catch (const std::exception& e)
			{
				std::cout << "EXCEPTION: " << e.what() << std::endl;
				std::cout << "COMMAND: " << command << std::endl;
				manager->game->logger.Log(e.what());
			}
		}

		// Check for functions defined by the user via scripting
		if (!commandFound)
		{
			if (userDefinedFunctions.count(parameters[0]) != 0)
			{
				commandFound = true;

				bool foundLabel = false;
				for (int i = 0; i < manager->labels.size(); i++)
				{
					std::string name = manager->GetLabelName(manager->labels[i]);
					//std::cout << name << std::endl;
					if (name == parameters[0])
					{
						foundLabel = true;
						break;
					}
				}

				if (foundLabel)
				{
					//			myfunction 123 [test] 456  <-- parameters
					// defsub	myfunction %0  $45	  %33  <-- definition

					// Grab parameters and place their values in the corresponding variables
					for (int i = 0; i < userDefinedFunctions[parameters[0]]->parameters.size(); i++)
					{
						int index = GetNumAlias(userDefinedFunctions[parameters[0]]->parameters[i].substr(1,
							userDefinedFunctions[parameters[0]]->parameters[i].size() - 1));

						// Set values for each variable as defined in the function definition
						switch (userDefinedFunctions[parameters[0]]->parameters[i][0])
						{
						case '$':
							stringVariables[index] = ParseStringValue(parameters[i + 1]);
							break;
						case '%':
							numberVariables[index] = ParseNumberValue(parameters[i + 1]);
							break;
						default:
							// Do nothing, maybe error message?
							std::cout << "Invalid parameter definition for " << parameters[0] << " function";
							break;
						}
					}

					CutsceneFunctions::GoSubroutine({ parameters[0], parameters[0] }, *this);
				}

			}

			if (!commandFound)
			{
				//std::cout << "ERROR: Command " << parameters[0] << " not found." << std::endl;
			}
		}
	}

	if (manager->currentLabel != nullptr && manager->GetLabelName(manager->currentLabel) != "define" && cTimer.GetTicks() >= 0)
	{
		//std::cout << "Command: " << command << std::endl;
		//std::cout << cTimer.GetTicks() << std::endl;
	}

	return finished;
}


std::string CutsceneCommands::ParseStringValue(const std::string& parameter)
{
	if (cacheParseStrings.count(parameter) != 0)
	{
		//return cacheParseStrings[parameter];
	}

	// Get the variable number to store the result in
	if (parameter[0] == '$')
		parseStringValue = GetStringVariable(GetNumAlias(parameter.substr(1, parameter.size() - 1)));
	else
		parseStringValue = GetStringAlias(parameter);

	parseStringValue.erase(std::remove(parseStringValue.begin(), parseStringValue.end(), '\"'), parseStringValue.end());

	cacheParseStrings[parameter] = parseStringValue;

	return parseStringValue;
}

int CutsceneCommands::ParseNumberValue(const std::string& parameter)
{
	if (cacheParseNumbers.count(parameter) != 0)
	{
		//return cacheParseNumbers[parameter];
	}

	// Get the variable number to store the result in
	if (parameter[0] == '%' || parameter[0] == '$')
	{
		parseNumberValue = GetNumberVariable(GetNumAlias(parameter.substr(1, parameter.size() - 1)));
	}
	else if (parameter[0] == '?')
	{
		if (GetArray(parameter))
		{
			parseNumberValue = arrayVariables[arrayIndex][vectorIndex];
		}
	}
	else
	{
		parseNumberValue = GetNumAlias(parameter);
	}

	cacheParseNumbers[parameter] = parseNumberValue;

	return parseNumberValue;
}



int CutsceneCommands::GetNumberVariable(const unsigned int key)
{
	if (numberVariables.find(key) == numberVariables.end())
	{
		return key;
	}
	else
	{
		return numberVariables[key];
	}
}

std::string CutsceneCommands::GetStringVariable(const unsigned int key)
{
	if (stringVariables.find(key) == stringVariables.end())
	{
		return "";
	}
	else
	{
		return stringVariables[key];
	}
}



std::string CutsceneCommands::GetStringAlias(const std::string& key)
{
	if (stralias.find(key) == stralias.end())
	{
		return key;
	}
	else
	{
		return stralias[key];
	}
}

int CutsceneCommands::GetNumAlias(const std::string& key)
{
	if (numalias.find(key) == numalias.end())
	{
		try
		{
			return std::stoi(key);
		}
		catch (const std::exception& ex)
		{
			std::cout << "ERROR: Numalias not defined for " << key << std::endl;
			std::cout << ex.what() << std::endl;
			return 0; //TODO: Should this be changed to -1?
		}
	}
	else
	{
		// TODO: The map contains unsigned ints, but they are returned signed!
		return numalias[key];
	}
}




// Parameters is the list of RGBA values,
// OR it only contains one string, which is the hexadecimal string
Color CutsceneCommands::ParseColorFromParameters(const std::vector<std::string>& parameters, const int index)
{
	Color color = { 255, 255, 255, 255 };
	std::vector<std::string> colorParams{ begin(parameters) + index, end(parameters) };

	if (colorParams[0][0] == '#') // check to see if it is hexadecimal
	{
		color = ParseColorHexadecimal(colorParams[0].c_str());
	}
	else // then it must be RGB or RGBA decimal such as 255 255 255 or 255 255 255 255
	{
		// Note: blue and red are swapped for endianness
		color.b = ParseNumberValue(colorParams[0]);
		color.g = ParseNumberValue(colorParams[1]);
		color.r = ParseNumberValue(colorParams[2]);

		if (colorParams.size() > 3)
			color.a = ParseNumberValue(colorParams[3]);
	};

	return color;
}


std::string CutsceneCommands::GetArrayName(const std::string& parameter)
{
	std::string arrayName = "";
	bool readingInName = true;
	int dimensions = std::count(parameter.begin(), parameter.end(), '[');
	int currentDimension = 0;

	for (int i = 1; i < parameter.size(); i++)
	{
		if (parameter[i] == '[')
		{
			readingInName = false;
		}
		else if (readingInName)
		{
			arrayName += parameter[i];
		}
	}

	return arrayName;
}

bool CutsceneCommands::GetArray(const std::string& parameter)
{
	arrayIndex = 0;
	vectorIndex = 0;

	// interrogate 0 2
	std::vector<std::string> parameters;
	std::istringstream ss(parameter);
	std::string token;

	while (std::getline(ss, token, ' '))
	{
		if (token != "")
		{
			parameters.push_back(token);
		}
	}

	/*
	std::string arrayName = parameters[0].substr(1, parameters[0].size() - 1);
	c.arrayIndex = c.GetNumAlias(arrayName);
	if (parameters.size() > 2)
	{
		c.vectorIndex = (c.ParseNumberValue(parameters[1]) * arrayNumbersPerSlot[c.arrayIndex]) + c.ParseNumberValue(parameters[2]);
	}
	else
	{
		c.vectorIndex = c.ParseNumberValue(parameters[1]);
	}
	*/

	std::string arrayName = GetArrayName(parameter);
	arrayIndex = GetNumAlias(arrayName);
	int numDimensions = arrayNumbersPerSlot[arrayIndex] > 0 ? 2 : 1;

	// If this array has more than one dimension...
	// TODO: Refactor and optimize this!
	std::vector<int> coordinates;
	std::string coord = "";
	bool readingInNumber = false;
	int seenDimensions = 0;
	int i = 0;

	while (seenDimensions < numDimensions)
	{
		if (i >= parameter.size())
		{
			manager->game->logger.Log("ERROR: Array " + parameter + " syntax error");
			return false;
		}

		if (parameter[i] == '[')
		{
			readingInNumber = true;
		}
		else if (parameter[i] == ']')
		{
			coordinates.emplace_back(ParseNumberValue(coord));
			coord = "";
			readingInNumber = false;
			seenDimensions++;
		}
		else if (readingInNumber)
		{
			coord += parameter[i];
		}

		i++;
	}

	if (numDimensions == 1)
	{
		vectorIndex = coordinates[0];
	}
	else
	{
		// [0][0] = (0 * 2) + 0 = 0
		// [0][1] = (0 * 2) + 1 = 1
		// [1][0] = (1 * 2) + 0 = 2
		// [1][1] = (1 * 2) + 1 = 3
		// [2][0] = (2 * 2) + 0 = 4

		vectorIndex = (coordinates[0] * arrayNumbersPerSlot[arrayIndex]) + coordinates[1];
	}

	return true;
}
