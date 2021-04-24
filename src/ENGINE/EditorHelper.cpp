#include "EditorHelper.h"
#include "Editor.h"
#include "Entity.h"
#include "Game.h"

EditorHelper::EditorHelper(Game* game)
{
	game->editor->helper = this;
	editor = game->editor;
}

EditorHelper::~EditorHelper()
{

}

void EditorHelper::CustomSave(std::ostringstream& level)
{

}

bool EditorHelper::CustomLoad(const std::string& etype, const std::string tokens[32])
{
	return false;
}

bool EditorHelper::ClickedCustomButton(const std::string& clickedName)
{
	return false;
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

void EditorHelper::PlaceTile(Tile& tile)
{

}

void EditorHelper::ToggleObjectMode(const std::string& mode)
{
	editor->SetLayer(DrawingLayer::OBJECT);
}

void EditorHelper::GrabEntity(int mouseX, int mouseY)
{
	glm::vec3 worldPosition = editor->game->ConvertFromScreenSpaceToWorldSpace(glm::vec2(mouseX, mouseY));
	Entity* grabbedEntity = editor->GetEntityAtWorldPosition(worldPosition);

	if (grabbedEntity != nullptr)
	{
		editor->grabbedEntities.emplace_back(grabbedEntity);
		editor->oldGrabbedPositions.emplace_back(grabbedEntity->GetPosition());
		editor->GRID_SIZE = grabbedEntity->GetGridSize();
	}
}