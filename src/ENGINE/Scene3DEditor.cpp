#include "Scene3DEditor.h"
#include "Scene3D.h"
#include "Game.h"
#include "Renderer.h"
#include "Camera.h"
#include "Shader.h"
#include "Text.h"
#include "Texture.h"
#include "InputManager.h"
#include "MenuManager.h"
#include "opengl_includes.h"
#include <glm/gtc/type_ptr.hpp>
#include <SDL2/SDL.h>
#include <cmath>
#include <cstdio>
#include <sstream>
#include <string>

namespace
{
	// Ray vs axis-aligned box (slab method). Returns the nearest positive hit
	// distance in tHit. origin/dir need not be normalized.
	bool RayAABB(const glm::vec3& origin, const glm::vec3& dir,
		const glm::vec3& bmin, const glm::vec3& bmax, float& tHit)
	{
		float tmin = -1e30f, tmax = 1e30f;
		for (int i = 0; i < 3; i++)
		{
			if (std::fabs(dir[i]) < 1e-8f)
			{
				if (origin[i] < bmin[i] || origin[i] > bmax[i])
					return false;
			}
			else
			{
				float inv = 1.0f / dir[i];
				float t1 = (bmin[i] - origin[i]) * inv;
				float t2 = (bmax[i] - origin[i]) * inv;
				if (t1 > t2) std::swap(t1, t2);
				tmin = std::max(tmin, t1);
				tmax = std::min(tmax, t2);
				if (tmin > tmax)
					return false;
			}
		}
		if (tmax < 0.0f)
			return false;             // whole box behind the ray
		tHit = (tmin >= 0.0f) ? tmin : tmax;
		return true;
	}

	// Parameter s along the axis line (a0 + s*u, u unit) of the point closest
	// to the ray (b0 + t*v). Returns false when the lines are near-parallel.
	bool ClosestAxisParam(const glm::vec3& a0, const glm::vec3& u,
		const glm::vec3& b0, const glm::vec3& v, float& s)
	{
		glm::vec3 vv = glm::normalize(v);
		glm::vec3 w0 = a0 - b0;
		float b = glm::dot(u, vv);
		float d = glm::dot(u, w0);
		float e = glm::dot(vv, w0);
		float denom = 1.0f - b * b;   // dot(u,u)=1, dot(vv,vv)=1
		if (std::fabs(denom) < 1e-5f)
			return false;
		s = (b * e - d) / denom;
		return true;
	}

	// World-space AABB of a character billboard (loose: a vertical box that
	// bounds the yaw-facing quad). Visual up is world -Y, so the top is at a
	// SMALLER y than the feet.
	void CharacterAABB(const Character3D* ch, glm::vec3& bmin, glm::vec3& bmax)
	{
		float aspect = 0.5f;
		if (ch->bodyTex != nullptr && ch->bodyTex->GetHeight() > 0)
			aspect = (float)ch->bodyTex->GetWidth() / (float)ch->bodyTex->GetHeight();
		float halfW = 0.5f * ch->worldHeight * aspect;
		glm::vec3 p = ch->position;
		bmin = glm::vec3(p.x - halfW, p.y - ch->worldHeight, p.z - halfW);
		bmax = glm::vec3(p.x + halfW, p.y, p.z + halfW);
	}

	std::string BaseName(const std::string& path)
	{
		size_t slash = path.find_last_of("/\\");
		return (slash == std::string::npos) ? path : path.substr(slash + 1);
	}

	std::string F(float v)
	{
		std::ostringstream ss;
		ss.precision(1);
		ss << std::fixed << v;
		return ss.str();
	}

	// Overlay text uses the rich-text path (per-glyph), which honors '\n' and
	// scales via SetScale. The font is rendered large (kOverlayFontSize) then
	// scaled down by kOverlayScale for crisp, legible text - same approach the
	// VN textbox uses.
	const int kOverlayFontSize = 96;
	const float kOverlayScale = 1.0f;

	// The object list holds many rows, so it renders smaller than the main
	// overlay to fit them all on screen. It sits in a right-side column.
	const float kListScale = 0.8f;
	const float kListWidthGui = 460.0f;   // also the clickable column width
	const float kListTopGui = 120.0f;
	const float kListMarginGui = 24.0f;
	const float kZoomBtnGui = 90.0f;   // width of the per-row "zoom" button (right edge)
	// Explicit GUI-space row pitch: rows are both drawn and hit-tested at this
	// spacing, so a click always maps to the row the user sees.
	const float kListRowGui = 52.0f;

	// Move / Rotate / Scale button bar (GUI-space layout). Fixed, compact
	// button size with large labels.
	const float kBtnX = 24.0f;
	const float kBtnY = 190.0f;
	const float kBtnGap = 16.0f;
	const float kBtnTextScale = 1.2f;
	const float kBtnPadX = 16.0f;
	const float kBtnPadY = 10.0f;
	// GetTextWidth/Height sum padded glyph textures, so the laid-out text is a
	// fixed fraction of them; these factors convert to on-screen GUI units.
	const float kBtnWFactor = 0.62f;
	const float kBtnHFactor = 0.44f;
	const char* kModeNames[3] = { "MOVE", "ROTATE", "SCALE" };
	const char* kActNames[7] = { "DELETE", "ADD", "NEW", "LOAD", "TAG", "MAT", "SHADOW" };
	const char* kAxisNames[4] = { "FREE", "X", "Y", "Z" };
	const char* kResetNames[3] = { "RESET POS", "RESET ROT", "RESET SCALE" };
	const char* kCamNames[4] = { "SAVE CAM", "NEW CAM", "SET DEF", "DEL CAM" };
	const char* kEditNames[3] = { "UNDO", "REDO", "RELOAD" };

	// Editable water-surface properties (WATER bar). Each: label, adjust step,
	// min, max. Order matches WaterField() below.
	struct WaterPropDef { const char* name; float step; float lo; float hi; };
	const WaterPropDef kWaterProps[] = {
		{ "AMPLITUDE",  1.0f,  0.0f,  40.0f },
		{ "WAVE SCALE", 0.1f,  0.2f,   4.0f },
		{ "SHORE FADE", 0.1f,  0.0f,   1.0f },
		{ "CHOPPY",     0.1f,  0.0f,   3.0f },
		{ "SPECULAR",   0.1f,  0.0f,   3.0f },
		{ "SHININESS", 20.0f,  8.0f, 400.0f },
		{ "OPACITY",    0.05f, 0.1f,   1.0f },
	};
	const int kWaterPropCount = 7;

	// Pointer to the WaterSurface field for property index i.
	float* WaterField(WaterSurface& w, int i)
	{
		switch (i)
		{
		case 0:  return &w.amplitude;
		case 1:  return &w.waveScale;
		case 2:  return &w.shoreFade;
		case 3:  return &w.choppy;
		case 4:  return &w.specular;
		case 5:  return &w.shininess;
		default: return &w.opacity;
		}
	}

	// "Add model" dropdown geometry.
	const float kDropRowGui = 48.0f;
	const float kDropWidthGui = 380.0f;
	const float kDropScale = 0.7f;
}

void Scene3DEditor::Toggle(Game& game)
{
	Scene3D& scene = Scene3D::Get();
	if (!active && !scene.active)
		return;   // nothing to edit

	active = !active;
	game.editing3D = active;

	if (active)
	{
		dragging = false;
		leftWasDown = false;
		ClearSelection();
		statusMsg = "3D editor ON";
		statusFrames = 180;
		std::cout << "Scene3DEditor: ON" << std::endl;
	}
	else
	{
		dragging = false;
		openDropdown = DropKind::None;
		namingScene = false;
		statusMsg.clear();
		std::cout << "Scene3DEditor: OFF" << std::endl;
	}
}

void Scene3DEditor::ClearSelection()
{
	selType = SelType::None;
	selIndex = -1;
	lastInfo.clear();
}

