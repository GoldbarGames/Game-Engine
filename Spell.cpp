#include "Spell.h"
#include "Game.h"
#include "PhysicsComponent.h"

Spell::Spell(std::string n) : name(n) {}

Spell::Spell() { }

Spell::~Spell()
{

}

bool Spell::Cast(Game& game)
{
	bool success = false;

	switch (activeSpell)
	{
	case 0:
		success = CastPush(game);
		break;
	default:
		break;
	}

	return success;
}

bool Spell::CastPush(Game& game)
{
	game.player->UpdateSpellAnimation("push");

	// Create a rectangle collider in front of the player (direction facing)
	SDL_Rect* spellRange = new SDL_Rect;

	spellRange->x = (int)game.player->GetCenter().x;
	spellRange->y = (int)game.player->GetCenter().y;
	spellRange->w = 40;
	spellRange->h = 52;

	// Begin to create a rectangle where the rectangle's center is at the player's center

	Sprite* sprite = game.player->GetSprite();
	Vector2 playerPivot = game.player->GetSprite()->pivot;

	// Get center of the yellow collision box, and use it as a vector2
	float yellowBoxCenterX = (spellRange->x + (spellRange->w / 2));
	float yellowBoxCenterY = (spellRange->y + (spellRange->h / 2));
	Vector2 collisionCenter = Vector2(yellowBoxCenterX, yellowBoxCenterY);

	// scale the pivot and subtract it from the collision center
	Vector2 scaledPivot = Vector2(playerPivot.x, playerPivot.y);
	Vector2 yellowRectanglePosition = collisionCenter - scaledPivot;

	// Set the final rectangle to be equal to this offset
	spellRange->x = (int)yellowRectanglePosition.x;
	spellRange->y = (int)yellowRectanglePosition.y;

	int DISTANCE_FROM_CENTER_X = 21;
	int DISTANCE_FROM_CENTER_Y = -26;

	// Add distance to the center so that it covers the entire cloud of wind

	if (game.player->scale.x < 0)
	{
		spellRange->w *= -1;
		spellRange->x -= DISTANCE_FROM_CENTER_X;
	}
	else
	{
		spellRange->x += DISTANCE_FROM_CENTER_X;
	}

	spellRange->y += DISTANCE_FROM_CENTER_Y;

	// This converts the rectangle into a positive one for the intersection code
	if (spellRange->w < 0)
	{
		spellRange->w *= -1;
		spellRange->x -= spellRange->w;
	}

	//std::cout << "Rect for push spell:" << std::endl;
	//std::cout << "(" << spellRange->x << "," << spellRange->y << "," <<
	//	spellRange->w << "," << spellRange->h << ")" << std::endl;
	//game.debugRectangles.push_back(spellRange);

	const float PUSH_SPEED = 0.5f;

	Vector2 pushVelocity = Vector2(PUSH_SPEED, 0.0f);
	if (game.player->scale.x < 0)
		pushVelocity = Vector2(-1 * PUSH_SPEED, 0.0f);

	// 4. If the collider intersects with anything that can be pushed,
	for (unsigned int i = 0; i < game.entities.size(); i++)
	{
		const SDL_Rect* theirBounds = game.entities[i]->GetBounds();

		if (HasIntersection(*spellRange, *theirBounds))
		{
			//TODO: Is there a better way to do this than to check the type?
			PhysicsComponent* entity = dynamic_cast<PhysicsComponent*>(game.entities[i]);
			if (entity != nullptr)
			{
				// 5. Then make that object move until it hits a wall
				if (entity->canBePushed)
					entity->Push(pushVelocity);
			}
		}
	}

	return true;
}