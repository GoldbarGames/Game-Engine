#include "SpellPush.h"
#include "Game.h"

SpellPush::SpellPush(std::string n) : Spell(n)
{
	//TODO: Actually implement unlocking of spells
	// and only casting the ones that are unlocked
	isUnlocked = true;
}

SpellPush::~SpellPush()
{

}

void SpellPush::Cast(Game& game)
{
	// 1. Set the player's animation to the PUSH spell casting animation
	game.player->GetAnimator()->SetState("PUSH");
	
	// 2. Prevent the player from pressing any other buttons during this time
	game.player->GetAnimator()->SetBool("isCastingSpell", true);

	// 3. Create a rectangle collider in front of the player (direction facing)
	SDL_Rect* spellRange = new SDL_Rect;
	spellRange->x = game.player->GetPosition().x + 20;
	spellRange->y = game.player->GetPosition().y;
	spellRange->w = 40;
	spellRange->h = 52;

	std::cout << "Rect for push spell:" << std::endl;
	std::cout << "(" << spellRange->x << "," << spellRange->y << "," <<
		spellRange->w << "," << spellRange->h << ")" << std::endl;

	game.debugRectangles.push_back(spellRange);

	const float PUSH_SPEED = 0.5f;

	Vector2 pushVelocity = Vector2(PUSH_SPEED, 0.0f);
	if (game.player->flip == SDL_FLIP_HORIZONTAL)
		pushVelocity = Vector2(-1 * PUSH_SPEED, 0.0f);

	// 4. If the collider intersects with anything that can be pushed,
	for (unsigned int i = 0; i < game.entities.size(); i++)
	{
		const SDL_Rect * theirBounds = game.entities[i]->GetBounds();
		if (SDL_HasIntersection(spellRange, theirBounds))
		{
			//TODO: Is there a better way to do this than to check the type?
			PhysicsEntity* entity = dynamic_cast<PhysicsEntity*>(game.entities[i]);
			if (entity != nullptr)
			{
				// 5. Then make that object move until it hits a wall
				if (entity->canBePushed)
					entity->Push(pushVelocity);
			}
		}
	}
}