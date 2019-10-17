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

	const int OFFSET = 0;

	if (game.player->flip == SDL_FLIP_HORIZONTAL)
		spellRange->x = game.player->GetCenter().x - (OFFSET);
	else
		spellRange->x = game.player->GetCenter().x + (OFFSET);

	spellRange->y = game.player->GetCenter().y;
	spellRange->w = 40 * Renderer::GetScale();
	spellRange->h = 52 * Renderer::GetScale();





	Vector2 entityPivot = game.player->GetSprite()->pivot;

	// Get center of the yellow collision box, and use it as a vector2
	float collisionCenterX = (spellRange->x + (spellRange->w / 2));
	float collisionCenterY = (spellRange->y + (spellRange->h / 2));
	Vector2 collisionCenter = Vector2(collisionCenterX + 0, collisionCenterY + 0);

	if (game.player->flip == SDL_FLIP_HORIZONTAL)
	{
		entityPivot.x = (game.player->GetSprite()->windowRect.w / Renderer::GetScale())
			- game.player->GetSprite()->pivot.x;
	}
		

	// scale the pivot and subtract it from the collision center
	Vector2 scaledPivot = Vector2(entityPivot.x * Renderer::GetScale(), 
		game.player->GetSprite()->pivot.y * Renderer::GetScale());


	Vector2 collision_offset = collisionCenter - scaledPivot;

	spellRange->x = collision_offset.x;
	spellRange->y = collision_offset.y;

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

		if (game.entities[i]->etype == "block")
		{
			int test = 0;
		}

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