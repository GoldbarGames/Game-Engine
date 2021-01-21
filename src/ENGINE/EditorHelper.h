#ifndef EDITORHELPER_H
#define EDITORHELPER_H
#pragma once

class Entity;
class Editor;
class Renderer;
class Vector2;

#include "leak_check.h"
#include <glm/vec3.hpp>

class KINJO_API EditorHelper
{
public:
	Editor* editor = nullptr;
	virtual void Render(const Renderer& renderer);
	virtual void OnEditorStart();
	virtual void OnEditorEnd();
	virtual void CreateLevelEnd();
	virtual void CreateLevelStart();
	virtual void PlaceObject(glm::vec3& snappedPosition);
	virtual void DeleteObject(bool shouldDeleteThis, Entity* entityToDelete);
	virtual void ToggleObjectMode(const std::string& mode);
	EditorHelper();
	~EditorHelper();
};

#endif