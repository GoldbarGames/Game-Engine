#include "Spell.h"
#include "Game.h"
#include "PhysicsInfo.h"

Spell::Spell(std::string n) : name(n) {}

Spell::Spell() { }

Spell::~Spell()
{

}

void Spell::Cast(Game& game)
{
	switch (activeSpell)
	{
	case 0:
		CastPush(game);
		break;
	default:
		break;
	}
}

void Spell::CastPush(Game& game)
{
	// 1. Set the player's animation to the PUSH spell casting animation
	game.player->GetAnimator()->SetState("PUSH");

	// 2. Prevent the player from pressing any other buttons during this time
	game.player->GetAnimator()->SetBool("isCastingSpell", true);

	// 3. Actually set the player's sprite to the PUSH casting sprite
	game.player->UpdateAnimator();

	// 3. Create a rectangle collider in front of the player (direction facing)
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

	if (game.player->flip == SDL_FLIP_HORIZONTAL)
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
	if (game.player->flip == SDL_FLIP_HORIZONTAL)
		pushVelocity = Vector2(-1 * PUSH_SPEED, 0.0f);

	// 4. If the collider intersects with anything that can be pushed,
	for (unsigned int i = 0; i < game.entities.size(); i++)
	{
		const SDL_Rect* theirBounds = game.entities[i]->GetBounds();

		// IMPORTANT: SDL_HasIntersection only works on positive rectangles!
		if (SDL_HasIntersection(spellRange, theirBounds))
		{
			//TODO: Is there a better way to do this than to check the type?
			PhysicsInfo* entity = dynamic_cast<PhysicsInfo*>(game.entities[i]);
			if (entity != nullptr)
			{
				// 5. Then make that object move until it hits a wall
				if (entity->canBePushed)
					entity->Push(pushVelocity);
			}
		}
	}
}