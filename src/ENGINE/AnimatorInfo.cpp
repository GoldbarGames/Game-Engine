#include "AnimatorInfo.h"
#include "leak_check.h"

AnimatorInfo::AnimatorInfo(const std::string& filePath)
{
	std::ifstream fin;

	//TODO: Customize prefix/suffix
	std::string animatorFile = "data/animators/" + filePath + ".animator";

	//TODO: Deal with issues involving extra whitespace (it breaks things)
	std::vector<std::string> stateNames;
	//mapStateNamesToNumbers[""] = 0;
	//stateMachines[""] = new AnimStateMachine();

	bool readingInConditions = false;
	// Read in the state machine animator file
	// std::cout << "Reading animator file:" << std::endl;
	fin.open(animatorFile);
	if (fin.is_open())
	{
		// We need to keep track of, for each line:
		// the state to go to
		// conditions to check
		// and for each condition:
		// 
		// variable type
		// variable name
		// check type (=, <, >)
		// expected value

		std::string stateName = "";
		int stateIndex = 1;
		int variableIndex = 0;
		size_t index = 0;

		std::vector<AnimCondition> conditions;
		std::string variableType = "";
		std::string variableName = "";
		std::string conditionCheck = "";
		std::string expectedValue = "";
		std::string nextStateName = "";

		for (std::string line; std::getline(fin, line); )
		{
			// Remove trailing \r if present (Windows line endings)
			if (!line.empty() && line.back() == '\r')
				line.pop_back();

			if (line.size() == 0)
				continue;

			// remove trailing spaces
			while (!line.empty() && line.back() == ' ')
				line.pop_back();

			if (line.front() == '*') // entering a new state
			{
				if (readingInConditions)
				{
					stateNames.clear();
					readingInConditions = false;
				}

				index = 1;
				stateName = ParseWord(line, '*', index);
				stateNames.push_back(stateName);
				mapStateNamesToNumbers[stateName] = stateIndex++;
			}
			else
			{
				readingInConditions = true;
				bool readLine = true;
				index = 0;

				nextStateName = ParseWord(line, ':', index);
				index++;

				conditions.clear();

				while (readLine)
				{
					variableType = ParseWord(line, ' ', index);
					variableName = ParseWord(line, ' ', index);
					conditionCheck = ParseWord(line, ' ', index);
					expectedValue = ParseWord(line, ' ', index);

					//TODO: If we really want to do this the right way
					// we just need to use a binary search tree (abstract syntax tree)

					readLine = (ParseWord(line, ' ', index) == "&&");

					if (variableType == "bool")
					{
						mapKeysBool[variableName] = variableIndex++;
					}
					else if (variableType == "float")
					{
						mapKeysFloat[variableName] = variableIndex++;
					}
					else if (variableType == "int")
					{
						mapKeysInt[variableName] = variableIndex++;
					}

					// Add this condition to the list of conditions for this state
					conditions.push_back(AnimCondition(nextStateName, variableName,
						conditionCheck, (expectedValue == "true")));

					// state name associated with a vector of structs (conditions)
				}

				// after parsing all conditions, assign them to the state(s)

				// TODO: If there are two ways to get to the next state from within a state,
				// the later way overwrites the first way (rather than both being options)
				for (size_t i = 0; i < stateNames.size(); i++)
				{
					if (stateMachines.count(stateNames[i]) != 1)
					{
						//std::cout << "Creating sm " << stateNames[i] << std::endl;
						stateMachines[stateNames[i]] = AnimStateMachine();
					}
					stateMachines[stateNames[i]].conditions[nextStateName] = conditions;
				}
			}
		}

		fin.close();
	}

	if (stateMachines.count("ANY") != 0)
	{
		for (auto& [key, machine] : stateMachines)
		{
			if (key != "ANY")
			{				
				for (auto& [nextState, anyCondition] : stateMachines["ANY"].conditions)
				{
					machine.conditions[nextState].insert(machine.conditions[nextState].end(), anyCondition.begin(), anyCondition.end());
				}
			}
		}
	}
}

AnimatorInfo::~AnimatorInfo()
{	
	
	for (auto& [key, stateMachine] : stateMachines)
	{
		//std::cout << "Deleting sm " << key << std::endl;
		//if (stateMachine != nullptr)
		//	delete_it(stateMachine);
	}	
}

AnimStateMachine::~AnimStateMachine() 
{

}

AnimCondition::~AnimCondition()
{

}