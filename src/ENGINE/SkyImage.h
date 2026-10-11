#ifndef KINJO_SKY_IMAGE_H
#define KINJO_SKY_IMAGE_H
#pragma once

// Engine-internal (not exported). A sky panorama a game paints itself, in linear
// HDR (Scene3D::SetSkyImage; SkyImage.cpp, shaders/sky_hdr.frag). While one is
// set it is the sky: the skybox draws it in place of the scene's panorama file
// (and of the sun and moon drawn over that, SkyBodies.h), and image-based
// lighting is captured from it, linear, with no sRGB decode.

class Renderer;

// The game's sky texture and a number that changes with each new image, or
// false while there is none
bool HdrSkyImage(unsigned int& texture, int& version);
// Skybox::Render: draws it on the sky sphere (radius r). False while there is none.
bool DrawHdrSky(const Renderer& renderer, float radius);
void ReleaseHdrSky();

#endif
