// Temporal anti-aliasing - see TemporalAA.h. Backend-agnostic: GPU work goes
// through RenderDevice, shaders through ShaderProgram.

#include "TemporalAA.h"
#include "ColorPipeline.h"
#include "RenderViews.h"
#include "../Camera.h"
#include "../Shader.h"
#include "../RenderState.h"
#include "../UniformBlocks.h"
#include "../UniformBufferCache.h"
#include "../globals.h"
#include <glm/glm.hpp>
#include <cstdlib>
#include <iostream>
#include <string>

namespace
{
	// --- settings -----------------------------------------------------------
	bool projectOn = true;   // renderer.dat `antialiasing`
	int forced = -1;         // KINJO_TAA: 0 = off, 1 = on, -1 = not set
	bool shadersFailed = false;

	// Weight of this frame's sample in the blend: ~10 frames of history. The
	// shader raises it a little while the image moves, which keeps motion
	// sharper (the history is resampled every frame then).
	const float kFeedback = 0.1f;
	const int kJitterPhases = 8;   // Halton(2,3), the length UE4 uses

	// --- frame state ----------------------------------------------------------
	bool frameActive = false;    // TAA resolves this frame
	bool resolved = false;       // ...and has: TemporalOutput is valid
	unsigned int frameCounter = 0;
	glm::vec2 jitter(0.0f);      // this frame's offset, in pixels

	bool cameraJittered = false;
	glm::mat4 unjitteredProjection(1.0f);
	glm::mat4 jitteredProjection(1.0f);

	bool motionWrites = false;      // the world pass is drawing (motion attachment enabled per draw)

	// --- per view (split screen, render/RenderViews.h) -------------------------
	// Everything that carries from one frame to the next: each view blends
	// into its own history, from its own last camera.
	struct ViewHistory
	{
		int width = 0, height = 0;
		TextureHandle history[2];       // RGBA16F, ping-pong: one is read, the other written
		TextureHandle display;          // RGBA16F: this frame's result, + weather drawn after TAA
		FramebufferHandle resolveFbo[2];   // history[i] + display (MRT)
		int writeIndex = 0;
		FramebufferHandle displayFbo;   // display + the world's depth (weather after TAA)
		unsigned int displayFboDepth = 0;
		bool historyValid = false;      // the history holds last frame's image
		bool havePrevViewProj = false;
		glm::mat4 prevViewProj = glm::mat4(1.0f);   // last frame's camera, unjittered
	};
	ViewHistory views[kMaxRenderViews];
	ViewHistory* cur = &views[0];        // the view drawing now (SetTemporalView)
	int currentView = 0;

	// std140 mirror of the GLSL "Motion" block (shaders/motion.glsl).
	struct MotionBlockData
	{
		glm::mat4 viewProj;
		glm::mat4 prevViewProj;
		int on;
		int pad[3];
	};
	static_assert(sizeof(MotionBlockData) == 144, "MotionBlockData must match shaders/motion.glsl");
	UniformBufferCache motionBlocks(UniformBlock::Motion, sizeof(MotionBlockData), 2);

	void BindMotionBlock(const glm::mat4& viewProj, const glm::mat4& prev, bool on)
	{
		MotionBlockData d = {};
		d.viewProj = viewProj;
		d.prevViewProj = prev;
		d.on = on ? 1 : 0;
		motionBlocks.Bind(&d);
	}

	// --- resources ----------------------------------------------------------
	VertexArrayHandle emptyVao;

	ShaderProgram* outlineShader = nullptr;   // the toon outline into the world, before the resolve
	bool outlineFailed = false;
	int outlineDepthLoc = -1, outlineMaskLoc = -1;

	ShaderProgram* taaShader = nullptr;
	struct Locs { int reproject = -1, size = -1, feedback = -1, useHistory = -1, jitter = -1,
		current = -1, history = -1, depth = -1, motion = -1; };
	Locs locs;

	float Halton(unsigned int index, unsigned int base)
	{
		float f = 1.0f, r = 0.0f;
		while (index > 0)
		{
			f /= (float)base;
			r += f * (float)(index % base);
			index /= base;
		}
		return r;
	}

