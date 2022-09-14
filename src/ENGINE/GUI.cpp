#include "GUI.h"
#include "Game.h"

void GUI::Init(Game* g)
{
	game = g;
	LoadData(g);
}

GUI::~GUI()
{
	for (auto& val : texts)
	{
		if (val != nullptr)
			delete_it(val);
	}
	texts.clear();

	for (auto& val : images)
	{
		if (val != nullptr)
			delete_it(val);
	}
	images.clear();
}

Text* GUI::GetText(const std::string& name)
{
	return texts[textNames[name]];
}

void GUI::Update()
{

}

void GUI::RenderStart()
{

}

void GUI::Render(const Renderer& renderer)
{
	if (game->showFPS)
		texts[0]->Render(renderer);

	if (game->showTimer)
		texts[1]->Render(renderer);

	if (currentGUI != "")
	{
		for (const auto& image : guiImages[currentGUI])
		{
			image->Render(renderer);
		}

		for (const auto& text : guiTexts[currentGUI])
		{
			text->Render(renderer);
		}
	}
}

void GUI::ResetText()
{
	// FPS text
	texts[0]->SetText("FPS:");
	texts[0]->SetPosition((game->screenWidth * 2) - (texts[0]->GetTextWidth() * 4), texts[0]->GetTextHeight());

	// Timer text
	texts[1]->SetText("");
	texts[1]->SetPosition((game->screenWidth) - (texts[1]->GetTextWidth() * 2), texts[1]->GetTextHeight());
}

void GUI::SaveData()
{
	std::string imageData = "";
	std::string textData = "";

	std::string line = "";

	for (const auto& image : images)
	{
		line = image->name + "`" + image->GetSprite()->GetFileName();
		imageData += line + "\n";
	}

	for (const auto& text : texts)
	{
		line = text->name + "`" + text->txt;
		textData += line + "\n";
	}

	// The GUI File

	std::string data = "";

	for (const auto& name : guiNames)
	{
		data += "*" + name + "\n";

		for (const auto& image : guiImages[name])
		{
			line = image->name;

			if (image->scale != glm::vec2(1, 1))
			{
				line += "`scale`" + std::to_string((int)image->scale.x) + "," + std::to_string((int)image->scale.y);
			}

			if (image->position != glm::vec3(0, 0, 0))
			{
				line += "`pos`" + std::to_string((int)image->position.x) + "," 
					+ std::to_string((int)image->position.y) + std::to_string((int)image->position.z);
			}

			data += line + "\n";
		}

		for (const auto& text : guiTexts[name])
		{
			line = text->name;

			if (text->scale != glm::vec2(1, 1))
			{
				line += "`scale`" + std::to_string((int)text->scale.x) + "," + std::to_string((int)text->scale.y);
			}

			if (text->position != glm::vec3(0, 0, 0))
			{
				line += "`pos`" + std::to_string((int)text->position.x) + ","
					+ std::to_string((int)text->position.y) + "," + std::to_string((int)text->position.z);
			}

			data += line + "\n";
		}
	}

	// Write to files

	std::ofstream fout;

	fout.open("data/gui/texts.dat");
	fout << textData;
	fout.close();

	fout.open("data/gui/images.dat");
	fout << imageData;
	fout.close();

	fout.open("data/gui/gui.dat");
	fout << data;
	fout.close();

	std::cout << "Saved GUI" << std::endl;
}

// Replace all texts with any variables
void GUI::UpdateText(const std::string& key)
{
	if (originalTexts[key] != "_")
	{
		Text* text = texts[textNames[key]];
		std::string txt = "";
		Color textColor = { 255, 255, 255, 255 };
		int letterIndex = 0;

		text->SetText("");
		while (letterIndex < originalTexts[key].size())
		{
			txt = game->cutsceneManager.ParseText(originalTexts[key], letterIndex, textColor, text);
			for (int i = 0; i < txt.size(); i++)
			{
				text->AddText(txt[i], textColor);
			}
		}
	}
}

