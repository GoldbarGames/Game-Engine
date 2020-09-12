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
		renderer.RenderDebugRect(spellRangeRect, Vector2(1, 1));
	}
}

void Spell::Update(Game& game)
{
	if (isCasting)
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
}


bool Spell::Cast(Game& game)
{
	bool success = false;

	affectedEntities.clear();

	// Stop moving horizontally at the start of casting a spell
	game.player->physics->velocity.x = 0;

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
	spellRangeRect.x = (int)game.player->position.x;
	spellRangeRect.y = (int)game.player->position.y;
	spellRangeRect.w = 64;
	spellRangeRect.h = 52;

	int DISTANCE_FROM_CENTER_X = 90;
	int DISTANCE_FROM_CENTER_Y = 0;

	// Add distance to the center so that it covers the entire cloud of wind
	if (game.player->scale.x < 0)
	{
		//spellRangeRect.w *= -1;
		spellRangeRect.x -= DISTANCE_FROM_CENTER_X;
	}
	else
	{
		spellRangeRect.x += DISTANCE_FROM_CENTER_X;
	}

	spellRangeRect.y += DISTANCE_FROM_CENTER_Y;

	//std::cout << "Rect for push spell:" << std::endl;
	//std::cout << "(" << spellRange->x << "," << spellRange->y << "," <<
	//	spellRange->w << "," << spellRange->h << ")" << std::endl;
	//game.debugRectangles.push_back(spellRange);

	const float PUSH_SPEED = 0.5f;

	Vector2 pushVelocity = Vector2(PUSH_SPEED, 0.0f);
	if (game.player->scale.x < 0)
		pushVelocity = Vector2(-1 * PUSH_SPEED, 0.0f);

	SDL_Rect newRectOurs, newRectTheirs;
	newRectOurs = ConvertCoordsFromCenterToTopLeft(spellRangeRect);

	// 4. If the collider intersects with anything that can be pushed,
	for (unsigned int i = 0; i < game.entities.size(); i++)
	{

		bool alreadyAffectedBySpell = false;
		for (int k = 0; k < affectedEntities.size(); k++)
		{
			if (game.entities[i] == affectedEntities[k])
			{
				alreadyAffectedBySpell = true;
				break;
			}
		}

		if (alreadyAffectedBySpell)
		{
			continue;
		}

		const SDL_Rect* theirBounds = game.entities[i]->GetBounds();
		newRectTheirs = ConvertCoordsFromCenterToTopLeft(*theirBounds);

		if (HasIntersection(newRectOurs, newRectTheirs))
		{
			//TODO: Is there a better way to do this than to check the type?
			Entity* entity = game.entities[i];
			if (entity->physics != nullptr)
			{
				// 5. Then make that object move until it hits a wall
				if (entity->physics->canBePushed)
				{
					entity->physics->Push(pushVelocity);
					affectedEntities.push_back(entity);
				}				
			}
		}
	}

	return true;
}