void Scene3DEditor::Update(Game& game)
{
	Scene3D& scene = Scene3D::Get();
	const Uint8* keys = SDL_GetKeyboardState(NULL);

	// Toggle on '2' (rising edge), only while a 3D scene is showing. Suppressed
	// while typing a scene name (so '2' types a digit instead of exiting).
	bool toggleDown = keys[SDL_SCANCODE_2] != 0;
	if (toggleDown && !toggleWasDown && !namingScene && (scene.active || active))
		Toggle(game);
	toggleWasDown = toggleDown;

	if (statusFrames > 0)
		statusFrames--;

	if (!active)
		return;

	if (!scene.active)   // scene unloaded out from under us
	{
		active = false;
		game.editing3D = false;
		return;
	}

	// Re-baseline the undo history whenever the scene changes underneath us
	// (first load, LOAD dropdown, NEW scene). F9/RELOAD reset it explicitly.
	if (scene.currentScene != historyScene)
		ResetHistory();

	// While naming a new scene, capture typed text and freeze everything else.
	if (namingScene)
	{
		UpdateNaming(keys, game);
		return;
	}

	// Keyboard undo/redo: Ctrl+Z / Ctrl+Y (and Ctrl+Shift+Z for redo).
	bool ctrl = keys[SDL_SCANCODE_LCTRL] || keys[SDL_SCANCODE_RCTRL];
	bool shift = keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT];
	bool zDown = keys[SDL_SCANCODE_Z] != 0;
	bool yDown = keys[SDL_SCANCODE_Y] != 0;
	if (ctrl && zDown && !undoKeyWasDown)
	{
		if (shift) Redo(game); else Undo(game);
	}
	if (ctrl && yDown && !redoKeyWasDown)
		Redo(game);
	undoKeyWasDown = ctrl && zDown;
	redoKeyWasDown = ctrl && yDown;

	Camera& cam = game.renderer.camera;
	const float w = (float)game.screenWidth;
	const float h = (float)game.screenHeight;

	// --- mouse (poll relative every frame so the first look never jumps) ---
	int mouseDX = 0, mouseDY = 0;
	Uint32 relButtons = SDL_GetRelativeMouseState(&mouseDX, &mouseDY);
	int mx = 0, my = 0;
	Uint32 mb = SDL_GetMouseState(&mx, &my);
	bool leftDown = (mb & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;
	bool rightHeld = (relButtons & SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0;
	bool wheelUp = game.inputManager.scrolledUp;
	bool wheelDown = game.inputManager.scrolledDown;

	// --- camera: a zoom-to-object glide takes precedence over the fly camera,
	// but any manual camera input (movement key / right-drag) cancels it. ---
	bool moveKey = keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_S]
		|| keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_E] || keys[SDL_SCANCODE_Q]
		|| wheelUp || wheelDown;
	if (camGliding && !moveKey && !rightHeld)
	{
		camGlideElapsed += game.dt / 1000.0f;
		float t = (camGlideDur > 0.0f) ? (camGlideElapsed / camGlideDur) : 1.0f;
		if (t > 1.0f) t = 1.0f;
		float s = t * t * (3.0f - 2.0f * t);
		cam.position = glm::mix(glideFromPos, glideToPos, s);
		cam.pitch = glideFromPitch + (glideToPitch - glideFromPitch) * s;
		float dyaw = glideToYaw - glideFromYaw;      // shortest-path yaw
		while (dyaw > 180.0f) dyaw -= 360.0f;
		while (dyaw < -180.0f) dyaw += 360.0f;
		cam.yaw = glideFromYaw + dyaw * s;
		cam.Update();
		if (t >= 1.0f) camGliding = false;
	}
	else
	{
		camGliding = false;   // manual input cancels the glide
		flyCam.Update(cam, game.dt, mouseDX, mouseDY, rightHeld, wheelUp, wheelDown);
	}

	// --- selection + drag (left mouse). Skip while looking with RMB. ---
	bool leftPressed = leftDown && !leftWasDown;
	bool leftReleased = !leftDown && leftWasDown;

	// Click priority: mode buttons, action buttons, add-dropdown, object list,
	// then the 3D scene.
	if (leftPressed && !rightHeld && ModeButtonClick(game, (float)mx, (float)my))
		leftPressed = false;   // consumed
	if (leftPressed && !rightHeld && AxisButtonClick(game, (float)mx, (float)my))
		leftPressed = false;   // consumed
	if (leftPressed && !rightHeld && ResetButtonClick(game, (float)mx, (float)my))
		leftPressed = false;   // consumed
	if (leftPressed && !rightHeld && CameraButtonClick(game, (float)mx, (float)my))
		leftPressed = false;   // consumed
	if (leftPressed && !rightHeld && EditButtonClick(game, (float)mx, (float)my))
		leftPressed = false;   // consumed
	if (leftPressed && !rightHeld && WaterButtonClick(game, (float)mx, (float)my))
		leftPressed = false;   // consumed
	if (leftPressed && !rightHeld && ActionButtonClick(game, (float)mx, (float)my))
		leftPressed = false;   // consumed
	if (leftPressed && !rightHeld && openDropdown != DropKind::None)
	{
		DropdownClick(game, (float)mx, (float)my);  // add model / load scene on a row hit
		openDropdown = DropKind::None;              // close on any click
		leftPressed = false;                        // consumed
	}
	if (leftPressed && !rightHeld && ListClick(game, (float)mx, (float)my))
		leftPressed = false;   // consumed
	if (leftPressed && !rightHeld && CameraListClick(game, (float)mx, (float)my))
		leftPressed = false;   // consumed

	if (leftPressed && !rightHeld)
	{
		// Press = pick whatever is under the cursor (updates the selection),
		// then arm a drag on it according to the current transform mode.
		PickAt(game, (float)mx, (float)my);

		if (HasSelection())
		{
			pressX = mx;
			pressY = my;

			// Capture the start values so rotate/scale edits are relative to
			// the press (no per-frame drift).
			Scene3DModel* selM = (selType == SelType::Model) ? scene.GetModels()[selIndex] : nullptr;
			Character3D* selC = (selType == SelType::Character) ? scene.GetCharacters()[selIndex] : nullptr;
			dragStartYaw = selM ? selM->yawDeg : 0.0f;
			dragStartPitch = selM ? selM->pitchDeg : 0.0f;
			dragStartRoll = selM ? selM->rollDeg : 0.0f;
			dragStartScale = selM ? selM->modelScale : 1.0f;
			dragStartScaleAxis = selM ? selM->scaleAxis : glm::vec3(1.0f);
			dragStartHeight = selC ? selC->worldHeight : 0.0f;

			// Axis lock comes from the on-screen X/Y/Z buttons (a toggle), not a
			// held key - so it can't slip mid-drag.
			dragAxis = lockAxis;

			if (xformMode == XformMode::Move)
			{
				glm::vec3 obj = SelectedPosition(game);
				glm::vec3 ro, rd;
				cam.ScreenPointToRay((float)mx, (float)my, w, h, ro, rd);

				if (dragAxis < 0)
				{
					dragGrabOffset = glm::vec3(0);
					if (std::fabs(rd.y) > 1e-6f)
					{
						float t = (obj.y - ro.y) / rd.y;
						if (t > 0.0f)
						{
							glm::vec3 hit = ro + t * rd;
							dragGrabOffset.x = obj.x - hit.x;
							dragGrabOffset.z = obj.z - hit.z;
						}
					}
				}
				else
				{
					glm::vec3 e(0.0f); e[dragAxis] = 1.0f;
					float s = 0.0f;
					ClosestAxisParam(obj, e, ro, rd, s);
					dragGrabOffset = glm::vec3(0.0f);
					dragGrabOffset[dragAxis] = s;   // reuse as "start param"
				}
			}
			dragging = true;
		}
	}

	if (leftReleased)
	{
		// End of a drag: commit ONE undo step for the whole A->...->C move
		// (no-op if nothing actually moved, e.g. a plain click-select).
		if (dragging)
			CommitEdit();
		dragging = false;
	}

	if (dragging && leftDown && HasSelection())
	{
		if (xformMode == XformMode::Move)
		{
			glm::vec3 ro, rd;
			cam.ScreenPointToRay((float)mx, (float)my, w, h, ro, rd);
			glm::vec3 obj = SelectedPosition(game);

			if (dragAxis < 0)
			{
				if (std::fabs(rd.y) > 1e-6f)
				{
					float t = (obj.y - ro.y) / rd.y;
					if (t > 0.0f)
					{
						glm::vec3 hit = ro + t * rd;
						obj.x = hit.x + dragGrabOffset.x;
						obj.z = hit.z + dragGrabOffset.z;
						SetSelectedPosition(game, obj);
					}
				}
			}
			else
			{
				glm::vec3 e(0.0f); e[dragAxis] = 1.0f;
				float s = 0.0f;
				if (ClosestAxisParam(obj, e, ro, rd, s))
				{
					float delta = s - dragGrabOffset[dragAxis];
					obj += e * delta;
					SetSelectedPosition(game, obj);
					dragGrabOffset[dragAxis] = s;
				}
			}
		}
		else if (xformMode == XformMode::Rotate)
		{
			// Horizontal drag spins the object about the locked axis (models
			// only - characters are camera-facing billboards). FREE = yaw.
			//   X -> pitch, Y -> yaw, Z -> roll.
			if (selType == SelType::Model)
			{
				Scene3DModel* m = scene.GetModels()[selIndex];
				float deg = (mx - pressX) * 0.5f;
				switch (dragAxis)
				{
				case 0:  m->pitchDeg = dragStartPitch + deg; break;
				case 2:  m->rollDeg  = dragStartRoll  + deg; break;
				default: m->yawDeg   = dragStartYaw   + deg; break;
				}
				scene.RecomputeModelBounds(m);
			}
		}
		else if (xformMode == XformMode::Scale)
		{
			// Horizontal drag scales: right = bigger. FREE scales models
			// uniformly; an axis lock scales only that axis (non-uniform).
			// Characters always scale their billboard height uniformly.
			float factor = 1.0f + (mx - pressX) * 0.004f;
			if (factor < 0.05f) factor = 0.05f;
			if (selType == SelType::Model)
			{
				Scene3DModel* m = scene.GetModels()[selIndex];
				if (dragAxis < 0)
				{
					m->modelScale = dragStartScale * factor;
					if (m->modelScale < 0.02f) m->modelScale = 0.02f;
				}
				else
				{
					float v = dragStartScaleAxis[dragAxis] * factor;
					if (v < 0.02f) v = 0.02f;
					m->scaleAxis[dragAxis] = v;
				}
				scene.RecomputeModelBounds(m);
			}
			else if (selType == SelType::Character)
			{
				Character3D* c = scene.GetCharacters()[selIndex];
				c->worldHeight = dragStartHeight * factor;
				if (c->worldHeight < 20.0f) c->worldHeight = 20.0f;
			}
		}
		RefreshInfoText(game);
	}

	leftWasDown = leftDown;
	lastMouseX = mx;
	lastMouseY = my;

	// --- save / revert / delete keys ---
	bool saveDown = keys[SDL_SCANCODE_F5] != 0;
	if (saveDown && !saveWasDown)
	{
		if (scene.SaveScene(game))
		{
			savedSnapshot = scene.SerializeToString();   // now clean (keeps undo history)
			statusMsg = "Saved " + scene.currentScene + ".scene";
			statusFrames = 180;
		}
		else
		{
			statusMsg = "SAVE FAILED";
			statusFrames = 180;
		}
	}
	saveWasDown = saveDown;

	bool revertDown = keys[SDL_SCANCODE_F9] != 0;
	if (revertDown && !revertWasDown)
	{
		ClearSelection();
		dragging = false;
		scene.Reload(game);
		ResetHistory();   // reloaded from disk = clean, fresh undo history
		statusMsg = "Reverted to saved scene";
		statusFrames = 180;
	}
	revertWasDown = revertDown;

	bool deleteDown = (keys[SDL_SCANCODE_DELETE] != 0) || (keys[SDL_SCANCODE_BACKSPACE] != 0);
	if (deleteDown && !deleteWasDown && HasSelection())
	{
		// Deletion changes the underlying vectors; for v1 keep it simple and
		// just clear the selection (true removal would require Scene3D support).
		statusMsg = "(delete not supported yet)";
		statusFrames = 150;
	}
	deleteWasDown = deleteDown;
}

void Scene3DEditor::PickAt(Game& game, float sx, float sy)
{
	Scene3D& scene = Scene3D::Get();
	Camera& cam = game.renderer.camera;
	glm::vec3 ro, rd;
	cam.ScreenPointToRay(sx, sy, (float)game.screenWidth, (float)game.screenHeight, ro, rd);

	float bestT = 1e30f;
	SelType bestType = SelType::None;
	int bestIndex = -1;

	const auto& models = scene.GetModels();
	for (size_t i = 0; i < models.size(); i++)
	{
		float t;
		if (RayAABB(ro, rd, models[i]->aabbMin, models[i]->aabbMax, t) && t < bestT)
		{
			bestT = t;
			bestType = SelType::Model;
			bestIndex = (int)i;
		}
	}

	const auto& chars = scene.GetCharacters();
	for (size_t i = 0; i < chars.size(); i++)
	{
		glm::vec3 bmin, bmax;
		CharacterAABB(chars[i], bmin, bmax);
		float t;
		if (RayAABB(ro, rd, bmin, bmax, t) && t < bestT)
		{
			bestT = t;
			bestType = SelType::Character;
			bestIndex = (int)i;
		}
	}

	selType = bestType;
	selIndex = bestIndex;
	RefreshInfoText(game);
}

glm::vec3 Scene3DEditor::SelectedPosition(Game& game) const
{
	Scene3D& scene = Scene3D::Get();
	if (selType == SelType::Model && selIndex >= 0 && selIndex < (int)scene.GetModels().size())
		return scene.GetModels()[selIndex]->position;
	if (selType == SelType::Character && selIndex >= 0 && selIndex < (int)scene.GetCharacters().size())
		return scene.GetCharacters()[selIndex]->position;
	return glm::vec3(0.0f);
}

void Scene3DEditor::SetSelectedPosition(Game& game, const glm::vec3& p)
{
	Scene3D& scene = Scene3D::Get();
	if (selType == SelType::Model && selIndex >= 0 && selIndex < (int)scene.GetModels().size())
	{
		Scene3DModel* m = scene.GetModels()[selIndex];
		m->position = p;
		scene.RecomputeModelBounds(m);   // keep the pick box tracking the model
	}
	else if (selType == SelType::Character && selIndex >= 0 && selIndex < (int)scene.GetCharacters().size())
	{
		scene.GetCharacters()[selIndex]->position = p;
	}
}

bool Scene3DEditor::SelectedAABB(Game& game, glm::vec3& outMin, glm::vec3& outMax) const
{
	Scene3D& scene = Scene3D::Get();
	if (selType == SelType::Model && selIndex >= 0 && selIndex < (int)scene.GetModels().size())
	{
		outMin = scene.GetModels()[selIndex]->aabbMin;
		outMax = scene.GetModels()[selIndex]->aabbMax;
		return true;
	}
	if (selType == SelType::Character && selIndex >= 0 && selIndex < (int)scene.GetCharacters().size())
	{
		CharacterAABB(scene.GetCharacters()[selIndex], outMin, outMax);
		return true;
	}
	return false;
}

