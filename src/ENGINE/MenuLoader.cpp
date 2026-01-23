#include "MenuLoader.h"
#include "MenuScreen.h"
#include "MenuButton.h"
#include "Game.h"
#include "Text.h"
#include "Entity.h"
#include "Sprite.h"
#include "CutsceneCommands.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <regex>

using json = nlohmann::json;

// MenuSlot implementation
glm::vec3 MenuSlot::GetPositionForIndex(int index) const
{
	float px = x;
	float py = y;

	if (layout == "vertical")
	{
		py += spacingY * index;
	}
	else if (layout == "horizontal")
	{
		px += spacingX * index;
	}
	else if (layout == "grid")
	{
		int col = index % columns;
		int row = index / columns;
		px += spacingX * col;
		py += spacingY * row;
	}

	return glm::vec3(px, py, 0);
}

// MenuDataItem implementation
std::string MenuDataItem::GetProperty(const std::string& key, const std::string& defaultVal) const
{
	auto it = properties.find(key);
	if (it != properties.end())
		return it->second;
	return defaultVal;
}

// DefaultMenuDataProvider implementation
std::vector<MenuDataItem> DefaultMenuDataProvider::GetDataItems(const std::string& dataSource, Game& game)
{
	// Default implementation returns empty - games should override
	std::vector<MenuDataItem> items;

	// Try to read from a data file if it exists
	std::string dataPath = "data/menu_data/" + dataSource + ".dat";
	if (FileExists(dataPath))
	{
		std::vector<std::string> lines = ReadStringsFromFile(dataPath);
		for (size_t i = 0; i < lines.size(); i++)
		{
			if (!lines[i].empty())
			{
				MenuDataItem item;
				item.name = lines[i];
				item.displayText = lines[i];
				item.id = static_cast<int>(i);
				items.push_back(item);
			}
		}
	}

	return items;
}

std::string DefaultMenuDataProvider::ResolveVariable(const std::string& varName, Game& game)
{
	CutsceneCommands& cmds = game.cutsceneManager.commands;

	// Check if it's a numalias
	if (cmds.numalias.count(varName) > 0)
	{
		int index = cmds.numalias[varName];
		if (cmds.stringVariables.count(index) > 0)
			return cmds.stringVariables[index];
		if (cmds.numberVariables.count(index) > 0)
			return std::to_string(cmds.numberVariables[index]);
	}

	// Check string alias
	if (cmds.stralias.count(varName) > 0)
		return cmds.stralias[varName];

	// Try parsing as a number index
	try
	{
		int index = std::stoi(varName);
		if (cmds.stringVariables.count(index) > 0)
			return cmds.stringVariables[index];
		if (cmds.numberVariables.count(index) > 0)
			return std::to_string(cmds.numberVariables[index]);
	}
	catch (...) {}

	return varName; // Return as-is if not found
}

bool DefaultMenuDataProvider::CheckCondition(const std::string& condition, const MenuDataItem& item, Game& game)
{
	if (condition.empty())
		return true;

	// Check item visibility
	if (condition == "visible")
		return item.visible;

	// Check a property
	if (condition.find(':') != std::string::npos)
	{
		size_t colonPos = condition.find(':');
		std::string prop = condition.substr(0, colonPos);
		std::string value = condition.substr(colonPos + 1);
		return item.GetProperty(prop) == value;
	}

	// Check as a boolean property
	std::string val = item.GetProperty(condition, "false");
	return val == "true" || val == "1";
}

// MenuLoader implementation
MenuLoader::MenuLoader()
{
	dataProvider = &defaultProvider;
}

MenuLoader::~MenuLoader()
{
}

void MenuLoader::SetDataProvider(MenuDataProvider* provider)
{
	if (provider)
		dataProvider = provider;
	else
		dataProvider = &defaultProvider;
}

bool MenuLoader::MenuFileExists(const std::string& filepath)
{
	return FileExists(filepath);
}

