#ifndef QUADTREE_H
#define QUADTREE_H
#pragma once

#include "Entity.h"
#include <glm/vec3.hpp>
#include <SDL2/SDL.h>
#include "leak_check.h"

class Renderer;

class KINJO_API QuadTree
{
private:
    QuadTree* children[4];

    glm::vec3 midpoint = glm::vec3(0,0,0);
    bool smallestSize = false;

    Sprite* debugSprite = nullptr;

public:

    // More than one entity can be in the same position
    // and therefore also within the same subtree
    std::vector<Entity*> entities;

    size_t MAX_ENTITIES = 100;
    int id = 0;

    // Hold details of the boundary of this node 
    SDL_Rect rect;
    glm::vec3 topLeft = glm::vec3(0,0,0);
    glm::vec3 botRight = glm::vec3(0,0,0);

    int depth = 1;
    Uint8 renderAlpha = 255;
    bool active = true;

    bool output = false;

    QuadTree();
    QuadTree(int i, int x, int y, int w, int h, int d=1);
    ~QuadTree();

    void SetCoords(int x, int y, int w, int h, int s);

    void Render(const Renderer& renderer);
    void RenderEntities(const Renderer& renderer, const std::vector<Entity*>& e);

    void Reset();
    void Insert(Entity* newEntity);
    QuadTree* SearchTree(Entity* e);
    void Retrieve(const SDL_Rect* bounds, std::vector<Entity*>& out, QuadTree* root);
    bool Contains(glm::vec3 point);
    std::vector<Entity*> GetEntities();

    QuadTree* GetChildContainingBounds(const SDL_Rect* bounds);
};

#endif