	bool EnsureShader()
	{
		if (shadersFailed)
			return false;
		if (taaShader != nullptr)
			return true;
		taaShader = new ShaderProgram(-1, "data/shaders/resolve.vert", "data/shaders/taa.frag");
		taaShader->SetNameString("taa");
		if (taaShader->GetID() == 0)
		{
			std::cout << "ERROR: temporal anti-aliasing shader failed to build; anti-aliasing disabled" << std::endl;
			delete taaShader;
			taaShader = nullptr;
			shadersFailed = true;
			return false;
		}
		const unsigned int id = taaShader->GetID();
		RenderDevice& device = Device();
		locs.reproject = ShaderProgram::DrawUniformLocation(id, "reproject");
		locs.size = ShaderProgram::DrawUniformLocation(id, "size");
		locs.feedback = ShaderProgram::DrawUniformLocation(id, "feedback");
		locs.useHistory = ShaderProgram::DrawUniformLocation(id, "useHistory");
		locs.jitter = ShaderProgram::DrawUniformLocation(id, "jitter");
		locs.current = device.UniformLocation(ProgramHandle(id), "currentColor");
		locs.history = device.UniformLocation(ProgramHandle(id), "historyColor");
		locs.depth = device.UniformLocation(ProgramHandle(id), "sceneDepth");
		locs.motion = device.UniformLocation(ProgramHandle(id), "sceneMotion");
		return true;
	}

	// The current view's targets.
	void ReleaseTargets()
	{
		RenderDevice& device = Device();
		for (TextureHandle* t : { &cur->history[0], &cur->history[1], &cur->display })
			if (*t)
				device.DestroyTexture(*t);
		for (FramebufferHandle* f : { &cur->resolveFbo[0], &cur->resolveFbo[1], &cur->displayFbo })
			if (*f)
				device.DestroyFramebuffer(*f);
		cur->displayFboDepth = 0;
		cur->width = cur->height = 0;
		cur->historyValid = false;
	}

	bool EnsureTargets(int w, int h)
	{
		if (cur->display && w == cur->width && h == cur->height)
			return true;
		ReleaseTargets();
		RenderDevice& device = Device();
		TextureDesc desc;
		desc.format = TextureFormat::RGBA16F;
		desc.width = w;
		desc.height = h;
		desc.filter = TextureFilter::Linear;   // Catmull-Rom history taps; bloom's first downsample
		desc.wrap = TextureWrap::ClampToEdge;
		cur->history[0] = device.CreateTexture(desc);
		cur->history[1] = device.CreateTexture(desc);
		cur->display = device.CreateTexture(desc);
		cur->width = w;
		cur->height = h;

		bool ok = true;
		for (int i = 0; i < 2; i++)
		{
			cur->resolveFbo[i] = device.CreateFramebuffer();
			device.AttachTexture(cur->resolveFbo[i], Attachment::Color0, cur->history[i]);
			device.AttachTexture(cur->resolveFbo[i], Attachment::Color1, cur->display);
			device.SetDrawBuffers(cur->resolveFbo[i], 2);
			std::string error;
			if (!device.IsFramebufferComplete(cur->resolveFbo[i], &error))
			{
				std::cout << "ERROR: temporal anti-aliasing target incomplete (" << error << "); anti-aliasing disabled" << std::endl;
				ok = false;
			}
		}
		if (!emptyVao)
			emptyVao = device.CreateVertexArray();
		device.BindFramebuffer(FramebufferHandle());
		if (!ok)
		{
			ReleaseTargets();
			shadersFailed = true;   // don't retry every frame
		}
		return ok;
	}
}

void LoadTemporalAASettings()
{
	auto config = GetMapStringsFromFile(RendererConfigPath());
	projectOn = true;
	if (config.count("antialiasing") > 0)
	{
		const std::string& v = config["antialiasing"];
		if (v == "none" || v == "off" || v == "0")
			projectOn = false;
		else if (v != "taa")
			std::cout << "WARNING: renderer.dat antialiasing '" << v << "' unknown (taa, none) - using taa" << std::endl;
	}
	if (const char* e = std::getenv("KINJO_TAA"))
		forced = (e[0] == '0') ? 0 : (e[0] == '1') ? 1 : -1;
	if (LinearWorkflow())
		std::cout << "Anti-aliasing: " << (projectOn ? "temporal (TAA)" : "none")
			<< (forced == 0 ? " (KINJO_TAA=0: off)" : forced == 1 ? " (KINJO_TAA=1: forced on)" : "") << std::endl;
}

bool TemporalAAWanted()
{
	const bool on = (forced >= 0) ? (forced == 1) : projectOn;
	return on && LinearWorkflow() && !shadersFailed;
}