bool MenuLoader::LoadMenu(MenuScreen& menu, const std::string& filepath, Game& game)
{
	if (!FileExists(filepath))
	{
		std::cout << "MenuLoader: File not found: " << filepath << std::endl;
		return false;
	}

	try
	{
		std::ifstream file(filepath);
		json data = json::parse(file);
		file.close();

		// Clear previous data
		templates.clear();
		slots.clear();

		// Parse basic menu properties
		if (data.contains("width"))
			; // Canvas size is informational only

		if (data.contains("canEscapeFrom"))
			menu.canEscapeFrom = data["canEscapeFrom"].get<bool>();

		if (data.contains("isDynamic"))
			menu.isDynamic = data["isDynamic"].get<bool>();

		if (data.contains("useMouse"))
			menu.useMouse = data["useMouse"].get<bool>();

		if (data.contains("rememberLastButton"))
			menu.rememberLastButton = data["rememberLastButton"].get<bool>();

		// Parse static elements
		if (data.contains("elements"))
		{
			for (const auto& elem : data["elements"])
			{
				ParseElement(menu, elem, game);
			}
		}

		// Parse templates
		if (data.contains("templates"))
		{
			for (const auto& tmpl : data["templates"])
			{
				ParseTemplate(tmpl);
			}
		}

		// Parse slots
		if (data.contains("slots"))
		{
			for (const auto& slotData : data["slots"])
			{
				ParseSlot(slotData);
			}
		}

		// Parse button navigation
		if (data.contains("navigation"))
		{
			const auto& nav = data["navigation"];
			if (nav.contains("auto") && nav["auto"].get<bool>())
			{
				bool useLeftRight = nav.contains("useLeftRight") ? nav["useLeftRight"].get<bool>() : true;
				bool useUpDown = nav.contains("useUpDown") ? nav["useUpDown"].get<bool>() : true;
				menu.AssignButtons(useLeftRight, useUpDown);
			}
			else if (nav.contains("links"))
			{
				// Manual navigation links
				for (const auto& link : nav["links"])
				{
					std::string btnName = link["button"].get<std::string>();
					BaseButton* btn = menu.GetButtonByName(btnName);
					if (btn)
					{
						if (link.contains("up"))
							btn->buttonPressedUp = menu.GetButtonByName(link["up"].get<std::string>());
						if (link.contains("down"))
							btn->buttonPressedDown = menu.GetButtonByName(link["down"].get<std::string>());
						if (link.contains("left"))
							btn->buttonPressedLeft = menu.GetButtonByName(link["left"].get<std::string>());
						if (link.contains("right"))
							btn->buttonPressedRight = menu.GetButtonByName(link["right"].get<std::string>());
					}
				}
			}
		}

		std::cout << "MenuLoader: Loaded menu from " << filepath << std::endl;
		return true;
	}
	catch (const std::exception& e)
	{
		std::cout << "MenuLoader: Error parsing " << filepath << ": " << e.what() << std::endl;
		return false;
	}
}

