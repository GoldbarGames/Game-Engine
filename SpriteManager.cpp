#include "SpriteManager.h"
#include "SDL.h"
#include <SDL_image.h>
#include <GL/glew.h>

SpriteManager::SpriteManager()
{
	//TODO: Find all PNG files in the assets folder automatically?
	//OR find a way to only load them when needed?

	LoadImage("assets/gui/icon.png");

	// Kaneko images
	LoadImage("assets/sprites/kaneko/wdk_blink.png");
	LoadImage("assets/sprites/kaneko/wdk_break.png");
	LoadImage("assets/sprites/kaneko/wdk_carry.png");
	LoadImage("assets/sprites/kaneko/wdk_climb_turn.png");
	LoadImage("assets/sprites/kaneko/wdk_debug.png");
	LoadImage("assets/sprites/kaneko/wdk_debug_air.png");
	LoadImage("assets/sprites/kaneko/wdk_debug_air_down.png");
	LoadImage("assets/sprites/kaneko/wdk_debug_air_up.png");
	LoadImage("assets/sprites/kaneko/wdk_debug_climb.png");
	LoadImage("assets/sprites/kaneko/wdk_debug_down.png");
	LoadImage("assets/sprites/kaneko/wdk_door_enter.png");
	LoadImage("assets/sprites/kaneko/wdk_double.png");
	LoadImage("assets/sprites/kaneko/wdk_fall.png");
	LoadImage("assets/sprites/kaneko/wdk_flash.png");
	LoadImage("assets/sprites/kaneko/wdk_float.png");
	LoadImage("assets/sprites/kaneko/wdk_freeze.png");
	LoadImage("assets/sprites/kaneko/wdk_hurt.png");
	LoadImage("assets/sprites/kaneko/wdk_idle.png");
	LoadImage("assets/sprites/kaneko/wdk_jump.png");
	LoadImage("assets/sprites/kaneko/wdk_jump2fall.png");
	LoadImage("assets/sprites/kaneko/wdk_ladder_climb.png");
	LoadImage("assets/sprites/kaneko/wdk_ladder_top.png");
	LoadImage("assets/sprites/kaneko/wdk_land.png");
	LoadImage("assets/sprites/kaneko/wdk_lookdown.png");
	LoadImage("assets/sprites/kaneko/wdk_lookup.png");
	LoadImage("assets/sprites/kaneko/wdk_pop.png");
	LoadImage("assets/sprites/kaneko/wdk_pop_detonate.png");
	LoadImage("assets/sprites/kaneko/wdk_pop_plant.png");
	LoadImage("assets/sprites/kaneko/wdk_protect_backward.png");
	LoadImage("assets/sprites/kaneko/wdk_protect_forward.png");
	LoadImage("assets/sprites/kaneko/wdk_push.png");
	LoadImage("assets/sprites/kaneko/wdk_reading_loop.png");
	LoadImage("assets/sprites/kaneko/wdk_reading_post.png");
	LoadImage("assets/sprites/kaneko/wdk_reading_pre.png");
	LoadImage("assets/sprites/kaneko/wdk_reading1.png");
	LoadImage("assets/sprites/kaneko/wdk_reading2.png");
	LoadImage("assets/sprites/kaneko/wdk_reading3.png");
	LoadImage("assets/sprites/kaneko/wdk_return_enter.png");
	LoadImage("assets/sprites/kaneko/wdk_return_exit.png");
	LoadImage("assets/sprites/kaneko/wdk_run.png");
	LoadImage("assets/sprites/kaneko/wdk_search.png");
	LoadImage("assets/sprites/kaneko/wdk_search_loop.png");
	LoadImage("assets/sprites/kaneko/wdk_search_start.png");
	LoadImage("assets/sprites/kaneko/wdk_search1.png");
	LoadImage("assets/sprites/kaneko/wdk_search2.png");
	LoadImage("assets/sprites/kaneko/wdk_seed_grow.png");
	LoadImage("assets/sprites/kaneko/wdk_seed_plant.png");
	LoadImage("assets/sprites/kaneko/wdk_short.png");
	LoadImage("assets/sprites/kaneko/wdk_short_grow.png");
	LoadImage("assets/sprites/kaneko/wdk_short_reverse.png");
	LoadImage("assets/sprites/kaneko/wdk_short_shrink.png");
	LoadImage("assets/sprites/kaneko/wdk_turbo_loop.png");
	LoadImage("assets/sprites/kaneko/wdk_turbo_start.png");
	LoadImage("assets/sprites/kaneko/wdk_turbo1.png");
	LoadImage("assets/sprites/kaneko/wdk_turbo2.png");
	LoadImage("assets/sprites/kaneko/wdk_walk.png");

	// Background images
	LoadImage("assets/bg/forest/forest_ground.png");
	LoadImage("assets/bg/forest/forest_sky1.png");
	LoadImage("assets/sprites/floor.png");

	LoadImage("assets/tiles/housetiles5.png");
	LoadImage("assets/tiles/foresttiles.png");
}

void SpriteManager::LoadImage(std::string const & image)
{
	images[image].reset(IMG_Load(image.c_str()));
}

SDL_Surface* SpriteManager::GetImage(std::string const& image)
{
	return images[image].get();
}

Vector2 SpriteManager::GetPivotPoint(std::string const& filename)
{
	return pivotPoints[filename];
}

SpriteManager::~SpriteManager()
{

}
