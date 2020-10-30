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

//TODO: Create a way to import backgrounds via level editor
// (different levels should use different backgrounds, etc.)

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
			BackgroundData* newBackgroundData = neww BackgroundData();
			newBackgroundData->name = backgroundName;
			bgData[backgroundName] = newBackgroundData;
		}
		else
		{
			//layerName = tokens[index++];

			offsetX = std::stoi(tokens[index++]);
			offsetY = std::stoi(tokens[index++]);
			filepath = tokens[index++];
			drawOrder = std::stoi(tokens[index++]);
			parallax = std::stof(tokens[index++]);

			BackgroundLayerData* newLayerData = neww BackgroundLayerData();
			newLayerData->offsetX = offsetX;
			newLayerData->offsetY = offsetY;
			newLayerData->filepath = filepath;
			newLayerData->drawOrder = drawOrder;
			newLayerData->parallax = parallax;

			bgData[backgroundName]->layers.push_back(newLayerData);
		}

		ss.getline(lineChar, 256);
	}
}

//TODO: Should this stuff go inside the Background class constructor?
void Background::CreateBackground(const std::string& n, Vector2 pos, 
	const SpriteManager& spriteManager, const Renderer& renderer)
{
	name = n;

	// SPECIAL CASE (TODO: Deal with special cases)
	if (name == "forest")
	{
		pos.y -= 200;
		// This is the blue sky (taking a white square and coloring it blue and increasing its size)
		Entity* blueBG = AddLayer(pos + Vector2(0, -1440), spriteManager, 
			renderer, "assets/gui/white.png", -99, 0.0f);
		blueBG->GetSprite()->color = { 0, 0, 83, 255 };
		blueBG->SetScale(Vector2(19.875f, 11.2f * 4));

		// This is the back ground (taking a white square and coloring it black and increasing its size)
		Entity* blackBG = AddLayer(pos + Vector2(0, 1440), spriteManager,
			renderer, "assets/gui/white.png", -99, 0.0f);
		blackBG->GetSprite()->color = { 0, 0, 0, 255 };
		blackBG->SetScale(Vector2(19.875f, 11.2f * 4));
	}

	BackgroundData* data = bgData[name];

	for (int i = 0; i < data->layers.size(); i++)
	{
		BackgroundLayerData* ld = data->layers[i];
		AddLayer(Vector2(pos.x + ld->offsetX, pos.y + ld->offsetY), spriteManager, renderer,
			ld->filepath, ld->drawOrder, ld->parallax);
	}
}


void Background::Render(const Renderer& renderer)
{
	for (unsigned int i = 0; i < layers.size(); i++)
	{
		layers[i]->Render(renderer);
	}
}

Entity* Background::AddLayer(const Vector2& offset, const SpriteManager& spriteManager,
	const Renderer& renderer, const std::string& filepath, int drawOrder, float parallax)
{	
	Entity* bg = neww BackgroundLayer(offset, parallax);
	bg->drawOrder = drawOrder;
	bg->GetSprite()->SetTexture(spriteManager.GetImage(filepath));
	bg->GetSprite()->SetShader(renderer.shaders[ShaderName::Default]);
	layers.emplace_back(bg);
	return bg;
}

void Background::DeleteLayers(Game& game)
{
	for (unsigned int i = 0; i < layers.size(); i++)
	{
		game.ShouldDeleteEntity(layers[i]);
	}
}

void Background::ResetBackground()
{
	// TODO: HEAP CORRUPTION DETECTED???
	for (unsigned int i = 0; i < layers.size(); i++)
		delete_it(layers[i]);
	
	layers.clear();
}

void Background::Save(std::unordered_map<std::string, std::string>& map)
{
	map["id"] = "0";
	map["type"] = "bg";
	map["positionX"] = std::to_string((int)position.x);
	map["positionY"] = std::to_string((int)position.y);
	map["subtype"] = name;
}