void MenuLoader::ParseElement(MenuScreen& menu, const json& elem, Game& game)
{
	std::string type = elem["type"].get<std::string>();
	std::string name = elem.contains("name") ? elem["name"].get<std::string>() : "";

	float x = elem.contains("x") ? elem["x"].get<float>() : 0;
	float y = elem.contains("y") ? elem["y"].get<float>() : 0;

	// Check visibility condition
	if (elem.contains("visible_if"))
	{
		std::string condition = elem["visible_if"].get<std::string>();
		std::string resolved = ResolveString(condition, game);
		if (resolved == "false" || resolved == "0" || resolved.empty())
			return;
	}

	if (type == "button")
	{
		std::string text = elem.contains("text") ? elem["text"].get<std::string>() : name;
		std::string filepath = elem.contains("filepath") ? elem["filepath"].get<std::string>() : "assets/gui/menu.png";
		int buttonId = elem.contains("button_id") ? elem["button_id"].get<int>() : 0;

		// Resolve any variables in text
		text = ResolveString(text, game);

		// Color
		Color color = { 255, 255, 255, 255 };
		if (elem.contains("color"))
		{
			auto& c = elem["color"];
			color.r = c[0].get<uint8_t>();
			color.g = c[1].get<uint8_t>();
			color.b = c[2].get<uint8_t>();
			color.a = c.size() > 3 ? c[3].get<uint8_t>() : 255;
		}

		MenuButton* btn = menu.AddButton(text, filepath, buttonId, glm::vec3(x, y, 0), game, color);
		btn->name = name;

		// Scale
		if (elem.contains("scale"))
		{
			auto& s = elem["scale"];
			float sx = s.size() > 0 ? s[0].get<float>() : 1.0f;
			float sy = s.size() > 1 ? s[1].get<float>() : sx;
			btn->scale = glm::vec2(sx, sy);
		}

		// Animations
		if (elem.contains("enter_animation"))
		{
			ParseAnimation(btn, elem["enter_animation"], menu, true);
		}
		if (elem.contains("exit_animation"))
		{
			ParseAnimation(btn, elem["exit_animation"], menu, false);
		}
	}
	else if (type == "text")
	{
		std::string text = elem.contains("text") ? elem["text"].get<std::string>() : "";
		text = ResolveString(text, game);

		float scaleX = 1.0f, scaleY = 1.0f;
		if (elem.contains("scale"))
		{
			auto& s = elem["scale"];
			scaleX = s.size() > 0 ? s[0].get<float>() : 1.0f;
			scaleY = s.size() > 1 ? s[1].get<float>() : scaleX;
		}

		bool centered = elem.contains("centered") ? elem["centered"].get<bool>() : false;

		Text* txt = menu.AddText(game.theFont, text, (int)x, (int)y, scaleX, scaleY, centered);

		// Color
		if (elem.contains("color"))
		{
			auto& c = elem["color"];
			Color color;
			color.r = c[0].get<uint8_t>();
			color.g = c[1].get<uint8_t>();
			color.b = c[2].get<uint8_t>();
			color.a = c.size() > 3 ? c[3].get<uint8_t>() : 255;
			txt->SetColor(color);
		}

		// Animations
		if (elem.contains("enter_animation"))
		{
			ParseAnimation(txt, elem["enter_animation"], menu, true);
		}
		if (elem.contains("exit_animation"))
		{
			ParseAnimation(txt, elem["exit_animation"], menu, false);
		}
	}
	else if (type == "image")
	{
		std::string filepath = elem.contains("filepath") ? elem["filepath"].get<std::string>() : "";
		filepath = ResolveString(filepath, game);

		float scaleX = 1.0f, scaleY = 1.0f;
		if (elem.contains("scale"))
		{
			auto& s = elem["scale"];
			scaleX = s.size() > 0 ? s[0].get<float>() : 1.0f;
			scaleY = s.size() > 1 ? s[1].get<float>() : scaleX;
		}

		int shader = elem.contains("shader") ? elem["shader"].get<int>() : 2;

		Entity* img = menu.AddImage(filepath, glm::vec3(x, y, 0), glm::vec2(scaleX, scaleY), game, shader);

		if (!name.empty())
			img->name = name;

		// Draw order
		if (elem.contains("draw_order"))
			img->drawOrder = elem["draw_order"].get<int>();

		// Color
		if (elem.contains("color"))
		{
			auto& c = elem["color"];
			Color color;
			color.r = c[0].get<uint8_t>();
			color.g = c[1].get<uint8_t>();
			color.b = c[2].get<uint8_t>();
			color.a = c.size() > 3 ? c[3].get<uint8_t>() : 255;
			img->SetColor(color);
		}

		// Animations
		if (elem.contains("enter_animation"))
		{
			ParseAnimation(img, elem["enter_animation"], menu, true);
		}
		if (elem.contains("exit_animation"))
		{
			ParseAnimation(img, elem["exit_animation"], menu, false);
		}
	}
}

