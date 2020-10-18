#ifndef SDL_HELPERS_H
#define SDL_HELPERS_H
#pragma once

#include <stdio.h>
#include <string>
#include <SDL.h>

// TODO: Can we replace this code with something better?
// NOTE: The below code is from https://halfgeek.org/wiki/Vertically_invert_a_surface_in_SDL
// Since this is only for taking screenshots, we just need something that works for right now.

#define SDL_LOCKIFMUST(s) (SDL_MUSTLOCK(s) ? SDL_LockSurface(s) : 0)
#define SDL_UNLOCKIFMUST(s) { if(SDL_MUSTLOCK(s)) SDL_UnlockSurface(s); }

int invertSDLSurfaceVertically(SDL_Surface* surface)
{
	Uint8* t;
	register Uint8* a, * b;
	Uint8* last;
	register Uint16 pitch;

	if (SDL_LOCKIFMUST(surface) < 0)
		return -2;

	/* do nothing unless at least two lines */
	if (surface->h < 2) {
		SDL_UNLOCKIFMUST(surface);
		return 0;
	}

	/* get a place to store a line */
	pitch = surface->pitch;
	t = (Uint8*)malloc(pitch);

	if (t == NULL) {
		SDL_UNLOCKIFMUST(surface);
		return -2;
	}

	/* get first line; it's about to be trampled */
	memcpy(t, surface->pixels, pitch);

	/* now, shuffle the rest so it's almost correct */
	a = (Uint8*)surface->pixels;
	last = a + pitch * (surface->h - 1);
	b = last;

	while (a < b) {
		memcpy(a, b, pitch);
		a += pitch;
		memcpy(b, a, pitch);
		b -= pitch;
	}

	/* in this shuffled state, the bottom slice is too far down */
	memmove(b, b + pitch, last - b);

	/* now we can put back that first row--in the last place */
	memcpy(last, t, pitch);

	/* everything is in the right place; close up. */
	free(t);
	SDL_UNLOCKIFMUST(surface);

	return 0;
}

#endif