// Smoothly move the fly camera to frame the current selection, keeping the
// current viewing direction (dolly in/out along the line to the object).
void Scene3DEditor::ZoomToSelected(Game& game)
{
	glm::vec3 mn, mx;
	if (!SelectedAABB(game, mn, mx))
		return;

	Camera& cam = game.renderer.camera;
	glm::vec3 center = (mn + mx) * 0.5f;
	float radius = glm::length(mx - mn) * 0.5f;
	if (radius < 10.0f) radius = 10.0f;
	// Cap the framed radius so a huge object (a lake / ground plane) doesn't push
	// the camera thousands of units back - past the skybox and far plane - which
	// would leave nothing on screen to edit. Frame part of it instead.
	if (radius > 1200.0f) radius = 1200.0f;

	// Distance so the bounding sphere fits the vertical FOV, plus margin.
	float fov = (cam.fov > 1.0f) ? cam.fov : 60.0f;
	float dist = radius / tanf(glm::radians(fov * 0.5f)) * 1.5f;
	if (dist < 120.0f) dist = 120.0f;
	if (dist > 2600.0f) dist = 2600.0f;   // keep inside the skybox / far clip plane

	glm::vec3 dir = cam.position - center;
	if (glm::length(dir) < 1e-3f) dir = glm::vec3(0, 0, 1);
	dir = glm::normalize(dir);
	glm::vec3 newPos = center + dir * dist;

	// Camera convention: view = lookAt(pos, pos - front). To look AT center,
	// front = pos - center; pitch = asin(front.y), yaw = atan2(front.z, front.x).
	glm::vec3 front = glm::normalize(newPos - center);
	glideToPitch = glm::degrees(asinf(glm::clamp(front.y, -1.0f, 1.0f)));
	glideToYaw = glm::degrees(atan2f(front.z, front.x));
	glideToPos = newPos;

	glideFromPos = cam.position;
	glideFromPitch = cam.pitch;
	glideFromYaw = cam.yaw;
	camGlideElapsed = 0.0f;
	camGliding = true;
}

// ------------------------------------------------------------ object list

void Scene3DEditor::EnsureObjectList(Game& game)
{
	Scene3D& scene = Scene3D::Get();
	int count = (int)(scene.GetModels().size() + scene.GetCharacters().size());
	std::string key = std::to_string((int)selType) + ":" + std::to_string(selIndex);

	// Rebuild only when the object set changes or the selection moves (so the
	// "> " marker follows). Cheap - happens on selection change, not per frame.
	if (listRows.empty() || listBuiltCount != count || key != lastListMarkerKey)
	{
		BuildObjectList(game);
		listBuiltCount = count;
		lastListMarkerKey = key;
	}
}

void Scene3DEditor::BuildObjectList(Game& game)
{
	Scene3D& scene = Scene3D::Get();

	// Build the entry list (header + models + characters) and a label per row.
	listEntries.clear();
	std::vector<std::string> labels;
	std::vector<bool> selected;

	listEntries.push_back({ SelType::None, -1 });      // header
	labels.push_back("SCENE OBJECTS");
	selected.push_back(false);

	const auto& models = scene.GetModels();
	for (size_t i = 0; i < models.size(); i++)
	{
		bool sel = (selType == SelType::Model && selIndex == (int)i);
		listEntries.push_back({ SelType::Model, (int)i });
		labels.push_back((sel ? "> " : "  ") + BaseName(models[i]->objPath));
		selected.push_back(sel);
	}
	const auto& chars = scene.GetCharacters();
	for (size_t i = 0; i < chars.size(); i++)
	{
		bool sel = (selType == SelType::Character && selIndex == (int)i);
		listEntries.push_back({ SelType::Character, (int)i });
		labels.push_back((sel ? "> " : "  ") + chars[i]->charName);
		selected.push_back(sel);
	}

	// Right-side column. GUI space is the fixed design width * MULTIPLIER.
	listX = game.designWidth * Camera::MULTIPLIER - kListWidthGui - kListMarginGui;

	// One Text per row, placed at an explicit Y (kListRowGui pitch) so clicks
	// hit exactly where the row is drawn. Reuse existing Text objects.
	for (size_t i = 0; i < labels.size(); i++)
	{
		if (i >= listRows.size())
		{
			Text* t = new Text(EnsureFont(game));
			t->isRichText = true;
			t->GetSprite()->keepPositionRelativeToCamera = true;
			t->GetSprite()->keepScaleRelativeToCamera = true;
			listRows.push_back(t);
		}
		Text* t = listRows[i];
		Color col = (i == 0) ? Color{ 150, 200, 255, 255 }
			: (selected[i] ? Color{ 255, 235, 120, 255 } : Color{ 225, 225, 225, 255 });
		t->SetText(labels[i], col);
		t->SetScale(glm::vec2(kListScale, kListScale));
		t->SetPosition(listX, kListTopGui + (float)i * kListRowGui);
	}
	// Hide any leftover rows from a previous, longer scene.
	for (size_t i = labels.size(); i < listRows.size(); i++)
		listRows[i]->shouldRender = false;
	for (size_t i = 0; i < labels.size(); i++)
		listRows[i]->shouldRender = true;
}

bool Scene3DEditor::ListClick(Game& game, float sx, float sy)
{
	if (listEntries.empty())
		return false;

	// Map window-pixel mouse to the fixed design GUI space (window px may be a
	// higher resolution than the design space the UI is laid out in).
	float gx = sx * (game.designWidth * Camera::MULTIPLIER) / (float)game.screenWidth;
	float gy = sy * (game.designHeight * Camera::MULTIPLIER) / (float)game.screenHeight;
	if (gx < listX || gx > listX + kListWidthGui || gy < kListTopGui)
		return false;

	int row = (int)((gy - kListTopGui) / kListRowGui);
	if (row < 0 || row >= (int)listEntries.size())
		return false;

	const ListEntry& e = listEntries[row];
	if (e.type == SelType::None)
		return false;  // header

	selType = e.type;
	selIndex = e.index;
	RefreshInfoText(game);
	// Clicking the NAME just selects (so you can edit properties without the
	// view jumping). Only the zoom button at the row's right edge zooms.
	if (gx >= listX + kListWidthGui - kZoomBtnGui)
		ZoomToSelected(game);
	return true;
}

void Scene3DEditor::RenderListZoomButtons(Game& game, const Renderer& renderer)
{
	if (listEntries.empty())
		return;
	if (zoomMarkerText == nullptr)
	{
		zoomMarkerText = new Text(EnsureFont(game));
		zoomMarkerText->isRichText = true;
		zoomMarkerText->GetSprite()->keepPositionRelativeToCamera = true;
		zoomMarkerText->GetSprite()->keepScaleRelativeToCamera = true;
	}

	const float bx = listX + kListWidthGui - kZoomBtnGui;
	const float bw = kZoomBtnGui - 8.0f;
	const float bh = kListRowGui - 12.0f;
	for (size_t i = 0; i < listEntries.size(); i++)
	{
		if (listEntries[i].type == SelType::None)
			continue;   // header row: no zoom button
		float by = kListTopGui + (float)i * kListRowGui + 4.0f;
		bool sel = (listEntries[i].type == selType && listEntries[i].index == selIndex);
		glm::vec4 bg = sel ? glm::vec4(0.20f, 0.42f, 0.55f, 0.9f)
			: glm::vec4(0.20f, 0.26f, 0.34f, 0.8f);
		DrawFilledRect(game, renderer, bx, by, bw, bh, bg);
		zoomMarkerText->SetText("ZOOM", { 220, 235, 255, 255 });
		zoomMarkerText->SetScale(glm::vec2(0.7f, 0.7f));
		zoomMarkerText->SetPosition(bx + 12.0f, by + 6.0f);
		zoomMarkerText->Render(renderer);
	}
}

FontInfo* Scene3DEditor::EnsureFont(Game& game)
{
	if (editorFont == nullptr)
		editorFont = game.CreateFont(game.menuManager->defaultFontName, kOverlayFontSize);
	return editorFont;
}

void Scene3DEditor::RefreshInfoText(Game& game)
{
	if (infoText == nullptr)
	{
		infoText = new Text(EnsureFont(game));
		infoText->isRichText = true;
		infoText->GetSprite()->keepPositionRelativeToCamera = true;
		infoText->GetSprite()->keepScaleRelativeToCamera = true;
	}

	Scene3D& scene = Scene3D::Get();
	std::string info;

	if (selType == SelType::Model && selIndex >= 0 && selIndex < (int)scene.GetModels().size())
	{
		Scene3DModel* m = scene.GetModels()[selIndex];
		glm::vec3 p = m->position;
		glm::vec3 es = m->EffectiveScale();
		std::string rot = "yaw " + F(m->yawDeg);
		if (m->pitchDeg != 0.0f || m->rollDeg != 0.0f)
			rot += " pitch " + F(m->pitchDeg) + " roll " + F(m->rollDeg);
		std::string scl = (m->scaleAxis == glm::vec3(1.0f))
			? ("scale " + F(m->modelScale))
			: ("scale (" + F(es.x) + ", " + F(es.y) + ", " + F(es.z) + ")");
		info = "MODEL  " + BaseName(m->objPath) + "\n"
			+ "pos (" + F(p.x) + ", " + F(p.y) + ", " + F(p.z) + ")\n"
			+ rot + "  " + scl
			+ (m->solid ? "  [solid]" : "")
			+ (m->interactionTag.empty() ? "" : ("\ntag: " + m->interactionTag))
			+ (m->materialName.empty() ? "" : ("\nmat: " + m->materialName));
	}
	else if (selType == SelType::Character && selIndex >= 0 && selIndex < (int)scene.GetCharacters().size())
	{
		Character3D* c = scene.GetCharacters()[selIndex];
		glm::vec3 p = c->position;
		info = "CHARACTER  " + c->charName + " (" + c->mode + ")\n"
			+ "pos (" + F(p.x) + ", " + F(p.y) + ", " + F(p.z) + ")\n"
			+ "height " + F(c->worldHeight);
	}
	else
	{
		info = "(nothing selected)";
	}

	const char* modeName[] = { "MOVE", "ROTATE", "SCALE" };
	const char* axisName[] = { "X", "Y", "Z" };
	if (HasSelection())
	{
		info += std::string("\nmode: ") + modeName[(int)xformMode];
		// Show the active axis lock; FREE means ground-slide / yaw / uniform.
		std::string lock = (lockAxis < 0) ? "FREE" : axisName[lockAxis];
		info += "   lock: " + lock;
	}

	if (info != lastInfo)
	{
		infoText->SetText(info, { 255, 255, 255, 255 });
		infoText->SetScale(glm::vec2(kOverlayScale, kOverlayScale));
		lastInfo = info;
	}
	infoText->SetPosition(24.0f, infoPanelY);
}

