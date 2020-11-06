#ifndef EDITORHELPER_H
#define EDITORHELPER_H
#pragma once

class Entity;
class Editor;
class Renderer;
class Vector2;

#include "leak_check.h"

class DECLSPEC EditorHelper
{
public:
	Editor* editor = nullptr;
	virtual void Render(const Renderer& renderer);
	virtual void CreateLevelEnd();
	virtual void CreateLevelStart();
	virtual void PlaceObject(Vector2& snappedPosition);
	virtual void DeleteObject(bool shouldDeleteThis, Entity* entityToDelete);
	EditorHelper();
	~EditorHelper();
};

#endif