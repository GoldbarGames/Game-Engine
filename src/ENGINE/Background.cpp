#include "Background.h"
#include "BackgroundLayer.h"
#include "Game.h"
#include "SpriteManager.h"
#include "Entity.h"
#include "Vector2.h"
#include "Sprite.h"
#include "Renderer.h"
#include <iterator>
#include <fstream>
#include <sstream>

std::unordered_map<std::string, BackgroundData*> Background::bgData;

Background::Background(const std::string& n, const Vector2& pos)
{
	name = n;
	position = pos;
	ReadBackgroundData("data/bg.dat");
}

Background::~Background()
{
	for (unsigned int i = 0; i < layers.size(); i++)
	{
		delete_it(layers[i]);
	}

	for (auto& [key, val] : bgData)
	{
		for (int i = 0; i < val->layers.size(); i++)
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

void Background::SpawnBackground(const std::string& n, Game& game)
{
	//DeleteLayers(game);
	ResetBackground();

	if (n != "None")
	{
		BackgroundData* data = bgData[n];

		// Create the backgrounds from file data
		unsigned int NUM_BGS = data->numBGs;
		unsigned int BG_WIDTH = data->width * Camera::MULTIPLIER;
		unsigned int X_OFFSET = data->xOffset;
		unsigned int Y_OFFSET = data->yOffset;

		for (unsigned int i = 0; i < NUM_BGS; i++)
		{
			glm::vec3 bgPos = glm::vec3((BG_WIDTH * i) + X_OFFSET, Y_OFFSET, 0);
			CreateBackground(n, bgPos, game.spriteManager, game.renderer);
		}

		game.SortEntities(layers);
	}
}

// This function creates one background, possibly composed of multiple layers.
// This function may be called multiple times to place copies of backgrounds next to each other.
void Background::CreateBackground(const std::string& n, glm::vec3 pos,
	const SpriteManager& spriteManager, const Renderer& renderer)
{
	name = n;
	BackgroundData* data = bgData[name];

	for (int i = 0; i < data->layers.size(); i++)
	{
		BackgroundLayerData* ld = data->layers[i];
		AddLayer(pos, *ld, spriteManager, renderer);
	}
}


void Background::Render(const Renderer& renderer)
{
	for (unsigned int i = 0; i < layers.size(); i++)
	{
		layers[i]->Render(renderer);
	}
}

Entity* Background::AddLayer(const glm::vec3& pos, const BackgroundLayerData& data,
	const SpriteManager& spriteManager, const Renderer& renderer)
{	
	Entity* bg = new BackgroundLayer(glm::vec3(pos.x + data.offsetX, pos.y + data.offsetY, pos.z), data.parallax);
	bg->drawOrder = data.drawOrder;
	bg->GetSprite()->SetTexture(spriteManager.GetImage(data.filepath));
	bg->GetSprite()->SetShader(renderer.shaders[ShaderName::Default]);
	bg->SetColor(data.color);
	bg->GetSprite()->color = data.color;
	bg->SetScale(Vector2(data.scaleX, data.scaleY));
	layers.emplace_back(bg);
	return bg;
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