void Scene3DEditor::Render(Game& game, const Renderer& renderer)
{
	if (!active)
		return;

	Scene3D& scene = Scene3D::Get();

	// Selection box + move gizmo (drawn on top, depth test off).
	if (HasSelection())
	{
		glm::vec3 bmin(0), bmax(0), origin(0);
		if (selType == SelType::Model)
		{
			Scene3DModel* m = scene.GetModels()[selIndex];
			bmin = m->aabbMin; bmax = m->aabbMax; origin = m->position;
		}
		else
		{
			Character3D* c = scene.GetCharacters()[selIndex];
			CharacterAABB(c, bmin, bmax);
			origin = c->position;
		}

		// 12 edges of the AABB (yellow)
		std::vector<glm::vec3> box;
		glm::vec3 c[8] = {
			{bmin.x,bmin.y,bmin.z},{bmax.x,bmin.y,bmin.z},
			{bmax.x,bmax.y,bmin.z},{bmin.x,bmax.y,bmin.z},
			{bmin.x,bmin.y,bmax.z},{bmax.x,bmin.y,bmax.z},
			{bmax.x,bmax.y,bmax.z},{bmin.x,bmax.y,bmax.z} };
		int edges[12][2] = {
			{0,1},{1,2},{2,3},{3,0}, {4,5},{5,6},{6,7},{7,4},
			{0,4},{1,5},{2,6},{3,7} };
		for (auto& e : edges)
		{
			box.push_back(c[e[0]]);
			box.push_back(c[e[1]]);
		}
		DrawLines(game, renderer, box, glm::vec4(1.0f, 0.92f, 0.2f, 1.0f));

		// Move gizmo: axis lines from the object origin. Visual up is -Y, so
		// the Y handle points to -Y. Length scales with the object size.
		float span = glm::length(bmax - bmin);
		float len = std::max(90.0f, span * 0.6f);
		auto axisLine = [&](const glm::vec3& dir, const glm::vec4& col, bool active)
		{
			std::vector<glm::vec3> seg{ origin, origin + dir * len };
			glm::vec4 cc = active ? glm::vec4(1, 1, 1, 1) : col;
			DrawLines(game, renderer, seg, cc);
		};
		// Highlight the currently locked axis (white) so it's obvious which one
		// a drag will edit.
		axisLine(glm::vec3(1, 0, 0), glm::vec4(1.0f, 0.25f, 0.25f, 1.0f), lockAxis == 0);
		axisLine(glm::vec3(0, -1, 0), glm::vec4(0.3f, 1.0f, 0.3f, 1.0f), lockAxis == 1);
		axisLine(glm::vec3(0, 0, 1), glm::vec4(0.35f, 0.5f, 1.0f, 1.0f), lockAxis == 2);
	}

	// Info panel: created here, but positioned + rendered AFTER the button bars
	// below (they set infoPanelY, which depends on whether the water panel shows).
	if (infoText == nullptr)
		RefreshInfoText(game);

	// Object list panel (right-side column of all models + characters)
	EnsureObjectList(game);
	for (size_t i = 0; i < listRows.size(); i++)
		if (listRows[i] != nullptr && listRows[i]->shouldRender)
			listRows[i]->Render(renderer);
	RenderListZoomButtons(game, renderer);

	// Camera list (below the object list, same right column)
	EnsureCameraList(game);
	for (size_t i = 0; i < camListRows.size(); i++)
		if (camListRows[i] != nullptr && camListRows[i]->shouldRender)
			camListRows[i]->Render(renderer);

	// Button bars (transform modes, actions, axis lock), the open dropdown, and
	// the new-scene / clue-tag prompt. Actions must render before the axis row
	// (which anchors under them).
	RenderModeButtons(game, renderer);
	RenderActionButtons(game, renderer);
	RenderAxisButtons(game, renderer);
	RenderResetButtons(game, renderer);
	RenderCameraButtons(game, renderer);
	RenderEditButtons(game, renderer);
	RenderWaterButtons(game, renderer);

	// Object-details text, below the button bars (infoPanelY is now current).
	// Hidden while a water object is selected - the water panel fills that space
	// and already shows the material; the selection box marks the object.
	if (infoText != nullptr && SelectedWater(game) == nullptr)
	{
		infoText->SetPosition(24.0f, infoPanelY);
		infoText->Render(renderer);
	}

	RenderDropdown(game, renderer);
	RenderNamePrompt(game, renderer);

	// Unsaved-changes indicator (top-centre, above the 3D view).
	if (dirtyText == nullptr)
	{
		dirtyText = new Text(EnsureFont(game));
		dirtyText->isRichText = true;
		dirtyText->GetSprite()->keepPositionRelativeToCamera = true;
		dirtyText->GetSprite()->keepScaleRelativeToCamera = true;
	}
	{
		bool dirty = IsDirty();
		dirtyText->SetText(dirty ? "UNSAVED CHANGES  (F5 to save)" : "no unsaved changes",
			dirty ? Color{ 255, 170, 60, 255 } : Color{ 120, 200, 120, 255 });
		dirtyText->SetScale(glm::vec2(0.8f, 0.8f));
		float gw = game.designWidth * Camera::MULTIPLIER;
		// Approx rendered half-width: GetTextWidth()*kBtnWFactor at scale 0.8.
		dirtyText->SetPosition(gw * 0.5f - dirtyText->GetTextWidth() * (kBtnWFactor * 0.8f * 0.5f), 20.0f);
		dirtyText->Render(renderer);
	}

	// Help / status line
	if (helpText == nullptr)
	{
		helpText = new Text(EnsureFont(game));
		helpText->isRichText = true;
		helpText->GetSprite()->keepPositionRelativeToCamera = true;
		helpText->GetSprite()->keepScaleRelativeToCamera = true;
		helpText->SetText(
			"3D EDITOR   [2] exit   RMB look   WASDQE move   wheel dolly   FREE/X/Y/Z lock axis\n"
			"LMB select + drag   Ctrl+Z undo  Ctrl+Y redo   F5 save   F9/RELOAD revert   click a camera to jump",
			{ 255, 255, 255, 255 });
		helpText->SetScale(glm::vec2(kOverlayScale, kOverlayScale));
	}
	if (helpText != nullptr)
	{
		helpText->SetPosition(24, 24);
		helpText->Render(renderer);
	}

	// Transient status flash (save/revert confirmations, etc.)
	if (statusFrames > 0 && !statusMsg.empty())
	{
		if (statusText == nullptr)
		{
			statusText = new Text(EnsureFont(game));
			statusText->isRichText = true;
			statusText->GetSprite()->keepPositionRelativeToCamera = true;
			statusText->GetSprite()->keepScaleRelativeToCamera = true;
		}
		if (statusText != nullptr)
		{
			if (statusMsg != lastStatus)
			{
				statusText->SetText(statusMsg, { 180, 255, 180, 255 });
				statusText->SetScale(glm::vec2(kOverlayScale, kOverlayScale));
				lastStatus = statusMsg;
			}
			statusText->SetPosition(24.0f, (game.designHeight * Camera::MULTIPLIER) - 120.0f);
			statusText->Render(renderer);
		}
	}
}

void Scene3DEditor::EnsureGL()
{
	if (lineShader == nullptr)
	{
		lineShader = new ShaderProgram(-1, "data/shaders/gizmo.vert", "data/shaders/gizmo.frag");
	}
	if (lineVAO == 0)
	{
		glGenVertexArrays(1, &lineVAO);
		glGenBuffers(1, &lineVBO);
		glBindVertexArray(lineVAO);
		glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * 64, nullptr, GL_DYNAMIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
		glBindVertexArray(0);
	}
}

void Scene3DEditor::DrawLines(Game& game, const Renderer& renderer,
	const std::vector<glm::vec3>& segments, const glm::vec4& color)
{
	if (segments.empty())
		return;
	EnsureGL();

	lineShader->UseShader();
	GLuint id = lineShader->GetID();
	glm::mat4 model(1.0f);
	glUniformMatrix4fv(glGetUniformLocation(id, "model"), 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(glGetUniformLocation(id, "view"), 1, GL_FALSE,
		glm::value_ptr(renderer.camera.CalculateViewMatrix()));
	glUniformMatrix4fv(glGetUniformLocation(id, "projection"), 1, GL_FALSE,
		glm::value_ptr(renderer.camera.projection));
	glUniform4fv(glGetUniformLocation(id, "gizmoColor"), 1, glm::value_ptr(color));

	glBindVertexArray(lineVAO);
	glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
	glBufferData(GL_ARRAY_BUFFER, segments.size() * sizeof(glm::vec3),
		segments.data(), GL_DYNAMIC_DRAW);

	GLboolean depthWas = glIsEnabled(GL_DEPTH_TEST);
	glDisable(GL_DEPTH_TEST);
	glDrawArrays(GL_LINES, 0, (GLsizei)segments.size());
	if (depthWas)
		glEnable(GL_DEPTH_TEST);

	glBindVertexArray(0);
}

void Scene3DEditor::DrawFilledRect(Game& game, const Renderer& renderer,
	float x, float y, float w, float h, const glm::vec4& color)
{
	EnsureGL();

	// Two triangles in GUI space, drawn with the GUI ortho projection so the
	// pixel coordinates map straight through (identity model/view).
	glm::vec3 verts[6] = {
		{x,     y,     0.0f}, {x + w, y,     0.0f}, {x + w, y + h, 0.0f},
		{x,     y,     0.0f}, {x + w, y + h, 0.0f}, {x,     y + h, 0.0f}
	};

	lineShader->UseShader();
	GLuint id = lineShader->GetID();
	glm::mat4 ident(1.0f);
	glUniformMatrix4fv(glGetUniformLocation(id, "model"), 1, GL_FALSE, glm::value_ptr(ident));
	glUniformMatrix4fv(glGetUniformLocation(id, "view"), 1, GL_FALSE, glm::value_ptr(ident));
	glUniformMatrix4fv(glGetUniformLocation(id, "projection"), 1, GL_FALSE,
		glm::value_ptr(renderer.camera.guiProjection));
	glUniform4fv(glGetUniformLocation(id, "gizmoColor"), 1, glm::value_ptr(color));

	glBindVertexArray(lineVAO);
	glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_DYNAMIC_DRAW);

	GLboolean depthWas = glIsEnabled(GL_DEPTH_TEST);
	glDisable(GL_DEPTH_TEST);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	if (depthWas)
		glEnable(GL_DEPTH_TEST);

	glBindVertexArray(0);
}

// ------------------------------------------------------- mode button bar

bool Scene3DEditor::ModeButtonClick(Game& game, float sx, float sy)
{
	if (!btnLaidOut)
		return false;
	// Map window-pixel mouse to the fixed design GUI space (window px may be a
	// higher resolution than the design space the UI is laid out in).
	float gx = sx * (game.designWidth * Camera::MULTIPLIER) / (float)game.screenWidth;
	float gy = sy * (game.designHeight * Camera::MULTIPLIER) / (float)game.screenHeight;
	for (int i = 0; i < 3; i++)
	{
		if (gx >= btnX[i] && gx <= btnX[i] + btnW[i]
			&& gy >= btnY[i] && gy <= btnY[i] + btnH[i])
		{
			xformMode = (XformMode)i;
			RefreshInfoText(game);
			return true;
		}
	}
	return false;
}

void Scene3DEditor::RenderModeButtons(Game& game, const Renderer& renderer)
{
	// Create the labels once, then size each button to hug its text.
	for (int i = 0; i < 3; i++)
	{
		if (modeButtonText[i] == nullptr)
		{
			modeButtonText[i] = new Text(EnsureFont(game));
			modeButtonText[i]->isRichText = true;
			modeButtonText[i]->GetSprite()->keepPositionRelativeToCamera = true;
			modeButtonText[i]->GetSprite()->keepScaleRelativeToCamera = true;
			modeButtonText[i]->SetText(kModeNames[i], { 255, 255, 255, 255 });
			modeButtonText[i]->SetScale(glm::vec2(kBtnTextScale, kBtnTextScale));
		}
	}

	// Buttons hug their own label: width from each label's measured text,
	// uniform height from the (constant) glyph height.
	float h = modeButtonText[0]->GetTextHeight() * kBtnHFactor + 2.0f * kBtnPadY;
	float x = kBtnX;
	for (int i = 0; i < 3; i++)
	{
		float tw = modeButtonText[i]->GetTextWidth() * kBtnWFactor;
		btnX[i] = x;
		btnY[i] = kBtnY;
		btnW[i] = tw + 2.0f * kBtnPadX;
		btnH[i] = h;
		x += btnW[i] + kBtnGap;
	}
	btnLaidOut = true;

	for (int i = 0; i < 3; i++)
	{
		bool active = ((int)xformMode == i);
		glm::vec4 bg = active ? glm::vec4(0.20f, 0.45f, 0.85f, 0.9f)
			: glm::vec4(0.14f, 0.14f, 0.16f, 0.8f);
		DrawFilledRect(game, renderer, btnX[i], btnY[i], btnW[i], btnH[i], bg);
		modeButtonText[i]->SetPosition(btnX[i] + kBtnPadX, btnY[i] + kBtnPadY);
		modeButtonText[i]->Render(renderer);
	}
}

