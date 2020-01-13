#include "PathNode.h"
#include "Renderer.h"

PathNode::PathNode(Vector2 pos)
{
	point = pos;

	rect.x = (int)pos.x;
	rect.y = (int)pos.y;
	rect.w = (TILE_SIZE / 2);
	rect.h = (TILE_SIZE / 2);

	renderRect.x = (int)pos.x;
	renderRect.y = (int)pos.y;
	renderRect.w = (TILE_SIZE / 2);
	renderRect.h = (TILE_SIZE / 2);
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
	renderRect.x = (int)(rect.x - cameraOffset.x);
	renderRect.y = (int)(rect.y - cameraOffset.y);
	renderRect.w = (int)(TILE_SIZE / 2);
	renderRect.h = (int)(TILE_SIZE / 2);
	return &renderRect;
}