void MenuLoader::ParseTemplate(const json& tmpl)
{
	MenuTemplate t;

	t.name = tmpl.contains("name") ? tmpl["name"].get<std::string>() : "";
	t.elementType = tmpl.contains("type") ? tmpl["type"].get<std::string>() : "button";
	t.dataSource = tmpl.contains("data_source") ? tmpl["data_source"].get<std::string>() : "";
	t.filter = tmpl.contains("filter") ? tmpl["filter"].get<std::string>() : "";

	t.layout = tmpl.contains("layout") ? tmpl["layout"].get<std::string>() : "grid";
	t.columns = tmpl.contains("columns") ? tmpl["columns"].get<int>() : 3;
	t.startX = tmpl.contains("start_x") ? tmpl["start_x"].get<float>() : 0;
	t.startY = tmpl.contains("start_y") ? tmpl["start_y"].get<float>() : 0;
	t.spacingX = tmpl.contains("spacing_x") ? tmpl["spacing_x"].get<float>() : 240;
	t.spacingY = tmpl.contains("spacing_y") ? tmpl["spacing_y"].get<float>() : 320;

	t.text = tmpl.contains("text") ? tmpl["text"].get<std::string>() : "{name}";
	t.filepathPattern = tmpl.contains("filepath_pattern") ? tmpl["filepath_pattern"].get<std::string>() : "";
	t.buttonIdName = tmpl.contains("button_id_name") ? tmpl["button_id_name"].get<std::string>() : "";
	t.buttonId = tmpl.contains("button_id") ? tmpl["button_id"].get<int>() : 0;
	t.width = tmpl.contains("width") ? tmpl["width"].get<float>() : 150;
	t.height = tmpl.contains("height") ? tmpl["height"].get<float>() : 40;
	t.scaleX = tmpl.contains("scale_x") ? tmpl["scale_x"].get<float>() : 1.0f;
	t.scaleY = tmpl.contains("scale_y") ? tmpl["scale_y"].get<float>() : 1.0f;

	templates.push_back(t);
}

void MenuLoader::ParseSlot(const json& slotData)
{
	MenuSlot slot;

	slot.name = slotData.contains("name") ? slotData["name"].get<std::string>() : "";
	slot.x = slotData.contains("x") ? slotData["x"].get<float>() : 0;
	slot.y = slotData.contains("y") ? slotData["y"].get<float>() : 0;
	slot.layout = slotData.contains("layout") ? slotData["layout"].get<std::string>() : "vertical";
	slot.columns = slotData.contains("columns") ? slotData["columns"].get<int>() : 1;
	slot.spacingX = slotData.contains("spacing_x") ? slotData["spacing_x"].get<float>() : 0;
	slot.spacingY = slotData.contains("spacing_y") ? slotData["spacing_y"].get<float>() : 120;

	slots.push_back(slot);
}

void MenuLoader::ParseAnimation(Entity* entity, const json& animData, MenuScreen& menu, bool isEnter)
{
	if (!entity)
		return;

	MenuAnimation* anim = isEnter ?
		menu.CreateEnterAnimation(entity) :
		menu.CreateExitAnimation(entity);

	if (!anim)
		return;

	// animData can be an array of keyframes
	if (animData.is_array())
	{
		for (const auto& kfData : animData)
		{
			int duration = kfData.contains("duration") ? kfData["duration"].get<int>() : 500;
			MenuAnimKeyframe* kf = anim->CreateKeyframe(duration);

			if (kfData.contains("target_position"))
			{
				auto& pos = kfData["target_position"];
				kf->targetPosition = glm::vec3(
					pos[0].get<float>(),
					pos[1].get<float>(),
					pos.size() > 2 ? pos[2].get<float>() : 0
				);
			}

			if (kfData.contains("target_color"))
			{
				auto& col = kfData["target_color"];
				kf->targetColor = glm::vec4(
					col[0].get<float>(),
					col[1].get<float>(),
					col[2].get<float>(),
					col.size() > 3 ? col[3].get<float>() : 255
				);
			}
		}
	}
}

void MenuLoader::ProcessTemplates(MenuScreen& menu, Game& game)
{
	for (const auto& tmpl : templates)
	{
		CreateTemplateElements(menu, tmpl, game);
	}
}

