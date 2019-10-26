#include "PathNode.h"
#include "Renderer.h"

PathNode::PathNode(Vector2 pos)
{
	point = pos;

	rect.x = pos.x;
	rect.y = pos.y;
	rect.w = (TILE_SIZE / 2) * Renderer::GetScale();
	rect.h = (TILE_SIZE / 2) * Renderer::GetScale();

	renderRect.x = pos.x;
	renderRect.y = pos.y;
	renderRect.w = (TILE_SIZE / 2) * Renderer::GetScale();
	renderRect.h = (TILE_SIZE / 2) * Renderer::GetScale();
}

PathNode::~PathNode()
{

}

const SDL_Rect* PathNode::GetRect()
{
	return &rect;
}

const SDL_Rect* PathNode::GetRenderRect()
{
	return &renderRect;
}

const SDL_Rect* PathNode::CalcRenderRect(Vector2 cameraOffset)
{
	renderRect.x = rect.x - cameraOffset.x;
	renderRect.y = rect.y - cameraOffset.y;
	renderRect.w = (TILE_SIZE / 2) * Renderer::GetScale();
	renderRect.h = (TILE_SIZE / 2) * Renderer::GetScale();
	return &renderRect;
}