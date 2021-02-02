#include "EditorHelper.h"
#include "Editor.h"
#include "Entity.h"
#include "Game.h"

EditorHelper::EditorHelper()
{

}

EditorHelper::~EditorHelper()
{

}

void EditorHelper::OnEditorStart()
{

}

void EditorHelper::OnEditorEnd()
{

}

void EditorHelper::CreateLevelEnd()
{
	
}

void EditorHelper::CreateLevelStart()
{

}

void EditorHelper::Render(const Renderer& renderer)
{

}

void EditorHelper::PlaceObject(glm::vec3& snappedPosition)
{

}

void EditorHelper::DeleteObject(bool shouldDeleteThis, Entity* entityToDelete)
{
	if (shouldDeleteThis)
	{
		editor->game->ShouldDeleteEntity(entityToDelete);
		return;
	}
}

void EditorHelper::PlaceTile(const Tile& tile)
{

}

void EditorHelper::ToggleObjectMode(const std::string& mode)
{
	editor->SetLayer(DrawingLayer::OBJECT);
}