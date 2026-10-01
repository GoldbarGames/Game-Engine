#version 330 core

// Weather particle shading: the sprite texture supplies a soft alpha falloff
// (white RGBA), tinted by uColor. Colour comes from the uniform, not the texture.

in vec2  TexCoord;
in float vAlpha;

uniform sampler2D theTexture;
#include "weather_draw.glsl"   // draw.uColor

out vec4 color;

void main()
{
    vec4 t = texture(theTexture, TexCoord);
    color = vec4(draw.uColor.rgb, draw.uColor.a * t.a * vAlpha);
    if (color.a < 0.01)
        discard;
}
