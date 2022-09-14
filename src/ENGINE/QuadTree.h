#ifndef QUADTREE_H
#define QUADTREE_H
#pragma once

#include "Vector2.h"
#include "Entity.h"
#include <SDL2/SDL.h>
#include "leak_check.h"

class Renderer;

class KINJO_API QuadTree
{
private:
    QuadTree* children[4];

    Vector2 midpoint = Vector2(0, 0);
    bool smallestSize = false;

    Sprite* debugSprite = nullptr;

public:

    // More than one entity can be in the same position
    // and therefore also within the same subtree
    std::vector<Entity*> entities;

    // Hold details of the boundary of this node 
    SDL_Rect rect;
    Vector2 topLeft = Vector2(0,0);
    Vector2 botRight = Vector2(0,0);

    int depth = 1;
    Uint8 renderAlpha = 255;
    bool active = true;

    QuadTree();
    QuadTree(int x, int y, int w, int h, int d=1);
    ~QuadTree();

    void SetCoords(int x, int y, int w, int h, int s);

    void Render(const Renderer& renderer);
    void RenderEntities(const Renderer& renderer, const std::vector<Entity*>& e);

    void Reset();
    void Insert(Entity* newEntity);
    QuadTree* SearchTree(Entity* e);
    void Retrieve(const SDL_Rect* bounds, std::vector<Entity*>& out, QuadTree* root);
    bool Contains(Vector2 point);
    std::vector<Entity*> GetEntities();

    QuadTree* GetInsertedChild(const SDL_Rect* bounds);
};

#endif