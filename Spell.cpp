#include "Spell.h"
#include "Game.h"
#include "PhysicsComponent.h"

Spell::Spell() 
{ 

}

Spell::~Spell()
{

}

void Spell::Render(const Renderer& renderer)
{
	if (isCasting)
	{
		if (spellRangeSprite == nullptr)
			spellRangeSprite = new Sprite(renderer.debugSprite->texture, renderer.debugSprite->shader);

		float rWidth = spellRangeSprite->texture->GetWidth();
		float rHeight = spellRangeSprite->texture->GetHeight();

		spellRangeSprite->SetScale(Vector2(spellRangeRect.w/rWidth, spellRangeRect.h/rHeight));
		spellRangeSprite->Render(Vector2(spellRangeRect.x, spellRangeRect.y), renderer);
	}
}

bool Spell::Cast(Game& game)
{
	bool success = false;

	game.player->UpdateSpellAnimation(names[activeSpell].c_str());
	game.player->GetSprite()->ResetFrame();

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
	// Create a rectangle collider in front of the player (direction facing)
	spellRangeRect.x = (int)game.player->GetCenter().x + game.player->position.x;
	spellRangeRect.y = (int)game.player->GetCenter().y + game.player->position.y;
	spellRangeRect.w = 40;
	spellRangeRect.h = 52;

	// Begin to create a rectangle where the rectangle's center is at the player's center

	Sprite* sprite = game.player->GetSprite();
	Vector2 playerPivot = game.player->GetSprite()->pivot;

	// Get center of the yellow collision box, and use it as a vector2
	float yellowBoxCenterX = (spellRangeRect.x + (spellRangeRect.w / 2));
	float yellowBoxCenterY = (spellRangeRect.y + (spellRangeRect.h / 2));
	Vector2 collisionCenter = Vector2(yellowBoxCenterX, yellowBoxCenterY);

	// scale the pivot and subtract it from the collision center
	Vector2 scaledPivot = Vector2(playerPivot.x, playerPivot.y);
	Vector2 yellowRectanglePosition = collisionCenter - scaledPivot;

	// Set the final rectangle to be equal to this offset
	spellRangeRect.x = (int)yellowRectanglePosition.x;
	spellRangeRect.y = (int)yellowRectanglePosition.y;

	int DISTANCE_FROM_CENTER_X = 21;
	int DISTANCE_FROM_CENTER_Y = -26;

	// Add distance to the center so that it covers the entire cloud of wind

	if (game.player->scale.x < 0)
	{
		spellRangeRect.w *= -1;
		spellRangeRect.x -= DISTANCE_FROM_CENTER_X;
	}
	else
	{
		spellRangeRect.x += DISTANCE_FROM_CENTER_X;
	}

	spellRangeRect.y += DISTANCE_FROM_CENTER_Y;

	// This converts the rectangle into a positive one for the intersection code
	if (spellRangeRect.w < 0)
	{
		spellRangeRect.w *= -1;
		spellRangeRect.x -= spellRangeRect.w;
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

		if (game.entities[i]->etype == "block")
			int test = 0;

		if (HasIntersection(spellRangeRect, *theirBounds))
		{
			//TODO: Is there a better way to do this than to check the type?
			Entity* entity = game.entities[i];
			if (entity->physics != nullptr)
			{
				// 5. Then make that object move until it hits a wall
				if (entity->physics->canBePushed)
					entity->physics->Push(pushVelocity);
			}
		}
	}

	return true;
}