bool Scene3DEditor::AxisButtonClick(Game& game, float sx, float sy)
{
	if (!axisBtnLaidOut)
		return false;
	float gx = sx * (game.designWidth * Camera::MULTIPLIER) / (float)game.screenWidth;
	float gy = sy * (game.designHeight * Camera::MULTIPLIER) / (float)game.screenHeight;
	for (int i = 0; i < kNumAxes; i++)
	{
		if (gx >= axisBtnX[i] && gx <= axisBtnX[i] + axisBtnW[i]
			&& gy >= axisBtnY[i] && gy <= axisBtnY[i] + axisBtnH[i])
		{
			lockAxis = i - 1;   // 0=FREE(-1), 1=X(0), 2=Y(1), 3=Z(2)
			RefreshInfoText(game);
			return true;
		}
	}
	return false;
}

void Scene3DEditor::RenderAxisButtons(Game& game, const Renderer& renderer)
{
	for (int i = 0; i < kNumAxes; i++)
	{
		if (axisBtnText[i] == nullptr)
		{
			axisBtnText[i] = new Text(EnsureFont(game));
			axisBtnText[i]->isRichText = true;
			axisBtnText[i]->GetSprite()->keepPositionRelativeToCamera = true;
			axisBtnText[i]->GetSprite()->keepScaleRelativeToCamera = true;
			axisBtnText[i]->SetText(kAxisNames[i], { 255, 255, 255, 255 });
			axisBtnText[i]->SetScale(glm::vec2(kBtnTextScale, kBtnTextScale));
		}
	}

	// Row below the action buttons (which laid out actBtnY/actBtnH first).
	float rowY = actBtnLaidOut ? (actBtnY[0] + actBtnH[0] + kBtnGap) : (kBtnY + 220.0f);
	float h = axisBtnText[0]->GetTextHeight() * kBtnHFactor + 2.0f * kBtnPadY;
	float x = kBtnX;
	for (int i = 0; i < kNumAxes; i++)
	{
		float tw = axisBtnText[i]->GetTextWidth() * kBtnWFactor;
		axisBtnX[i] = x;
		axisBtnY[i] = rowY;
		axisBtnW[i] = tw + 2.0f * kBtnPadX;
		axisBtnH[i] = h;
		x += axisBtnW[i] + kBtnGap;
	}
	axisBtnLaidOut = true;

	// A small "LOCK:" prefix would need its own label; the active highlight
	// reads clearly enough. FREE is green-ish when active; X/Y/Z are tinted
	// red/green/blue so the lock axis is obvious.
	const glm::vec4 activeCol[kNumAxes] = {
		glm::vec4(0.25f, 0.55f, 0.30f, 0.95f),   // FREE
		glm::vec4(0.80f, 0.25f, 0.25f, 0.95f),   // X
		glm::vec4(0.25f, 0.70f, 0.30f, 0.95f),   // Y
		glm::vec4(0.30f, 0.45f, 0.85f, 0.95f),   // Z
	};
	for (int i = 0; i < kNumAxes; i++)
	{
		bool active = (lockAxis == i - 1);
		glm::vec4 bg = active ? activeCol[i] : glm::vec4(0.14f, 0.14f, 0.16f, 0.8f);
		DrawFilledRect(game, renderer, axisBtnX[i], axisBtnY[i], axisBtnW[i], axisBtnH[i], bg);
		axisBtnText[i]->SetPosition(axisBtnX[i] + kBtnPadX, axisBtnY[i] + kBtnPadY);
		axisBtnText[i]->Render(renderer);
	}
}

bool Scene3DEditor::ResetButtonClick(Game& game, float sx, float sy)
{
	if (!resetBtnLaidOut)
		return false;
	float gx = sx * (game.designWidth * Camera::MULTIPLIER) / (float)game.screenWidth;
	float gy = sy * (game.designHeight * Camera::MULTIPLIER) / (float)game.screenHeight;
	for (int i = 0; i < kNumResets; i++)
	{
		if (gx >= resetBtnX[i] && gx <= resetBtnX[i] + resetBtnW[i]
			&& gy >= resetBtnY[i] && gy <= resetBtnY[i] + resetBtnH[i])
		{
			if (!HasSelection())
			{
				statusMsg = "Select an object first";
				statusFrames = 150;
				return true;
			}
			Scene3D& scene = Scene3D::Get();
			Scene3DModel* m = (selType == SelType::Model) ? scene.GetModels()[selIndex] : nullptr;
			Character3D* c = (selType == SelType::Character) ? scene.GetCharacters()[selIndex] : nullptr;

			if (i == 0)   // RESET POS -> scene origin
			{
				SetSelectedPosition(game, glm::vec3(0.0f));
				statusMsg = "Position reset to origin";
			}
			else if (i == 1)   // RESET ROT
			{
				if (m != nullptr)
				{
					m->yawDeg = m->pitchDeg = m->rollDeg = 0.0f;
					scene.RecomputeModelBounds(m);
				}
				statusMsg = "Rotation reset";
			}
			else               // RESET SCALE
			{
				if (m != nullptr)
				{
					m->modelScale = 1.0f;
					m->scaleAxis = glm::vec3(1.0f);
					scene.RecomputeModelBounds(m);
				}
				statusMsg = "Scale reset";
			}
			(void)c;
			statusFrames = 150;
			dragging = false;
			RefreshInfoText(game);
			CommitEdit();
			return true;
		}
	}
	return false;
}

void Scene3DEditor::RenderResetButtons(Game& game, const Renderer& renderer)
{
	for (int i = 0; i < kNumResets; i++)
	{
		if (resetBtnText[i] == nullptr)
		{
			resetBtnText[i] = new Text(EnsureFont(game));
			resetBtnText[i]->isRichText = true;
			resetBtnText[i]->GetSprite()->keepPositionRelativeToCamera = true;
			resetBtnText[i]->GetSprite()->keepScaleRelativeToCamera = true;
			resetBtnText[i]->SetText(kResetNames[i], { 255, 255, 255, 255 });
			resetBtnText[i]->SetScale(glm::vec2(kBtnTextScale, kBtnTextScale));
		}
	}

	// Row below the axis-lock buttons (which laid out axisBtnY/axisBtnH first).
	float rowY = axisBtnLaidOut ? (axisBtnY[0] + axisBtnH[0] + kBtnGap) : (kBtnY + 300.0f);
	float h = resetBtnText[0]->GetTextHeight() * kBtnHFactor + 2.0f * kBtnPadY;
	float x = kBtnX;
	for (int i = 0; i < kNumResets; i++)
	{
		float tw = resetBtnText[i]->GetTextWidth() * kBtnWFactor;
		resetBtnX[i] = x;
		resetBtnY[i] = rowY;
		resetBtnW[i] = tw + 2.0f * kBtnPadX;
		resetBtnH[i] = h;
		x += resetBtnW[i] + kBtnGap;
	}
	resetBtnLaidOut = true;

	for (int i = 0; i < kNumResets; i++)
	{
		// Amber-ish; the reset buttons are momentary (no persistent state).
		glm::vec4 bg = glm::vec4(0.45f, 0.34f, 0.14f, 0.85f);
		DrawFilledRect(game, renderer, resetBtnX[i], resetBtnY[i], resetBtnW[i], resetBtnH[i], bg);
		resetBtnText[i]->SetPosition(resetBtnX[i] + kBtnPadX, resetBtnY[i] + kBtnPadY);
		resetBtnText[i]->Render(renderer);
	}
}

// ------------------------------------------------- camera-management buttons

std::string Scene3DEditor::NextCameraName() const
{
	const std::vector<std::string>& order = Scene3D::Get().CameraOrder();
	for (int n = 1; n < 1000; n++)
	{
		std::string cand = "cam" + std::to_string(n);
		if (std::find(order.begin(), order.end(), cand) == order.end())
			return cand;
	}
	return "cam";
}

void Scene3DEditor::JumpToCameraByName(Game& game, const std::string& name)
{
	Scene3D& scene = Scene3D::Get();
	if (scene.JumpToCamera(game, name))
	{
		currentCamName = name;
		const std::vector<std::string>& order = scene.CameraOrder();
		for (int i = 0; i < (int)order.size(); i++)
			if (order[i] == name) { camCycleIndex = i; break; }
		camGliding = false;   // land the fly camera here (no editor glide fighting it)
	}
}

bool Scene3DEditor::CameraButtonClick(Game& game, float sx, float sy)
{
	if (!camBtnLaidOut)
		return false;
	float gx = sx * (game.designWidth * Camera::MULTIPLIER) / (float)game.screenWidth;
	float gy = sy * (game.designHeight * Camera::MULTIPLIER) / (float)game.screenHeight;
	for (int i = 0; i < kNumCamBtns; i++)
	{
		if (gx >= camBtnX[i] && gx <= camBtnX[i] + camBtnW[i]
			&& gy >= camBtnY[i] && gy <= camBtnY[i] + camBtnH[i])
		{
			Scene3D& scene = Scene3D::Get();
			Camera& cam = game.renderer.camera;
			Scene3D::CamPose pose;
			pose.position = cam.position;
			pose.pitch = cam.pitch;
			pose.yaw = cam.yaw;

			switch (i)
			{
			case 0:   // SAVE CAM - overwrite the SELECTED camera with the current
					  // view; if none is selected, make a new one and select it.
			{
				if (currentCamName.empty())
				{
					currentCamName = NextCameraName();
					scene.AddOrUpdateCamera(currentCamName, pose);
					statusMsg = "Saved new camera '" + currentCamName + "' (F5 to save file)";
				}
				else
				{
					scene.AddOrUpdateCamera(currentCamName, pose);
					statusMsg = "Saved view to '" + currentCamName + "' (F5 to save file)";
				}
				statusFrames = 220;
				break;
			}
			case 1:   // NEW CAM - always add a new camera at the current view
			{
				std::string name = NextCameraName();
				scene.AddOrUpdateCamera(name, pose);
				currentCamName = name;
				statusMsg = "Added new camera '" + name + "' (F5 to save file)";
				statusFrames = 220;
				break;
			}
			case 2:   // SET DEF - make the selected camera the startup one
				if (!currentCamName.empty() && scene.SetDefaultCamera(currentCamName))
				{
					camCycleIndex = 0;
					statusMsg = "'" + currentCamName + "' is now the startup camera (F5 to save file)";
					statusFrames = 220;
				}
				else
				{
					statusMsg = "Pick a camera in the list first";
					statusFrames = 160;
				}
				break;
			case 3:   // DEL CAM
				if (!currentCamName.empty() && scene.RemoveCamera(currentCamName))
				{
					statusMsg = "Deleted camera '" + currentCamName + "' (F5 to save file)";
					statusFrames = 200;
					currentCamName.clear();
					camCycleIndex = -1;
				}
				else
				{
					statusMsg = "Pick a camera first (keep at least 1)";
					statusFrames = 180;
				}
				break;
			}
			CommitEdit();   // camera edits are undoable too
			return true;
		}
	}
	return false;
}

// ------------------------------------------------------ camera list panel

void Scene3DEditor::EnsureCameraList(Game& game)
{
	Scene3D& scene = Scene3D::Get();
	// Rebuild when the camera set, the selection, or the object-list length
	// (which sets our Y offset) changes.
	std::string key = std::to_string(scene.CameraOrder().size()) + "|" + currentCamName
		+ "|" + std::to_string(listEntries.size());
	if (camListRows.empty() || key != camListMarkerKey)
	{
		BuildCameraList(game);
		camListMarkerKey = key;
	}
}