void GUI::LoadData(Game* g)
{
	images.clear();
	imageNames.clear();

	texts.clear();
	textNames.clear();

	guiNames.clear();
	guiImages.clear();
	guiTexts.clear();

	int index = 0;
	std::string key = "";
	std::string val = "";
	std::string txt = "";

	std::vector<std::string> lines = ReadStringsFromFile("data/gui/texts.dat");

	for (const auto& line : lines)
	{
		index = 0;
		key = ParseWord(line, '`', index);
		val = ParseWord(line, '`', index);

		Text* text = new Text(game->theFont);
		text->SetText("");

		if (val != "_")
		{
			Color textColor = { 255, 255, 255, 255 };
			int letterIndex = 0;

			//text->SetTextAsOneSprite(txt, textColor);
			while (letterIndex < val.size())
			{
				txt = game->cutsceneManager.ParseText(val, letterIndex, textColor, text);
				for (int i = 0; i < txt.size(); i++)
				{
					text->AddText(txt[i], textColor);
				}
			}
		}

		text->name = key;
		text->GetSprite()->keepPositionRelativeToCamera = true;
		text->GetSprite()->keepScaleRelativeToCamera = true;
		text->isRichText = true;

		texts.emplace_back(text);
		textNames[key] = texts.size() - 1;
		originalTexts[key] = val;
	}

	ResetText();

	lines = ReadStringsFromFile("data/gui/images.dat");
	for (const auto& line : lines)
	{
		index = 0;
		key = ParseWord(line, '`', index);
		val = ParseWord(line, '`', index);

		Entity* image = new Entity(glm::vec3(0, 0, 0));
		image->name = key;
		image->GetSprite()->SetTexture(g->spriteManager.GetImage(val));
		image->GetSprite()->keepPositionRelativeToCamera = true;
		image->GetSprite()->keepScaleRelativeToCamera = true;

		images.emplace_back(image);
		imageNames[key] = images.size() - 1;
	}

	lines = ReadStringsFromFile("data/gui/gui.dat");
	std::string current = "";

	for (const auto& line : lines)
	{
		if (line[0] == '*') // new gui
		{
			index = 1;
			current = ParseWord(line, '\n', index);
			guiNames.emplace_back(current);
		}
		else
		{
			index = 0;
			std::string imageName = "";
			std::string argKey = "";
			std::string argValue = "";
			int argNumber = 0;

			std::unordered_map<std::string, std::string> imageArgs;

			imageName = ParseWord(line, '`', index);

			bool isText = false;

			Text* text = nullptr;
			Entity* entity = nullptr;

			isText = (imageNames.count(imageName) == 0);

			if (isText)
			{
				text = texts[textNames[imageName]]; // TODO: Make this less error-prone
				if (text == nullptr)
				{
					std::cout << "ERROR: Could not load GUI data for " << imageName << std::endl;
					continue;
				}

				guiTexts[current].emplace_back(text);
			}
			else
			{
				entity = images[imageNames[imageName]];
				guiImages[current].emplace_back(entity);
			}

			while (index < line.size())
			{
				argNumber++;
				if (argNumber % 2 == 0)
				{
					argValue = ParseWord(line, '`', index);
					imageArgs[argKey] = argValue;
				}
				else
				{
					argKey = ParseWord(line, '`', index);
				}
			}

			for (auto& [key, val] : imageArgs)
			{
				index = 0;
				if (key == "pos")
				{
					while (val[index] != ',' && val[index] != '\0')
					{
						imageArgs["pos_x"] += val[index];
						index++;
					}
					index++;
					while (val[index] != ',' && val[index] != '\0')
					{
						imageArgs["pos_y"] += val[index];
						index++;
					}
					index++;
					while (val[index] != ',' && val[index] != '\0')
					{
						imageArgs["pos_z"] += val[index];
						index++;
					}

					if (isText)
					{
						text->SetPosition(atoi(imageArgs["pos_x"].c_str()),
							atoi(imageArgs["pos_y"].c_str()));
					}
					else
					{
						entity->position = glm::vec3(atoi(imageArgs["pos_x"].c_str()),
							atoi(imageArgs["pos_y"].c_str()), atoi(imageArgs["pos_z"].c_str()));
					}

				}
				else if (key == "scale")
				{
					while (val[index] != ',' && val[index] != '\0')
					{
						imageArgs["scale_x"] += val[index];
						index++;
					}
					index++;
					while (val[index] != ',' && val[index] != '\0')
					{
						imageArgs["scale_y"] += val[index];
						index++;
					}
					index++;

					entity->SetScale(glm::vec2(atoi(imageArgs["scale_x"].c_str()),
						atoi(imageArgs["scale_y"].c_str())));
				}
			}
		}
		
	}

}