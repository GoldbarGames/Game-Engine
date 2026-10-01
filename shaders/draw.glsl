#ifndef KINJO_DRAW_GLSL
#define KINJO_DRAW_GLSL
// Per-draw values ("push constants"). A shader declares its own
//     struct DrawData { ... };
//     PER_DRAW(DrawData);
// and reads the values as draw.<name>. On GL this is a plain struct uniform
// named `draw` (the engine sets it per member, e.g. "draw.model"); a Vulkan
// build redefines PER_DRAW as a push-constant block, so a DrawData must stay
// within 128 bytes (std430: mat3 = 48, vec3 = 16).
//
// A program's vertex and fragment shaders must declare the SAME DrawData if
// both use it (the shared sprite one is in sprite_draw.glsl).
#define PER_DRAW(T) uniform T draw
#endif