void MenuLoader::CreateTemplateElements(MenuScreen& menu, const MenuTemplate& tmpl, Game& game)
{
	if (tmpl.dataSource.empty())
		return;

	// Get data items from the provider
	std::vector<MenuDataItem> items = dataProvider->GetDataItems(tmpl.dataSource, game);

	int visibleIndex = 0;
	for (size_t i = 0; i < items.size(); i++)
	{
		const MenuDataItem& item = items[i];

		// Check filter condition
		if (!tmpl.filter.empty() && !dataProvider->CheckCondition(tmpl.filter, item, game))
			continue;

		// Calculate position
		glm::vec3 pos;
		if (tmpl.layout == "grid")
		{
			int col = visibleIndex % tmpl.columns;
			int row = visibleIndex / tmpl.columns;
			pos = glm::vec3(tmpl.startX + (tmpl.spacingX * col),
			                tmpl.startY + (tmpl.spacingY * row), 0);
		}
		else if (tmpl.layout == "horizontal")
		{
			pos = glm::vec3(tmpl.startX + (tmpl.spacingX * visibleIndex), tmpl.startY, 0);
		}
		else // vertical
		{
			pos = glm::vec3(tmpl.startX, tmpl.startY + (tmpl.spacingY * visibleIndex), 0);
		}

		// Resolve placeholders
		std::string resolvedText = ResolveString(tmpl.text, game, &item);
		std::string resolvedPath = ResolveString(tmpl.filepathPattern, game, &item);

		// Create the element
		if (tmpl.elementType == "button")
		{
			int btnId = tmpl.buttonId;
			if (!tmpl.buttonIdName.empty())
				btnId = ResolveButtonId(tmpl.buttonIdName, tmpl.buttonId, game);

			MenuButton* btn = menu.AddButton(resolvedText, resolvedPath, btnId, pos, game);
			btn->name = item.name;
			btn->scale = glm::vec2(tmpl.scaleX, tmpl.scaleY);

			// Notify data provider for customization
			dataProvider->OnTemplateElementCreated(tmpl.name, item, btn, game);
		}
		else if (tmpl.elementType == "text")
		{
			Text* txt = menu.AddText(game.theFont, resolvedText,
			                         (int)pos.x, (int)pos.y, tmpl.scaleX, tmpl.scaleY, false);
		}
		else if (tmpl.elementType == "image")
		{
			Entity* img = menu.AddImage(resolvedPath, pos,
			                            glm::vec2(tmpl.scaleX, tmpl.scaleY), game, 2);
			img->name = item.name;
		}

		visibleIndex++;
	}
}

void MenuLoader::ProcessSlots(MenuScreen& menu, Game& game)
{
	for (auto& slot : slots)
	{
		dataProvider->PopulateSlot(slot.name, slot, menu, game);
	}
}

std::string MenuLoader::ResolveString(const std::string& input, Game& game, const MenuDataItem* item)
{
	std::string result = input;

	// Replace {property} placeholders with item properties
	if (item)
	{
		std::regex itemPattern("\\{(\\w+)\\}");
		std::smatch match;
		std::string temp = result;

		while (std::regex_search(temp, match, itemPattern))
		{
			std::string placeholder = match[0].str();
			std::string propName = match[1].str();

			std::string replacement;
			if (propName == "name")
				replacement = item->name;
			else if (propName == "text" || propName == "displayText")
				replacement = item->displayText;
			else if (propName == "image" || propName == "imagePath")
				replacement = item->imagePath;
			else if (propName == "id")
				replacement = std::to_string(item->id);
			else
				replacement = item->GetProperty(propName, placeholder);

			size_t pos = result.find(placeholder);
			if (pos != std::string::npos)
				result.replace(pos, placeholder.length(), replacement);

			temp = match.suffix().str();
		}
	}

	// Replace ${variable} placeholders with cutscene variables
	std::regex varPattern("\\$\\{(\\w+)\\}");
	std::smatch match;
	std::string temp = result;

	while (std::regex_search(temp, match, varPattern))
	{
		std::string placeholder = match[0].str();
		std::string varName = match[1].str();

		std::string replacement = dataProvider->ResolveVariable(varName, game);

		size_t pos = result.find(placeholder);
		if (pos != std::string::npos)
			result.replace(pos, placeholder.length(), replacement);

		temp = match.suffix().str();
	}

	return result;
}

MenuSlot* MenuLoader::GetSlot(const std::string& name)
{
	for (auto& slot : slots)
	{
		if (slot.name == name)
			return &slot;
	}
	return nullptr;
}

int MenuLoader::ResolveButtonId(const std::string& idName, int defaultId, Game& game)
{
	// Could look up from an enum mapping or cutscene variable
	// For now, just return the default
	return defaultId;
}
