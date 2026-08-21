#version 330 core

// Attribute-less full-screen triangle: three verts that cover the whole screen
// in clip space. Bound VAO can be empty; positions come from gl_VertexID.
out vec2 vNdc;   // clip-space xy in [-1,1], for the top-weighted flash gradient

void main()
{
    vec2 p = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
    vec2 pos = p * 2.0 - 1.0;
    vNdc = pos;
    gl_Position = vec4(pos, 0.0, 1.0);
}
