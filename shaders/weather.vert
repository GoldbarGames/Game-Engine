#version 330 core

// Weather particle billboard. One instanced quad per particle. Snow = a small
// camera-facing flake with a sideways sway; rain = a streak stretched along the
// fall direction and rotated to face the camera about that axis.

layout (location = 0) in vec2 corner;      // quad corner in [-0.5, 0.5]
layout (location = 1) in vec2 uv;          // 0..1
layout (location = 3) in vec4 iPosSeed;    // per-instance: xyz = world pos, w = seed
layout (location = 4) in vec3 iVel;        // per-instance velocity (uMode 2 = fountain streak)

#include "camera.glsl"

#include "weather_draw.glsl"   // uMode, uTime, uCamPos, uFallDir, uSize, uLength, uSway

out vec2  TexCoord;
out float vAlpha;

void main()
{
    vec3 ipos = iPosSeed.xyz;
    float seed = iPosSeed.w;

    // Camera basis in world space, read from the view matrix rows.
    vec3 camRight = vec3(view[0][0], view[1][0], view[2][0]);
    vec3 camUp    = vec3(view[0][1], view[1][1], view[2][1]);

    vec3 worldPos;
    if (draw.uMode == 1 || draw.uMode == 2)
    {
        // Streak: long axis along a direction, width axis facing the camera. Rain
        // uses the global fall direction; the fountain (mode 2) uses each droplet's
        // own velocity so rising drops streak up, falling drops down, arcs angled.
        vec3 vdir = (draw.uMode == 2) ? iVel : draw.uFallDir;
        vec3 dir   = (length(vdir) > 0.0001) ? normalize(vdir) : vec3(0.0, 1.0, 0.0);
        vec3 toCam = normalize(draw.uCamPos - ipos);
        vec3 right = normalize(cross(dir, toCam));
        worldPos = ipos + right * (corner.x * draw.uSize) + dir * (corner.y * draw.uLength);
    }
    else
    {
        // Snow: camera-facing quad, with a per-particle sideways sway.
        float sway = sin(draw.uTime * 1.6 + seed * 6.2831853) * draw.uSway;
        vec3 center = ipos + camRight * sway;
        worldPos = center + camRight * (corner.x * draw.uSize) + camUp * (corner.y * draw.uSize);
    }

    gl_Position = projection * view * vec4(worldPos, 1.0);
    TexCoord = uv;
    vAlpha = mix(0.65, 1.0, fract(seed * 7.13));
}