void Scene3DEditor::BuildCameraList(Game& game)
{
	Scene3D& scene = Scene3D::Get();
	const std::vector<std::string>& order = scene.CameraOrder();

	std::vector<std::string> labels;
	std::vector<Color> colors;
	camListRowCam.clear();

	labels.push_back("SCENE CAMERAS");
	colors.push_back(Color{ 150, 200, 255, 255 });
	camListRowCam.push_back("");   // header

	for (size_t i = 0; i < order.size(); i++)
	{
		bool sel = (order[i] == currentCamName);
		bool def = (i == 0);
		std::string lbl = (sel ? "> " : "  ") + order[i] + (def ? "  (start)" : "");
		labels.push_back(lbl);
		colors.push_back(sel ? Color{ 255, 235, 120, 255 } : Color{ 210, 225, 235, 255 });
		camListRowCam.push_back(order[i]);
	}

	// Same right-side column as the object list, placed below it.
	camListX = game.designWidth * Camera::MULTIPLIER - kListWidthGui - kListMarginGui;
	camListTop = kListTopGui + (float)listEntries.size() * kListRowGui + 40.0f;

	for (size_t i = 0; i < labels.size(); i++)
	{
		if (i >= camListRows.size())
		{
			Text* t = new Text(EnsureFont(game));
			t->isRichText = true;
			t->GetSprite()->keepPositionRelativeToCamera = true;
			t->GetSprite()->keepScaleRelativeToCamera = true;
			camListRows.push_back(t);
		}
		Text* t = camListRows[i];
		t->SetText(labels[i], colors[i]);
		t->SetScale(glm::vec2(kListScale, kListScale));
		t->SetPosition(camListX, camListTop + (float)i * kListRowGui);
	}
	for (size_t i = labels.size(); i < camListRows.size(); i++)
		camListRows[i]->shouldRender = false;
	for (size_t i = 0; i < labels.size(); i++)
		camListRows[i]->shouldRender = true;
}

bool Scene3DEditor::CameraListClick(Game& game, float sx, float sy)
{
	if (camListRowCam.empty())
		return false;
	float gx = sx * (game.designWidth * Camera::MULTIPLIER) / (float)game.screenWidth;
	float gy = sy * (game.designHeight * Camera::MULTIPLIER) / (float)game.screenHeight;
	if (gx < camListX || gx > camListX + kListWidthGui || gy < camListTop)
		return false;

	int row = (int)((gy - camListTop) / kListRowGui);
	if (row < 1 || row >= (int)camListRowCam.size())
		return false;   // row 0 is the header

	JumpToCameraByName(game, camListRowCam[row]);
	statusMsg = "Camera: " + camListRowCam[row];
	statusFrames = 160;
	return true;
}

// ------------------------------------------------ undo / redo / dirty state

bool Scene3DEditor::IsDirty() const
{
	return Scene3D::Get().SerializeToString() != savedSnapshot;
}

void Scene3DEditor::ResetHistory()
{
	undoStack.clear();
	redoStack.clear();
	baselineSnapshot = Scene3D::Get().SerializeToString();
	savedSnapshot = baselineSnapshot;
	historyScene = Scene3D::Get().currentScene;
}

void Scene3DEditor::CommitEdit()
{
	// Snapshot the scene now; if it differs from the baseline (the state since
	// the last commit) push the baseline as an undo step. Called at the END of
	// an edit (mouse release / button action), so a drag's intermediate frames
	// collapse into one step.
	std::string cur = Scene3D::Get().SerializeToString();
	if (cur == baselineSnapshot)
		return;
	undoStack.push_back(baselineSnapshot);
	redoStack.clear();
	baselineSnapshot = cur;
	// Cap the history so a long session can't grow without bound.
	const size_t kMaxUndo = 100;
	if (undoStack.size() > kMaxUndo)
		undoStack.erase(undoStack.begin());
}

void Scene3DEditor::LoadSnapshot(Game& game, const std::string& snap)
{
	// Rebuild the scene from a snapshot without moving the editor's fly camera.
	Scene3D::Get().LoadFromString(game, snap, Scene3D::Get().currentScene, false);
	ClearSelection();
	dragging = false;
	openDropdown = DropKind::None;
	RefreshInfoText(game);
}

void Scene3DEditor::Undo(Game& game)
{
	if (undoStack.empty())
	{
		statusMsg = "Nothing to undo";
		statusFrames = 140;
		return;
	}
	redoStack.push_back(baselineSnapshot);
	baselineSnapshot = undoStack.back();
	undoStack.pop_back();
	LoadSnapshot(game, baselineSnapshot);
	statusMsg = "Undo  (" + std::to_string(undoStack.size()) + " left)";
	statusFrames = 160;
}

void Scene3DEditor::Redo(Game& game)
{
	if (redoStack.empty())
	{
		statusMsg = "Nothing to redo";
		statusFrames = 140;
		return;
	}
	undoStack.push_back(baselineSnapshot);
	baselineSnapshot = redoStack.back();
	redoStack.pop_back();
	LoadSnapshot(game, baselineSnapshot);
	statusMsg = "Redo  (" + std::to_string(redoStack.size()) + " left)";
	statusFrames = 160;
}

bool Scene3DEditor::EditButtonClick(Game& game, float sx, float sy)
{
	if (!editBtnLaidOut)
		return false;
	float gx = sx * (game.designWidth * Camera::MULTIPLIER) / (float)game.screenWidth;
	float gy = sy * (game.designHeight * Camera::MULTIPLIER) / (float)game.screenHeight;
	for (int i = 0; i < kNumEditBtns; i++)
	{
		if (gx >= editBtnX[i] && gx <= editBtnX[i] + editBtnW[i]
			&& gy >= editBtnY[i] && gy <= editBtnY[i] + editBtnH[i])
		{
			if (i == 0) Undo(game);
			else if (i == 1) Redo(game);
			else            // RELOAD (discard changes) - same as F9
			{
				if (Scene3D::Get().Reload(game))
				{
					ClearSelection();
					dragging = false;
					ResetHistory();
					statusMsg = "Reloaded from disk (changes discarded)";
					statusFrames = 180;
				}
			}
			return true;
		}
	}
	return false;
}

void Scene3DEditor::RenderEditButtons(Game& game, const Renderer& renderer)
{
	for (int i = 0; i < kNumEditBtns; i++)
	{
		if (editBtnText[i] == nullptr)
		{
			editBtnText[i] = new Text(EnsureFont(game));
			editBtnText[i]->isRichText = true;
			editBtnText[i]->GetSprite()->keepPositionRelativeToCamera = true;
			editBtnText[i]->GetSprite()->keepScaleRelativeToCamera = true;
			editBtnText[i]->SetText(kEditNames[i], { 255, 255, 255, 255 });
			editBtnText[i]->SetScale(glm::vec2(kBtnTextScale, kBtnTextScale));
		}
	}

	float rowY = camBtnLaidOut ? (camBtnY[0] + camBtnH[0] + kBtnGap) : (kBtnY + 460.0f);
	float h = editBtnText[0]->GetTextHeight() * kBtnHFactor + 2.0f * kBtnPadY;
	float x = kBtnX;
	for (int i = 0; i < kNumEditBtns; i++)
	{
		float tw = editBtnText[i]->GetTextWidth() * kBtnWFactor;
		editBtnX[i] = x;
		editBtnY[i] = rowY;
		editBtnW[i] = tw + 2.0f * kBtnPadX;
		editBtnH[i] = h;
		x += editBtnW[i] + kBtnGap;
	}
	editBtnLaidOut = true;

	// This is the last button row: anchor the object-details panel below it.
	infoPanelY = rowY + h + 2.0f * kBtnGap;

	for (int i = 0; i < kNumEditBtns; i++)
	{
		// UNDO/REDO dim when their stack is empty; RELOAD is red-tinted.
		bool avail = (i == 0) ? !undoStack.empty() : (i == 1) ? !redoStack.empty() : true;
		glm::vec4 bg = (i == 2) ? glm::vec4(0.45f, 0.20f, 0.20f, 0.85f)
			: (avail ? glm::vec4(0.24f, 0.24f, 0.30f, 0.9f) : glm::vec4(0.14f, 0.14f, 0.16f, 0.6f));
		DrawFilledRect(game, renderer, editBtnX[i], editBtnY[i], editBtnW[i], editBtnH[i], bg);
		editBtnText[i]->SetPosition(editBtnX[i] + kBtnPadX, editBtnY[i] + kBtnPadY);
		editBtnText[i]->Render(renderer);
	}
}

// ------------------------------------------------- water-surface tuning bar

Scene3DModel* Scene3DEditor::SelectedWater(Game& game) const
{
	if (selType != SelType::Model || selIndex < 0)
		return nullptr;
	const std::vector<Scene3DModel*>& models = Scene3D::Get().GetModels();
	if (selIndex >= (int)models.size())
		return nullptr;
	Scene3DModel* m = models[selIndex];
	return (m != nullptr && m->IsWater()) ? m : nullptr;
}

void Scene3DEditor::RenderWaterButtons(Game& game, const Renderer& renderer)
{
	Scene3DModel* w = SelectedWater(game);
	if (w == nullptr)
	{
		waterPanelLaidOut = false;   // hidden -> not hit-testable
		return;
	}

	auto ensure = [&](Text*& t) {
		if (t == nullptr)
		{
			t = new Text(EnsureFont(game));
			t->isRichText = true;
			t->GetSprite()->keepPositionRelativeToCamera = true;
			t->GetSprite()->keepScaleRelativeToCamera = true;
		}
	};
	ensure(waterMinusText); ensure(waterPlusText);
	ensure(waterLabelText); ensure(waterHeaderText);

	// Fixed button geometry (the [-]/[+] columns share x across all rows).
	waterMinusText->SetText("  -  ", { 255, 255, 255, 255 });
	waterPlusText->SetText("  +  ", { 255, 255, 255, 255 });
	waterMinusText->SetScale(glm::vec2(kBtnTextScale, kBtnTextScale));
	waterPlusText->SetScale(glm::vec2(kBtnTextScale, kBtnTextScale));
	waterBtnW = waterMinusText->GetTextWidth() * kBtnWFactor + 2.0f * kBtnPadX;
	waterRowH = waterMinusText->GetTextHeight() * kBtnHFactor + 2.0f * kBtnPadY;
	waterMinusX = kBtnX;
	waterPlusX = waterMinusX + waterBtnW + kBtnGap;
	const float labelX = waterPlusX + waterBtnW + kBtnGap + 8.0f;
	const float rowPitch = waterRowH + 8.0f;

	// Header, then one row per property below the edit-button bar.
	float topY = editBtnLaidOut ? (editBtnY[0] + editBtnH[0] + kBtnGap) : (kBtnY + 520.0f);
	waterHeaderText->SetText("WATER PROPERTIES", { 150, 220, 255, 255 });
	waterHeaderText->SetScale(glm::vec2(kBtnTextScale, kBtnTextScale));
	waterHeaderText->SetPosition(kBtnX, topY);
	waterHeaderText->Render(renderer);

	float rowY0 = topY + waterRowH + 6.0f;
	for (int i = 0; i < kNumWaterProps; i++)
	{
		float rowY = rowY0 + (float)i * rowPitch;
		waterRowY[i] = rowY;

		// [-] and [+]
		DrawFilledRect(game, renderer, waterMinusX, rowY, waterBtnW, waterRowH,
			glm::vec4(0.40f, 0.22f, 0.22f, 0.92f));
		DrawFilledRect(game, renderer, waterPlusX, rowY, waterBtnW, waterRowH,
			glm::vec4(0.18f, 0.36f, 0.28f, 0.92f));
		waterMinusText->SetPosition(waterMinusX + kBtnPadX, rowY + kBtnPadY);
		waterMinusText->Render(renderer);
		waterPlusText->SetPosition(waterPlusX + kBtnPadX, rowY + kBtnPadY);
		waterPlusText->Render(renderer);

		// NAME: VALUE  (SetText resets scale, so set scale after each SetText)
		char buf[96];
		snprintf(buf, sizeof(buf), "%s: %.2f", kWaterProps[i].name, *WaterField(w->water, i));
		waterLabelText->SetText(buf, { 235, 240, 245, 255 });
		waterLabelText->SetScale(glm::vec2(kBtnTextScale, kBtnTextScale));
		waterLabelText->SetPosition(labelX, rowY + kBtnPadY);
		waterLabelText->Render(renderer);
	}
	waterPanelLaidOut = true;

	// Push the object-details text below the whole panel.
	infoPanelY = rowY0 + (float)kNumWaterProps * rowPitch + kBtnGap;
}

