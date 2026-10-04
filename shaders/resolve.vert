#version 330

// Full-screen triangle with no vertex data (gl_VertexID 0, 1, 2 cover the
// screen). The linear workflow's Resolve pass (src/ENGINE/render/ColorPipeline.cpp).
void main()
{
	vec2 p = vec2(gl_VertexID == 1 ? 3.0 : -1.0, gl_VertexID == 2 ? 3.0 : -1.0);
	gl_Position = vec4(p, 0.0, 1.0);
}
