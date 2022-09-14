#include "QuadTree.h"
#include "Renderer.h"
#include "Sprite.h"
#include "Game.h"

QuadTree::QuadTree() : QuadTree(0, 0, 0, 0)
{
   
}

QuadTree::QuadTree(int x, int y, int w, int h, int d)
{
    children[0] = nullptr;
    children[1] = nullptr;
    children[2] = nullptr;
    children[3] = nullptr;

    depth = d;

    SetCoords(x, y, w, h, 500);
}

void QuadTree::SetCoords(int x, int y, int w, int h, int s)
{
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

    smallestSize = abs(topLeft.x - botRight.x) <= s &&
        abs(topLeft.y - botRight.y) <= s;
}

QuadTree::~QuadTree()
{
    for (int i = 0; i < 4; i++)
    {
        if (children[i] != nullptr)
            delete children[i];
    }
}

void QuadTree::RenderEntities(const Renderer& renderer, const std::vector<Entity*>& e)
{
   return;

    if (debugSprite == nullptr)
        debugSprite = new Sprite(renderer.shaders[4]);

    for (int i = 0; i < e.size(); i++)
    {
        int colorIndex = i % 6;
        Uint8 c = 255;

        switch (colorIndex)
        {
        case 0:
            debugSprite->color = { c, c, c, 255 };
            break;
        case 1:
            debugSprite->color = { c, 0, 0, 255 };
            break;
        case 2:
            debugSprite->color = { c, 128, 0, 255 };
            break;
        case 3:
            debugSprite->color = { c, c, 0, 255 };
            break;
        case 4:
            debugSprite->color = { 0, c, 0, 255 };
            break;
        case 5:
            debugSprite->color = { 0, c, c, 255 };
            break;
        default:
            debugSprite->color = { c, c, c, 255 };
            break;
        }

        float rWidth = 1;
        float rHeight = 1;

        float targetWidth = e[i]->GetSprite()->frameWidth;
        float targetHeight = e[i]->GetSprite()->frameHeight;

        debugSprite->pivot = glm::vec2(0, 0);
        renderer.debugScale = glm::vec2(targetWidth / rWidth, targetHeight / rHeight);
        debugSprite->Render(e[i]->position, renderer, renderer.debugScale);
    }

    /*
    for (int i = 0; i < 4; i++)
    {
        if (children[i] != nullptr)
        {
            children[i]->RenderEntities(renderer, e);
        }
    }
    */
}

void QuadTree::Render(const Renderer& renderer)
{
   return;

    if (renderer.game->debugMode)
    {
        if (debugSprite == nullptr)
            debugSprite = new Sprite(renderer.shaders[4]);

        int colorIndex = depth % 6;
        Uint8 c = (int)renderAlpha;

        switch (colorIndex)
        {
        case 0:
            debugSprite->color = { c, c, c, 255 };
            break;
        case 1:
            debugSprite->color = { c, 0, 0, 255 };
            break;
        case 2:
            debugSprite->color = { c, 128, 0, 255 };
            break;
        case 3:
            debugSprite->color = { c, c, 0, 255 };
            break;
        case 4:
            debugSprite->color = { 0, c, 0, 255 };
            break;
        case 5:
            debugSprite->color = { 0, c, c, 255 };
            break;
        default:
            debugSprite->color = { c, c, c, 255 };
            break;
        }

        debugSprite->pivot = glm::vec2(0, 0);
        renderer.debugScale = glm::vec2(rect.w, rect.h);
        debugSprite->Render(glm::vec3(rect.x, rect.y, 0), renderer, renderer.debugScale);

        for (int i = 0; i < 4; i++)
        {
            if (children[i] != nullptr)
            {
                children[i]->renderAlpha = renderAlpha - (i*20);
                children[i]->Render(renderer);
            }
        }


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
            children[i]->active = false;
            //delete children[i];
            //children[i] = nullptr;
        }            
    }
}

//TODO: Improve performance
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

    entities.emplace_back(newEntity);
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

            children[0] = new QuadTree(x + subWidth, y, subWidth, subHeight, depth+1);
            children[1] = new QuadTree(x, y, subWidth, subHeight, depth + 1);
            children[2] = new QuadTree(x, y + subHeight, subWidth, subHeight, depth + 1);
            children[3] = new QuadTree(x + subWidth, y + subHeight, subWidth, subHeight, depth + 1);
        }
        else if (!children[0]->active)
        {
            children[0]->active = true;
            children[1]->active = true;
            children[2]->active = true;
            children[3]->active = true;
        }

        int i = 0;
        while (i < entities.size())
        {
            QuadTree* child = GetInsertedChild(newEntity->GetBounds());
            if (child != nullptr)
            {
                child->Insert(entities[i]);         
                std::vector<Entity*>::iterator it = entities.begin() + i;
                entities.erase(it);
            }
            else
            {
                i++;
            }
            i++;
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
void QuadTree::Retrieve(const SDL_Rect* bounds, std::vector<Entity*>& out, QuadTree* root)
{
    QuadTree* child = GetInsertedChild(bounds);
    if (child != nullptr)
    {
        child->Retrieve(bounds, out, root);
    }

    // If no children, we have reached the smallest quadrant containing the entity
    // so we want to check adjacent quadrants for collisions too
    if (children[0] == nullptr && root != nullptr)
    {
        SDL_Rect newRect;
        newRect.x = bounds->x;
        newRect.y = bounds->y;
        newRect.w = bounds->w;
        newRect.h = bounds->h;

        // Recursively retrieve entities from adjacent quadrants

        SDL_Rect leftBounds = newRect;
        leftBounds.x -= rect.w / 2;

        SDL_Rect rightBounds = newRect;
        rightBounds.x += rect.w / 2;

        SDL_Rect topBounds = newRect;
        topBounds.y -= rect.h / 2;

        SDL_Rect bottomBounds = newRect;
        bottomBounds.y += rect.h / 2;

        QuadTree* tree = root;

        tree = tree->GetInsertedChild(&leftBounds);
        if (tree != nullptr)
        {
            tree->Retrieve(&leftBounds, out, nullptr);
        }

        tree = root;
        tree = tree->GetInsertedChild(&rightBounds);
        if (tree != nullptr)
        {
            tree->Retrieve(&rightBounds, out, nullptr);
        }

        tree = root;
        tree = tree->GetInsertedChild(&topBounds);
        if (tree != nullptr)
        {
            tree->Retrieve(&topBounds, out, nullptr);
        }

        tree = root;
        tree = tree->GetInsertedChild(&bottomBounds);
        if (tree != nullptr)
        {
            tree->Retrieve(&bottomBounds, out, nullptr);
        }
    }
    
    if (entities.size() > 0)
    {
        //TODO: Optimize this
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