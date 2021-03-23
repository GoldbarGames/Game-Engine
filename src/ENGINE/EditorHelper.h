#ifndef EDITORHELPER_H
#define EDITORHELPER_H
#pragma once

class Entity;
class Editor;
class Renderer;
class Vector2;

#include "leak_check.h"
#include <glm/vec3.hpp>
class Tile;
class Game;

class KINJO_API EditorHelper
{
public:
	Editor* editor = nullptr;

	virtual void CustomSave(std::ostringstream& level);
	virtual bool CustomLoad(const std::string& etype, const std::string tokens[32]);
	virtual bool ClickedCustomButton(const std::string& clickedName);
	virtual void Render(const Renderer& renderer);
	virtual void OnEditorStart();
	virtual void OnEditorEnd();
	virtual void CreateLevelEnd();
	virtual void CreateLevelStart();
	virtual void PlaceObject(glm::vec3& snappedPosition);
	virtual void DeleteObject(bool shouldDeleteThis, Entity* entityToDelete);
	virtual void PlaceTile(Tile& tile);
	virtual void ToggleObjectMode(const std::string& mode);
	EditorHelper(Game* game);
	~EditorHelper();
};

#endif