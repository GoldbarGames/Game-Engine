#version 330 core

// Lightning flash, drawn full-screen with additive blending so it brightens the
// scene beneath it. The light comes from the sky, so it's strongest at the top
// of the frame and eases off toward the ground - this keeps the lower scene
// readable instead of washing the whole screen to a flat white.
#include "draw.glsl"
// Linear-aware: the flash is light added to the scene, so in a linear target
// its (authored, sRGB) colour is decoded and scaled linearly.
#include "target.glsl"

struct DrawData
{
    vec3  uFlashColor;
    float uFlashIntensity;
};
PER_DRAW(DrawData);

in vec2 vNdc;
out vec4 FragColor;

void main()
{
    float topward = vNdc.y * 0.5 + 0.5;         // 0 bottom .. 1 top
    float grad = mix(0.45, 1.0, topward);       // brighter up high
    // Alpha ZERO, not one.
    //
    // The blend is additive (GL_ONE, GL_ONE), so the alpha written here is
    // ADDED to whatever is already in the buffer - and a 1.0 makes the whole
    // screen opaque. That matters because the engine renders the scene into a
    // framebuffer cleared to (0,0,0,0) and then composites it over the default
    // framebuffer, which is cleared to the sky colour. The sky is what shows
    // THROUGH the transparent parts of the scene buffer.
    //
    // So a flash that writes alpha 1 makes the buffer opaque everywhere and
    // hides the sky behind the buffer's own black - the scene, the characters
    // and the weather still draw correctly over it, and the sky goes black.
    // Additive blending ignores the source alpha for colour anyway, so writing
    // zero costs nothing and leaves the transparency alone.
    // Alpha carries the flash's OWN strength - it is not decoration, and it is
    // not 1.0.
    //
    // The engine renders the scene into a framebuffer cleared to (0,0,0,0) and
    // composites it over a background cleared to the sky colour, so the sky is
    // what shows THROUGH the transparent parts of the scene buffer, and the
    // buffer's colour is weighted by its own alpha when it composites.
    //
    // Two ways to get this wrong, and both were got wrong in turn:
    //   alpha 1.0 - the whole screen goes opaque, the sky is hidden behind the
    //               buffer's own cleared black, and everything already drawn
    //               still looks right. That is the black-sky bug.
    //   alpha 0.0 - fixes that, and throws the flash away: the sky stays
    //               transparent, so the colour added here is multiplied by
    //               almost nothing and never appears.
    //
    // The answer is premultiplied: emit alpha equal to the flash's strength, so
    // at rest it adds nothing at all and leaves the sky alone, and during a
    // strike it becomes as opaque as it is bright.
    float a = draw.uFlashIntensity * grad;
    FragColor = vec4(TargetColor(draw.uFlashColor) * a, a);
}
