#include "QuadTree.h"

QuadTree::QuadTree() : QuadTree(0, 0, 0, 0)
{
   
}

QuadTree::QuadTree(int x, int y, int w, int h)
{
    children[0] = nullptr;
    children[1] = nullptr;
    children[2] = nullptr;
    children[3] = nullptr;

    rect.x = x;
    rect.y = y;
    rect.w = w;
    rect.h = h;

    topLeft.x = rect.x;
    topLeft.y = rect.y;
    botRight.x = rect.x + rect.w;
    botRight.y = rect.y + rect.h;

    midpoint.x = (topLeft.x + botRight.x) / 2;
    midpoint.y = (topLeft.y + botRight.y) / 2;

    smallestSize = abs(topLeft.x - botRight.x) <= 1000 &&
        abs(topLeft.y - botRight.y) <= 1000;
}

QuadTree::~QuadTree()
{
    for (int i = 0; i < 4; i++)
    {
        if (children[i] != nullptr)
            delete children[i];
    }
}

void QuadTree::Update()
{
    std::vector<Entity*> newList;
    std::vector<Entity*> insertList;

    for (int i = 0; i < entities.size(); i++)
    {
        if (entities[i] != nullptr)
        {
            if (entities[i]->position.x == entities[i]->lastPosition.x
                && entities[i]->position.y == entities[i]->lastPosition.y)
            {
                newList.push_back(entities[i]);
            }
            else
            {
                insertList.push_back(entities[i]);
            }
        }
    }

    entities = std::vector<Entity*>(newList);

    for (int i = 0; i < 4; i++)
    {
        if (children[i] != nullptr)
            children[i]->Update();
    }

    for (int i = 0; i < insertList.size(); i++)
    {
        Insert(insertList[i]);
    }
}

std::vector<Entity*> QuadTree::GetEntities()
{
    return entities;
}

// Remove all entities from each tree
void QuadTree::Reset()
{
    entities.clear();

    for (int i = 0; i < 4; i++)
    {
        if (children[i] != nullptr)
        {
            children[i]->Reset();
            children[i] = nullptr;
        }            
    }
}

void QuadTree::Insert(Entity* newEntity)
{
    if (newEntity == nullptr)
        return;

    // If there are no child nodes, or the object cannot fit in them,
    // then the object is added to the parent node.
    if (children[0] != nullptr)
    {
        QuadTree* child = GetInsertedChild(newEntity->GetBounds());
        if (child != nullptr)
        {
            child->Insert(newEntity);
            return;
        }        
    }

    entities.push_back(newEntity);
    newEntity->quadrant = this;

    int MAX_ENTITIES = 4;
    if (entities.size() > MAX_ENTITIES && !smallestSize)
    {
        // Subdivide
        if (children[0] == nullptr)
        {
            int subWidth = (rect.w / 2);
            int subHeight = (rect.h / 2);
            int x = rect.x;
            int y = rect.y;

            children[0] = new QuadTree(x + subWidth, y, subWidth, subHeight);
            children[1] = new QuadTree(x, y, subWidth, subHeight);
            children[2] = new QuadTree(x, y + subHeight, subWidth, subHeight);
            children[3] = new QuadTree(x + subWidth, y + subHeight, subWidth, subHeight);
        }

        int i = 0;
        while (i < entities.size())
        {
            QuadTree* child = GetInsertedChild(newEntity->GetBounds());
            if (child != nullptr)
            {
                child->Insert(entities[i]);
                entities.erase(entities.begin() + i);
            }
            else
            {
                i++;
            }
        }
    }

}

QuadTree* QuadTree::GetInsertedChild(const SDL_Rect* bounds)
{
    QuadTree* child = nullptr;

    if (children[0] != nullptr)
    {
        for (int i = 0; i < 4; i++)
        {
            if (HasIntersection(children[i]->rect, *bounds))
            {
                child = children[i];
                break;
            }
        }
    }   

    return child;
}

// Return the tree that the given entity is inside
QuadTree* QuadTree::SearchTree(Entity* e)
{
    QuadTree* result = nullptr;

    for (int i = 0; i < entities.size(); i++)
    {
        if (entities[i] == e)
            return this;
    }

    for (int i = 0; i < 4; i++)
    {
        if (result != nullptr && children[i] != nullptr)
            result = children[i]->SearchTree(e);
    }

    return result;
}

// Returns all entities contained within a point by adding them to the (initially empty) vector
void QuadTree::Retrieve(const SDL_Rect* bounds, std::vector<Entity*>& out)
{
    QuadTree* child = GetInsertedChild(bounds);
    if (child != nullptr && children[0] != nullptr)
    {
        child->Retrieve(bounds, out);
    }
    
    if (entities.size() > 0)
    {
        out.insert(std::end(out), std::begin(entities), std::end(entities));
    }

    return;
}

bool QuadTree::Contains(Vector2 point)
{
    return (point.x >= topLeft.x &&
        point.x <= botRight.x &&
        point.y >= topLeft.y &&
        point.y <= botRight.y);
}