bool Scene3DEditor::WaterButtonClick(Game& game, float sx, float sy)
{
	if (!waterPanelLaidOut)
		return false;
	float gx = sx * (game.designWidth * Camera::MULTIPLIER) / (float)game.screenWidth;
	float gy = sy * (game.designHeight * Camera::MULTIPLIER) / (float)game.screenHeight;

	for (int i = 0; i < kNumWaterProps; i++)
	{
		if (gy < waterRowY[i] || gy > waterRowY[i] + waterRowH)
			continue;
		bool onMinus = (gx >= waterMinusX && gx <= waterMinusX + waterBtnW);
		bool onPlus  = (gx >= waterPlusX  && gx <= waterPlusX  + waterBtnW);
		if (!onMinus && !onPlus)
			return false;

		Scene3DModel* w = SelectedWater(game);
		if (w == nullptr)
			return false;

		const WaterPropDef& pd = kWaterProps[i];
		float* f = WaterField(w->water, i);
		float nv = *f + (onMinus ? -pd.step : pd.step);
		if (nv < pd.lo) nv = pd.lo;
		if (nv > pd.hi) nv = pd.hi;
		*f = nv;

		char msg[96];
		snprintf(msg, sizeof(msg), "%s = %.2f  (F5 to save)", pd.name, nv);
		statusMsg = msg;
		statusFrames = 120;
		RefreshInfoText(game);
		CommitEdit();   // undoable, marks the scene dirty
		return true;
	}
	return false;
}

void Scene3DEditor::RenderCameraButtons(Game& game, const Renderer& renderer)
{
	for (int i = 0; i < kNumCamBtns; i++)
	{
		if (camBtnText[i] == nullptr)
		{
			camBtnText[i] = new Text(EnsureFont(game));
			camBtnText[i]->isRichText = true;
			camBtnText[i]->GetSprite()->keepPositionRelativeToCamera = true;
			camBtnText[i]->GetSprite()->keepScaleRelativeToCamera = true;
			camBtnText[i]->SetText(kCamNames[i], { 255, 255, 255, 255 });
			camBtnText[i]->SetScale(glm::vec2(kBtnTextScale, kBtnTextScale));
		}
	}

	// Row below the reset buttons (laid out first). This is the last button row,
	// so it anchors the object-details panel below it.
	float rowY = resetBtnLaidOut ? (resetBtnY[0] + resetBtnH[0] + kBtnGap) : (kBtnY + 380.0f);
	float h = camBtnText[0]->GetTextHeight() * kBtnHFactor + 2.0f * kBtnPadY;
	float x = kBtnX;
	for (int i = 0; i < kNumCamBtns; i++)
	{
		float tw = camBtnText[i]->GetTextWidth() * kBtnWFactor;
		camBtnX[i] = x;
		camBtnY[i] = rowY;
		camBtnW[i] = tw + 2.0f * kBtnPadX;
		camBtnH[i] = h;
		x += camBtnW[i] + kBtnGap;
	}
	camBtnLaidOut = true;

	bool naming = namingScene && promptMode == PromptMode::CameraName;
	for (int i = 0; i < kNumCamBtns; i++)
	{
		// Teal-ish; ADD brightens while its name prompt is up.
		glm::vec4 bg = (i == 0 && naming) ? glm::vec4(0.18f, 0.62f, 0.62f, 0.95f)
			: glm::vec4(0.14f, 0.34f, 0.40f, 0.85f);
		DrawFilledRect(game, renderer, camBtnX[i], camBtnY[i], camBtnW[i], camBtnH[i], bg);
		camBtnText[i]->SetPosition(camBtnX[i] + kBtnPadX, camBtnY[i] + kBtnPadY);
		camBtnText[i]->Render(renderer);
	}
}

// ------------------------------------ action buttons, dropdowns, naming

void Scene3DEditor::RenderActionButtons(Game& game, const Renderer& renderer)
{
	for (int i = 0; i < kNumActions; i++)
	{
		if (actBtnText[i] == nullptr)
		{
			actBtnText[i] = new Text(EnsureFont(game));
			actBtnText[i]->isRichText = true;
			actBtnText[i]->GetSprite()->keepPositionRelativeToCamera = true;
			actBtnText[i]->GetSprite()->keepScaleRelativeToCamera = true;
			actBtnText[i]->SetText(kActNames[i], { 255, 255, 255, 255 });
			actBtnText[i]->SetScale(glm::vec2(kBtnTextScale, kBtnTextScale));
		}
	}

	// SHADOW button (index 6): dynamic label with the current caster light
	// (AUTO = auto-pick the strongest). SetText resets scale, so re-apply it.
	{
		const std::string& caster = Scene3D::Get().shadowCasterLight;
		actBtnText[6]->SetText("SHADOW: " + (caster.empty() ? std::string("AUTO") : caster),
			{ 255, 255, 255, 255 });
		actBtnText[6]->SetScale(glm::vec2(kBtnTextScale, kBtnTextScale));
	}

	// Second row, just below the mode buttons (which laid out btnY/btnH first).
	float rowY = btnY[0] + btnH[0] + kBtnGap;
	float h = actBtnText[0]->GetTextHeight() * kBtnHFactor + 2.0f * kBtnPadY;
	float x = kBtnX;
	for (int i = 0; i < kNumActions; i++)
	{
		float tw = actBtnText[i]->GetTextWidth() * kBtnWFactor;
		actBtnX[i] = x;
		actBtnY[i] = rowY;
		actBtnW[i] = tw + 2.0f * kBtnPadX;
		actBtnH[i] = h;
		x += actBtnW[i] + kBtnGap;
	}
	actBtnLaidOut = true;

	// 0 DELETE (red when a selection exists), 1 ADD (green, bright when open),
	// 2 NEW (amber, bright while naming), 3 LOAD (blue, bright when open),
	// 4 CLUE (purple; bright while tagging, dim when no model is selected).
	bool tagging = namingScene && promptMode == PromptMode::Tag;
	bool naming = namingScene && promptMode == PromptMode::NewScene;
	glm::vec4 bgs[kNumActions] = {
		HasSelection() ? glm::vec4(0.70f, 0.20f, 0.20f, 0.9f) : glm::vec4(0.20f, 0.14f, 0.14f, 0.7f),
		openDropdown == DropKind::AddModel ? glm::vec4(0.20f, 0.62f, 0.30f, 0.95f) : glm::vec4(0.15f, 0.42f, 0.22f, 0.85f),
		naming ? glm::vec4(0.80f, 0.60f, 0.15f, 0.95f) : glm::vec4(0.45f, 0.36f, 0.14f, 0.85f),
		openDropdown == DropKind::LoadScene ? glm::vec4(0.25f, 0.45f, 0.85f, 0.95f) : glm::vec4(0.18f, 0.30f, 0.55f, 0.85f),
		tagging ? glm::vec4(0.55f, 0.30f, 0.75f, 0.95f)
			: (selType == SelType::Model ? glm::vec4(0.36f, 0.22f, 0.48f, 0.85f) : glm::vec4(0.20f, 0.16f, 0.24f, 0.7f)),
		// 5 MAT (teal; bright when its dropdown is open, dim with no model)
		openDropdown == DropKind::MatSelect ? glm::vec4(0.20f, 0.60f, 0.62f, 0.95f)
			: (selType == SelType::Model ? glm::vec4(0.16f, 0.40f, 0.42f, 0.85f) : glm::vec4(0.16f, 0.24f, 0.24f, 0.7f)),
		// 6 SHADOW (indigo; scene-global point-light shadow caster)
		glm::vec4(0.30f, 0.24f, 0.52f, 0.9f),
	};
	for (int i = 0; i < kNumActions; i++)
	{
		DrawFilledRect(game, renderer, actBtnX[i], actBtnY[i], actBtnW[i], actBtnH[i], bgs[i]);
		actBtnText[i]->SetPosition(actBtnX[i] + kBtnPadX, actBtnY[i] + kBtnPadY);
		actBtnText[i]->Render(renderer);
	}
}

bool Scene3DEditor::ActionButtonClick(Game& game, float sx, float sy)
{
	if (!actBtnLaidOut)
		return false;
	// Map window-pixel mouse to the fixed design GUI space (window px may be a
	// higher resolution than the design space the UI is laid out in).
	float gx = sx * (game.designWidth * Camera::MULTIPLIER) / (float)game.screenWidth;
	float gy = sy * (game.designHeight * Camera::MULTIPLIER) / (float)game.screenHeight;
	for (int i = 0; i < kNumActions; i++)
	{
		if (gx >= actBtnX[i] && gx <= actBtnX[i] + actBtnW[i]
			&& gy >= actBtnY[i] && gy <= actBtnY[i] + actBtnH[i])
		{
			switch (i)
			{
			case 0:
				DeleteSelected(game);
				break;
			case 1:
				if (openDropdown == DropKind::AddModel) openDropdown = DropKind::None;
				else OpenAddDropdown(game);
				break;
			case 2:
				StartNaming(PromptMode::NewScene);
				break;
			case 3:
				if (openDropdown == DropKind::LoadScene) openDropdown = DropKind::None;
				else OpenLoadDropdown(game);
				break;
			case 4:
				// Attach an interaction tag to the selected model (models only).
				if (selType == SelType::Model)
					StartNaming(PromptMode::Tag);
				else
				{
					statusMsg = "Select a model first to tag";
					statusFrames = 150;
				}
				break;
			case 5:
				// Assign a material to the selected model (models only).
				if (selType != SelType::Model)
				{
					statusMsg = "Select a model first to set its material";
					statusFrames = 150;
				}
				else if (openDropdown == DropKind::MatSelect)
					openDropdown = DropKind::None;
				else
					OpenMatDropdown(game);
				break;
			case 6:
			{
				// Cycle the point-light shadow caster: AUTO -> each point light -> AUTO.
				Scene3D& sc = Scene3D::Get();
				std::vector<std::string> names = sc.PointLightNames();
				if (names.empty())
				{
					statusMsg = "No point lights in this scene";
					statusFrames = 150;
					break;
				}
				// Build [AUTO, name0, name1, ...] and advance from the current one.
				int cur = 0;  // 0 = AUTO
				for (int n = 0; n < (int)names.size(); n++)
					if (names[n] == sc.shadowCasterLight) { cur = n + 1; break; }
				int next = (cur + 1) % ((int)names.size() + 1);
				sc.shadowCasterLight = (next == 0) ? std::string() : names[next - 1];
				statusMsg = "Shadow caster: " +
					(sc.shadowCasterLight.empty() ? std::string("AUTO (all lights)") : sc.shadowCasterLight)
					+ "  (F5 to save)";
				statusFrames = 180;
				CommitEdit();
				break;
			}
			}
			return true;
		}
	}
	return false;
}

void Scene3DEditor::DeleteSelected(Game& game)
{
	if (!HasSelection())
		return;
	Scene3D& scene = Scene3D::Get();
	bool ok = (selType == SelType::Model) ? scene.RemoveModel(game, selIndex)
		: scene.RemoveCharacter(game, selIndex);
	if (ok)
	{
		ClearSelection();
		dragging = false;
		statusMsg = "Deleted object";
		statusFrames = 150;
		CommitEdit();
	}
}

int Scene3DEditor::DropdownCount() const
{
	if (openDropdown == DropKind::AddModel) return (int)addPalette.size();
	if (openDropdown == DropKind::LoadScene) return (int)sceneList.size();
	if (openDropdown == DropKind::MatSelect) return (int)matList.size();
	return 0;
}

