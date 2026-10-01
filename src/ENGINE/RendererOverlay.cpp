// Renderer overlay API: flat-colored rects (GUI space) and line segments (world
// space) for menus, HUDs and editor gizmos. Games call Renderer::DrawRect /
// DrawLines3D instead of owning their own VAO + shader + GL state juggling.
//
// The device objects are file-local rather than Renderer members so the API
// could be added without changing Renderer's layout (games built against the
// previous engine keep working). Vertices stream through the transient ring.

#include "leak_check.h"
#include "render/RenderDevice.h"
#include "Renderer.h"
#include "Shader.h"
#include "RenderState.h"
#include "TransientBuffer.h"
#include <glm/gtc/type_ptr.hpp>

namespace
{
	// Written as GLSL ES 3.00 for the web build. Desktop runs it through
	// ShaderProgram::ApplyVersion, which swaps the #version line for the
	// context's desktop version (precision qualifiers are legal no-ops there).
	// Per-draw values in the engine's DrawData shape (shaders/draw.glsl; self-
	// contained here so the overlay never depends on shader files): 80 bytes.
	const char* OVERLAY_VERT =
		"#version 300 es\n"
		"precision highp float;\n"
		"layout (location = 0) in vec3 pos;\n"
		"struct DrawData { mat4 mvp; vec4 color; };\n"
		"uniform DrawData draw;\n"
		"void main()\n"
		"{\n"
		"    gl_Position = draw.mvp * vec4(pos, 1.0);\n"
		"}\n";

	const char* OVERLAY_FRAG =
		"#version 300 es\n"
		"precision highp float;\n"
		"struct DrawData { mat4 mvp; vec4 color; };\n"
		"uniform DrawData draw;\n"
		"out vec4 FragColor;\n"
		"void main()\n"
		"{\n"
		"    FragColor = draw.color;\n"
		"}\n";

	struct OverlayResources
	{
		ShaderProgram* shader = nullptr;
		int mvpLoc = -1;
		int colorLoc = -1;
		VertexArrayHandle vao;
		BufferHandle vbo;
		bool failed = false;   // shader didn't build: stop retrying every frame
	};

	OverlayResources overlay;

	bool EnsureOverlay()
	{
		if (overlay.failed)
			return false;
		if (overlay.shader != nullptr)
			return true;

		std::string vert = ShaderProgram::ApplyVersion(OVERLAY_VERT);
		std::string frag = ShaderProgram::ApplyVersion(OVERLAY_FRAG);
		overlay.shader = new ShaderProgram(-1, vert.c_str(), frag.c_str(), true);
		overlay.shader->SetNameString("overlay");
		// GetID() is 0 when the program failed to compile or link
		if (overlay.shader->GetID() == 0)
		{
			std::cout << "ERROR: Renderer overlay shader failed to build; overlay drawing disabled" << std::endl;
			overlay.failed = true;
			return false;
		}
		overlay.mvpLoc = Device().UniformLocation(ProgramHandle(overlay.shader->GetID()), "draw.mvp");
		overlay.colorLoc = Device().UniformLocation(ProgramHandle(overlay.shader->GetID()), "draw.color");

		overlay.vao = Device().CreateVertexArray();
		overlay.vbo = Device().CreateBuffer(sizeof(glm::vec3) * 64, nullptr, BufferUsage::Dynamic);
		Device().SetVertexAttribute(overlay.vao, 0, overlay.vbo, 3, sizeof(glm::vec3), 0);
		return true;
	}

	// One overlay draw: upload the vertices, then draw them on top of
	// everything with standard alpha blending. The pipeline's state is undone
	// when the scope ends (depth writes are left as they were).
	void DrawOverlay(Primitive mode, const glm::vec3* verts, size_t count,
		const glm::mat4& mvp, const glm::vec4& color)
	{
		PipelineDesc pipeline;
		pipeline.shader = overlay.shader;
		pipeline.state = CurrentRenderState();
		pipeline.state.blend = BlendMode::Alpha;
		pipeline.state.depthTest = false;
		pipeline.state.cull = CullMode::None;
		ScopedPipeline bound(pipeline);

		Device().SetUniform((int)(overlay.mvpLoc), mvp);
		Device().SetUniform((int)(overlay.colorLoc), color);

		// Vertices for this draw: streamed through the transient ring, or (if it
		// is unavailable) re-specified into the overlay's own buffer, which lets
		// the driver orphan the old store instead of stalling on it.
		const TransientAlloc a = TransientUpload(verts, count * sizeof(glm::vec3));
		if (a.Valid())
		{
			Device().SetVertexAttribute(overlay.vao, 0, BufferHandle(a.buffer), 3, sizeof(glm::vec3), a.offset);
		}
		else
		{
			Device().ReplaceBuffer(overlay.vbo, count * sizeof(glm::vec3), verts, BufferUsage::Dynamic);
			Device().SetVertexAttribute(overlay.vao, 0, overlay.vbo, 3, sizeof(glm::vec3), 0);
		}

		Device().Draw(overlay.vao, mode, 0, (int)count);
	}
}

void Renderer::DrawRect(float x, float y, float w, float h, const glm::vec4& color) const
{
	if (!EnsureOverlay())
		return;

	const glm::vec3 verts[6] = {
		{ x,     y,     0.0f }, { x + w, y,     0.0f }, { x + w, y + h, 0.0f },
		{ x,     y,     0.0f }, { x + w, y + h, 0.0f }, { x,     y + h, 0.0f }
	};
	// GUI space maps straight through the GUI ortho projection (identity view).
	DrawOverlay(Primitive::Triangles, verts, 6, camera.guiProjection, color);
	drawCallsPerFrame++;
}

void Renderer::DrawLines3D(const std::vector<glm::vec3>& segments, const glm::vec4& color) const
{
	if (segments.size() < 2 || !EnsureOverlay())
		return;

	size_t count = segments.size() & ~(size_t)1;   // whole segments only
	glm::mat4 mvp = camera.projection * camera.CalculateViewMatrix();
	DrawOverlay(Primitive::Lines, segments.data(), count, mvp, color);
	drawCallsPerFrame++;
}

void Renderer::ReleaseOverlayResources()
{
	if (overlay.shader != nullptr)
		delete_it(overlay.shader);
	if (overlay.vbo)
		Device().DestroyBuffer(overlay.vbo);
	if (overlay.vao)
		Device().DestroyVertexArray(overlay.vao);
	overlay = OverlayResources();
}
