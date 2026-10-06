#version 330 core

// Weather particle shading: the sprite texture supplies a soft alpha falloff
// (white RGBA), tinted by uColor. Colour comes from the uniform, not the texture.

in vec2  TexCoord;
in float vAlpha;
in vec3  vWorldPos;

uniform sampler2D theTexture;
#include "weather_draw.glsl"   // draw.uColor, draw.uCamPos, draw.uDistFog

out vec4 color;

void main()
{
    vec4 t = texture(theTexture, TexCoord);
    color = vec4(draw.uColor.rgb, draw.uColor.a * t.a * vAlpha);

    // Distance fog (render/DistanceFog.h): the particle fades out instead of
    // taking the fog colour. Over a background the fog has already reached,
    // that comes to the same thing, and it keeps this shader in gamma space.
    if (draw.uDistFog.z > 0.0)
    {
        float d = distance(vWorldPos, draw.uCamPos);
        float fog = clamp((d - draw.uDistFog.x) / max(draw.uDistFog.y - draw.uDistFog.x, 0.001), 0.0, 1.0) * draw.uDistFog.z;
        color.a *= 1.0 - fog;
    }

    if (color.a < 0.01)
        discard;
}
