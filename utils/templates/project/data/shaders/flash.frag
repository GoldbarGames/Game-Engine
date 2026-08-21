#version 330 core

// Lightning flash, drawn full-screen with additive blending so it brightens the
// scene beneath it. The light comes from the sky, so it's strongest at the top
// of the frame and eases off toward the ground - this keeps the lower scene
// readable instead of washing the whole screen to a flat white.
uniform vec3  uFlashColor;
uniform float uFlashIntensity;

in vec2 vNdc;
out vec4 FragColor;

void main()
{
    float topward = vNdc.y * 0.5 + 0.5;         // 0 bottom .. 1 top
    float grad = mix(0.45, 1.0, topward);       // brighter up high
    FragColor = vec4(uFlashColor * uFlashIntensity * grad, 1.0);
}