void BeginTemporalFrame(bool active, int w, int h)
{
	resolved = false;
	motionWrites = false;
	BindMotionBlock(glm::mat4(1.0f), glm::mat4(1.0f), false);   // the block always holds a valid "off"
	frameActive = active && TemporalAAWanted() && w > 0 && h > 0 && EnsureShader() && EnsureTargets(w, h);
	if (!frameActive)
	{
		cur->historyValid = false;   // the next active frame starts afresh
		cur->havePrevViewProj = false;
		jitter = glm::vec2(0.0f);
		return;
	}
	if (currentView == 0)
		frameCounter++;   // once a frame: every split-screen view takes the same step of the sequence
	const unsigned int phase = (frameCounter % kJitterPhases) + 1;   // Halton from index 1 (0 is the origin)
	jitter = glm::vec2(Halton(phase, 2) - 0.5f, Halton(phase, 3) - 0.5f);
}

bool TemporalAAActive()
{
	return frameActive;
}

void JitterCamera(Camera& camera)
{
	if (!frameActive || cameraJittered || camera.useOrthoCamera)
		return;
	unjitteredProjection = camera.projection;
	// A pixel offset in clip space: adding to the third column shifts x/w and
	// y/w by a constant for every depth (perspective divides by -z, which that
	// column's w component carries).
	glm::mat4 p = camera.projection;
	p[2][0] += jitter.x * 2.0f / (float)cur->width;
	p[2][1] += jitter.y * 2.0f / (float)cur->height;
	jitteredProjection = p;
	camera.projection = p;
	cameraJittered = true;

	// Motion vectors compare this frame's camera with last frame's, both
	// without the jitter (TAA's output pixels are the unjittered ones).
	const glm::mat4 viewProj = unjitteredProjection * camera.CalculateViewMatrix();
	const bool havePrev = cur->historyValid && cur->havePrevViewProj;
	BindMotionBlock(viewProj, havePrev ? cur->prevViewProj : viewProj, havePrev);
}

void SetMotionWrites(bool on)
{
	motionWrites = on && frameActive && WorldMotionTexture();
}

bool MotionWritesActive()
{
	return motionWrites;
}

unsigned int TemporalFrameIndex()
{
	return frameActive ? frameCounter : 0;
}

void RestoreCamera(Camera& camera)
{
	if (!cameraJittered)
		return;
	// Only undo our own change: if something rebuilt the projection meanwhile,
	// that newer one stands.
	if (camera.projection == jitteredProjection)
		camera.projection = unjitteredProjection;
	cameraJittered = false;
}

void ResolveTemporalAA(TextureHandle worldColor, TextureHandle depth, const Camera& camera,
	int screenWidth, int screenHeight)
{
	if (!frameActive || !worldColor || !depth || taaShader == nullptr)
		return;
	RenderDevice& device = Device();

	// Reprojection uses the unjittered cameras: the output pixel stands for the
	// unjittered pixel centre, and last frame's image was resolved the same way.
	const glm::mat4 projection = cameraJittered ? unjitteredProjection : camera.projection;
	const glm::mat4 viewProj = projection * camera.CalculateViewMatrix();
	const bool useHistory = cur->historyValid && cur->havePrevViewProj;
	const glm::mat4 reproject = useHistory ? cur->prevViewProj * glm::inverse(viewProj) : glm::mat4(1.0f);

	{
		RenderState s = CurrentRenderState();
		s.blend = BlendMode::Off;
		s.depthTest = false;
		s.depthWrite = false;
		s.cull = CullMode::None;
		ScopedRenderState scope(s);

		device.BindFramebuffer(cur->resolveFbo[cur->writeIndex]);
		device.SetViewport(0, 0, cur->width, cur->height);
		taaShader->UseShader();
		device.BindTexture(0, worldColor);
		device.BindTexture(1, cur->history[1 - cur->writeIndex]);
		device.BindTexture(2, depth);
		device.BindTexture(3, WorldMotionTexture());
		device.SetUniform(locs.current, 0);
		device.SetUniform(locs.history, 1);
		device.SetUniform(locs.depth, 2);
		device.SetUniform(locs.motion, 3);
		device.SetUniform(locs.reproject, reproject);
		device.SetUniform(locs.size, glm::vec2((float)cur->width, (float)cur->height));
		device.SetUniform(locs.feedback, kFeedback);
		device.SetUniform(locs.useHistory, useHistory ? 1 : 0);
		// The projection offset moved the image by -jitter pixels, so texel q
		// shows what the unjittered camera sees at q + 0.5 + jitter.
		device.SetUniform(locs.jitter, jitter);
		device.Draw(emptyVao, Primitive::Triangles, 0, 3);
	}
	device.BindFramebuffer(FramebufferHandle());
	device.SetViewport(0, 0, screenWidth, screenHeight);

	cur->prevViewProj = viewProj;
	cur->havePrevViewProj = true;
	cur->historyValid = true;
	cur->writeIndex = 1 - cur->writeIndex;
	resolved = true;
}