// Populate dropdownRows with the given labels and mark the dropdown open.
static void FillDropdownRows(std::vector<Text*>& rows, const std::vector<std::string>& labels,
	FontInfo* font, float scale)
{
	for (size_t i = 0; i < labels.size(); i++)
	{
		if (i >= rows.size())
		{
			Text* t = new Text(font);
			t->isRichText = true;
			t->GetSprite()->keepPositionRelativeToCamera = true;
			t->GetSprite()->keepScaleRelativeToCamera = true;
			rows.push_back(t);
		}
		rows[i]->SetText(labels[i], { 235, 235, 235, 255 });
		rows[i]->SetScale(glm::vec2(scale, scale));
		rows[i]->shouldRender = true;
	}
	for (size_t i = labels.size(); i < rows.size(); i++)
		rows[i]->shouldRender = false;
}

void Scene3DEditor::OpenAddDropdown(Game& game)
{
	addPalette = Scene3D::Get().GetModelPalette();
	openDropdown = DropKind::AddModel;
	dropdownAnchor = 1;
	std::vector<std::string> labels;
	for (const auto& d : addPalette) labels.push_back(BaseName(d.obj));
	FillDropdownRows(dropdownRows, labels, EnsureFont(game), kDropScale);
}

void Scene3DEditor::OpenLoadDropdown(Game& game)
{
	sceneList = Scene3D::Get().GetSceneList();
	openDropdown = DropKind::LoadScene;
	dropdownAnchor = 3;
	FillDropdownRows(dropdownRows, sceneList, EnsureFont(game), kDropScale);
}

void Scene3DEditor::OpenMatDropdown(Game& game)
{
	matList.clear();
	matList.push_back("(none)");   // row 0 clears the material
	for (const std::string& n : MaterialLibrary::Get().Names())
		matList.push_back(n);
	openDropdown = DropKind::MatSelect;
	dropdownAnchor = 5;
	FillDropdownRows(dropdownRows, matList, EnsureFont(game), kDropScale);
}

void Scene3DEditor::RenderDropdown(Game& game, const Renderer& renderer)
{
	int count = DropdownCount();
	if (openDropdown == DropKind::None || count == 0)
		return;

	dropdownX = actBtnX[dropdownAnchor];
	dropdownTop = actBtnY[dropdownAnchor] + actBtnH[dropdownAnchor] + 8.0f;

	float bgH = count * kDropRowGui + 16.0f;
	DrawFilledRect(game, renderer, dropdownX - 8.0f, dropdownTop - 8.0f,
		kDropWidthGui, bgH, glm::vec4(0.08f, 0.08f, 0.10f, 0.96f));

	for (int i = 0; i < count && i < (int)dropdownRows.size(); i++)
	{
		dropdownRows[i]->SetPosition(dropdownX, dropdownTop + (float)i * kDropRowGui);
		dropdownRows[i]->Render(renderer);
	}
}

bool Scene3DEditor::DropdownClick(Game& game, float sx, float sy)
{
	int count = DropdownCount();
	if (openDropdown == DropKind::None || count == 0)
		return false;

	// Map window-pixel mouse to the fixed design GUI space (window px may be a
	// higher resolution than the design space the UI is laid out in).
	float gx = sx * (game.designWidth * Camera::MULTIPLIER) / (float)game.screenWidth;
	float gy = sy * (game.designHeight * Camera::MULTIPLIER) / (float)game.screenHeight;
	if (gx < dropdownX - 8.0f || gx > dropdownX - 8.0f + kDropWidthGui || gy < dropdownTop)
		return false;

	int row = (int)((gy - dropdownTop) / kDropRowGui);
	if (row < 0 || row >= count)
		return false;

	Scene3D& scene = Scene3D::Get();
	if (openDropdown == DropKind::LoadScene)
	{
		scene.Load(game, sceneList[row]);
		ClearSelection();
		dragging = false;
		statusMsg = "Loaded " + sceneList[row];
		statusFrames = 150;
		return true;
	}

	if (openDropdown == DropKind::MatSelect)
	{
		if (selType == SelType::Model && selIndex >= 0 && selIndex < (int)scene.GetModels().size())
		{
			Scene3DModel* m = scene.GetModels()[selIndex];
			if (row == 0)   // "(none)"
			{
				m->materialName.clear();
				m->material = nullptr;
				statusMsg = "Cleared material (F5 to save)";
			}
			else
			{
				m->materialName = matList[row];
				m->material = MaterialLibrary::Get().Find(m->materialName);
				statusMsg = "Material: " + m->materialName + " (F5 to save)";
			}
			statusFrames = 180;
			RefreshInfoText(game);
			CommitEdit();
		}
		return true;
	}

	// AddModel: place the new model where the camera looks at the floor (y=0).
	Camera& cam = game.renderer.camera;
	glm::vec3 ro, rd;
	cam.ScreenPointToRay(game.screenWidth * 0.5f, game.screenHeight * 0.5f,
		(float)game.screenWidth, (float)game.screenHeight, ro, rd);
	glm::vec3 pos(0.0f, 0.0f, 0.0f);
	if (std::fabs(rd.y) > 1e-4f)
	{
		float t = (0.0f - ro.y) / rd.y;
		if (t > 0.0f && t < 100000.0f)
			pos = ro + t * rd;
	}
	pos.y = 0.0f;

	Scene3DModel* m = scene.AddModelInstance(game, addPalette[row], pos);
	if (m != nullptr)
	{
		selType = SelType::Model;
		selIndex = (int)scene.GetModels().size() - 1;
		RefreshInfoText(game);
		statusMsg = "Added " + BaseName(addPalette[row].obj);
		statusFrames = 150;
		CommitEdit();
	}
	return true;
}

// ---------------------------------------------------- new-scene naming

void Scene3DEditor::StartNaming(PromptMode mode)
{
	namingScene = true;
	promptMode = mode;
	nameBuffer.clear();
	// When tagging, seed the buffer with the model's existing tag so it can be
	// edited (or cleared to remove the tag).
	if (mode == PromptMode::Tag && selType == SelType::Model)
	{
		Scene3D& scene = Scene3D::Get();
		if (selIndex >= 0 && selIndex < (int)scene.GetModels().size())
			nameBuffer = scene.GetModels()[selIndex]->interactionTag;
	}
	openDropdown = DropKind::None;
	dragging = false;
	// Seed the previous-key snapshot so keys already held don't register.
	const Uint8* keys = SDL_GetKeyboardState(NULL);
	for (int i = 0; i < SDL_NUM_SCANCODES; i++) prevKeys[i] = keys[i];
}

void Scene3DEditor::UpdateNaming(const Uint8* keys, Game& game)
{
	auto edge = [&](int sc) { return keys[sc] && !prevKeys[sc]; };

	// Scene names are lowercase filename-safe; tags are UPPERCASE.
	bool upper = (promptMode == PromptMode::Tag);
	for (int sc = SDL_SCANCODE_A; sc <= SDL_SCANCODE_Z; sc++)
		if (edge(sc) && nameBuffer.size() < 40)
			nameBuffer += (char)((upper ? 'A' : 'a') + (sc - SDL_SCANCODE_A));
	for (int sc = SDL_SCANCODE_1; sc <= SDL_SCANCODE_9; sc++)
		if (edge(sc) && nameBuffer.size() < 40)
			nameBuffer += (char)('1' + (sc - SDL_SCANCODE_1));
	if (edge(SDL_SCANCODE_0) && nameBuffer.size() < 40) nameBuffer += '0';
	if (edge(SDL_SCANCODE_MINUS) && nameBuffer.size() < 40) nameBuffer += '_';
	if (edge(SDL_SCANCODE_BACKSPACE) && !nameBuffer.empty()) nameBuffer.pop_back();

	bool confirm = edge(SDL_SCANCODE_RETURN) || edge(SDL_SCANCODE_KP_ENTER);
	bool cancel = edge(SDL_SCANCODE_ESCAPE);

	for (int i = 0; i < SDL_NUM_SCANCODES; i++) prevKeys[i] = keys[i];

	if (cancel)
	{
		namingScene = false;
		statusMsg = (promptMode == PromptMode::Tag) ? "Tag cancelled"
			: (promptMode == PromptMode::CameraName) ? "Add camera cancelled"
			: "New scene cancelled";
		statusFrames = 120;
		return;
	}
	if (!confirm)
		return;

	if (promptMode == PromptMode::CameraName)
	{
		namingScene = false;
		if (nameBuffer.empty())
		{
			statusMsg = "Camera needs a name";
			statusFrames = 150;
			return;
		}
		// Capture the live fly-camera pose (the editor is frozen while naming,
		// so this is exactly the view the user framed).
		Scene3D& scene = Scene3D::Get();
		Camera& cam = game.renderer.camera;
		Scene3D::CamPose tmp;
		bool existed = scene.GetCameraPose(nameBuffer, tmp);
		Scene3D::CamPose pose;
		pose.position = cam.position;
		pose.pitch = cam.pitch;
		pose.yaw = cam.yaw;
		scene.AddOrUpdateCamera(nameBuffer, pose);
		currentCamName = nameBuffer;
		const std::vector<std::string>& order = scene.CameraOrder();
		for (int i = 0; i < (int)order.size(); i++)
			if (order[i] == nameBuffer) { camCycleIndex = i; break; }
		statusMsg = std::string(existed ? "Updated camera '" : "Added camera '")
			+ nameBuffer + "' at current view (F5 to save)";
		statusFrames = 220;
		return;
	}

	if (promptMode == PromptMode::Tag)
	{
		namingScene = false;
		Scene3D& scene = Scene3D::Get();
		if (selType == SelType::Model && selIndex >= 0 && selIndex < (int)scene.GetModels().size())
		{
			// An empty buffer clears the tag; otherwise apply it. The engine is
			// tag-agnostic (games decide what a tag means), so no validation.
			Scene3DModel* m = scene.GetModels()[selIndex];
			m->interactionTag = nameBuffer;
			m->tagTriggered = false;
			if (nameBuffer.empty())
				statusMsg = "Cleared tag (F5 to save)";
			else
				statusMsg = "Tagged '" + nameBuffer + "' (F5 to save)";
			statusFrames = 220;
			RefreshInfoText(game);
			CommitEdit();
		}
		return;
	}

	// NewScene
	if (!nameBuffer.empty())
	{
		std::string name = nameBuffer;
		namingScene = false;
		if (Scene3D::Get().NewScene(game, name))
		{
			ClearSelection();
			statusMsg = "New scene: " + name + " (add objects, then F5 to save)";
			statusFrames = 220;
		}
		else
		{
			statusMsg = "Could not create scene '" + name + "'";
			statusFrames = 180;
		}
	}
}

void Scene3DEditor::RenderNamePrompt(Game& game, const Renderer& renderer)
{
	if (!namingScene)
		return;
	if (namePromptText == nullptr)
	{
		namePromptText = new Text(EnsureFont(game));
		namePromptText->isRichText = true;
		namePromptText->GetSprite()->keepPositionRelativeToCamera = true;
		namePromptText->GetSprite()->keepScaleRelativeToCamera = true;
	}

	float w = (float)game.designWidth * Camera::MULTIPLIER;
	float boxW = 1200.0f, boxH = 220.0f;
	float boxX = (w - boxW) * 0.5f, boxY = 300.0f;
	DrawFilledRect(game, renderer, boxX, boxY, boxW, boxH, glm::vec4(0.05f, 0.07f, 0.12f, 0.97f));

	std::string txt;
	if (promptMode == PromptMode::Tag)
		txt = "TAG (blank = clear):\n" + nameBuffer + "_\n(Enter = apply,  Esc = cancel)";
	else if (promptMode == PromptMode::CameraName)
		txt = "CAMERA NAME (current view):\n" + nameBuffer + "_\n(Enter = save,  Esc = cancel)";
	else
		txt = "NEW SCENE NAME:\n" + nameBuffer + "_\n(Enter = create,  Esc = cancel)";
	namePromptText->SetText(txt, { 255, 255, 255, 255 });
	namePromptText->SetScale(glm::vec2(kOverlayScale, kOverlayScale));
	namePromptText->SetPosition(boxX + 40.0f, boxY + 30.0f);
	namePromptText->Render(renderer);
}

void Scene3DEditor::Deselect(Game& game)
{
	ClearSelection();
}
