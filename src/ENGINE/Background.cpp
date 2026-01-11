#include "Background.h"
#include "BackgroundLayer.h"
#include "Game.h"
#include "SpriteManager.h"
#include "Entity.h"
#include "Sprite.h"
#include "Renderer.h"
#include <iterator>
#include <fstream>
#include <sstream>

std::unordered_map<std::string, BackgroundData*> Background::bgData;

Background::Background(const std::string& n, const glm::vec3& pos)
{
	name = n;
	position = pos;
	ReadBackgroundData("data/config/bg.dat");
}

Background::~Background()
{
	for (size_t i = 0; i < layers.size(); i++)
	{
		delete_it(layers[i]);
	}

	for (auto& [key, val] : bgData)
	{
		for (size_t i = 0; i < val->layers.size(); i++)
		{
			delete_it(val->layers[i]);
		}
		delete_it(val);
	}
}

void Background::ReadBackgroundData(const std::string& dataFilePath)
{
	// Get data from the file
	std::ifstream fin;
	fin.open(dataFilePath);

	std::string data = "";
	for (std::string line; std::getline(fin, line); )
	{
		data += line + "\n";
	}

	fin.close();

	// Go through the data and add all states
	std::stringstream ss{ data };

	char lineChar[256];
	ss.getline(lineChar, 256);

	std::string backgroundName = "";
	std::string layerName = "";

	int offsetX, offsetY, drawOrder = 0;
	std::string filepath = "";
	float parallax = 0.0f;
	int index = 0;

	while (ss.good() && !ss.eof())
	{
		std::istringstream buf(lineChar);
		std::istream_iterator<std::string> beg(buf), end;
		std::vector<std::string> tokens(beg, end);

		index = 0;

		if (tokens.size() == 0)
			break;

		if (tokens[0][0] == '*')
		{
			index++;
			backgroundName = tokens[index++];
			BackgroundData* newBackgroundData = new BackgroundData();
			newBackgroundData->name = backgroundName;
			newBackgroundData->width = std::stoi(tokens[index++]);
			newBackgroundData->height = std::stoi(tokens[index++]);
			newBackgroundData->numBGs = std::stoi(tokens[index++]);
			newBackgroundData->xOffset = std::stoi(tokens[index++]);
			newBackgroundData->yOffset = std::stoi(tokens[index++]);
			bgData[backgroundName] = newBackgroundData;
		}
		else
		{
			BackgroundLayerData* newLayerData = new BackgroundLayerData();
			newLayerData->offsetX = std::stoi(tokens[index++]);
			newLayerData->offsetY = std::stoi(tokens[index++]);
			newLayerData->filepath = tokens[index++];
			newLayerData->drawOrder = std::stoi(tokens[index++]);
			newLayerData->parallax = std::stof(tokens[index++]);

			if (tokens.size() == 7)
			{
				newLayerData->scaleX = std::stof(tokens[index++]);
				newLayerData->scaleY = std::stof(tokens[index++]);
			}
			else if (tokens.size() == 8)
			{
				newLayerData->scaleX = std::stof(tokens[index++]);
				newLayerData->scaleY = std::stof(tokens[index++]);
				newLayerData->shader = std::stoi(tokens[index++]);
			}
			else if (tokens.size() == 11)
			{
				newLayerData->scaleX = std::stof(tokens[index++]);
				newLayerData->scaleY = std::stof(tokens[index++]);
				newLayerData->color.r = std::stoi(tokens[index++]);
				newLayerData->color.g = std::stoi(tokens[index++]);
				newLayerData->color.b = std::stoi(tokens[index++]);
				newLayerData->color.a = std::stoi(tokens[index++]);
			}

			bgData[backgroundName]->layers.push_back(newLayerData);
		}

		ss.getline(lineChar, 256);
	}
}

void Background::SpawnBackground(const std::string& n, int x, int y, Game& game)
{
	//DeleteLayers(game);
	ResetBackground();

	if (n != "None" && n != "none")
	{
		std::vector<Entity*> layerEntities;
		BackgroundData* data = bgData[n];

		// TODO: Crashes here when level tries to load undefined BG

		// Create the backgrounds from file data
		unsigned int NUM_BGS = data->numBGs;
		unsigned int BG_WIDTH = data->width * Camera::MULTIPLIER;
		unsigned int X_OFFSET = data->xOffset + x;
		unsigned int Y_OFFSET = data->yOffset + y;

		for (unsigned int i = 0; i < NUM_BGS; i++)
		{
			glm::vec3 bgPos = glm::vec3((BG_WIDTH * i) + X_OFFSET, Y_OFFSET, 0);
			name = n;
			BackgroundData* data = bgData[name];

			for (size_t i = 0; i < data->layers.size(); i++)
			{
				BackgroundLayerData bld = *data->layers[i];

				Entity* bg = new BackgroundLayer(glm::vec3(bgPos.x + bld.offsetX, bgPos.y + bld.offsetY, bgPos.z), bld.parallax);
				bg->drawOrder = bld.drawOrder;
				bg->GetSprite()->SetTexture(game.spriteManager.GetImage(bld.filepath));
				bg->GetSprite()->SetShader(game.renderer.shaders[bld.shader]);
				bg->SetColor(bld.color);
				bg->GetSprite()->color = bld.color;
				bg->SetScale(glm::vec2(bld.scaleX, bld.scaleY));
				layerEntities.emplace_back(bg);
			}
		}

		game.SortEntities(layerEntities);

		for (size_t i = 0; i < layerEntities.size(); i++)
		{
			BackgroundLayer* bl = static_cast<BackgroundLayer*>(layerEntities[i]);
			layers.emplace_back(bl);
		}
	}


}

void Background::Render(const Renderer& renderer)
{
	for (unsigned int i = 0; i < layers.size(); i++)
	{
		layers[i]->Render(renderer);
	}
}

// Use this for clearing the list of layers without leaks
void Background::ResetBackground()
{
	for (unsigned int i = 0; i < layers.size(); i++)
		delete_it(layers[i]);
	
	layers.clear();
}

void Background::Save(std::unordered_map<std::string, std::string>& map)
{
	if (name == "")
	{
		name = "None";
	}

	map["id"] = "0";
	map["type"] = "bg";
	map["positionX"] = std::to_string((int)position.x);
	map["positionY"] = std::to_string((int)position.y);
	map["subtype"] = name;
}