bool DrawWorldOutline(TextureHandle depth, TextureHandle mask)
{
	if (!frameActive || outlineFailed || !depth || !mask)
		return false;
	if (outlineShader == nullptr)
	{
		outlineShader = new ShaderProgram(-1, "data/shaders/resolve.vert", "data/shaders/outline_world.frag");
		outlineShader->SetNameString("outline_world");
		if (outlineShader->GetID() == 0)
		{
			std::cout << "ERROR: in-world outline shader failed to build; the composite draws the outline" << std::endl;
			delete outlineShader;
			outlineShader = nullptr;
			outlineFailed = true;
			return false;
		}
		outlineDepthLoc = Device().UniformLocation(ProgramHandle(outlineShader->GetID()), "depthTex");
		outlineMaskLoc = Device().UniformLocation(ProgramHandle(outlineShader->GetID()), "maskTex");
	}
	if (!BindWorldColorOnly())
		return false;

	RenderDevice& device = Device();
	RenderState s = CurrentRenderState();
	s.blend = BlendMode::Alpha;   // alpha = how much outline: mix(world, outline, edge)
	s.depthTest = false;
	s.depthWrite = false;
	s.cull = CullMode::None;
	ScopedRenderState scope(s);
	outlineShader->UseShader();
	device.BindTexture(1, depth);
	device.BindTexture(2, mask);
	device.SetUniform(outlineDepthLoc, 1);
	device.SetUniform(outlineMaskLoc, 2);
	device.Draw(emptyVao, Primitive::Triangles, 0, 3);
	device.BindFramebuffer(FramebufferHandle());
	return true;
}

TextureHandle TemporalHistory(glm::mat4& prevViewProjOut)
{
	if (!frameActive || !cur->historyValid || !cur->havePrevViewProj)
		return TextureHandle();
	prevViewProjOut = cur->prevViewProj;
	return cur->history[1 - cur->writeIndex];
}

TextureHandle TemporalOutput()
{
	return resolved ? cur->display : TextureHandle();
}

bool BindTemporalOutputTarget(TextureHandle depthStencil)
{
	if (!resolved || !depthStencil)
		return false;
	RenderDevice& device = Device();
	if (!cur->displayFbo || cur->displayFboDepth != depthStencil.id)
	{
		if (cur->displayFbo)
			device.DestroyFramebuffer(cur->displayFbo);
		cur->displayFbo = device.CreateFramebuffer();
		device.AttachTexture(cur->displayFbo, Attachment::Color0, cur->display);
		device.AttachTexture(cur->displayFbo, Attachment::DepthStencil, depthStencil);
		cur->displayFboDepth = depthStencil.id;
		std::string error;
		if (!device.IsFramebufferComplete(cur->displayFbo, &error))
			std::cout << "ERROR: anti-aliased world target incomplete (" << error << ")" << std::endl;
	}
	device.BindFramebuffer(cur->displayFbo);
	device.SetViewport(0, 0, cur->width, cur->height);
	SetTargetLinear(true);
	return true;
}

void ResetTemporalHistory()
{
	for (ViewHistory& view : views)
		view.historyValid = false;
}

void SetTemporalView(int index)
{
	currentView = (index >= 0 && index < kMaxRenderViews) ? index : 0;
	cur = &views[currentView];
}

void ReleaseTemporalAA()
{
	for (ViewHistory& view : views)
	{
		cur = &view;
		ReleaseTargets();
		view = ViewHistory();
	}
	SetTemporalView(0);
	if (emptyVao)
		Device().DestroyVertexArray(emptyVao);
	delete taaShader;
	taaShader = nullptr;
	delete outlineShader;
	outlineShader = nullptr;
	shadersFailed = outlineFailed = false;
	frameActive = resolved = cameraJittered = false;
}
