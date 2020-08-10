#include "Background.h"
#include "BackgroundLayer.h"
#include "Game.h"
#include "SpriteManager.h"
#include "Entity.h"
#include "Vector2.h"
#include <iterator>

//TODO: Create a way to import backgrounds via level editor
// (different levels should use different backgrounds, etc.)

std::unordered_map<std::string, BackgroundData*> Background::bgData;

Background::Background(std::string n, Vector2 pos)
{
	name = n;
	position = pos;
	ReadBackgroundData("data/bg.dat");
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

			BackgroundLayerData* newLayerData = new BackgroundLayerData();
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
void Background::CreateBackground(std::string n, Vector2 pos, SpriteManager* spriteManager, const Renderer& renderer)
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
		blueBG->GetSprite()->SetScale(Vector2(19.875f, 11.2f * 4));
	}

	BackgroundData* data = bgData[name];

	for (int i = 0; i < data->layers.size(); i++)
	{
		BackgroundLayerData* ld = data->layers[i];
		AddLayer(Vector2(pos.x + ld->offsetX, pos.y + ld->offsetY), spriteManager, renderer,
			ld->filepath, ld->drawOrder, ld->parallax);
	}
}


Background::~Background()
{
	for (unsigned int i = 0; i < layers.size(); i++)
	{
		delete_it(layers[i]);
	}
}

void Background::Render(const Renderer& renderer)
{
	for (unsigned int i = 0; i < layers.size(); i++)
	{
		layers[i]->Render(renderer);
	}
}

Entity* Background::AddLayer(Vector2 offset, SpriteManager* spriteManager, const Renderer& renderer, std::string filepath, int drawOrder, float parallax)
{
	Sprite* layer = new Sprite(1, spriteManager, filepath, 
		renderer.shaders[ShaderName::Default], Vector2(0, 0));
	Entity* bg = new BackgroundLayer(offset, parallax);
	bg->drawOrder = drawOrder;
	bg->SetSprite(*layer);
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
	for (unsigned int i = 0; i < layers.size(); i++)
		delete_it(layers[i]);
	
	layers.clear();
}

void Background::Save(std::ostringstream& level)
{
	level << "0 bg " << position.x << " " << position.y << " " << name << std::endl;
}