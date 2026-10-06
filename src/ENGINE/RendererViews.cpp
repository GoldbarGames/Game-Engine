// Split screen: the views a game sets (Renderer::SetViews) and the view the
// world chain is drawing now (render/RenderViews.h). Game::Render runs the
// views; this file only keeps the list and switches the per-view modules.

#include "Renderer.h"
#include "Game.h"
#include "Scene3DInternal.h"
#include "render/RenderViews.h"
#include "render/TemporalAA.h"
#include "render/ClusteredLights.h"
#include <vector>

namespace
{
	std::vector<RenderView> views;   // as the game set them (usable ones only)

	bool drawing = false;            // a view's world chain is running
	int currentIndex = 0;
	int targetWidth = 0;
	int targetHeight = 0;
	FrameBuffer* targetFrameBuffer = nullptr;

	void SwitchModules(int index)
	{
		SetTemporalView(index);
		Scene3DInternal::SetShadowView(index);
		SetLightClusterView(index);
	}
}

void Renderer::SetViews(const RenderView* list, int count)
{
	views.clear();
	for (int i = 0; list != nullptr && i < count && (int)views.size() < kMaxRenderViews; i++)
	{
		const RenderView& view = list[i];
		if (view.camera != nullptr && view.width > 0.0f && view.height > 0.0f)
			views.push_back(view);
	}
}

void Renderer::ClearViews()
{
	views.clear();
}

int Renderer::ViewCount() const
{
	return (int)views.size();
}

const RenderView* Renderer::GetViews() const
{
	return views.empty() ? nullptr : views.data();
}

void BeginViewRender(int index, int width, int height, FrameBuffer* target)
{
	drawing = true;
	currentIndex = (index >= 0 && index < kMaxRenderViews) ? index : 0;
	targetWidth = width;
	targetHeight = height;
	targetFrameBuffer = target;
	SwitchModules(currentIndex);
	Scene3DInternal::ResetGpuWorldDraw();   // this view's world pass draws the GPU-driven models too
}

void EndViewRender()
{
	drawing = false;
	currentIndex = 0;
	targetFrameBuffer = nullptr;
	SwitchModules(0);
}

int CurrentViewIndex()
{
	return currentIndex;
}

bool RenderingViews()
{
	return drawing;
}

int ViewTargetWidth(const Game& game)
{
	return drawing ? targetWidth : game.screenWidth;
}

int ViewTargetHeight(const Game& game)
{
	return drawing ? targetHeight : game.screenHeight;
}

FrameBuffer* ViewTargetFrameBuffer(const Game& game)
{
	return (drawing && targetFrameBuffer != nullptr) ? targetFrameBuffer : game.mainFrameBuffer;
}
