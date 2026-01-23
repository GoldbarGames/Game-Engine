#ifndef MENULOADER_H
#define MENULOADER_H
#pragma once

#include "leak_check.h"
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <functional>
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include <nlohmann/json.hpp>

class Game;
class MenuScreen;
class BaseButton;
class Entity;
class Text;

using json = nlohmann::json;

// Represents a slot where dynamic content can be injected
struct KINJO_API MenuSlot
{
	std::string name;
	float x = 0;
	float y = 0;
	std::string layout = "vertical";  // "vertical", "horizontal", "grid"
	int columns = 1;
	float spacingX = 0;
	float spacingY = 120;

	// Elements added to this slot
	std::vector<BaseButton*> buttons;
	std::vector<Text*> texts;
	std::vector<Entity*> images;

	// Calculate position for item at index
	glm::vec3 GetPositionForIndex(int index) const;
};

// Template for repeating elements based on data
struct KINJO_API MenuTemplate
{
	std::string name;
	std::string elementType = "button";  // "button", "text", "image"
	std::string dataSource;              // Name of the data source to iterate
	std::string filter;                  // Optional filter condition

	// Layout settings
	std::string layout = "grid";
	int columns = 3;
	float startX = 0;
	float startY = 0;
	float spacingX = 240;
	float spacingY = 320;

	// Element properties (can use {variable} placeholders)
	std::string text = "{name}";
	std::string filepathPattern = "";
	std::string buttonIdName = "";
	int buttonId = 0;
	float width = 150;
	float height = 40;
	float scaleX = 1.0f;
	float scaleY = 1.0f;
};

// Data item for templates - represents one item in a data source
struct KINJO_API MenuDataItem
{
	std::string name;
	std::string displayText;
	std::string imagePath;
	int id = 0;
	bool visible = true;
	std::unordered_map<std::string, std::string> properties;

	std::string GetProperty(const std::string& key, const std::string& defaultVal = "") const;
};

// Interface for providing dynamic data to menus
class KINJO_API MenuDataProvider
{
public:
	virtual ~MenuDataProvider() = default;

	// Get data items for a named data source (e.g., "spells", "inventory", "save_files")
	virtual std::vector<MenuDataItem> GetDataItems(const std::string& dataSource, Game& game) = 0;

	// Resolve a variable reference (e.g., "${player_name}")
	virtual std::string ResolveVariable(const std::string& varName, Game& game) = 0;

	// Check a condition (e.g., "unlocked", "has_item:key")
	virtual bool CheckCondition(const std::string& condition, const MenuDataItem& item, Game& game) = 0;

	// Called after a template element is created - allows customization
	virtual void OnTemplateElementCreated(const std::string& templateName,
	                                       const MenuDataItem& item,
	                                       BaseButton* button,
	                                       Game& game) {}

	// Called when a slot needs to be populated
	virtual void PopulateSlot(const std::string& slotName, MenuSlot& slot,
	                          MenuScreen& menu, Game& game) {}
};

// Default data provider that uses cutscene variables
class KINJO_API DefaultMenuDataProvider : public MenuDataProvider
{
public:
	std::vector<MenuDataItem> GetDataItems(const std::string& dataSource, Game& game) override;
	std::string ResolveVariable(const std::string& varName, Game& game) override;
	bool CheckCondition(const std::string& condition, const MenuDataItem& item, Game& game) override;
};

// Main menu loader class
class KINJO_API MenuLoader
{
public:
	MenuLoader();
	~MenuLoader();

	// Set the data provider for dynamic content
	void SetDataProvider(MenuDataProvider* provider);

	// Load a menu from a .menu file
	bool LoadMenu(MenuScreen& menu, const std::string& filepath, Game& game);

	// Process templates and create elements
	void ProcessTemplates(MenuScreen& menu, Game& game);

	// Process slots - call after ProcessTemplates
	void ProcessSlots(MenuScreen& menu, Game& game);

	// Resolve variable placeholders in a string (e.g., "${player_name}" or "{name}")
	std::string ResolveString(const std::string& input, Game& game,
	                          const MenuDataItem* item = nullptr);

	// Get a slot by name
	MenuSlot* GetSlot(const std::string& name);

	// Check if menu file exists
	static bool MenuFileExists(const std::string& filepath);

private:
	MenuDataProvider* dataProvider = nullptr;
	DefaultMenuDataProvider defaultProvider;

	std::vector<MenuTemplate> templates;
	std::vector<MenuSlot> slots;

	// Parsing helpers
	void ParseElement(MenuScreen& menu, const nlohmann::json& elem, Game& game);
	void ParseTemplate(const nlohmann::json& tmpl);
	void ParseSlot(const nlohmann::json& slotData);
	void ParseAnimation(Entity* entity, const nlohmann::json& animData,
	                    MenuScreen& menu, bool isEnter);

	// Create elements from template
	void CreateTemplateElements(MenuScreen& menu, const MenuTemplate& tmpl, Game& game);

	// Get button ID from name or number
	int ResolveButtonId(const std::string& idName, int defaultId, Game& game);
};

#endif
