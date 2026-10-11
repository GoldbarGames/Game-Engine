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
#include <glm/gtc/type_ptr.hpp>
#include <SDL2/SDL.h>
#include <cmath>
#include <cstdio>
#include <sstream>
#include <string>
#include <algorithm>
#include <filesystem>
#include "SceneMaterial.h"
#include "Scene3DInternal.h"
#include "render/ColorPipeline.h"
#include "render/AmbientOcclusion.h"
#include "render/ClusteredLights.h"
#include "render/ColorGrading.h"
#include "render/DepthOfField.h"
#include "render/Environment.h"
#include "render/VolumetricFog.h"
#include "render/DistanceFog.h"
#include "render/Multisample.h"
#include "editor/EditorUI.h"
#include "render/Reflections.h"
#include "render/TemporalAA.h"
#include "render/TextureFiles.h"
#include <fstream>
#include <cctype>
#include <functional>
#include <set>

namespace
{
	// --- test switches (screenshot / scripted checks; no effect otherwise) ---
	// KINJO_EDITOR=1: open the editor as soon as a 3D scene is up.
	// KINJO_EDITOR_CLICKS="x,y;x,y;...": left-clicks in window pixels, fed one
	// every few frames through the editor's normal click path (buttons, panels,
	// dropdowns; a click that reaches the scene picks there). "x,y>x2,y2" is a
	// drag instead: press at x,y, move over a few frames, release at x2,y2;
	// "save" presses F5; "name:<text>" types <text> into an open name prompt
	// and presses Enter; "wheel:x,y,n" turns the mouse wheel n notches over
	// x,y, one a frame (negative = towards you: a list scrolls down).
	struct ScriptedClick { int x, y; bool drag; int x2, y2; bool save; std::string typed; int wheel; };
	std::vector<ScriptedClick> scriptedClicks;
	size_t scriptedNext = 0;
	int scriptedWait = 0;
	int scriptedDragFrame = -1;         // the frame of a drag in progress
	bool scriptedSave = false;          // a scripted F5 this frame
	const int kScriptedDragFrames = 8;
	bool scriptLoaded = false;

	void LoadEditorScript()
	{
		if (scriptLoaded)
			return;
		scriptLoaded = true;
		const char* v = std::getenv("KINJO_EDITOR_CLICKS");
		if (v == nullptr)
			return;
		std::stringstream ss(v);
		std::string item;
		while (std::getline(ss, item, ';'))
		{
			ScriptedClick c = { 0, 0, false, 0, 0, false, std::string(), 0 };
			if (item.compare(0, 5, "name:") == 0)
				c.typed = item.substr(5);
			else if (item == "save")
				c.save = true;
			else if (std::sscanf(item.c_str(), "wheel:%d,%d,%d", &c.x, &c.y, &c.wheel) == 3)
			{
				if (c.wheel == 0)
					continue;
			}
			else if (std::sscanf(item.c_str(), "%d,%d>%d,%d", &c.x, &c.y, &c.x2, &c.y2) == 4)
				c.drag = true;
			else if (std::sscanf(item.c_str(), "%d,%d", &c.x, &c.y) != 2)
				continue;
			scriptedClicks.push_back(c);
		}
		scriptedWait = 3;    // let the editor lay its buttons out first (capture runs are only a few frames)
	}

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
	// NOTE: scales recalibrated 2026-08-18 for the glyph-atlas text metrics
	// (rendered size ~84*scale/glyph wide, ~96*scale tall; GetTextWidth cells
	// are ~59px/glyph). The old 1.2/1.0/0.8 scales predate the atlas and drew
	// text ~2.5x bigger than the button/row rects laid out for it.
	const float kOverlayScale = 0.21f;

	// The scales here are absolute. Text::SetScale multiplies by the game's
	// Text::defaultScale, which DB1-derived games (DB2, FOIAQuest...) set to
	// 0.125, so divide it back out or the labels shrink to specks there.
	glm::vec2 EditorTextScale(float s)
	{
		const glm::vec2 d = Text::defaultScale;
		return glm::vec2(d.x != 0.0f ? s / d.x : s, d.y != 0.0f ? s / d.y : s);
	}

	// The object list holds many rows, so it renders smaller than the main
	// overlay to fit them all on screen. It sits in a right-side column.
	const float kListScale = 0.16f;
	const float kListWidthGui = 460.0f;   // also the clickable column width
	const float kListTopGui = 120.0f;
	const float kListMarginGui = 24.0f;
	const float kZoomBtnGui = 90.0f;   // width of the per-row "zoom" button (right edge)
	// Explicit GUI-space row pitch: rows are both drawn and hit-tested at this
	// spacing, so a click always maps to the row the user sees.
	const float kListRowGui = 52.0f;
	// The OBJECTS/CAMERAS tab bar occupies the top row (kListTopGui); the active
	// list's rows start one row below it.
	const float kListContentTop = kListTopGui + kListRowGui;

	// Move / Rotate / Scale button bar (GUI-space layout). Fixed, compact
	// button size with large labels.
	const float kBtnX = 24.0f;
	const float kBtnY = 190.0f;
	const float kBtnGap = 16.0f;
	const float kBtnTextScale = 0.22f;
	const float kBtnPadX = 16.0f;
	const float kBtnPadY = 10.0f;
	// GetTextWidth/Height sum padded glyph textures, so the laid-out text is a
	// fixed fraction of them; these factors convert to on-screen GUI units.
	const float kBtnWFactor = 0.80f;
	const float kBtnHFactor = 0.44f;
	const char* kModeNames[3] = { "MOVE", "ROTATE", "SCALE" };
	const char* kActNames[14] = { "DELETE", "CLONE", "ADD", "NEW", "LOAD", "TAG", "MAT", "SHADOW", "WEATHER", "FOUNTAIN", "SEASON", "SLOT", "MAP", "TILE" };
// Half-size of an anchor's pick / selection box (anchors are points, so give them a
// small cube to click and frame).
static const float kAnchorHalf = 26.0f;
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

	// Editable fountain-jet properties (FOUNTAIN panel). Order matches the
	// read/write switch in RenderFountainButtons / FountainButtonClick.
	struct FountainPropDef { const char* name; float step; float lo; float hi; };
	const FountainPropDef kFountainProps[] = {
		{ "JET SPEED", 20.0f, 100.0f, 900.0f },
		{ "FALL DIST", 10.0f,  40.0f, 500.0f },
		{ "SPREAD",     5.0f,   0.0f, 200.0f },
		{ "DROP SIZE",  1.0f,   2.0f,  30.0f },
		{ "COUNT",     20.0f,  20.0f, 900.0f },
		{ "STRETCH",    0.5f,   0.0f,  12.0f },   // 0 = round dots, higher = longer streaks
	};

	// Read/write fountain property i through Scene3D's accessors (COUNT is int).
	float GetFountainProp(Scene3D& s, int i)
	{
		switch (i)
		{
		case 0:  return s.GetFountainJetSpeed();
		case 1:  return s.GetFountainFallDist();
		case 2:  return s.GetFountainSpread();
		case 3:  return s.GetFountainDropSize();
		case 4:  return (float)s.GetFountainCount();
		default: return s.GetFountainStretch();
		}
	}
	void SetFountainProp(Scene3D& s, int i, float v)
	{
		switch (i)
		{
		case 0:  s.SetFountainJetSpeed(v); break;
		case 1:  s.SetFountainFallDist(v); break;
		case 2:  s.SetFountainSpread(v); break;
		case 3:  s.SetFountainDropSize(v); break;
		case 4:  s.SetFountainCount((int)(v + 0.5f)); break;
		default: s.SetFountainStretch(v); break;
		}
	}

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

	// Centre a button's label inside its rect. Text glyphs are centre-anchored,
	// so SetPosition fixes the label's LEFT edge (x) and its VERTICAL CENTRE (y);
	// GetRenderedWidth/Height give the true on-screen text extent (no fudge
	// factor), so the label lands dead-centre both ways.
	void CenterLabel(Text* t, float bx, float by, float bw, float bh)
	{
		t->SetPosition(bx + (bw - t->GetRenderedWidth()) * 0.5f, by + bh * 0.5f);
	}

	// "Add model" dropdown geometry.
	const float kDropRowGui = 48.0f;
	const float kDropWidthGui = 380.0f;
	const float kDropScale = 0.17f;

	// --- LOOK panel: the scene's rendering look ----------------------------
	// Its state lives here rather than in the class: Scene3DEditor is exported
	// and games embed it by value, so a new data member would change its size.
	enum class LookProp
	{
		Exposure, Bloom, SkyLight, SkyShine, Ao, AoRadius,
		Shadows, Fog, FogFalloff, FogGlow, FogDrift, FogRed, FogGreen, FogBlue, Weather,
		DistFogNear, DistFogFar, DistFogRed, DistFogGreen, DistFogBlue,
		Focus, Aperture, PickFocus, Grade, Lut,
		Sky, AoView, LightCount, SkySize, Project, Toon,
	};
	// A row is a value with [-] [+] or a full-width button. A value steps by
	// adding `step`, or by multiplying when `mul` (then `floor` is where + starts
	// from zero, and - below it drops back to zero).
	struct LookRowDef
	{
		int column;
		LookProp prop;
		const char* name;
		bool button;
		float step;
		bool mul;
		float lo, hi, floor;
		const char* fmt;
	};
	const LookRowDef kLookRows[] = {
		{ 0, LookProp::Exposure,   "EXPOSURE",    false, 1.1f,   true,  0.05f,  20.0f,    0.0f,    "%.2f" },
		{ 0, LookProp::Bloom,      "BLOOM",       false, 0.01f,  false, 0.0f,   1.0f,     0.0f,    "%.2f" },
		{ 0, LookProp::SkyLight,   "SKY LIGHT",   false, 0.1f,   false, 0.0f,   4.0f,     0.0f,    "%.1f" },
		{ 0, LookProp::SkyShine,   "SKY SHINE",   false, 0.1f,   false, 0.0f,   4.0f,     0.0f,    "%.1f" },
		{ 0, LookProp::Ao,         "AO",          false, 0.1f,   false, 0.0f,   1.0f,     0.0f,    "%.1f" },
		{ 0, LookProp::AoRadius,   "AO RADIUS",   false, 10.0f,  false, 10.0f,  400.0f,   0.0f,    "%.0f" },
		{ 0, LookProp::Toon,       "TOON & OUTLINE...", true, 0.0f, false, 0.0f,  0.0f,     0.0f,    "" },
		{ 1, LookProp::Fog,        "FOG",         false, 1.25f,  true,  0.0f,   0.02f,    0.0001f, "%.5f" },
		{ 1, LookProp::FogFalloff, "FOG FALLOFF", false, 1.25f,  true,  0.0f,   0.05f,    0.0005f, "%.4f" },
		{ 1, LookProp::FogGlow,    "FOG GLOW",    false, 0.05f,  false, -0.95f, 0.95f,    0.0f,    "%.2f" },
		{ 1, LookProp::FogDrift,   "FOG DRIFT",   false, 0.05f,  false, 0.0f,   1.0f,     0.0f,    "%.2f" },
		{ 1, LookProp::FogRed,     "FOG RED",     false, 0.05f,  false, 0.0f,   1.0f,     0.0f,    "%.2f" },
		{ 1, LookProp::FogGreen,   "FOG GREEN",   false, 0.05f,  false, 0.0f,   1.0f,     0.0f,    "%.2f" },
		{ 1, LookProp::FogBlue,    "FOG BLUE",    false, 0.05f,  false, 0.0f,   1.0f,     0.0f,    "%.2f" },
		{ 1, LookProp::DistFogNear, "DISTFOG NEAR", false, 1.15f, true, 0.0f,   100000.0f, 50.0f,  "%.0f" },
		{ 1, LookProp::DistFogFar, "DISTFOG FAR", false, 1.15f,  true,  0.0f,   100000.0f, 50.0f,  "%.0f" },
		{ 1, LookProp::DistFogRed, "DISTFOG RED", false, 0.05f,  false, 0.0f,   1.0f,     0.0f,    "%.2f" },
		{ 1, LookProp::DistFogGreen, "DISTFOG GREEN", false, 0.05f, false, 0.0f, 1.0f,    0.0f,    "%.2f" },
		{ 1, LookProp::DistFogBlue, "DISTFOG BLUE", false, 0.05f, false, 0.0f,  1.0f,     0.0f,    "%.2f" },
		{ 2, LookProp::Focus,      "FOCUS",       false, 1.15f,  true,  10.0f,  50000.0f, 0.0f,    "%.0f" },
		{ 2, LookProp::Aperture,   "APERTURE",    false, 0.5f,   false, 0.0f,   40.0f,    0.0f,    "%.1f" },
		{ 2, LookProp::PickFocus,  "PICK FOCUS",  true,  0.0f,   false, 0.0f,   0.0f,     0.0f,    "" },
		{ 2, LookProp::Grade,      "GRADE",       false, 0.1f,   false, 0.0f,   1.0f,     0.0f,    "%.1f" },
		{ 2, LookProp::Lut,        "LUT",         true,  0.0f,   false, 0.0f,   0.0f,     0.0f,    "" },
		{ 2, LookProp::Project,    "PROJECT SETTINGS...", true, 0.0f, false, 0.0f, 0.0f,   0.0f,    "" },
		{ 3, LookProp::Sky,        "SKY",         true,  0.0f,   false, 0.0f,   0.0f,     0.0f,    "" },
		{ 3, LookProp::SkySize,    "SKY SIZE",    false, 1.15f,  true,  500.0f, 20000.0f, 0.0f,    "%.0f" },
		{ 3, LookProp::Shadows,    "SHADOWS",     false, 1.25f,  true,  500.0f, 40000.0f, 0.0f,    "%.0f" },
		{ 3, LookProp::Weather,    "WEATHER",     false, 0.1f,   false, 0.1f,   1.0f,     0.0f,    "%.1f" },
		{ 3, LookProp::AoView,     "AO VIEW",     true,  0.0f,   false, 0.0f,   0.0f,     0.0f,    "" },
		{ 3, LookProp::LightCount, "LIGHT COUNT", true,  0.0f,   false, 0.0f,   0.0f,     0.0f,    "" },
	};
	const int kLookRowCount = (int)(sizeof(kLookRows) / sizeof(kLookRows[0]));
	const char* kLookColumnNames[4] = { "LIGHT", "FOG", "CAMERA & GRADE", "SCENE" };

	bool lookPickFocus = false;         // the next click in the scene sets the focus
	// The debug views as they were before the panel first changed them, so
	// closing the editor puts them back (-1 = untouched).
	int lookAoViewBefore = -1;
	int lookLightCountBefore = -1;


	// --- lights -------------------------------------------------------------------
	glm::vec3 dragStartLightDir = glm::vec3(0.0f, 1.0f, 0.0f);   // a spot's aim when a ROTATE drag began
	float dragStartLightRange = 300.0f;                          // a light's range when a SCALE drag began
	const float kLightPickHalf = 30.0f; // half-size of a light marker's pick box

	enum class LightProp
	{
		Intensity, Range, Flash, FlashPhase, Inner, Outer, AimTurn, AimTilt,
		Temp, Red, Green, Blue,
		OnOff, Shadow, Rename, AddPoint, AddSpot, SceneLight, Guard,
		Sun, SunTemp, SunHeading, SunHeight, Ambient, AmbientTemp,
		SunRed, SunGreen, SunBlue, AmbientRed, AmbientGreen, AmbientBlue,
	};
	struct LightStepDef
	{
		LightProp prop;
		const char* name;
		float step;
		bool mul;
		float lo, hi, floor;
		const char* fmt;
	};
	// Temperature rows step through kKelvins instead (step 0).
	const LightStepDef kLightSteps[] = {
		{ LightProp::Intensity,   "INTENSITY",    0.1f,  false, 0.0f,   20.0f,    0.0f,  "%.1f" },
		{ LightProp::Range,       "RANGE",        1.15f, true,  20.0f,  20000.0f, 0.0f,  "%.0f" },
		{ LightProp::Flash,       "FLASH",        0.5f,  false, 0.0f,   12.0f,    0.0f,  "%.1f a second" },
		{ LightProp::FlashPhase,  "FLASH PHASE",  0.05f, false, 0.0f,   0.95f,    0.0f,  "%.2f" },
		{ LightProp::Inner,       "INNER",        1.0f,  false, 1.0f,   89.0f,    0.0f,  "%.0f deg" },
		{ LightProp::Outer,       "OUTER",        1.0f,  false, 1.0f,   89.0f,    0.0f,  "%.0f deg" },
		{ LightProp::AimTurn,     "AIM TURN",     15.0f, false, -1e9f,  1e9f,     0.0f,  "%.0f deg" },
		{ LightProp::AimTilt,     "AIM TILT",     5.0f,  false, -90.0f, 90.0f,    0.0f,  "%.0f deg" },
		{ LightProp::Temp,        "TEMP",         0.0f,  false, 0.0f,   0.0f,     0.0f,  "%.0f K" },
		{ LightProp::Red,         "RED",          0.05f, false, 0.0f,   2.0f,     0.0f,  "%.2f" },
		{ LightProp::Green,       "GREEN",        0.05f, false, 0.0f,   2.0f,     0.0f,  "%.2f" },
		{ LightProp::Blue,        "BLUE",         0.05f, false, 0.0f,   2.0f,     0.0f,  "%.2f" },
		{ LightProp::Sun,         "SUN",          0.1f,  false, 0.0f,   4.0f,     0.0f,  "%.1f" },
		{ LightProp::SunTemp,     "SUN TEMP",     0.0f,  false, 0.0f,   0.0f,     0.0f,  "%.0f K" },
		{ LightProp::SunHeading,  "SUN HEADING",  15.0f, false, -1e9f,  1e9f,     0.0f,  "%.0f deg" },
		{ LightProp::SunHeight,   "SUN HEIGHT",   5.0f,  false, 5.0f,   90.0f,    0.0f,  "%.0f deg" },
		{ LightProp::Ambient,     "AMBIENT",      1.25f, true,  0.0f,   2.0f,     0.01f, "%.2f" },
		{ LightProp::AmbientTemp, "AMBIENT TEMP", 0.0f,  false, 0.0f,   0.0f,     0.0f,  "%.0f K" },
		{ LightProp::SunRed,      "SUN RED",      0.05f, false, 0.0f,   2.0f,     0.0f,  "%.2f" },
		{ LightProp::SunGreen,    "SUN GREEN",    0.05f, false, 0.0f,   2.0f,     0.0f,  "%.2f" },
		{ LightProp::SunBlue,     "SUN BLUE",     0.05f, false, 0.0f,   2.0f,     0.0f,  "%.2f" },
		{ LightProp::AmbientRed,  "AMBIENT RED",  0.02f, false, 0.0f,   2.0f,     0.0f,  "%.2f" },
		{ LightProp::AmbientGreen,"AMBIENT GREEN",0.02f, false, 0.0f,   2.0f,     0.0f,  "%.2f" },
		{ LightProp::AmbientBlue, "AMBIENT BLUE", 0.02f, false, 0.0f,   2.0f,     0.0f,  "%.2f" },
	};

	const LightStepDef* FindLightStep(LightProp p)
	{
		for (const LightStepDef& d : kLightSteps)
			if (d.prop == p)
				return &d;
		return nullptr;
	}

	// A colour temperature as RGB, brightest channel 1 (Tanner Helland's fit to
	// the blackbody curve).
	glm::vec3 KelvinColor(float kelvin)
	{
		const float t = kelvin / 100.0f;
		float r, g, b;
		if (t <= 66.0f)
		{
			r = 255.0f;
			g = 99.4708025861f * std::log(t) - 161.1195681661f;
		}
		else
		{
			r = 329.698727446f * std::pow(t - 60.0f, -0.1332047592f);
			g = 288.1221695283f * std::pow(t - 60.0f, -0.0755148492f);
		}
		if (t >= 66.0f)
			b = 255.0f;
		else if (t <= 19.0f)
			b = 0.0f;
		else
			b = 138.5177312231f * std::log(t - 10.0f) - 305.0447927307f;
		const glm::vec3 c = glm::clamp(glm::vec3(r, g, b) / 255.0f, 0.0f, 1.0f);
		return c / std::max(std::max(c.r, c.g), std::max(c.b, 1e-4f));
	}

	// Candle 1800 K ... incandescent 2700 ... daylight 6500 ... blue sky 12000.
	const float kKelvins[] = { 1800, 2200, 2700, 3000, 3500, 4000, 4500, 5000, 5500, 6500, 7500, 9000, 12000 };
	const int kKelvinCount = (int)(sizeof(kKelvins) / sizeof(kKelvins[0]));

	float Brightness(const glm::vec3& c)
	{
		return std::max(std::max(c.r, c.g), c.b);
	}

	// The preset nearest a colour's hue, and whether the colour is that preset.
	int NearestKelvin(const glm::vec3& color, bool& exact)
	{
		const float m = Brightness(color);
		const glm::vec3 hue = (m > 1e-5f) ? color / m : glm::vec3(1.0f);
		int best = 9;
		float bestD = 1e9f;
		for (int i = 0; i < kKelvinCount; i++)
		{
			const float d = glm::length(KelvinColor(kKelvins[i]) - hue);
			if (d < bestD)
			{
				bestD = d;
				best = i;
			}
		}
		exact = bestD < 0.02f;
		return best;
	}

	// The next preset up or down, keeping the colour's brightness. A custom
	// colour snaps to its nearest preset first.
	glm::vec3 StepKelvin(const glm::vec3& color, int dir, float& kelvin)
	{
		bool exact = false;
		int i = NearestKelvin(color, exact);
		if (exact)
			i = std::min(std::max(i + dir, 0), kKelvinCount - 1);
		float m = Brightness(color);
		if (m <= 1e-5f)
			m = 1.0f;
		kelvin = kKelvins[i];
		return KelvinColor(kelvin) * m;
	}

	// A direction as turn (degrees about the vertical; 0 = +Z) and tilt
	// (degrees below the horizon: world up is -Y, so +Y points down).
	void DirToTurnTilt(const glm::vec3& d, float& turn, float& tilt)
	{
		const glm::vec3 n = (glm::length(d) > 1e-6f) ? glm::normalize(d) : glm::vec3(0.0f, 1.0f, 0.0f);
		tilt = glm::degrees(std::asin(glm::clamp(n.y, -1.0f, 1.0f)));
		turn = (std::fabs(n.x) + std::fabs(n.z) > 1e-5f) ? glm::degrees(std::atan2(n.x, n.z)) : 0.0f;
	}

	glm::vec3 TurnTiltToDir(float turn, float tilt)
	{
		const float t = glm::radians(turn), p = glm::radians(tilt);
		return glm::vec3(std::cos(p) * std::sin(t), std::sin(p), std::cos(p) * std::cos(t));
	}

	float WrapDegrees(float d)
	{
		d = std::fmod(d, 360.0f);
		if (d < 0.0f)
			d += 360.0f;
		return d;
	}

	// A value row's number (or `text` instead) for the selected light, or the
	// sun / ambient light when `pl` and `sl` are null.
	void ReadLightValue(LightProp p, const ScenePointLight* pl, const SceneSpotLight* sl, const Scene3D& scene,
		float& value, std::string& text)
	{
		value = 0.0f;
		text.clear();
		const glm::vec3 color = pl ? pl->color : sl ? sl->color : glm::vec3(1.0f);
		bool exact = false;
		switch (p)
		{
		case LightProp::Intensity:
			value = pl ? ((pl->flashHz > 0.0f) ? pl->flashPeak : pl->intensity) : sl ? sl->intensity : 0.0f;
			break;
		case LightProp::Range: value = pl ? pl->range : sl ? sl->range : 0.0f; break;
		case LightProp::Flash:
			value = pl ? pl->flashHz : 0.0f;
			if (value <= 0.0f) text = "off";
			break;
		case LightProp::FlashPhase: value = pl ? pl->flashPhase : 0.0f; break;
		case LightProp::Inner: value = sl ? sl->innerDeg : 0.0f; break;
		case LightProp::Outer: value = sl ? sl->outerDeg : 0.0f; break;
		case LightProp::AimTurn:
		case LightProp::AimTilt:
		{
			float turn = 0.0f, tilt = 90.0f;
			if (sl)
				DirToTurnTilt(sl->dir, turn, tilt);
			value = (p == LightProp::AimTurn) ? WrapDegrees(turn) : tilt;
			if (p == LightProp::AimTilt && tilt > 89.5f) text = "90 deg (straight down)";
			break;
		}
		case LightProp::Temp:
			value = kKelvins[NearestKelvin(color, exact)];
			if (!exact) text = "custom";
			break;
		case LightProp::Red: value = color.r; break;
		case LightProp::Green: value = color.g; break;
		case LightProp::Blue: value = color.b; break;
		case LightProp::Sun:
			value = scene.GetDirectionalLight().diffuse;
			if (value <= 0.0f) text = "off";
			break;
		case LightProp::SunTemp:
			value = kKelvins[NearestKelvin(scene.GetDirectionalLight().color, exact)];
			if (!exact) text = "custom";
			break;
		case LightProp::SunHeading:
		case LightProp::SunHeight:
		{
			float turn = 0.0f, tilt = 0.0f;
			DirToTurnTilt(scene.GetDirectionalLight().dir, turn, tilt);
			value = (p == LightProp::SunHeading) ? WrapDegrees(turn) : tilt;
			break;
		}
		case LightProp::Ambient: value = Brightness(scene.GetAmbientLight()); break;
		case LightProp::SunRed: value = scene.GetDirectionalLight().color.r; break;
		case LightProp::SunGreen: value = scene.GetDirectionalLight().color.g; break;
		case LightProp::SunBlue: value = scene.GetDirectionalLight().color.b; break;
		case LightProp::AmbientRed: value = scene.GetAmbientLight().r; break;
		case LightProp::AmbientGreen: value = scene.GetAmbientLight().g; break;
		case LightProp::AmbientBlue: value = scene.GetAmbientLight().b; break;
		case LightProp::AmbientTemp:
			value = kKelvins[NearestKelvin(scene.GetAmbientLight(), exact)];
			if (!exact) text = "custom";
			break;
		default:
			break;
		}
	}

	// --- TOON & OUTLINE (the LOOK page) ---------------------------------------
	enum class ToonKind { OnOff, Value, GameValues, Back };
	struct ToonRowDef
	{
		int column;
		ToonKind kind;
		Scene3DInternal::ToonSetting setting;
		const char* name;
		int component;       // outline colour: 0 red, 1 green, 2 blue
		float step;
		bool mul;
		float lo, hi;
		const char* fmt;
	};
	const ToonRowDef kToonRows[] = {
		{ 0, ToonKind::OnOff,      Scene3DInternal::ToonSetting::CelShading,        "CEL SHADING",  0, 0.0f,  false, 0.0f, 0.0f,  "" },
		{ 0, ToonKind::OnOff,      Scene3DInternal::ToonSetting::Outline,           "OUTLINE",      0, 0.0f,  false, 0.0f, 0.0f,  "" },
		{ 0, ToonKind::OnOff,      Scene3DInternal::ToonSetting::OutlineCharacters, "OUTLINE CHARACTERS", 0, 0.0f, false, 0.0f, 0.0f, "" },
		{ 0, ToonKind::GameValues, Scene3DInternal::ToonSetting::Count,             "THE GAME'S SETTINGS", 0, 0.0f, false, 0.0f, 0.0f, "" },
		{ 0, ToonKind::Back,       Scene3DInternal::ToonSetting::Count,             "BACK TO THE LOOK", 0, 0.0f, false, 0.0f, 0.0f, "" },
		{ 1, ToonKind::Value,      Scene3DInternal::ToonSetting::OutlineWidth,      "WIDTH",        0, 0.5f,  false, 0.5f, 12.0f, "%.1f px" },
		{ 1, ToonKind::Value,      Scene3DInternal::ToonSetting::OutlineDepth,      "EDGES",        0, 1.25f, true,  0.1f, 50.0f, "%.2f" },
		{ 1, ToonKind::Value,      Scene3DInternal::ToonSetting::OutlineColor,      "RED",          0, 0.05f, false, 0.0f, 1.0f,  "%.2f" },
		{ 1, ToonKind::Value,      Scene3DInternal::ToonSetting::OutlineColor,      "GREEN",        1, 0.05f, false, 0.0f, 1.0f,  "%.2f" },
		{ 1, ToonKind::Value,      Scene3DInternal::ToonSetting::OutlineColor,      "BLUE",         2, 0.05f, false, 0.0f, 1.0f,  "%.2f" },
	};
	const int kToonRowCount = (int)(sizeof(kToonRows) / sizeof(kToonRows[0]));

	// The Scene3D field a value row edits.
	float& ToonField(const ToonRowDef& d)
	{
		Scene3D& scene = Scene3D::Get();
		switch (d.setting)
		{
		case Scene3DInternal::ToonSetting::OutlineWidth: return scene.outlineWidth;
		case Scene3DInternal::ToonSetting::OutlineDepth: return scene.outlineDepthThreshold;
		default: return scene.outlineColor[d.component];
		}
	}

	float ToonValue(const ToonRowDef& d)
	{
		return ToonField(d);
	}

	// --- PROJECT (renderer.dat) ------------------------------------------------
	std::set<std::string> projectRestartKeys;   // restart-only keys changed this session
	std::unordered_map<std::string, std::string> projectConfig;   // the file as last read

	enum class ProjectKind { Value, OnOff, Choice, Lut, Back };
	struct ProjectRowDef
	{
		int column;
		ProjectKind kind;
		const char* key;     // renderer.dat key
		const char* name;
		float step;
		bool mul;
		float lo, hi, floor;
		const char* fmt;
		const char* def;     // the engine's value when the key is absent
		bool restart;        // only takes effect after a restart
	};
	const ProjectRowDef kProjectRows[] = {
		{ 0, ProjectKind::OnOff,  "linearLighting",     "LINEAR",           0.0f,  false, 0.0f,  0.0f,     0.0f,    "",     "0",          true },
		{ 0, ProjectKind::Choice, "tonemap",            "TONEMAP",          0.0f,  false, 0.0f,  0.0f,     0.0f,    "",     "agx_punchy", false },
		{ 0, ProjectKind::Value,  "exposure",           "EXPOSURE",         1.1f,  true,  0.05f, 20.0f,    0.0f,    "%.2f", "1",          false },
		{ 0, ProjectKind::Value,  "bloom",              "BLOOM",            0.01f, false, 0.0f,  1.0f,     0.0f,    "%.2f", "0.04",       false },
		{ 0, ProjectKind::Lut,    "colorGrade",         "LUT",              0.0f,  false, 0.0f,  0.0f,     0.0f,    "",     "none",       false },
		{ 0, ProjectKind::Value,  "colorGradeStrength", "GRADE",            0.1f,  false, 0.0f,  1.0f,     0.0f,    "%.1f", "1",          false },
		{ 0, ProjectKind::Back,   "",                   "BACK TO THIS SCENE", 0.0f, false, 0.0f, 0.0f,     0.0f,    "",     "",           false },
		{ 1, ProjectKind::Value,  "ibl",                "SKY LIGHT",        0.1f,  false, 0.0f,  4.0f,     0.0f,    "%.1f", "1",          false },
		{ 1, ProjectKind::Value,  "iblSpecular",        "SKY SHINE",        0.1f,  false, 0.0f,  4.0f,     0.0f,    "%.1f", "",           false },
		{ 1, ProjectKind::Value,  "ao",                 "AO",               0.1f,  false, 0.0f,  1.0f,     0.0f,    "%.1f", "1",          false },
		{ 1, ProjectKind::Value,  "aoRadius",           "AO RADIUS",        10.0f, false, 10.0f, 400.0f,   0.0f,    "%.0f", "60",         false },
		{ 1, ProjectKind::Value,  "shadowCascades",     "SHADOW MAPS",      1.0f,  false, 0.0f,  4.0f,     0.0f,    "%.0f", "4",          false },
		{ 1, ProjectKind::Value,  "shadowDistance",     "SHADOW REACH",     1.25f, true,  500.0f, 40000.0f, 0.0f,   "%.0f", "5000",       false },
		{ 1, ProjectKind::Value,  "shadowSoftness",     "SHADOW SOFTNESS",  0.25f, false, 0.0f,  4.0f,     0.0f,    "%.2f", "1",          false },
		{ 1, ProjectKind::Value,  "shadowMinCasterHeight", "SMALLEST CASTER", 1.25f, true, 0.0f,  1000.0f,  0.01f,   "%.2f", "15",         false },
		{ 2, ProjectKind::Value,  "fog",                "FOG",              1.25f, true,  0.0f,  0.02f,    0.0001f, "%.5f", "0",          false },
		{ 2, ProjectKind::Value,  "fogHeightFalloff",   "FOG FALLOFF",      1.25f, true,  0.0f,  0.05f,    0.0005f, "%.4f", "0.004",      false },
		{ 2, ProjectKind::Value,  "fogAnisotropy",      "FOG GLOW",         0.05f, false, -0.95f, 0.95f,   0.0f,    "%.2f", "0.5",        false },
		{ 2, ProjectKind::Value,  "fogNoise",           "FOG DRIFT",        0.05f, false, 0.0f,  1.0f,     0.0f,    "%.2f", "0",          false },
		{ 2, ProjectKind::Value,  "fogDistance",        "FOG REACH",        1.25f, true,  500.0f, 40000.0f, 0.0f,   "%.0f", "6000",       false },
		{ 2, ProjectKind::OnOff,  "volumetricFog",      "VOLUMETRIC FOG",   0.0f,  false, 0.0f,  0.0f,     0.0f,    "",     "1",          false },
		{ 2, ProjectKind::OnOff,  "distanceFog",        "DISTANCE FOG",     0.0f,  false, 0.0f,  0.0f,     0.0f,    "",     "0",          false },
		{ 2, ProjectKind::Value,  "distanceFogNear",    "DIST. FOG STARTS", 1.15f, true,  0.0f,  100000.0f, 50.0f,  "%.0f", "1000",       false },
		{ 2, ProjectKind::Value,  "distanceFogFar",     "DIST. FOG FULL",   1.15f, true,  0.0f,  100000.0f, 50.0f,  "%.0f", "5000",       false },
		{ 3, ProjectKind::Choice, "antialiasing",       "ANTI-ALIASING",    0.0f,  false, 0.0f,  0.0f,     0.0f,    "",     "taa",        false },
		{ 3, ProjectKind::Choice, "msaa",               "MSAA",             0.0f,  false, 0.0f,  0.0f,     0.0f,    "",     "0",          false },
		{ 3, ProjectKind::OnOff,  "reflections",        "REFLECTIONS",      0.0f,  false, 0.0f,  0.0f,     0.0f,    "",     "1",          false },
		{ 3, ProjectKind::Value,  "reflectionDistance", "REFLECT REACH",    1.25f, true,  50.0f, 20000.0f, 0.0f,    "%.0f", "1500",       false },
		{ 3, ProjectKind::OnOff,  "depthOfField",       "DEPTH OF FIELD",   0.0f,  false, 0.0f,  0.0f,     0.0f,    "",     "1",          false },
		{ 3, ProjectKind::OnOff,  "clusteredLights",    "CLUSTERED LIGHTS", 0.0f,  false, 0.0f,  0.0f,     0.0f,    "",     "1",          false },
		{ 3, ProjectKind::OnOff,  "gpuDriven",          "GPU-DRIVEN",       0.0f,  false, 0.0f,  0.0f,     0.0f,    "",     "1",          true },
		{ 3, ProjectKind::Value,  "anisotropy",         "ANISOTROPY",       2.0f,  true,  1.0f,  16.0f,    0.0f,    "%.0f", "8",          false },
	};
	const int kProjectRowCount = (int)(sizeof(kProjectRows) / sizeof(kProjectRows[0]));

	bool ProjectHas(const ProjectRowDef& d)
	{
		return projectConfig.count(d.key) > 0 && !projectConfig[d.key].empty();
	}

	std::string ProjectValue(const ProjectRowDef& d)
	{
		if (ProjectHas(d))
			return projectConfig[d.key];
		if (std::string(d.key) == "iblSpecular")   // defaults to ibl
			return projectConfig.count("ibl") > 0 ? projectConfig["ibl"] : "1";
		return d.def;
	}

	float ProjectNumber(const ProjectRowDef& d)
	{
		try { return std::stof(ProjectValue(d)); } catch (...) { return 0.0f; }
	}

	std::vector<std::string> ProjectChoices(const ProjectRowDef& d)
	{
		if (std::string(d.key) == "tonemap")
			return { "none", "agx", "agx_punchy", "aces" };
		if (std::string(d.key) == "msaa")
			return { "0", "2", "4", "8" };
		return { "taa", "none" };
	}

	// Set one renderer.dat key: replace its line (or append one), keeping every
	// other line, comment and line ending as it was.
	bool WriteRendererSetting(const std::string& key, const std::string& value, std::string& message)
	{
		const std::string& path = RendererConfigPath();
		std::vector<std::string> lines;
		std::vector<bool> crlf;
		{
			std::ifstream in(path, std::ios::binary);
			std::string line;
			while (std::getline(in, line))
			{
				const bool cr = !line.empty() && line.back() == '\r';
				if (cr)
					line.pop_back();
				lines.push_back(line);
				crlf.push_back(cr);
			}
		}
		bool found = false;
		for (std::string& line : lines)
		{
			std::istringstream ss(line);
			std::string tok;
			if ((ss >> tok) && tok == key)
			{
				line = key + " " + value;
				found = true;
			}
		}
		if (!found)
		{
			lines.push_back(key + " " + value);
			crlf.push_back(!crlf.empty() && crlf.back());
		}
		std::ofstream out(path, std::ios::binary | std::ios::trunc);
		if (!out.is_open())
		{
			message = "cannot write " + path;
			return false;
		}
		for (size_t i = 0; i < lines.size(); i++)
			out << lines[i] << (crlf[i] ? "\r\n" : "\n");
		return true;
	}

	// (Re-reading renderer.dat into everything that can change while running is
	// the engine's ReloadRenderSettingsLive, render/RenderSettings.cpp: a game's
	// own values, Renderer::SetRenderSetting, stay on top of the file's.)

	// --- MATERIAL ---------------------------------------------------------------

	enum class MatProp
	{
		Lighting, Specular, Shininess, Metallic, Roughness, Fresnel, Opacity,
		TintR, TintG, TintB, GlowR, GlowG, GlowB,
		TileU, TileV, NormalMap, NormalStrength, NormalMode,
		Assign, NewMat, Outline, Season,
	};
	struct MatStepDef
	{
		MatProp prop;
		const char* name;
		float step;
		bool mul;
		float lo, hi, floor;
		const char* fmt;
	};
	// Tiling steps by its size instead (step 0).
	const MatStepDef kMatSteps[] = {
		{ MatProp::Specular,       "SPECULAR",        0.05f, false, 0.0f,  4.0f,    0.0f, "%.2f" },
		{ MatProp::Shininess,      "SHININESS",       1.25f, true,  1.0f,  1000.0f, 0.0f, "%.0f" },
		{ MatProp::Metallic,       "METALLIC",        0.05f, false, 0.0f,  1.0f,    0.0f, "%.2f" },
		{ MatProp::Roughness,      "ROUGHNESS",       0.05f, false, 0.02f, 1.0f,    0.0f, "%.2f" },
		{ MatProp::Fresnel,        "FRESNEL",         0.05f, false, 0.0f,  2.0f,    0.0f, "%.2f" },
		{ MatProp::Opacity,        "OPACITY",         0.05f, false, 0.05f, 1.0f,    0.0f, "%.2f" },
		{ MatProp::TintR,          "TINT RED",        0.05f, false, 0.0f,  2.0f,    0.0f, "%.2f" },
		{ MatProp::TintG,          "TINT GREEN",      0.05f, false, 0.0f,  2.0f,    0.0f, "%.2f" },
		{ MatProp::TintB,          "TINT BLUE",       0.05f, false, 0.0f,  2.0f,    0.0f, "%.2f" },
		{ MatProp::GlowR,          "GLOW RED",        0.1f,  false, 0.0f,  20.0f,   0.0f, "%.2f" },
		{ MatProp::GlowG,          "GLOW GREEN",      0.1f,  false, 0.0f,  20.0f,   0.0f, "%.2f" },
		{ MatProp::GlowB,          "GLOW BLUE",       0.1f,  false, 0.0f,  20.0f,   0.0f, "%.2f" },
		{ MatProp::TileU,          "TILE ACROSS",     0.0f,  false, 0.25f, 500.0f,  0.0f, "%.2f" },
		{ MatProp::TileV,          "TILE DOWN",       0.0f,  false, 0.25f, 500.0f,  0.0f, "%.2f" },
		{ MatProp::NormalStrength, "NORMAL DEPTH",    0.1f,  false, 0.0f,  4.0f,    0.0f, "%.1f" },
	};

	const MatStepDef* FindMatStep(MatProp p)
	{
		for (const MatStepDef& d : kMatSteps)
			if (d.prop == p)
				return &d;
		return nullptr;
	}

	// The material field a value row edits.
	float* MatField(SceneMaterial& m, MatProp p)
	{
		switch (p)
		{
		case MatProp::Specular: return &m.specular;
		case MatProp::Shininess: return &m.shininess;
		case MatProp::Metallic: return &m.metallic;
		case MatProp::Roughness: return &m.roughness;
		case MatProp::Fresnel: return &m.fresnel;
		case MatProp::Opacity: return &m.opacity;
		case MatProp::TintR: return &m.tint.x;
		case MatProp::TintG: return &m.tint.y;
		case MatProp::TintB: return &m.tint.z;
		case MatProp::GlowR: return &m.emissive.x;
		case MatProp::GlowG: return &m.emissive.y;
		case MatProp::GlowB: return &m.emissive.z;
		case MatProp::TileU: return &m.uvTile.x;
		case MatProp::TileV: return &m.uvTile.y;
		case MatProp::NormalStrength: return &m.normalStrength;
		default: return nullptr;
		}
	}

	// Tiling steps a quarter at a time up to 2, whole tiles to 10, then 5s.
	float StepTiling(float v, int dir)
	{
		const float ref = (dir > 0) ? v : v - 1e-4f;
		const float step = (ref < 2.0f) ? 0.25f : (ref < 10.0f) ? 1.0f : 5.0f;
		const float nv = std::round((v + dir * step) / step) * step;
		return std::min(std::max(nv, 0.25f), 500.0f);
	}

	const char* LightingName(LightingModel l)
	{
		return l == LightingModel::PBR ? "PBR" : l == LightingModel::Water ? "WATER" : "PHONG";
	}

	bool IsGltfPath(const std::string& path)
	{
		const size_t dot = path.find_last_of('.');
		const std::string ext = (dot == std::string::npos) ? std::string() : path.substr(dot);
		return ext == ".gltf" || ext == ".glb" || ext == ".GLTF" || ext == ".GLB";
	}

	// --- undo snapshots: the scene text plus the material library ------------
	// Material edits join undo and the unsaved flag, so a snapshot carries both.
	// A scene load re-reads materials.txt, so restoring a snapshot re-applies
	// its materials afterwards.
	const std::string kMaterialsMarker = "\n#@materials\n";

	std::string EditorSnapshot()
	{
		return Scene3D::Get().SerializeToString() + kMaterialsMarker + MaterialLibrary::Get().Serialize();
	}

	// Models hold pointers into the material list, which can move when the
	// editor adds a material.
	void RefreshMaterialPointers()
	{
		const MaterialLibrary& library = MaterialLibrary::Get();
		for (Scene3DModel* m : Scene3D::Get().GetModels())
			m->material = m->materialName.empty() ? nullptr : library.Find(m->materialName);
	}

	std::string UniqueLightName(Scene3D& scene, const std::string& base)
	{
		for (int n = 1;; n++)
		{
			const std::string name = base + std::to_string(n);
			bool taken = false;
			for (const ScenePointLight& l : scene.GetPointLights())
				taken = taken || l.name == name;
			for (const SceneSpotLight& l : scene.GetSpotLights())
				taken = taken || l.name == name;
			if (!taken)
				return name;
		}
	}

	// What a value row shows. own = the scene sets it (saved with it); otherwise
	// it is the project's renderer.dat default, drawn grey. text replaces the
	// number (e.g. "off").
	struct LookValue { float value = 0.0f; bool own = false; std::string text; };

	LookValue ReadLook(LookProp p)
	{
		Scene3D& scene = Scene3D::Get();
		LookValue v;
		switch (p)
		{
		case LookProp::Exposure:
			v.value = EffectiveExposure();
			v.own = SceneExposure() > 0.0f;
			break;
		case LookProp::Bloom:
			v.value = EffectiveBloom();
			v.own = SceneBloom() >= 0.0f;
			break;
		case LookProp::SkyLight:
		case LookProp::SkyShine:
		{
			float d = -1.0f, sp = -1.0f, pd = 1.0f, ps = 1.0f;
			GetSceneIBL(d, sp);
			GetProjectIBL(pd, ps);
			const float own = (p == LookProp::SkyLight) ? d : sp;
			v.own = own >= 0.0f;
			v.value = v.own ? own : ((p == LookProp::SkyLight) ? pd : ps);
			if (!scene.HasSky())
				v.text = "no sky";
			break;
		}
		case LookProp::Ao:
		case LookProp::AoRadius:
		{
			float st = -1.0f, r = -1.0f, pst = 1.0f, pr = 60.0f;
			GetSceneAO(st, r);
			GetProjectAO(pst, pr);
			if (p == LookProp::Ao) { v.own = st >= 0.0f; v.value = v.own ? st : pst; }
			else { v.own = r > 0.0f; v.value = v.own ? r : pr; }
			break;
		}
		case LookProp::Shadows:
		{
			const float d = Scene3DInternal::SceneShadowDistance();
			v.own = d > 0.0f;
			v.value = v.own ? d : Scene3DInternal::ProjectShadowDistance();
			break;
		}
		case LookProp::Fog:
		case LookProp::FogFalloff:
		case LookProp::FogGlow:
		case LookProp::FogDrift:
		case LookProp::FogRed:
		case LookProp::FogGreen:
		case LookProp::FogBlue:
		{
			FogSettings f;
			v.own = GetSceneFog(f);
			if (!v.own)
				f = FogInForce();
			v.value = (p == LookProp::Fog) ? f.density : (p == LookProp::FogFalloff) ? f.heightFalloff
				: (p == LookProp::FogGlow) ? f.anisotropy : (p == LookProp::FogDrift) ? f.noise
				: (p == LookProp::FogRed) ? f.color.r : (p == LookProp::FogGreen) ? f.color.g : f.color.b;
			break;
		}
		case LookProp::DistFogNear:
		case LookProp::DistFogFar:
		case LookProp::DistFogRed:
		case LookProp::DistFogGreen:
		case LookProp::DistFogBlue:
		{
			DistanceFogSettings f;
			v.own = GetSceneDistanceFog(f);
			if (!v.own)
				f = DistanceFogInForce();
			v.value = (p == LookProp::DistFogNear) ? f.nearDistance : (p == LookProp::DistFogFar) ? f.farDistance
				: (p == LookProp::DistFogRed) ? f.color.r : (p == LookProp::DistFogGreen) ? f.color.g : f.color.b;
			break;
		}
		case LookProp::Weather:
			v.own = scene.GetWeather() != Scene3D::WeatherType::None;
			v.value = scene.GetWeatherIntensity();
			if (!v.own)
				v.text = "none";
			break;
		case LookProp::Focus:
		case LookProp::Aperture:
		{
			float focus = 0.0f, aperture = 0.0f;
			GetSceneDepthOfField(focus, aperture);
			v.own = aperture > 0.0f;
			v.value = (p == LookProp::Focus) ? focus : aperture;
			if (p == LookProp::Focus && !DepthOfFieldTarget().empty())
				v.text = DepthOfFieldTarget();
			else if (p == LookProp::Aperture && aperture <= 0.0f)
				v.text = "off";
			break;
		}
		case LookProp::SkySize:
			v.own = true;   // the scene's own; there is no project default
			v.value = scene.GetSkyRadius();
			if (!scene.HasSky())
				v.text = "no sky";
			break;
		case LookProp::Grade:
		{
			std::string path, projectPath;
			float strength = 1.0f, projectStrength = 1.0f;
			GetSceneColorGrade(path, strength);
			GetProjectColorGrade(projectPath, projectStrength);
			if (path.empty())
			{
				v.value = projectStrength;
				if (projectPath.empty())
					v.text = "no LUT";
			}
			else
			{
				v.own = true;
				v.value = strength;
				if (path == "none")
					v.text = "off";
			}
			break;
		}
		default:
			break;
		}
		return v;
	}


	std::string LookBaseName(const std::string& path)
	{
		const size_t slash = path.find_last_of("/\\");
		return (slash == std::string::npos) ? path : path.substr(slash + 1);
	}

	std::string LookLower(std::string t)
	{
		for (char& c : t)
			c = (char)std::tolower((unsigned char)c);
		return t;
	}

	// --- the interface's layout and state (GUI units) ----------------------------
	namespace ui = EditorUI;
	const float kBarH = 60.0f;          // toolbar
	const float kStatusH = 46.0f;       // status bar
	const float kInspectorW = 760.0f;   // left
	const float kOutlinerW = 470.0f;    // right
	enum class InspectorTab { Object, Scene, Look, Material, Camera, Project };
	InspectorTab inspectorTab = InspectorTab::Object;
	float tabScroll[6] = { 0, 0, 0, 0, 0, 0 };
	float outlinerScroll = 0.0f;
	bool uiHidden = false;              // Tab: the panels hidden, the whole view showing
	std::string lastSelKey;             // the selection last seen (a new one shows its page)
	std::string pendingDiscard;         // an action waiting for its second click (unsaved changes)
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
		// The LOOK panel's debug views are for editing: put them back.
		lookPickFocus = false;
		if (lookAoViewBefore >= 0)
			SetAmbientOcclusionDebugView(lookAoViewBefore == 1);
		if (lookLightCountBefore >= 0)
			SetClusterDebugView(lookLightCountBefore == 1);
		lookAoViewBefore = lookLightCountBefore = -1;
		pendingDiscard.clear();
		ui::Reset();
		std::cout << "Scene3DEditor: OFF" << std::endl;
	}
}

bool Scene3DEditor::ViewArea(Game& game, float& x, float& y, float& w, float& h) const
{
	if (!active)
		return false;
	const float gw = game.designWidth * Camera::MULTIPLIER;
	const float gh = game.designHeight * Camera::MULTIPLIER;
	if (uiHidden)
	{
		x = y = 0.0f;
		w = gw;
		h = gh;
		return true;
	}
	x = kInspectorW;
	y = kBarH;
	w = gw - kInspectorW - kOutlinerW;
	h = gh - kBarH - kStatusH;
	return true;
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
	static bool autoOpened = false;
	if (!autoOpened && scene.active && !active)
	{
		autoOpened = true;
		const char* open = std::getenv("KINJO_EDITOR");
		if (open != nullptr && open[0] == '1')
			Toggle(game);
	}

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
		// A scripted name (KINJO_EDITOR_CLICKS "name:<text>"): type it, press Enter.
		LoadEditorScript();
		if (scriptedNext < scriptedClicks.size() && !scriptedClicks[scriptedNext].typed.empty() && --scriptedWait <= 0)
		{
			static Uint8 typedKeys[SDL_NUM_SCANCODES];
			std::copy(keys, keys + SDL_NUM_SCANCODES, typedKeys);
			typedKeys[SDL_SCANCODE_RETURN] = 1;
			prevKeys[SDL_SCANCODE_RETURN] = 0;
			nameBuffer = scriptedClicks[scriptedNext].typed;
			std::cout << "Scene3DEditor: scripted name " << nameBuffer << std::endl;
			scriptedNext++;
			scriptedWait = 2;
			UpdateNaming(typedKeys, game);
			return;
		}
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

	// --- selection + drag (left mouse). Skip while looking with RMB. ---
	bool leftPressed = leftDown && !leftWasDown;
	bool leftReleased = !leftDown && leftWasDown;

	// A scripted test click (KINJO_EDITOR_CLICKS) stands in for the mouse. While
	// clicks remain, auto-screenshots wait (a Debug capture takes about a frame's
	// worth of the gap, so the run would otherwise end mid-script).
	LoadEditorScript();
	if (scriptedNext < scriptedClicks.size() && game.autoScreenshots > 0)
		game.screenshotTimer.Start(game.autoScreenshots);
	if (scriptedNext < scriptedClicks.size() && (scriptedDragFrame >= 0 || --scriptedWait <= 0))
	{
		ScriptedClick& c = scriptedClicks[scriptedNext];
		if (c.save)
		{
			scriptedSave = true;
			std::cout << "Scene3DEditor: scripted save" << std::endl;
			scriptedNext++;
			scriptedWait = 2;
		}
		else if (c.wheel != 0)
		{
			// One notch this frame, over x,y, until they're all turned.
			mx = c.x;
			my = c.y;
			wheelUp = c.wheel > 0;
			wheelDown = c.wheel < 0;
			std::cout << "Scene3DEditor: scripted wheel " << (wheelUp ? "up" : "down") << " at " << mx << "," << my << std::endl;
			c.wheel += wheelUp ? -1 : 1;
			if (c.wheel == 0)
			{
				scriptedNext++;
				scriptedWait = 2;
			}
		}
		else if (!c.drag)
		{
			mx = c.x;
			my = c.y;
			leftPressed = true;
			leftDown = true;   // released next frame, like a real click
			std::cout << "Scene3DEditor: scripted click " << mx << "," << my << " (window "
				<< game.screenWidth << "x" << game.screenHeight << ")" << std::endl;
			scriptedNext++;
			scriptedWait = 2;
		}
		else
		{
			// Press, hold while moving, release: the button state is the script's.
			const int f = ++scriptedDragFrame;
			const float t = (float)f / (float)kScriptedDragFrames;
			mx = (int)std::lround(c.x + (c.x2 - c.x) * t);
			my = (int)std::lround(c.y + (c.y2 - c.y) * t);
			leftPressed = (f == 0);
			leftDown = (f < kScriptedDragFrames);
			leftReleased = (f == kScriptedDragFrames);
			if (f == 0)
				std::cout << "Scene3DEditor: scripted drag " << c.x << "," << c.y << " > " << c.x2 << "," << c.y2 << std::endl;
			if (leftReleased)
			{
				scriptedDragFrame = -1;
				scriptedNext++;
				scriptedWait = 2;
			}
		}
	}

	// Tab hides / shows the panels.
	{
		static bool hideWasDown = false;
		const bool hideDown = keys[SDL_SCANCODE_TAB] != 0;
		if (hideDown && !hideWasDown)
			uiHidden = !uiHidden;
		hideWasDown = hideDown;
	}

	// The interface first: a press or wheel it uses doesn't reach the view.
	if (!uiHidden && !rightHeld)
	{
		ui::Input in;
		in.x = mx * (game.designWidth * Camera::MULTIPLIER) / w;
		in.y = my * (game.designHeight * Camera::MULTIPLIER) / h;
		in.pressed = leftPressed;
		in.down = leftDown;
		in.released = leftReleased;
		in.wheel = wheelUp ? 1 : wheelDown ? -1 : 0;
		const ui::Used used = ui::HandleInput(in);
		if (used.press)
			leftPressed = false;
		if (used.wheel)
			wheelUp = wheelDown = false;
	}

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

	if (leftPressed && !rightHeld && lookPickFocus)
	{
		LookFocusAt(game, (float)mx, (float)my);   // LOOK's PICK FOCUS: aim the depth of field
		leftPressed = false;
	}

	if (tileMode)
	{
		// Grid tile editing replaces selection/drag entirely (UI buttons above
		// already consumed their clicks).
		UpdateTileMode(game, leftPressed && !rightHeld, mx, my, w, h, mb);
		leftPressed = false;   // never falls through to PickAt/drag
	}

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
			if (ScenePointLight* pl = SelectedPointLight())
				dragStartLightRange = pl->range;
			if (SceneSpotLight* sl = SelectedSpotLight())
			{
				dragStartLightRange = sl->range;
				dragStartLightDir = sl->dir;
			}

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
			else if (SceneSpotLight* sl = SelectedSpotLight())
			{
				// Aim a spot: FREE drags the aim around (sideways turns it, up and
				// down tilts it); Y lock only turns it, X / Z only tilt it.
				float turn = 0.0f, tilt = 90.0f;
				DirToTurnTilt(dragStartLightDir, turn, tilt);
				const float dx = (mx - pressX) * 0.5f, dy = (my - pressY) * 0.5f;
				if (dragAxis == 1)
					turn += dx;
				else if (dragAxis == 0 || dragAxis == 2)
					tilt += dx;
				else
				{
					turn += dx;
					tilt += dy;
				}
				sl->dir = TurnTiltToDir(turn, std::min(std::max(tilt, -90.0f), 90.0f));
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
			else if (ScenePointLight* pl = SelectedPointLight())
				pl->range = std::max(dragStartLightRange * factor, 20.0f);   // a light scales its reach
			else if (SceneSpotLight* sl = SelectedSpotLight())
				sl->range = std::max(dragStartLightRange * factor, 20.0f);
		}
		RefreshInfoText(game);
	}

	leftWasDown = leftDown;
	lastMouseX = mx;
	lastMouseY = my;

	// --- save / revert / delete keys ---
	bool saveDown = keys[SDL_SCANCODE_F5] != 0 || scriptedSave;
	scriptedSave = false;
	if (saveDown && !saveWasDown)
		SaveAll(game);
	saveWasDown = saveDown;

	bool revertDown = keys[SDL_SCANCODE_F9] != 0;
	if (revertDown && !revertWasDown)
		RevertAll(game);
	revertWasDown = revertDown;

	// Delete/Backspace = same as the DELETE button. Suppressed while typing a
	// name so Backspace edits the text instead of deleting the selection.
	bool deleteDown = (keys[SDL_SCANCODE_DELETE] != 0) || (keys[SDL_SCANCODE_BACKSPACE] != 0);
	if (deleteDown && !deleteWasDown && HasSelection() && !namingScene)
		DeleteSelected(game);
	deleteWasDown = deleteDown;

	// Ctrl+D = CLONE (same as the button).
	static bool cloneWasDown = false;
	bool cloneDown = ctrl && keys[SDL_SCANCODE_D] != 0;
	if (cloneDown && !cloneWasDown && HasSelection() && !namingScene)
		CloneSelected(game);
	cloneWasDown = cloneDown;

	// [ / ] = this scene's exposure down / up 10% (linear workflow; F5 saves it
	// as the scene's `exposure` token, so it also joins undo and the dirty flag).
	static bool exposureDownWas = false;
	static bool exposureUpWas = false;
	const bool exposureDown = keys[SDL_SCANCODE_LEFTBRACKET] != 0;
	const bool exposureUp = keys[SDL_SCANCODE_RIGHTBRACKET] != 0;
	if (((exposureDown && !exposureDownWas) || (exposureUp && !exposureUpWas)) && !namingScene)
	{
		if (!LinearWorkflow())
		{
			statusMsg = "Exposure needs linearLighting 1 in data/config/renderer.dat";
		}
		else
		{
			float e = (scene.GetExposure() > 0.0f) ? scene.GetExposure() : EffectiveExposure();
			e *= exposureUp ? 1.1f : (1.0f / 1.1f);
			e = std::min(std::max(e, 0.05f), 20.0f);
			scene.SetExposure(e);
			std::ostringstream msg;
			msg.setf(std::ios::fixed);
			msg.precision(2);
			msg << "Exposure " << e << "  ([ ] adjust, F5 saves it to the scene)";
			statusMsg = msg.str();
			CommitEdit();
		}
		statusFrames = 180;
	}
	exposureDownWas = exposureDown;
	exposureUpWas = exposureUp;

	// A new selection shows its settings (the MATERIAL page stays put, so you
	// can click through models' materials).
	const std::string selKey = HasSelection() ? std::to_string((int)selType) + ":" + std::to_string(selIndex) : std::string();
	if (selKey != lastSelKey)
	{
		if (!selKey.empty() && inspectorTab != InspectorTab::Material)
			inspectorTab = InspectorTab::Object;
		lastSelKey = selKey;
	}
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

	// Lights first: their markers are small and usually sit inside a lamp or a
	// room, whose boxes would otherwise always be hit first.
	const glm::vec3 lh(kLightPickHalf);
	for (size_t i = 0; i < scene.GetPointLights().size(); i++)
	{
		float t;
		const glm::vec3 c = scene.GetPointLights()[i].pos;
		if (RayAABB(ro, rd, c - lh, c + lh, t) && t < bestT)
		{
			bestT = t;
			bestType = SelType::PointLight;
			bestIndex = (int)i;
		}
	}
	for (size_t i = 0; i < scene.GetSpotLights().size(); i++)
	{
		float t;
		const glm::vec3 c = scene.GetSpotLights()[i].pos;
		if (RayAABB(ro, rd, c - lh, c + lh, t) && t < bestT)
		{
			bestT = t;
			bestType = SelType::SpotLight;
			bestIndex = (int)i;
		}
	}
	if (bestIndex >= 0)
	{
		selType = bestType;
		selIndex = bestIndex;
		RefreshInfoText(game);
		return;
	}

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

	const auto& anchors = scene.Anchors();
	for (size_t i = 0; i < anchors.size(); i++)
	{
		glm::vec3 c = anchors[i].position;
		glm::vec3 h(kAnchorHalf);
		float t;
		if (RayAABB(ro, rd, c - h, c + h, t) && t < bestT)
		{
			bestT = t;
			bestType = SelType::Anchor;
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
	if (selType == SelType::Anchor && selIndex >= 0 && selIndex < (int)scene.Anchors().size())
		return scene.Anchors()[selIndex].position;
	if (const ScenePointLight* pl = SelectedPointLight())
		return pl->pos;
	if (const SceneSpotLight* sl = SelectedSpotLight())
		return sl->pos;
	return glm::vec3(0.0f);
}

ScenePointLight* Scene3DEditor::SelectedPointLight() const
{
	std::vector<ScenePointLight>& lights = Scene3D::Get().GetPointLights();
	return (selType == SelType::PointLight && selIndex >= 0 && selIndex < (int)lights.size()) ? &lights[selIndex] : nullptr;
}

SceneSpotLight* Scene3DEditor::SelectedSpotLight() const
{
	std::vector<SceneSpotLight>& lights = Scene3D::Get().GetSpotLights();
	return (selType == SelType::SpotLight && selIndex >= 0 && selIndex < (int)lights.size()) ? &lights[selIndex] : nullptr;
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
	else if (selType == SelType::Anchor && selIndex >= 0 && selIndex < (int)scene.Anchors().size())
	{
		scene.Anchors()[selIndex].position = p;
	}
	else if (ScenePointLight* pl = SelectedPointLight())
	{
		pl->pos = p;
	}
	else if (SceneSpotLight* sl = SelectedSpotLight())
	{
		sl->pos = p;
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
	if (selType == SelType::Anchor && selIndex >= 0 && selIndex < (int)scene.Anchors().size())
	{
		glm::vec3 c = scene.Anchors()[selIndex].position;
		outMin = c - glm::vec3(kAnchorHalf);
		outMax = c + glm::vec3(kAnchorHalf);
		return true;
	}
	if (SelectedPointLight() != nullptr || SelectedSpotLight() != nullptr)
	{
		const glm::vec3 c = SelectedPosition(game);
		outMin = c - glm::vec3(kLightPickHalf);
		outMax = c + glm::vec3(kLightPickHalf);
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


FontInfo* Scene3DEditor::EnsureFont(Game& game)
{
	if (editorFont == nullptr)
		editorFont = game.CreateFont(game.menuManager->defaultFontName, kOverlayFontSize);
	return editorFont;
}

void Scene3DEditor::RefreshInfoText(Game& game)
{
	// The OBJECT page reads the selection live each frame: nothing to rebuild.
	(void)game;
}

void Scene3DEditor::Render(Game& game, const Renderer& renderer)
{
	if (!active)
		return;

	Scene3D& scene = Scene3D::Get();

	// Tile-mode hover highlight (grid cell under the mouse).
	if (tileMode)
		RenderTileHighlight(game, renderer);

	// Light markers: every point and spot light, the selected one with its reach.
	RenderLightGizmos(game, renderer);

	// Selection box + move gizmo (drawn on top, depth test off).
	glm::vec3 bmin(0), bmax(0);
	if (HasSelection() && SelectedAABB(game, bmin, bmax))
	{
		const glm::vec3 origin = SelectedPosition(game);

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
		renderer.DrawLines3D(box, glm::vec4(1.0f, 0.92f, 0.2f, 1.0f));

		// Move gizmo: axis lines from the object origin. Visual up is -Y, so
		// the Y handle points to -Y. Length scales with the object size.
		float span = glm::length(bmax - bmin);
		float len = std::max(90.0f, span * 0.6f);
		auto axisLine = [&](const glm::vec3& dir, const glm::vec4& col, bool active)
		{
			std::vector<glm::vec3> seg{ origin, origin + dir * len };
			glm::vec4 cc = active ? glm::vec4(1, 1, 1, 1) : col;
			renderer.DrawLines3D(seg, cc);
		};
		// Highlight the currently locked axis (white) so it's obvious which one
		// a drag will edit.
		axisLine(glm::vec3(1, 0, 0), glm::vec4(1.0f, 0.25f, 0.25f, 1.0f), lockAxis == 0);
		axisLine(glm::vec3(0, -1, 0), glm::vec4(0.3f, 1.0f, 0.3f, 1.0f), lockAxis == 1);
		axisLine(glm::vec3(0, 0, 1), glm::vec4(0.35f, 0.5f, 1.0f, 1.0f), lockAxis == 2);
	}

	// Anchor markers (schedule stand-points): a small cyan post + base cross + a
	// tick pointing the placed character's facing, drawn for every anchor so the
	// designer can see and click them. The selected one also gets the yellow box.
	{
		const auto& anchors = scene.Anchors();
		std::vector<glm::vec3> seg;
		for (const Scene3D::SceneAnchor& a : anchors)
		{
			glm::vec3 p = a.position;
			glm::vec3 top = p + glm::vec3(0.0f, -70.0f, 0.0f);   // up = -Y
			seg.push_back(p);   seg.push_back(top);              // post
			seg.push_back(p - glm::vec3(kAnchorHalf, 0, 0)); seg.push_back(p + glm::vec3(kAnchorHalf, 0, 0));
			seg.push_back(p - glm::vec3(0, 0, kAnchorHalf)); seg.push_back(p + glm::vec3(0, 0, kAnchorHalf));
			float yr = glm::radians(a.yaw);                      // facing tick at the top
			glm::vec3 dir(std::sin(yr), 0.0f, -std::cos(yr));
			seg.push_back(top); seg.push_back(top + dir * 34.0f);
		}
		if (!seg.empty())
			renderer.DrawLines3D(seg, glm::vec4(0.2f, 0.95f, 1.0f, 1.0f));
	}

	// The interface.
	const float gw = game.designWidth * Camera::MULTIPLIER;
	const float gh = game.designHeight * Camera::MULTIPLIER;
	ui::Begin(renderer, EnsureFont(game), gw, gh);
	if (uiHidden)
	{
		const std::string note = "PANELS HIDDEN  -  Tab shows them";
		const float w = ui::TextWidth(note) + 32.0f;
		ui::Button("hidden.show", { gw - w - 12.0f, 10.0f, w, ui::kRowH }, note, []() { uiHidden = false; },
			"Show the editor's panels again (Tab)");
		ui::End();
		return;
	}
	RenderToolbar(game, renderer);
	RenderInspector(game, renderer);
	RenderOutliner(game, renderer);
	RenderMinimap(game, renderer);
	RenderStatusBar(game, renderer);
	RenderNamePrompt(game, renderer);
	ui::End();
}

// ------------------------------------------------------- mode button bar


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


// ------------------------------------------------------ camera list panel

// ------------------------------------------------- object/camera list tabs


// ------------------------------------------------ undo / redo / dirty state

bool Scene3DEditor::IsDirty() const
{
	return EditorSnapshot() != savedSnapshot;
}

void Scene3DEditor::ResetHistory()
{
	undoStack.clear();
	redoStack.clear();
	baselineSnapshot = EditorSnapshot();
	savedSnapshot = baselineSnapshot;
	historyScene = Scene3D::Get().currentScene;
	listScroll = 0;   // start a freshly-loaded scene's list at the top
}

void Scene3DEditor::CommitEdit()
{
	// Snapshot the scene now; if it differs from the baseline (the state since
	// the last commit) push the baseline as an undo step. Called at the END of
	// an edit (mouse release / button action), so a drag's intermediate frames
	// collapse into one step.
	std::string cur = EditorSnapshot();
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
	// Rebuild the scene from a snapshot without moving the editor's fly camera,
	// then its materials (the load re-read them from the file).
	const size_t at = snap.find(kMaterialsMarker);
	Scene3D::Get().LoadFromString(game, (at == std::string::npos) ? snap : snap.substr(0, at),
		Scene3D::Get().currentScene, false);
	if (at != std::string::npos)
	{
		MaterialLibrary::Get().ApplySerialized(game, snap.substr(at + kMaterialsMarker.size()));
		RefreshMaterialPointers();
	}
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


// ------------------------------------------------- fountain-jet tuning bar


// ------------------------------------ action buttons, dropdowns, naming


void Scene3DEditor::DeleteSelected(Game& game)
{
	if (!HasSelection())
		return;
	Scene3D& scene = Scene3D::Get();
	bool ok = false;
	if (ScenePointLight* pl = SelectedPointLight())
	{
		if (scene.shadowCasterLight == pl->name)
			scene.shadowCasterLight.clear();   // back to AUTO
		scene.GetPointLights().erase(scene.GetPointLights().begin() + selIndex);
		ok = true;
	}
	else if (SelectedSpotLight() != nullptr)
	{
		scene.GetSpotLights().erase(scene.GetSpotLights().begin() + selIndex);
		ok = true;
	}
	else
	{
		ok = (selType == SelType::Model) ? scene.RemoveModel(game, selIndex)
			: (selType == SelType::Anchor) ? scene.RemoveAnchorAt(selIndex)
			: (selType == SelType::Character) ? scene.RemoveCharacter(game, selIndex)
			: false;
	}
	if (ok)
	{
		ClearSelection();
		dragging = false;
		statusMsg = "Deleted object";
		statusFrames = 150;
		CommitEdit();
	}
}

void Scene3DEditor::CloneSelected(Game& game)
{
	// A light: a copy one tile over with its own name (scripts address lights by name).
	if (SelectedPointLight() != nullptr || SelectedSpotLight() != nullptr)
	{
		Scene3D& scene = Scene3D::Get();
		if (ScenePointLight* pl = SelectedPointLight())
		{
			ScenePointLight copy = *pl;
			copy.name = UniqueLightName(scene, "light");
			copy.pos.x += 100.0f;
			copy.fade.active = false;
			scene.GetPointLights().push_back(copy);
			selIndex = (int)scene.GetPointLights().size() - 1;
		}
		else
		{
			SceneSpotLight copy = *SelectedSpotLight();
			copy.name = UniqueLightName(scene, "spot");
			copy.pos.x += 100.0f;
			copy.fade.active = false;
			scene.GetSpotLights().push_back(copy);
			selIndex = (int)scene.GetSpotLights().size() - 1;
		}
		RefreshInfoText(game);
		CommitEdit();
		statusMsg = "Cloned light (drag to place, F5 to save)";
		statusFrames = 200;
		return;
	}
	if (selType != SelType::Model || selIndex < 0)
	{
		statusMsg = "Select a model to clone";
		statusFrames = 150;
		return;
	}
	Scene3D& scene = Scene3D::Get();
	Scene3DModel* src = scene.GetModels()[selIndex];

	// One tile over so the copy is visibly separate (and grid-friendly).
	glm::vec3 pos = src->position + glm::vec3(100.0f, 0.0f, 0.0f);
	Scene3D::ModelDef def{ src->objPath, src->texPath, src->solid };
	Scene3DModel* nm = scene.AddModelInstance(game, def, pos);
	if (nm == nullptr)
		return;
	nm->yawDeg = src->yawDeg;
	nm->pitchDeg = src->pitchDeg;
	nm->rollDeg = src->rollDeg;
	nm->modelScale = src->modelScale;
	nm->scaleAxis = src->scaleAxis;
	nm->materialName = src->materialName;
	nm->material = src->material;
	nm->walkable = src->walkable;
	nm->water = src->water;
	nm->guard = src->guard;
	// interactionTag deliberately NOT copied - tags are usually unique lookups
	// (torch_1, volcano_door, ...); a silent duplicate would break their logic.
	scene.RecomputeModelBounds(nm);
	scene.RebuildSolids();

	// Select the clone so it can be dragged into place immediately.
	selType = SelType::Model;
	selIndex = (int)scene.GetModels().size() - 1;
	RefreshInfoText(game);
	CommitEdit();
	statusMsg = src->interactionTag.empty()
		? "Cloned (drag to place)"
		: "Cloned WITHOUT its tag (tags stay unique) - drag to place";
	statusFrames = 200;
}

// --------------------------------------------------------- aerial minimap


void Scene3DEditor::RenderMinimap(Game& game, const Renderer& renderer)
{
	if (!showMinimap)
		return;
	Scene3D& scene = Scene3D::Get();

	const float gw = game.designWidth * Camera::MULTIPLIER;
	const float gh = game.designHeight * Camera::MULTIPLIER;

	// Panel: the view's middle, between the inspector and the object list.
	float panelX = kInspectorW + 16.0f;
	float panelTop = kBarH + 16.0f;
	float panelRight = gw - kOutlinerW - 16.0f;
	float panelBottom = gh - kStatusH - 16.0f;
	float panelW = panelRight - panelX;
	float panelH = panelBottom - panelTop;
	if (panelW < 120.0f || panelH < 120.0f)
		return;
	minimapX = panelX; minimapY = panelTop; minimapW = panelW; minimapH = panelH;

	// Frame + backdrop (a light border rect behind a dark fill).
	renderer.DrawRect(panelX - 2, panelTop - 2, panelW + 4, panelH + 4,
		glm::vec4(0.45f, 0.55f, 0.62f, 0.9f));
	ui::Panel({ panelX, panelTop, panelW, panelH }, glm::vec4(0.05f, 0.06f, 0.08f, 0.94f));

	// World XZ bounds over everything worth showing (+ the live camera so its
	// marker is always on-map).
	float minX = 1e9f, maxX = -1e9f, minZ = 1e9f, maxZ = -1e9f;
	auto addPt = [&](float x, float z) {
		minX = std::min(minX, x); maxX = std::max(maxX, x);
		minZ = std::min(minZ, z); maxZ = std::max(maxZ, z);
	};
	for (Scene3DModel* m : scene.GetModels())
		if (m != nullptr) { addPt(m->aabbMin.x, m->aabbMin.z); addPt(m->aabbMax.x, m->aabbMax.z); }
	for (const Scene3D::SceneAnchor& a : scene.Anchors())
		addPt(a.position.x, a.position.z);
	for (Character3D* c : scene.GetCharacters())
		if (c != nullptr) addPt(c->position.x, c->position.z);
	glm::vec3 camp = game.renderer.camera.position;
	addPt(camp.x, camp.z);
	if (minX > maxX) { minX = -500; maxX = 500; minZ = -500; maxZ = 500; }   // empty scene

	// Pad the bounds a touch, then fit uniformly (preserve aspect) into the inner
	// region below a title strip.
	float span = std::max(maxX - minX, maxZ - minZ);
	float pad = 0.06f * span + 40.0f;
	minX -= pad; maxX += pad; minZ -= pad; maxZ += pad;
	float wSpanX = maxX - minX, wSpanZ = maxZ - minZ;

	const float titleH = 46.0f;
	float innerX = panelX + 14.0f, innerY = panelTop + titleH;
	float innerW = panelW - 28.0f, innerH = panelH - titleH - 14.0f;
	float sc = std::min(innerW / wSpanX, innerH / wSpanZ);
	float offX = innerX + (innerW - wSpanX * sc) * 0.5f;
	float offY = innerY + (innerH - wSpanZ * sc) * 0.5f;
	auto toX = [&](float wx) { return offX + (wx - minX) * sc; };   // world +X -> right
	auto toY = [&](float wz) { return offY + (wz - minZ) * sc; };   // world +Z -> down
	auto dot = [&](float cx, float cy, float half, const glm::vec4& col) {
		renderer.DrawRect(cx - half, cy - half, 2 * half, 2 * half, col);
	};

	// Models: translucent footprints so overlaps read and small props aren't hidden
	// by a big floor. Water bluish; guard-hidden dimmed. Selected drawn last, bright.
	int selModel = (selType == SelType::Model) ? selIndex : -1;
	const auto& models = scene.GetModels();
	for (int pass = 0; pass < 2; pass++)
	{
		for (int i = 0; i < (int)models.size(); i++)
		{
			Scene3DModel* m = models[i];
			if (m == nullptr) continue;
			bool sel = (i == selModel);
			if ((pass == 0) == sel) continue;   // pass 0: others, pass 1: selection on top
			float x0 = toX(m->aabbMin.x), x1 = toX(m->aabbMax.x);
			float y0 = toY(m->aabbMin.z), y1 = toY(m->aabbMax.z);
			if (x1 < x0) std::swap(x0, x1);
			if (y1 < y0) std::swap(y0, y1);
			float w = std::max(x1 - x0, 2.0f), h = std::max(y1 - y0, 2.0f);
			glm::vec4 col = sel ? glm::vec4(1.0f, 0.92f, 0.25f, 0.95f)
				: m->IsWater() ? glm::vec4(0.25f, 0.5f, 0.85f, 0.5f)
				: m->guardHidden ? glm::vec4(0.4f, 0.4f, 0.45f, 0.28f)
				: glm::vec4(0.62f, 0.66f, 0.72f, 0.55f);
			renderer.DrawRect(x0, y0, w, h, col);
		}
	}

	// Saved cameras (cyan), anchors (orange), characters (magenta) - on top of props.
	for (const std::string& cn : scene.CameraOrder())
	{
		Scene3D::CamPose cp;
		if (scene.GetCameraPose(cn, cp))
			dot(toX(cp.position.x), toY(cp.position.z), 4.0f, glm::vec4(0.2f, 0.85f, 0.95f, 0.95f));
	}
	for (const Scene3D::SceneAnchor& a : scene.Anchors())
		dot(toX(a.position.x), toY(a.position.z), 5.0f, glm::vec4(0.95f, 0.62f, 0.18f, 0.95f));
	for (Character3D* c : scene.GetCharacters())
		if (c != nullptr)
			dot(toX(c->position.x), toY(c->position.z), 5.0f, glm::vec4(0.9f, 0.35f, 0.85f, 0.95f));

	// Live fly-camera: a green marker plus a short heading trail. The camera looks
	// along -front (lookAt target = position - front); front is derived from yaw as
	// (cos yaw, -, sin yaw) on the horizontal plane, so the view heading is -that.
	float cxm = toX(camp.x), cym = toY(camp.z);
	float yr = glm::radians(game.renderer.camera.yaw);
	glm::vec2 vdir(-std::cos(yr), -std::sin(yr));
	if (glm::length(vdir) > 1e-4f)
	{
		vdir = glm::normalize(vdir);
		for (int s = 1; s <= 5; s++)
			dot(cxm + vdir.x * s * 6.0f, cym + vdir.y * s * 6.0f, 2.0f, glm::vec4(0.5f, 1.0f, 0.55f, 0.85f));
	}
	dot(cxm, cym, 6.0f, glm::vec4(0.3f, 1.0f, 0.4f, 1.0f));

	// Title (scene name) and a legend.
	ui::Label("map.title", "MAP  -  " + (scene.currentScene.empty() ? std::string("(scene)") : scene.currentScene),
		panelX + 14.0f, panelTop + 22.0f, ui::Colour::heading);
	ui::Label("map.legend", ui::Fit("grey = model   yellow = selected   blue = water   cyan = camera   orange = slot   "
		"magenta = person   green = you", panelW - 28.0f), panelX + 14.0f, panelBottom - 56.0f, ui::Colour::dim);
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
	if (mode == PromptMode::Guard)
	{
		Scene3D& scene = Scene3D::Get();
		if (selType == SelType::Model && selIndex >= 0 && selIndex < (int)scene.GetModels().size())
			nameBuffer = scene.GetModels()[selIndex]->guard;
		else if (const ScenePointLight* pl = SelectedPointLight())
			nameBuffer = pl->guard;
	}
	if (mode == PromptMode::CameraName)
		nameBuffer = currentCamName;
	if (mode == PromptMode::SlotName)
	{
		const std::vector<Scene3D::SceneAnchor>& anchors = Scene3D::Get().Anchors();
		if (selType == SelType::Anchor && selIndex >= 0 && selIndex < (int)anchors.size())
			nameBuffer = anchors[selIndex].name;
	}
	if (mode == PromptMode::LightName)
	{
		if (const ScenePointLight* pl = SelectedPointLight())
			nameBuffer = pl->name;
		else if (const SceneSpotLight* sl = SelectedSpotLight())
			nameBuffer = sl->name;
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

	if (promptMode == PromptMode::Guard)
	{
		// Guards are tokens like  arrested  !arrested  clue:KNIFE  key=value
		// t>=18:00  day=2, separated by spaces: Shift for capitals, ! and &;
		// ; types ':', , and . type '<' and '>'.
		const bool shift = keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT];
		auto put = [&](char c) { if (nameBuffer.size() < 80) nameBuffer += c; };
		for (int sc = SDL_SCANCODE_A; sc <= SDL_SCANCODE_Z; sc++)
			if (edge(sc))
				put((char)((shift ? 'A' : 'a') + (sc - SDL_SCANCODE_A)));
		static const char kShiftedDigits[10] = { ')', '!', '@', '#', '$', '%', '^', '&', '*', '(' };
		for (int d = 1; d <= 9; d++)
			if (edge(SDL_SCANCODE_1 + d - 1))
				put(shift ? kShiftedDigits[d] : (char)('0' + d));
		if (edge(SDL_SCANCODE_0)) put(shift ? kShiftedDigits[0] : '0');
		if (edge(SDL_SCANCODE_MINUS)) put('_');
		if (edge(SDL_SCANCODE_SPACE)) put(' ');
		if (edge(SDL_SCANCODE_SEMICOLON)) put(':');
		if (edge(SDL_SCANCODE_EQUALS)) put('=');
		if (edge(SDL_SCANCODE_COMMA)) put('<');
		if (edge(SDL_SCANCODE_PERIOD)) put('>');
		if (edge(SDL_SCANCODE_BACKSPACE) && !nameBuffer.empty()) nameBuffer.pop_back();
	}
	else
	{
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
	}

	bool confirm = edge(SDL_SCANCODE_RETURN) || edge(SDL_SCANCODE_KP_ENTER);
	bool cancel = edge(SDL_SCANCODE_ESCAPE);

	for (int i = 0; i < SDL_NUM_SCANCODES; i++) prevKeys[i] = keys[i];

	if (cancel)
	{
		namingScene = false;
		statusMsg = (promptMode == PromptMode::Tag) ? "Tag cancelled"
			: (promptMode == PromptMode::CameraName || promptMode == PromptMode::SlotName) ? "Rename cancelled"
			: (promptMode == PromptMode::LightName) ? "Rename cancelled"
			: (promptMode == PromptMode::MaterialName) ? "New material cancelled"
			: (promptMode == PromptMode::Guard) ? "Guard unchanged"
			: "New scene cancelled";
		statusFrames = 120;
		return;
	}
	if (!confirm)
		return;

	if (promptMode == PromptMode::CameraName)
	{
		// Rename the chosen camera.
		namingScene = false;
		statusFrames = 240;
		if (currentCamName.empty() || nameBuffer == currentCamName)
			return;
		if (nameBuffer.empty())
		{
			statusMsg = "A camera needs a name (scripts use it)";
			return;
		}
		if (!Scene3D::Get().RenameCamera(currentCamName, nameBuffer))
		{
			statusMsg = "There is already a camera called " + nameBuffer;
			return;
		}
		statusMsg = "Renamed camera " + currentCamName + " to " + nameBuffer
			+ "  - scripts that use the old name (scene3d cam / glide) need updating; F5 saves";
		currentCamName = nameBuffer;
		CommitEdit();
		return;
	}

	if (promptMode == PromptMode::SlotName)
	{
		// Rename the selected slot.
		namingScene = false;
		statusFrames = 260;
		std::vector<Scene3D::SceneAnchor>& anchors = Scene3D::Get().Anchors();
		if (selType != SelType::Anchor || selIndex < 0 || selIndex >= (int)anchors.size())
			return;
		std::string& name = anchors[selIndex].name;
		if (nameBuffer == name)
			return;
		if (nameBuffer.empty())
		{
			statusMsg = "A slot needs a name (the schedule uses it)";
			return;
		}
		for (const Scene3D::SceneAnchor& a : anchors)
			if (a.name == nameBuffer) { statusMsg = "There is already a slot called " + nameBuffer; return; }
		Scene3DInternal::RenameSceneLine("slot", name, nameBuffer);
		statusMsg = "Renamed slot " + name + " to " + nameBuffer
			+ "  - the game's schedule finds slots by name: update it too; F5 saves";
		name = nameBuffer;
		CommitEdit();
		return;
	}

	if (promptMode == PromptMode::Guard)
	{
		namingScene = false;
		statusFrames = 300;
		Scene3D& scene = Scene3D::Get();
		std::string guard = nameBuffer;
		while (!guard.empty() && guard.back() == ' ') guard.pop_back();
		while (!guard.empty() && guard.front() == ' ') guard.erase(guard.begin());
		std::string* target = nullptr;
		bool* hidden = nullptr;
		std::string what;
		if (selType == SelType::Model && selIndex >= 0 && selIndex < (int)scene.GetModels().size())
		{
			Scene3DModel* m = scene.GetModels()[selIndex];
			target = &m->guard;
			hidden = &m->guardHidden;
			what = BaseName(m->objPath);
		}
		else if (ScenePointLight* pl = SelectedPointLight())
		{
			target = &pl->guard;
			hidden = &pl->guardHidden;
			what = pl->name;
		}
		if (target == nullptr)
			return;
		*target = guard;
		if (guard.empty())
		{
			*hidden = false;   // nothing hides it any more
			statusMsg = what + ": no guard, always there  (F5 to save)";
		}
		else
		{
			statusMsg = what + " is there only while: " + guard + "  (the game evaluates it; F5 to save)";
		}
		RefreshInfoText(game);
		CommitEdit();
		return;
	}

	if (promptMode == PromptMode::MaterialName)
	{
		namingScene = false;
		statusFrames = 220;
		Scene3D& scene = Scene3D::Get();
		if (selType != SelType::Model || selIndex < 0 || selIndex >= (int)scene.GetModels().size())
			return;
		if (nameBuffer.empty())
		{
			statusMsg = "A material needs a name";
			return;
		}
		for (const std::string& n : MaterialLibrary::Get().Names())
			if (n == nameBuffer) { statusMsg = "There is already a material called " + nameBuffer; return; }
		// A copy of the model's current material (or the plain default).
		Scene3DModel* m = scene.GetModels()[selIndex];
		const SceneMaterial* current = m->materialName.empty() ? nullptr : MaterialLibrary::Get().Find(m->materialName);
		SceneMaterial made = current ? *current : SceneMaterial();
		// Its lines kept beside the class (read before Add: the list may move)
		MaterialLibrary& library = MaterialLibrary::Get();
		const std::string glowMap = current ? library.EmissiveMapPath(*current) : std::string();
		const std::string roughMap = current ? library.RoughnessMapPath(*current) : std::string();
		const std::string splat = current ? library.SplatLayersPath(*current) : std::string();
		float windHeight = 0.0f;
		const float wind = current ? library.Wind(*current, &windHeight) : 0.0f;
		const float flutter = current ? library.WindFlutter(*current) : 1.0f;
		const float translucency = current ? library.Translucency(*current) : 0.0f;
		const float cutout = current ? library.Cutout(*current) : 0.0f;
		const bool fade = current && library.FadesNearLine(*current);
		const int shadow = current ? library.ShadowChoice(*current) : 0;
		made.name = nameBuffer;
		SceneMaterial* added = library.Add(game, made);
		library.SetEmissiveMap(game, *added, glowMap);
		library.SetRoughnessMap(game, *added, roughMap);
		if (!splat.empty())
			library.SetSplatLayers(game, *added, splat);
		library.SetWind(*added, wind, windHeight);
		library.SetWindFlutter(*added, flutter);
		library.SetTranslucency(*added, translucency);
		library.SetCutout(*added, cutout);
		library.SetFadesNearLine(*added, fade);
		library.SetShadowChoice(*added, shadow);
		RefreshMaterialPointers();
		m->materialName = nameBuffer;
		m->material = MaterialLibrary::Get().Find(nameBuffer);
		statusMsg = "New material " + nameBuffer + " for " + BaseName(m->objPath) + "  (F5 saves it to "
			+ MaterialLibrary::Get().LoadedPath() + ")";
		RefreshInfoText(game);
		CommitEdit();
		return;
	}

	if (promptMode == PromptMode::LightName)
	{
		namingScene = false;
		statusFrames = 200;
		Scene3D& scene = Scene3D::Get();
		std::string* name = nullptr;
		if (ScenePointLight* pl = SelectedPointLight())
			name = &pl->name;
		else if (SceneSpotLight* sl = SelectedSpotLight())
			name = &sl->name;
		if (name == nullptr)
			return;
		if (nameBuffer.empty())
		{
			statusMsg = "A light needs a name (scripts use it)";
			return;
		}
		if (nameBuffer == *name)
			return;
		for (const ScenePointLight& l : scene.GetPointLights())
			if (l.name == nameBuffer) { statusMsg = "Another light is already called " + nameBuffer; return; }
		for (const SceneSpotLight& l : scene.GetSpotLights())
			if (l.name == nameBuffer) { statusMsg = "Another light is already called " + nameBuffer; return; }
		if (scene.shadowCasterLight == *name)
			scene.shadowCasterLight = nameBuffer;   // keep it the shadow caster
		Scene3DInternal::RenameSceneLine(SelectedPointLight() ? "point" : "spot", *name, nameBuffer);
		statusMsg = "Renamed " + *name + " to " + nameBuffer + "  (scene3d light " + nameBuffer + " ...; F5 to save)";
		*name = nameBuffer;
		RefreshInfoText(game);
		CommitEdit();
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
	(void)renderer;
	if (!namingScene)
		return;
	const float gw = game.designWidth * Camera::MULTIPLIER;
	const float gh = game.designHeight * Camera::MULTIPLIER;
	std::string title, hint = "Enter = OK      Esc = cancel";
	switch (promptMode)
	{
	case PromptMode::Tag: title = "TAG  -  the game finds the model by it (DB2: a clue id). Blank removes it."; break;
	case PromptMode::CameraName: title = "CAMERA NAME  -  scripts use it: scene3d cam <name>, scene3d glide <name>"; break;
	case PromptMode::SlotName: title = "SLOT NAME  -  the game's schedule stands characters on slots by name"; break;
	case PromptMode::LightName: title = "LIGHT NAME  -  scripts use it: scene3d light <name> on | off ..."; break;
	case PromptMode::MaterialName: title = "NEW MATERIAL  -  starts as a copy of the current one"; break;
	case PromptMode::Guard:
		title = "GUARD  -  it's only there while this holds. Blank = always.";
		hint = "e.g.  arrested   !arrested   clue:KNIFE   t>=18:00   day=2        Enter = OK   Esc = cancel";
		break;
	default: title = "NEW SCENE  -  name it (letters, digits, _)"; break;
	}
	ui::Panel({ 0.0f, 0.0f, gw, gh }, glm::vec4(0.0f, 0.0f, 0.0f, 0.35f));
	const ui::Rect box{ (gw - 1300.0f) * 0.5f, gh * 0.30f, 1300.0f, 220.0f };
	ui::Panel(box, ui::Colour::bar);
	ui::Fill({ box.x, box.y, box.w, 3.0f }, glm::vec4(0.23f, 0.53f, 0.90f, 1.0f));
	ui::Label("prompt.title", ui::Fit(title, box.w - 60.0f), box.x + 30.0f, box.y + 40.0f, ui::Colour::heading);
	ui::TextField("prompt.field", { box.x + 30.0f, box.y + 78.0f, box.w - 60.0f, 50.0f }, nameBuffer + "_", "",
		nullptr, "", true);
	ui::Label("prompt.hint", ui::Fit(hint, box.w - 60.0f), box.x + 30.0f, box.y + 172.0f, ui::Colour::dim);
}

void Scene3DEditor::Deselect(Game& game)
{
	ClearSelection();
}

// ------------------------------------------------------------------ tile mode
// Grid-snapped tile editing for scenes built from 100-unit box tiles (e.g.
// Eggwhite's generated volcano). See the header comment on tileMode.

namespace
{
	// Slab ray-vs-AABB returning the entry distance (0 when starting inside).
	bool TileRayAABB(const glm::vec3& ro, const glm::vec3& rd,
		const glm::vec3& lo, const glm::vec3& hi, float& outT)
	{
		float tmin = -1e12f, tmax = 1e12f;
		for (int a = 0; a < 3; a++)
		{
			if (std::fabs(rd[a]) < 1e-9f)
			{
				if (ro[a] < lo[a] || ro[a] > hi[a]) return false;
				continue;
			}
			float t1 = (lo[a] - ro[a]) / rd[a];
			float t2 = (hi[a] - ro[a]) / rd[a];
			if (t1 > t2) std::swap(t1, t2);
			tmin = std::max(tmin, t1);
			tmax = std::min(tmax, t2);
			if (tmin > tmax) return false;
		}
		if (tmax < 0.0f) return false;
		outT = std::max(tmin, 0.0f);
		return true;
	}

	// "assets/textures/gen/russet_ground_16x12.png" -> "russet_ground"
	std::string TileBaseName(const std::string& texPath)
	{
		size_t slash = texPath.find_last_of("/\\");
		std::string n = (slash == std::string::npos) ? texPath : texPath.substr(slash + 1);
		size_t dot = n.find_last_of('.');
		if (dot != std::string::npos) n = n.substr(0, dot);
		// strip a trailing _<digits>x<digits>
		size_t us = n.find_last_of('_');
		if (us != std::string::npos)
		{
			std::string suf = n.substr(us + 1);
			size_t x = suf.find('x');
			if (x != std::string::npos && x > 0 && x + 1 < suf.size()
				&& suf.find_first_not_of("0123456789", 0) == x
				&& suf.find_first_not_of("0123456789", x + 1) == std::string::npos)
				n = n.substr(0, us);
		}
		return n;
	}

	// Best single-tile texture for a (possibly merged-sheet) texture path:
	// prefer <dir>/<base>_1x1.png, else the path unchanged.
	std::string SingleTileTexture(const std::string& texPath)
	{
		std::string base = TileBaseName(texPath);
		size_t slash = texPath.find_last_of("/\\");
		std::string dir = (slash == std::string::npos) ? "" : texPath.substr(0, slash + 1);
		std::string one = dir + base + "_1x1.png";
		if (one != texPath)
		{
			try { if (std::filesystem::exists(one)) return one; }
			catch (...) {}
		}
		return texPath;
	}

	bool TileEditable(const Scene3DModel* m)
	{
		return m != nullptr && !m->guardHidden
			&& m->interactionTag.empty()
			&& std::fabs(m->yawDeg) < 0.5f
			&& std::fabs(m->pitchDeg) < 0.5f
			&& std::fabs(m->rollDeg) < 0.5f;
	}
}

void Scene3DEditor::UpdateTileMode(Game& game, bool leftPressed, int mx, int my,
	float w, float h, Uint32 mb)
{
	Scene3D& scene = Scene3D::Get();
	Camera& cam = game.renderer.camera;

	// --- hover: nearest box-tile model under the mouse ray -------------
	glm::vec3 ro, rd;
	cam.ScreenPointToRay((float)mx, (float)my, w, h, ro, rd);
	hoverValid = false;
	hoverModelIndex = -1;
	float bestT = 1e12f;
	const std::vector<Scene3DModel*>& models = scene.GetModels();
	for (int i = 0; i < (int)models.size(); i++)
	{
		Scene3DModel* m = models[i];
		if (m == nullptr || m->guardHidden) continue;
		if (m->objPath.find("box.obj") == std::string::npos) continue;
		float t;
		if (!TileRayAABB(ro, rd, m->aabbMin, m->aabbMax, t)) continue;
		if (t < bestT) { bestT = t; hoverModelIndex = i; }
	}

	glm::vec3 hit;
	if (hoverModelIndex >= 0)
	{
		hit = ro + rd * bestT;
		Scene3DModel* m = models[hoverModelIndex];
		hoverTopY = m->aabbMin.y;      // up = -Y: min y = the top face
		tilePlaneY = hoverTopY;        // adds on empty cells use this height
		if (TileEditable(m))
		{
			// Eyedropper: the hovered tile's profile becomes the add-brush.
			brush.obj = m->objPath;
			brush.tex = SingleTileTexture(m->texPath);
			brush.mat = m->materialName;
			brush.sy = m->scaleAxis.y * m->modelScale;
			brush.yOfs = m->position.y - hoverTopY;
			brush.solid = m->solid;
			brush.walk = m->walkable;
			brush.has = true;
		}
	}
	else
	{
		// Nothing hit: hover the sticky horizontal plane (the last tile's top),
		// so adds extend a floor outward at the same level.
		if (std::fabs(rd.y) < 1e-6f) return;
		float t = (tilePlaneY - ro.y) / rd.y;
		if (t <= 0.0f) return;
		hit = ro + rd * t;
		hoverTopY = tilePlaneY;
	}
	hoverCellX = (int)std::floor((hit.x + 50.0f) / 100.0f);
	hoverCellZ = (int)std::floor((hit.z + 50.0f) / 100.0f);
	hoverValid = true;

	// --- clicks --------------------------------------------------------
	bool rightDown = (mb & SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0;
	bool rightPressedNow = rightDown && !rightWasDown;
	bool rightReleased = !rightDown && rightWasDown;
	if (rightPressedNow) { rPressX = mx; rPressY = my; }
	rightWasDown = rightDown;

	if (leftPressed && hoverModelIndex >= 0)
		TileRetexture(game);
	// Right CLICK (press+release with barely any motion) = add/remove; a
	// right-DRAG stays camera look, exactly as outside tile mode.
	else if (rightReleased && std::abs(mx - rPressX) + std::abs(my - rPressY) < 8)
		TileAddOrRemove(game);
}

int Scene3DEditor::EnsureSingleTile(Game& game, int modelIndex)
{
	Scene3D& scene = Scene3D::Get();
	const std::vector<Scene3DModel*>& models = scene.GetModels();
	Scene3DModel* m = models[modelIndex];

	float sizeX = m->aabbMax.x - m->aabbMin.x;
	float sizeZ = m->aabbMax.z - m->aabbMin.z;
	if (sizeX <= 150.0f && sizeZ <= 150.0f)
		return modelIndex;   // already a single tile

	int nx = (int)std::lround(sizeX / 100.0f);
	int nz = (int)std::lround(sizeZ / 100.0f);
	if (nx < 1 || nz < 1 || nx * nz > 512)
	{
		statusMsg = "Model too odd to tile-split";
		statusFrames = 150;
		return -1;
	}

	std::string tex = SingleTileTexture(m->texPath);
	std::string obj = m->objPath;
	std::string mat = m->materialName;
	float sy = m->scaleAxis.y * m->modelScale;
	float yPos = m->position.y;
	bool solid = m->solid, walk = m->walkable;
	float x0 = m->aabbMin.x, z0 = m->aabbMin.z;

	// Remove the merged box FIRST (keeps indices simple), then lay the tiles.
	scene.RemoveModel(game, modelIndex);

	for (int i = 0; i < nx; i++)
	{
		for (int j = 0; j < nz; j++)
		{
			glm::vec3 p(x0 + (i + 0.5f) * 100.0f, yPos, z0 + (j + 0.5f) * 100.0f);
			Scene3D::ModelDef def{ obj, tex, solid };
			Scene3DModel* nm = scene.AddModelInstance(game, def, p);
			if (nm == nullptr) continue;
			nm->scaleAxis = glm::vec3(1.0f, sy, 1.0f);
			nm->walkable = walk;
			nm->materialName = mat;
			nm->material = mat.empty() ? nullptr : MaterialLibrary::Get().Find(mat);
			scene.RecomputeModelBounds(nm);
		}
	}
	scene.RebuildSolids();

	// Find the new tile at the hovered cell.
	const std::vector<Scene3DModel*>& after = scene.GetModels();
	float cx = hoverCellX * 100.0f, cz = hoverCellZ * 100.0f;
	for (int i = 0; i < (int)after.size(); i++)
	{
		Scene3DModel* t = after[i];
		if (t == nullptr || t->objPath != obj) continue;
		if (std::fabs(t->position.x - cx) < 1.0f && std::fabs(t->position.z - cz) < 1.0f
			&& std::fabs(t->position.y - yPos) < 1.0f)
			return i;
	}
	return -1;
}

void Scene3DEditor::TileRetexture(Game& game)
{
	Scene3D& scene = Scene3D::Get();
	Scene3DModel* m = scene.GetModels()[hoverModelIndex];
	if (!TileEditable(m))
	{
		statusMsg = "Tagged/rotated prop - edit it in normal mode";
		statusFrames = 150;
		return;
	}

	int idx = EnsureSingleTile(game, hoverModelIndex);
	if (idx < 0) return;
	m = scene.GetModels()[idx];

	// Distinct tile types in this scene, keyed by texture base name.
	std::vector<std::string> bases;
	for (Scene3DModel* o : scene.GetModels())
	{
		if (o == nullptr || o->objPath.find("box.obj") == std::string::npos) continue;
		if (!o->interactionTag.empty()) continue;
		std::string b = TileBaseName(o->texPath);
		if (std::find(bases.begin(), bases.end(), b) == bases.end())
			bases.push_back(b);
	}
	std::sort(bases.begin(), bases.end());
	if (bases.size() < 2)
	{
		statusMsg = "Only one tile type in this scene";
		statusFrames = 150;
		return;
	}

	std::string cur = TileBaseName(m->texPath);
	size_t ci = 0;
	for (size_t i = 0; i < bases.size(); i++) if (bases[i] == cur) ci = i;
	const std::string& next = bases[(ci + 1) % bases.size()];

	// Adopt the target type's full profile from a representative tile.
	Scene3DModel* rep = nullptr;
	for (Scene3DModel* o : scene.GetModels())
	{
		if (o == m || o == nullptr) continue;
		if (o->objPath.find("box.obj") == std::string::npos) continue;
		if (!o->interactionTag.empty()) continue;
		if (TileBaseName(o->texPath) == next) { rep = o; break; }
	}
	if (rep != nullptr)
	{
		m->texPath = SingleTileTexture(rep->texPath);
		m->materialName = rep->materialName;
		m->material = rep->material;
		m->solid = rep->solid;
		m->walkable = rep->walkable;
	}
	else
	{
		m->texPath = SingleTileTexture(m->texPath);
	}
	m->texture = Scene3DInternal::SceneColorTexture(game, m->texPath);
	scene.RebuildSolids();
	CommitEdit();
	statusMsg = "Tile -> " + next;
	statusFrames = 120;
}

void Scene3DEditor::TileAddOrRemove(Game& game)
{
	Scene3D& scene = Scene3D::Get();
	if (hoverModelIndex >= 0)
	{
		Scene3DModel* m = scene.GetModels()[hoverModelIndex];
		if (!TileEditable(m))
		{
			statusMsg = "Tagged/rotated prop - delete it in normal mode";
			statusFrames = 150;
			return;
		}
		int idx = EnsureSingleTile(game, hoverModelIndex);
		if (idx < 0) return;
		scene.RemoveModel(game, idx);
		hoverModelIndex = -1;
		CommitEdit();
		statusMsg = "Tile removed";
		statusFrames = 90;
		return;
	}

	if (!brush.has)
	{
		statusMsg = "Hover a tile first to copy its type (eyedropper)";
		statusFrames = 150;
		return;
	}
	glm::vec3 pos(hoverCellX * 100.0f, tilePlaneY + brush.yOfs, hoverCellZ * 100.0f);
	Scene3D::ModelDef def{ brush.obj, brush.tex, brush.solid };
	Scene3DModel* nm = scene.AddModelInstance(game, def, pos);
	if (nm == nullptr) return;
	nm->scaleAxis = glm::vec3(1.0f, brush.sy, 1.0f);
	nm->walkable = brush.walk;
	nm->materialName = brush.mat;
	nm->material = brush.mat.empty() ? nullptr : MaterialLibrary::Get().Find(brush.mat);
	scene.RecomputeModelBounds(nm);
	scene.RebuildSolids();
	CommitEdit();
	statusMsg = "Tile added (" + TileBaseName(brush.tex) + ")";
	statusFrames = 90;
}

void Scene3DEditor::RenderTileHighlight(Game& game, const Renderer& renderer)
{
	if (!hoverValid)
		return;
	float cx = hoverCellX * 100.0f, cz = hoverCellZ * 100.0f;
	float y = hoverTopY - 3.0f;   // a hair above the surface (up = -Y)
	std::vector<glm::vec3> segs = {
		{ cx - 50, y, cz - 50 }, { cx + 50, y, cz - 50 },
		{ cx + 50, y, cz - 50 }, { cx + 50, y, cz + 50 },
		{ cx + 50, y, cz + 50 }, { cx - 50, y, cz + 50 },
		{ cx - 50, y, cz + 50 }, { cx - 50, y, cz - 50 },
		// a small cross so the cell reads even over busy textures
		{ cx - 15, y, cz }, { cx + 15, y, cz },
		{ cx, y, cz - 15 }, { cx, y, cz + 15 },
	};
	// Yellow = occupied (edit/remove), green = empty (add here).
	glm::vec4 color = (hoverModelIndex >= 0)
		? glm::vec4(1.0f, 0.9f, 0.2f, 1.0f)
		: glm::vec4(0.3f, 1.0f, 0.4f, 1.0f);
	renderer.DrawLines3D(segs, color);
}

// ------------------------------------------------------------ LOOK panel


void Scene3DEditor::LookFocusAt(Game& game, float sx, float sy)
{
	Scene3D& scene = Scene3D::Get();
	Camera& cam = game.renderer.camera;
	glm::vec3 ro, rd;
	cam.ScreenPointToRay(sx, sy, (float)game.screenWidth, (float)game.screenHeight, ro, rd);

	// The nearest model or character box under the cursor.
	float bestT = 1e30f;
	std::string what;
	for (Scene3DModel* m : scene.GetModels())
	{
		float t;
		if (RayAABB(ro, rd, m->aabbMin, m->aabbMax, t) && t < bestT)
		{
			bestT = t;
			what = BaseName(m->objPath);
		}
	}
	for (Character3D* ch : scene.GetCharacters())
	{
		glm::vec3 bmin, bmax;
		CharacterAABB(ch, bmin, bmax);
		float t;
		if (RayAABB(ro, rd, bmin, bmax, t) && t < bestT)
		{
			bestT = t;
			what = ch->charName;
		}
	}
	lookPickFocus = false;
	statusFrames = 200;
	if (bestT >= 1e29f)
	{
		statusMsg = "Nothing there to focus on";
		return;
	}

	// Depth of field measures focus along the view direction.
	const glm::vec4 v = cam.CalculateViewMatrix() * glm::vec4(ro + rd * bestT, 1.0f);
	const float focus = std::max(-v.z, 10.0f);
	float oldFocus = 500.0f, aperture = 0.0f;
	GetSceneDepthOfField(oldFocus, aperture);
	if (aperture <= 0.0f)
		aperture = 6.0f;   // turn it on with a moderate blur
	scene.SetDepthOfField(focus, aperture);
	char buf[96];
	snprintf(buf, sizeof(buf), "Focus %.0f on %s, aperture %.1f  (F5 to save)", focus, what.c_str(), aperture);
	statusMsg = buf;
	CommitEdit();
}

// ------------------------------------------------------------ LIGHTS panel

void Scene3DEditor::RenderLightGizmos(Game& game, const Renderer& renderer)
{
	Scene3D& scene = Scene3D::Get();
	// The light's own hue at full brightness, so a dim lamp still shows; grey when off.
	auto hue = [](const glm::vec3& c, bool lit)
	{
		if (!lit)
			return glm::vec4(0.45f, 0.45f, 0.50f, 1.0f);
		const float m = Brightness(c);
		return glm::vec4((m > 1e-4f) ? c / m : glm::vec3(1.0f), 1.0f);
	};
	auto marker = [](std::vector<glm::vec3>& seg, const glm::vec3& p, float r)
	{
		const glm::vec3 a[6] = { p + glm::vec3(r, 0, 0), p - glm::vec3(r, 0, 0), p + glm::vec3(0, r, 0),
			p - glm::vec3(0, r, 0), p + glm::vec3(0, 0, r), p - glm::vec3(0, 0, r) };
		const int e[12][2] = { {0,2},{0,3},{0,4},{0,5},{1,2},{1,3},{1,4},{1,5},{2,4},{4,3},{3,5},{5,2} };
		for (const auto& k : e)
		{
			seg.push_back(a[k[0]]);
			seg.push_back(a[k[1]]);
		}
	};
	auto circle = [](std::vector<glm::vec3>& seg, const glm::vec3& c, const glm::vec3& u, const glm::vec3& v, float r)
	{
		const int n = 40;
		for (int i = 0; i < n; i++)
		{
			const float a0 = 6.2831853f * i / n, a1 = 6.2831853f * (i + 1) / n;
			seg.push_back(c + (u * std::cos(a0) + v * std::sin(a0)) * r);
			seg.push_back(c + (u * std::cos(a1) + v * std::sin(a1)) * r);
		}
	};

	const std::vector<ScenePointLight>& points = scene.GetPointLights();
	for (size_t i = 0; i < points.size(); i++)
	{
		const ScenePointLight& l = points[i];
		const bool sel = (selType == SelType::PointLight && selIndex == (int)i);
		const glm::vec4 col = hue(l.color, l.on && !l.guardHidden);
		std::vector<glm::vec3> seg;
		marker(seg, l.pos, sel ? 26.0f : 18.0f);
		renderer.DrawLines3D(seg, col);
		if (sel)
		{
			// Its reach: three rings at the range.
			seg.clear();
			circle(seg, l.pos, glm::vec3(1, 0, 0), glm::vec3(0, 0, 1), l.range);
			circle(seg, l.pos, glm::vec3(1, 0, 0), glm::vec3(0, 1, 0), l.range);
			circle(seg, l.pos, glm::vec3(0, 1, 0), glm::vec3(0, 0, 1), l.range);
			renderer.DrawLines3D(seg, glm::vec4(glm::vec3(col), 0.55f));
		}
	}

	const std::vector<SceneSpotLight>& spots = scene.GetSpotLights();
	for (size_t i = 0; i < spots.size(); i++)
	{
		const SceneSpotLight& l = spots[i];
		const bool sel = (selType == SelType::SpotLight && selIndex == (int)i);
		const glm::vec4 col = hue(l.color, l.on);
		std::vector<glm::vec3> seg;
		marker(seg, l.pos, sel ? 22.0f : 14.0f);

		// The cone: short for every spot, out to its range when selected.
		const glm::vec3 n = (glm::length(l.dir) > 1e-6f) ? glm::normalize(l.dir) : glm::vec3(0, 1, 0);
		const glm::vec3 helper = (std::fabs(n.y) < 0.9f) ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0);
		const glm::vec3 u = glm::normalize(glm::cross(n, helper));
		const glm::vec3 v = glm::cross(n, u);
		const float len = sel ? l.range : 110.0f;
		const float outer = len * std::tan(glm::radians(std::min(l.outerDeg, 85.0f)));
		const glm::vec3 end = l.pos + n * len;
		for (int k = 0; k < 8; k++)
		{
			const float a = 6.2831853f * k / 8.0f;
			seg.push_back(l.pos);
			seg.push_back(end + (u * std::cos(a) + v * std::sin(a)) * outer);
		}
		circle(seg, end, u, v, outer);
		renderer.DrawLines3D(seg, sel ? col : glm::vec4(glm::vec3(col), 0.8f));
		if (sel)
		{
			seg.clear();
			circle(seg, end, u, v, len * std::tan(glm::radians(std::min(l.innerDeg, 85.0f))));
			renderer.DrawLines3D(seg, glm::vec4(glm::vec3(col), 0.5f));
		}
	}
}


void Scene3DEditor::AddLight(Game& game, bool spot)
{
	Scene3D& scene = Scene3D::Get();
	// Where the view's centre meets the floor (y = 0), else 400 ahead; then raised
	// (up is -Y) to lamp or ceiling height.
	Camera& cam = game.renderer.camera;
	glm::vec3 ro, rd;
	cam.ScreenPointToRay(game.screenWidth * 0.5f, game.screenHeight * 0.5f,
		(float)game.screenWidth, (float)game.screenHeight, ro, rd);
	float t = 400.0f;
	if (std::fabs(rd.y) > 1e-4f)
	{
		const float tp = -ro.y / rd.y;
		if (tp > 1.0f && tp < 8000.0f)
			t = tp;
	}
	glm::vec3 pos = ro + rd * t;
	pos.y -= spot ? 320.0f : 160.0f;

	if (spot)
	{
		SceneSpotLight l;
		l.name = UniqueLightName(scene, "spot");
		l.pos = pos;
		l.dir = glm::vec3(0.0f, 1.0f, 0.0f);   // straight down
		l.color = KelvinColor(4000.0f);
		l.range = 900.0f;
		l.intensity = 2.0f;
		l.innerDeg = 20.0f;
		l.outerDeg = 32.0f;
		scene.GetSpotLights().push_back(l);
		selType = SelType::SpotLight;
		selIndex = (int)scene.GetSpotLights().size() - 1;
		statusMsg = "Added " + l.name + " (drag to place, ROTATE to aim, F5 to save)";
	}
	else
	{
		ScenePointLight l;
		l.name = UniqueLightName(scene, "light");
		l.pos = pos;
		l.color = KelvinColor(2700.0f);   // a warm lamp
		l.range = 600.0f;
		l.intensity = 1.5f;
		l.flashPeak = l.intensity;
		scene.GetPointLights().push_back(l);
		selType = SelType::PointLight;
		selIndex = (int)scene.GetPointLights().size() - 1;
		statusMsg = "Added " + l.name + " (drag to place, F5 to save)";
	}
	statusFrames = 220;
	RefreshInfoText(game);
	CommitEdit();
}

// ---------------------------------------------------------- MATERIAL panel


// ------------------------------------------------------------- GUARD button


// ----------------------------------------------------- PROJECT SETTINGS page


// --------------------------------------------------------- TOON & OUTLINE page

// ============================================================== the interface
// A toolbar, a tabbed inspector on the left, the object list on the right and
// a status bar, drawn with editor/EditorUI.h's widgets (one look per kind of
// control). Every frame Render declares them; Update's input runs them.

Scene3DModel* Scene3DEditor::SelectedModel() const
{
	const std::vector<Scene3DModel*>& models = Scene3D::Get().GetModels();
	return (selType == SelType::Model && selIndex >= 0 && selIndex < (int)models.size()) ? models[selIndex] : nullptr;
}

bool Scene3DEditor::ConfirmDiscard(const std::string& what)
{
	// A click that would throw away unsaved work asks first: the same click
	// again, while the warning shows, goes ahead.
	if (!IsDirty() || (pendingDiscard == what && statusFrames > 0))
	{
		pendingDiscard.clear();
		return true;
	}
	pendingDiscard = what;
	statusMsg = "There are unsaved changes: SAVE first, or do that again to throw them away";
	statusFrames = 300;
	return false;
}

void Scene3DEditor::SaveAll(Game& game)
{
	Scene3D& scene = Scene3D::Get();
	if (scene.SaveScene(game))
	{
		// The materials too: only those that differ from the file are written.
		std::string materials;
		const bool materialsSaved = MaterialLibrary::Get().SaveChanges(materials);
		statusMsg = "Saved " + scene.currentScene + ".scene";
		if (!materialsSaved)
			statusMsg += ", but MATERIALS FAILED: " + materials;
		else if (materials != "materials unchanged")
			statusMsg += "; " + materials;
		if (materialsSaved)
			savedSnapshot = EditorSnapshot();   // now clean (keeps undo history)
		statusFrames = 220;
	}
	else
	{
		statusMsg = "SAVE FAILED";
		statusFrames = 180;
	}
}

void Scene3DEditor::RevertAll(Game& game)
{
	if (!ConfirmDiscard("reload"))
		return;
	ClearSelection();
	dragging = false;
	if (Scene3D::Get().Reload(game))
	{
		ResetHistory();   // reloaded from disk = clean, fresh undo history
		statusMsg = "Reloaded from disk (changes thrown away)";
	}
	else
	{
		statusMsg = "Reload failed";
	}
	statusFrames = 180;
}

void Scene3DEditor::LoadSceneByName(Game& game, const std::string& name)
{
	if (!ConfirmDiscard("load " + name))
		return;
	Scene3D::Get().Load(game, name);
	ClearSelection();
	dragging = false;
	statusMsg = "Loaded " + name;
	statusFrames = 150;
}

void Scene3DEditor::AddSlot(Game& game)
{
	// A named schedule anchor where the view's centre meets the ground plane
	// (y = 0), else 300 ahead. Auto-named slot1, slot2...; selected so it can be
	// dragged into place.
	Scene3D& sc = Scene3D::Get();
	Camera& cam = game.renderer.camera;
	glm::vec3 ro, rd;
	cam.ScreenPointToRay(game.screenWidth * 0.5f, game.screenHeight * 0.5f,
		(float)game.screenWidth, (float)game.screenHeight, ro, rd);
	float t = 300.0f;
	if (std::fabs(rd.y) > 1e-4f)
	{
		const float tp = -ro.y / rd.y;
		if (tp > 1.0f && tp < 8000.0f)
			t = tp;
	}
	Scene3D::SceneAnchor a;
	for (int n = (int)sc.Anchors().size() + 1;; n++)
	{
		a.name = "slot" + std::to_string(n);
		bool taken = false;
		for (const Scene3D::SceneAnchor& other : sc.Anchors())
			taken = taken || other.name == a.name;
		if (!taken)
			break;
	}
	a.position = ro + rd * t;
	sc.Anchors().push_back(a);
	selType = SelType::Anchor;
	selIndex = (int)sc.Anchors().size() - 1;
	statusMsg = "Added " + a.name + "  (drag it into place; F5 saves)";
	statusFrames = 200;
	CommitEdit();
}

void Scene3DEditor::AddModelFromPalette(Game& game, int index)
{
	if (index < 0 || index >= (int)addPalette.size())
		return;
	// Where the view's centre meets the floor (y = 0).
	Scene3D& scene = Scene3D::Get();
	Camera& cam = game.renderer.camera;
	glm::vec3 ro, rd;
	cam.ScreenPointToRay(game.screenWidth * 0.5f, game.screenHeight * 0.5f,
		(float)game.screenWidth, (float)game.screenHeight, ro, rd);
	glm::vec3 pos(0.0f);
	if (std::fabs(rd.y) > 1e-4f)
	{
		const float t = -ro.y / rd.y;
		if (t > 0.0f && t < 100000.0f)
			pos = ro + t * rd;
	}
	pos.y = 0.0f;
	if (scene.AddModelInstance(game, addPalette[index], pos) != nullptr)
	{
		selType = SelType::Model;
		selIndex = (int)scene.GetModels().size() - 1;
		statusMsg = "Added " + BaseName(addPalette[index].obj) + "  (drag it into place; F5 saves)";
		statusFrames = 150;
		CommitEdit();
	}
}

void Scene3DEditor::ResetTransform(Game& game, int which)
{
	if (!HasSelection())
		return;
	Scene3D& scene = Scene3D::Get();
	Scene3DModel* m = SelectedModel();
	if (which == 0)
	{
		SetSelectedPosition(game, glm::vec3(0.0f));
		statusMsg = "Moved to the scene's origin";
	}
	else if (which == 1)
	{
		if (m != nullptr)
		{
			m->yawDeg = m->pitchDeg = m->rollDeg = 0.0f;
			scene.RecomputeModelBounds(m);
		}
		if (SceneSpotLight* sl = SelectedSpotLight())
			sl->dir = glm::vec3(0.0f, 1.0f, 0.0f);   // a spot aims straight down
		statusMsg = "Rotation reset";
	}
	else
	{
		if (m != nullptr)
		{
			m->modelScale = 1.0f;
			m->scaleAxis = glm::vec3(1.0f);
			scene.RecomputeModelBounds(m);
		}
		statusMsg = "Size reset";
	}
	statusFrames = 150;
	dragging = false;
	CommitEdit();
}

void Scene3DEditor::CameraAction(Game& game, int which)
{
	Scene3D& scene = Scene3D::Get();
	Camera& cam = game.renderer.camera;
	Scene3D::CamPose pose;
	pose.position = cam.position;
	pose.pitch = cam.pitch;
	pose.yaw = cam.yaw;
	statusFrames = 220;
	switch (which)
	{
	case 0:   // the current view into the chosen camera (a new one if none is chosen)
		if (currentCamName.empty())
			currentCamName = NextCameraName();
		scene.AddOrUpdateCamera(currentCamName, pose);
		statusMsg = "Saved the view to '" + currentCamName + "'  (F5 saves the scene)";
		break;
	case 1:   // a new camera at the current view
		currentCamName = NextCameraName();
		scene.AddOrUpdateCamera(currentCamName, pose);
		statusMsg = "New camera '" + currentCamName + "' at the current view  (F5 saves the scene)";
		break;
	case 2:   // the chosen camera opens the scene
		if (!currentCamName.empty() && scene.SetDefaultCamera(currentCamName))
		{
			camCycleIndex = 0;
			statusMsg = "The scene now opens on '" + currentCamName + "'  (F5 saves the scene)";
		}
		else
			statusMsg = "Pick a camera in the list first";
		break;
	default:  // delete the chosen camera
		if (!currentCamName.empty() && scene.RemoveCamera(currentCamName))
		{
			statusMsg = "Deleted camera '" + currentCamName + "'  (F5 saves the scene)";
			currentCamName.clear();
			camCycleIndex = -1;
		}
		else
			statusMsg = "Pick a camera first (a scene keeps at least one)";
		break;
	}
	CommitEdit();   // camera edits are undoable too
}

// ---------------------------------------------------------------- toolbar

void Scene3DEditor::RenderToolbar(Game& game, const Renderer& renderer)
{
	(void)renderer;
	Scene3D& scene = Scene3D::Get();
	const float gw = game.designWidth * Camera::MULTIPLIER;
	ui::Panel({ 0.0f, 0.0f, gw, kBarH }, ui::Colour::bar);
	const float y = (kBarH - ui::kRowH) * 0.5f;
	float x = 12.0f;
	auto place = [&](float w) { const ui::Rect r{ x, y, w, ui::kRowH }; x += w + ui::kGap; return r; };
	auto snug = [](const std::string& s) { return ui::TextWidth(s) + 32.0f; };
	auto separator = [&]()
	{
		x += 6.0f;
		ui::Fill({ x, y + 6.0f, 2.0f, ui::kRowH - 12.0f }, ui::Colour::line);
		x += 8.0f + ui::kGap;
	};

	ui::Dropdown("bar.scene", place(260.0f), scene.currentScene.empty() ? "(no scene)" : scene.currentScene,
		[]() { return Scene3D::Get().GetSceneList(); },
		[this, &game](int i)
		{
			const std::vector<std::string> list = Scene3D::Get().GetSceneList();
			if (i >= 0 && i < (int)list.size())
				LoadSceneByName(game, list[i]);
		},
		"The scene you're editing. Pick another to load it.");
	ui::Button("bar.new", place(snug("NEW")), "NEW", [this]()
		{
			if (ConfirmDiscard("new"))
				StartNaming(PromptMode::NewScene);
		},
		"Make a new, empty scene and switch to it");
	const bool dirty = dragging || baselineSnapshot != savedSnapshot;
	ui::Button("bar.save", place(snug("SAVE")), "SAVE", [this, &game]() { SaveAll(game); },
		"Save the scene, and any materials you changed (F5)", dirty ? ui::Tone::Accent : ui::Tone::Normal);
	ui::Button("bar.reload", place(snug("RELOAD")), "RELOAD", [this, &game]() { RevertAll(game); },
		"Throw away unsaved changes: load the scene again from disk (F9)", ui::Tone::Danger);
	separator();
	ui::Button("bar.undo", place(snug("UNDO")), "UNDO", [this, &game]() { Undo(game); },
		"Undo the last change (Ctrl+Z)", ui::Tone::Normal, !undoStack.empty());
	ui::Button("bar.redo", place(snug("REDO")), "REDO", [this, &game]() { Redo(game); },
		"Redo what you undid (Ctrl+Y)", ui::Tone::Normal, !redoStack.empty());
	separator();
	ui::Choice("bar.mode", place(3.0f * snug("ROTATE")), { "MOVE", "ROTATE", "SCALE" }, (int)xformMode,
		[this](int i) { xformMode = (XformMode)i; },
		"What dragging the selection in the view does: MOVE it, ROTATE it (a spot light: aim it), or SCALE it (a light: its range)");
	ui::Choice("bar.axis", place(snug("FREE") + 3.0f * 48.0f), { "FREE", "X", "Y", "Z" }, lockAxis + 1,
		[this](int i) { lockAxis = i - 1; },
		"Lock drags to one axis. FREE: move along the ground, rotate about the vertical, scale evenly");
	separator();
	ui::Dropdown("bar.add", place(150.0f), "ADD",
		[this]()
		{
			addPalette = Scene3D::Get().GetModelPalette();
			std::vector<std::string> items = { "Point light", "Spot light", "Slot (where a character stands)" };
			for (const Scene3D::ModelDef& d : addPalette)
				items.push_back(BaseName(d.obj));
			return items;
		},
		[this, &game](int i)
		{
			if (i == 0) AddLight(game, false);
			else if (i == 1) AddLight(game, true);
			else if (i == 2) AddSlot(game);
			else AddModelFromPalette(game, i - 3);
		},
		"Add a light, a slot or a model, where the middle of the view meets the floor");
	ui::Toggle("bar.tile", place(snug("TILE") + 40.0f), "TILE", tileMode, [this, &game]()
		{
			tileMode = !tileMode;
			if (tileMode)
				Deselect(game);
			hoverValid = false;
			statusMsg = tileMode ? "TILE mode: hover picks a tile type, click changes a tile, right-click adds / removes"
				: "Tile mode off";
			statusFrames = 240;
		},
		"Tile mode: edit grid-tile scenes cell by cell (hover = pick a type, click = change, right-click = add / remove)");
	ui::Toggle("bar.map", place(snug("MAP") + 40.0f), "MAP", showMinimap, [this]() { showMinimap = !showMinimap; },
		"Show a map of the scene from above");

	// The right end: what's unsaved, hiding the panels, leaving.
	float rx = gw - 12.0f;
	auto placeRight = [&](float w) { rx -= w; const ui::Rect r{ rx, y, w, ui::kRowH }; rx -= ui::kGap; return r; };
	ui::Button("bar.exit", placeRight(snug("EXIT")), "EXIT", [this, &game]() { Toggle(game); },
		"Leave the 3D editor (2)");
	ui::Button("bar.hide", placeRight(snug("HIDE PANELS")), "HIDE PANELS", []() { uiHidden = true; },
		"Hide the editor's panels to see the whole view (Tab brings them back)");
	const std::string state = dirty ? "UNSAVED CHANGES" : "ALL SAVED";
	const float sw = ui::TextWidth(state) + 26.0f;
	rx -= sw + 10.0f;
	if (dirty)
		ui::Fill({ rx, kBarH * 0.5f - 6.0f, 12.0f, 12.0f }, glm::vec4(1.0f, 0.67f, 0.25f, 1.0f));
	ui::Label("bar.state", state, rx + 22.0f, kBarH * 0.5f, dirty ? ui::Colour::warning : ui::Colour::faint);
}

// -------------------------------------------------------------- inspector

namespace
{
	// Rows of the inspector: a label and one control each.
	ui::NumberSpec Spec(float step, bool mul, float lo, float hi, const char* fmt, float floor = 0.0f, bool drag = true)
	{
		ui::NumberSpec s;
		s.step = step;
		s.mul = mul;
		s.lo = lo;
		s.hi = hi;
		s.fmt = fmt;
		s.floor = floor;
		s.drag = drag;
		return s;
	}

	const char* const kResetHelp = "Click the name to go back to the default";

	void NumberRow(ui::Column& col, const std::string& id, const std::string& label, float value,
		const ui::NumberSpec& spec, std::function<void(float)> set, std::function<void()> commit,
		const std::string& help, bool own = true, std::function<void()> reset = nullptr,
		const std::string& text = std::string(), bool enabled = true)
	{
		ui::Rect c;
		if (col.Row(id, label, c, own, reset, kResetHelp))
			ui::Number(id, c, value, text, spec, set, commit, help, enabled);
	}

	void ToggleRow(ui::Column& col, const std::string& id, const std::string& label, bool on,
		std::function<void()> click, const std::string& help, bool own = true,
		std::function<void()> reset = nullptr, bool enabled = true)
	{
		ui::Rect c;
		if (col.Row(id, label, c, own, reset, kResetHelp))
			ui::Toggle(id, c, on ? "On" : "Off", on, click, help, enabled);
	}

	void ChoiceRow(ui::Column& col, const std::string& id, const std::string& label,
		const std::vector<std::string>& options, int chosen, std::function<void(int)> pick,
		const std::string& help, bool own = true, std::function<void()> reset = nullptr)
	{
		ui::Rect c;
		if (col.Row(id, label, c, own, reset, kResetHelp))
			ui::Choice(id, c, options, chosen, pick, help);
	}

	void DropdownRow(ui::Column& col, const std::string& id, const std::string& label, const std::string& value,
		std::function<std::vector<std::string>()> items, std::function<void(int)> pick,
		const std::string& help, bool own = true, std::function<void()> reset = nullptr)
	{
		ui::Rect c;
		if (col.Row(id, label, c, own, reset, kResetHelp))
			ui::Dropdown(id, c, value, items, pick, help);
	}

	void StepperRow(ui::Column& col, const std::string& id, const std::string& label, const std::string& text,
		std::function<void(int)> step, const std::string& help)
	{
		ui::Rect c;
		if (col.Row(id, label, c))
			ui::Stepper(id, c, text, step, help);
	}

	void TextRow(ui::Column& col, const std::string& id, const std::string& label, const std::string& value,
		const std::string& placeholder, std::function<void()> click, const std::string& help)
	{
		ui::Rect c;
		if (col.Row(id, label, c))
			ui::TextField(id, c, value, placeholder, click, help);
	}

	struct Action
	{
		std::string label;
		std::function<void()> run;
		std::string help;
		ui::Tone tone = ui::Tone::Normal;
		bool enabled = true;
	};

	// A row of buttons sharing the column's width.
	void Buttons(ui::Column& col, const std::string& id, const std::vector<Action>& actions)
	{
		ui::Rect r;
		if (!col.Line(r) || actions.empty())
			return;
		const int n = (int)actions.size();
		const float w = (r.w - ui::kGap * (n - 1)) / n;
		for (int i = 0; i < n; i++)
			ui::Button(id + "." + std::to_string(i), { r.x + i * (w + ui::kGap), r.y, w, r.h }, actions[i].label,
				actions[i].run, actions[i].help, actions[i].tone, actions[i].enabled);
	}

	// What the open list's rows stand for (paths, names), when they differ from
	// what it shows. One list is open at a time.
	std::vector<std::string> listValues;

	// --- LOOK values -----------------------------------------------------------

	const LookRowDef* LookDef(LookProp p)
	{
		for (const LookRowDef& d : kLookRows)
			if (d.prop == p)
				return &d;
		return nullptr;
	}

	void ApplyLook(LookProp p, float nv)
	{
		Scene3D& scene = Scene3D::Get();
		switch (p)
		{
		case LookProp::Exposure: scene.SetExposure(nv); break;
		case LookProp::Bloom: scene.SetBloom(nv); break;
		case LookProp::SkyLight:
		case LookProp::SkyShine:
		{
			// Both values: the .scene line can't hold one without the other.
			const float diffuse = ReadLook(LookProp::SkyLight).value;
			const float specular = ReadLook(LookProp::SkyShine).value;
			if (p == LookProp::SkyLight)
				scene.SetIBL(nv, specular);
			else
				scene.SetIBL(diffuse, nv);
			break;
		}
		case LookProp::Ao:
		case LookProp::AoRadius:
		{
			float strength = -1.0f, radius = -1.0f;
			GetSceneAO(strength, radius);
			if (p == LookProp::Ao)
				scene.SetAmbientOcclusion(nv, radius);
			else
				scene.SetAmbientOcclusion(strength, nv);
			break;
		}
		case LookProp::Shadows: Scene3DInternal::SetSceneShadowDistance(nv); break;
		case LookProp::Fog:
		case LookProp::FogFalloff:
		case LookProp::FogGlow:
		case LookProp::FogDrift:
		case LookProp::FogRed:
		case LookProp::FogGreen:
		case LookProp::FogBlue:
		{
			// The fog in force becomes the scene's own, with this one value changed.
			FogSettings f;
			if (!GetSceneFog(f))
				f = FogInForce();
			if (p == LookProp::Fog) f.density = nv;
			else if (p == LookProp::FogFalloff) f.heightFalloff = nv;
			else if (p == LookProp::FogGlow) f.anisotropy = nv;
			else if (p == LookProp::FogDrift) f.noise = nv;
			else if (p == LookProp::FogRed) f.color.r = nv;
			else if (p == LookProp::FogGreen) f.color.g = nv;
			else f.color.b = nv;
			SetSceneFog(true, f, 0.0f);
			break;
		}
		case LookProp::DistFogNear:
		case LookProp::DistFogFar:
		case LookProp::DistFogRed:
		case LookProp::DistFogGreen:
		case LookProp::DistFogBlue:
		{
			// Likewise the distance fog in force, keeping near no further than far.
			DistanceFogSettings f;
			if (!GetSceneDistanceFog(f))
				f = DistanceFogInForce();
			if (p == LookProp::DistFogNear) { f.nearDistance = nv; f.farDistance = std::max(f.farDistance, nv); }
			else if (p == LookProp::DistFogFar) { f.farDistance = nv; f.nearDistance = std::min(f.nearDistance, nv); }
			else if (p == LookProp::DistFogRed) f.color.r = nv;
			else if (p == LookProp::DistFogGreen) f.color.g = nv;
			else f.color.b = nv;
			SetSceneDistanceFog(true, f, 0.0f);
			break;
		}
		case LookProp::Weather:
			if (scene.GetWeather() != Scene3D::WeatherType::None)
				scene.SetWeather(scene.GetWeather(), nv);
			break;
		case LookProp::Focus:
		case LookProp::Aperture:
		{
			float focus = 500.0f, aperture = 0.0f;
			GetSceneDepthOfField(focus, aperture);
			if (p == LookProp::Focus)
				scene.SetDepthOfField(nv, aperture);
			else
				scene.SetDepthOfField(focus, nv);
			break;
		}
		case LookProp::SkySize:
			if (scene.HasSky())
				scene.SetSkyRadius(nv);
			break;
		case LookProp::Grade:
		{
			std::string path, projectPath;
			float strength = 1.0f, projectStrength = 1.0f;
			GetSceneColorGrade(path, strength);
			GetProjectColorGrade(projectPath, projectStrength);
			if (path.empty())
				path = projectPath;   // fade the project's look for this scene
			if (!path.empty() && path != "none")
				scene.SetColorGrade(path, nv);
			break;
		}
		default:
			break;
		}
	}

	// Back to the project's value (the fog rows go together, as do the two sky
	// light and the two AO rows: one .scene line holds each group).
	void ResetLook(LookProp p)
	{
		Scene3D& scene = Scene3D::Get();
		switch (p)
		{
		case LookProp::Exposure: scene.SetExposure(0.0f); break;
		case LookProp::Bloom: scene.SetBloom(-1.0f); break;
		case LookProp::SkyLight:
		case LookProp::SkyShine: scene.SetIBL(-1.0f, -1.0f); break;
		case LookProp::Ao:
		case LookProp::AoRadius: scene.SetAmbientOcclusion(-1.0f, -1.0f); break;
		case LookProp::Shadows: Scene3DInternal::SetSceneShadowDistance(-1.0f); break;
		case LookProp::Fog:
		case LookProp::FogFalloff:
		case LookProp::FogGlow:
		case LookProp::FogDrift:
		case LookProp::FogRed:
		case LookProp::FogGreen:
		case LookProp::FogBlue: SetSceneFog(false, FogSettings(), 0.0f); break;
		case LookProp::DistFogNear:
		case LookProp::DistFogFar:
		case LookProp::DistFogRed:
		case LookProp::DistFogGreen:
		case LookProp::DistFogBlue: SetSceneDistanceFog(false, DistanceFogSettings(), 0.0f); break;
		case LookProp::Focus:
		case LookProp::Aperture: scene.SetDepthOfField(500.0f, 0.0f); break;
		case LookProp::Grade: scene.SetColorGrade("", 1.0f); break;
		default: break;
		}
	}

	bool LookHasDefault(LookProp p)
	{
		return p != LookProp::Weather && p != LookProp::SkySize;
	}

	const char* LookHelp(LookProp p)
	{
		switch (p)
		{
		case LookProp::Exposure: return "How bright the whole image is, before tonemapping";
		case LookProp::Bloom: return "How much very bright light (lamps, glints, glowing materials) glows";
		case LookProp::SkyLight: return "How much light the sky gives surfaces (needs a sky)";
		case LookProp::SkyShine: return "How strongly surfaces reflect the sky (needs a sky)";
		case LookProp::Ao: return "Soft shadow in corners and under objects (ambient light only)";
		case LookProp::AoRadius: return "How far ambient occlusion looks for nearby surfaces, in world units";
		case LookProp::Shadows: return "How far from the camera sun shadows reach, in world units";
		case LookProp::Fog: return "Fog thickness (0 = none; rain and snow bring their own)";
		case LookProp::FogFalloff: return "How fast the fog thins with height";
		case LookProp::FogGlow: return "How much the fog glows looking towards a light (god rays)";
		case LookProp::FogDrift: return "Drifting patches in the fog";
		case LookProp::FogRed:
		case LookProp::FogGreen:
		case LookProp::FogBlue: return "The fog's colour";
		case LookProp::DistFogNear: return "Distance fog: how far from the camera it starts, in world units";
		case LookProp::DistFogFar: return "Distance fog: how far from the camera everything is fog, in world units";
		case LookProp::DistFogRed:
		case LookProp::DistFogGreen:
		case LookProp::DistFogBlue: return "The distance fog's colour";
		case LookProp::Weather: return "How heavy the rain, snow or storm is";
		case LookProp::Focus: return "How far away things are sharp (depth of field)";
		case LookProp::Aperture: return "How blurred things away from the focus get (0 = off)";
		case LookProp::SkySize: return "The sky sphere's radius: beyond the scene, inside the camera's far plane";
		case LookProp::Grade: return "How strongly the colour grade (LUT) applies";
		default: return "";
		}
	}

	void LookRow(ui::Column& col, LookProp p, const std::string& label, std::function<void()> commit, bool enabled = true)
	{
		const LookRowDef* d = LookDef(p);
		if (d == nullptr)
			return;
		const LookValue v = ReadLook(p);
		std::function<void()> reset;
		if (LookHasDefault(p) && v.own)
			reset = [p, commit]() { ResetLook(p); commit(); };
		NumberRow(col, std::string("look.") + d->name, label, v.value,
			Spec(d->step, d->mul, d->lo, d->hi, d->fmt, d->floor),
			[p](float nv) { ApplyLook(p, nv); }, commit, LookHelp(p), v.own, reset, v.text, enabled);
	}

	// --- lists -----------------------------------------------------------------

	// Every sky panorama the game's scenes use, and the "...sky..." images beside
	// the current one. Value "" = no sky.
	void SkyChoices(std::vector<std::string>& labels, std::vector<std::string>& values)
	{
		labels = { "(no sky)" };
		values = { "" };
		std::vector<std::string> skies;
		const std::string current = Scene3D::Get().GetAuthoredSky();
		namespace fs = std::filesystem;
		try
		{
			for (const auto& e : fs::directory_iterator("data/scenes"))
			{
				if (!e.is_regular_file() || e.path().extension() != ".scene")
					continue;
				std::ifstream in(e.path());
				std::string line;
				while (std::getline(in, line))
				{
					std::istringstream ls(line);
					std::string tag, path;
					if ((ls >> tag >> path) && tag == "sky")
						skies.push_back(path);
				}
			}
			const std::string folder = current.empty() ? std::string() : fs::path(current).parent_path().generic_string();
			if (!folder.empty() && fs::is_directory(folder))
			{
				for (const auto& e : fs::directory_iterator(folder))
				{
					const std::string name = LookLower(e.path().filename().generic_string());
					const std::string ext = LookLower(e.path().extension().generic_string());
					if (e.is_regular_file() && name.find("sky") != std::string::npos
						&& (ext == ".png" || ext == ".jpg" || ext == ".jpeg"))
						skies.push_back(folder + "/" + e.path().filename().generic_string());
				}
			}
		}
		catch (const std::exception&) {}
		if (!current.empty())
			skies.push_back(current);
		std::sort(skies.begin(), skies.end());
		skies.erase(std::unique(skies.begin(), skies.end()), skies.end());
		for (const std::string& s : skies)
		{
			labels.push_back(LookBaseName(s));
			values.push_back(s);
		}
	}

	// Strip LUTs in data/luts. Value "" = the project's, "none" = off.
	void LutChoices(std::vector<std::string>& labels, std::vector<std::string>& values, bool projectRow)
	{
		labels.clear();
		values.clear();
		if (!projectRow)
		{
			labels.push_back("(the project's)");
			values.push_back("");
		}
		labels.push_back("(none)");
		values.push_back("none");
		std::vector<std::string> files;
		try
		{
			namespace fs = std::filesystem;
			if (fs::is_directory("data/luts"))
				for (const auto& e : fs::directory_iterator("data/luts"))
					if (e.is_regular_file() && LookLower(e.path().extension().generic_string()) == ".png")
						files.push_back("data/luts/" + e.path().filename().generic_string());
		}
		catch (const std::exception&) {}
		std::sort(files.begin(), files.end());
		for (const std::string& f : files)
		{
			labels.push_back(LookBaseName(f));
			values.push_back(f);
		}
	}

	enum class MapKind { Normal, Glow, Roughness };

	// Whether an image's file name (lower case, no extension) marks it as a map
	// of this kind: a normal map has "normal" or a _n / _nrm / _nor / _norm
	// suffix; a glow map "glow" or "emissive", or _e; a roughness map "rough",
	// or _orm / _mr.
	bool IsMapName(MapKind kind, const std::string& stem)
	{
		auto endsWith = [&](const std::string& sfx)
		{
			return stem.size() > sfx.size() && stem.compare(stem.size() - sfx.size(), sfx.size(), sfx) == 0;
		};
		auto has = [&](const char* word) { return stem.find(word) != std::string::npos; };
		switch (kind)
		{
		case MapKind::Normal:
			return has("normal") || endsWith("_n") || endsWith("_nrm") || endsWith("_nor") || endsWith("_norm");
		case MapKind::Glow:
			return has("glow") || has("emissive") || endsWith("_e");
		case MapKind::Roughness:
			return has("rough") || endsWith("_orm") || endsWith("_mr");
		}
		return false;
	}

	// "(none)", the current map, then maps of that kind (by file name: IsMapName)
	// beside it, beside the model's texture and in assets/textures.
	void MapChoices(const Scene3DModel* model, const std::string& current, MapKind kind,
		std::vector<std::string>& labels, std::vector<std::string>& values)
	{
		labels = { "(none)" };
		values = { "" };
		if (model == nullptr)
			return;
		auto add = [&](const std::string& value)
		{
			if (std::find(values.begin(), values.end(), value) == values.end())
			{
				labels.push_back(LookBaseName(value));
				values.push_back(value);
			}
		};
		if (!current.empty())
			add(current);
		auto folderOf = [](const std::string& path)
		{
			const size_t slash = path.find_last_of("/\\");
			return (slash == std::string::npos) ? std::string() : path.substr(0, slash);
		};
		std::vector<std::string> folders;
		if (!current.empty())
			folders.push_back(folderOf(current));
		folders.push_back(folderOf(model->texPath));
		folders.push_back("assets/textures");
		std::vector<std::string> found;
		namespace fs = std::filesystem;
		for (const std::string& folder : folders)
		{
			if (folder.empty())
				continue;
			try
			{
				if (!fs::is_directory(folder))
					continue;
				for (const auto& e : fs::directory_iterator(folder))
				{
					if (!e.is_regular_file())
						continue;
					const std::string ext = LookLower(e.path().extension().generic_string());
					const std::string stem = LookLower(e.path().stem().generic_string());
					const std::string path = folder + "/" + e.path().filename().generic_string();
					if ((ext == ".png" || ext == ".jpg") && IsMapName(kind, stem)
						&& std::find(found.begin(), found.end(), path) == found.end())
						found.push_back(path);
				}
			}
			catch (const std::exception&) {}
		}
		std::sort(found.begin(), found.end());
		for (const std::string& f : found)
			add(f);
	}

	// --- light values ----------------------------------------------------------

	// Set a light's (or, with no light, the sun's / ambient light's) value.
	void ApplyLightValue(LightProp prop, float nv, ScenePointLight* pl, SceneSpotLight* sl, Scene3D& scene)
	{
		glm::vec3* color = pl ? &pl->color : sl ? &sl->color : nullptr;
		switch (prop)
		{
		case LightProp::Intensity:
			if (pl)
			{
				pl->intensity = nv;
				pl->fade.active = false;
				if (pl->flashHz > 0.0f)
					pl->flashPeak = nv;   // a strobe blinks at this peak
			}
			if (sl)
			{
				sl->intensity = nv;
				sl->fade.active = false;
			}
			break;
		case LightProp::Range:
			if (pl) pl->range = nv;
			if (sl) sl->range = nv;
			break;
		case LightProp::Flash:
			if (pl)
			{
				if (pl->flashHz <= 0.0f && nv > 0.0f)
					pl->flashPeak = pl->intensity;
				if (nv <= 0.0f && pl->flashHz > 0.0f)
					pl->intensity = pl->flashPeak;   // steady again
				pl->flashHz = nv;
			}
			break;
		case LightProp::FlashPhase:
			if (pl) pl->flashPhase = nv;
			break;
		case LightProp::Inner:
			if (sl)
			{
				sl->innerDeg = nv;
				sl->outerDeg = std::max(sl->outerDeg, nv);
			}
			break;
		case LightProp::Outer:
			if (sl)
			{
				sl->outerDeg = nv;
				sl->innerDeg = std::min(sl->innerDeg, nv);
			}
			break;
		case LightProp::AimTurn:
		case LightProp::AimTilt:
			if (sl)
			{
				float turn = 0.0f, tilt = 90.0f;
				DirToTurnTilt(sl->dir, turn, tilt);
				if (prop == LightProp::AimTurn)
					turn = nv;
				else
					tilt = nv;
				sl->dir = TurnTiltToDir(turn, tilt);
			}
			break;
		case LightProp::Red: if (color) color->r = nv; break;
		case LightProp::Green: if (color) color->g = nv; break;
		case LightProp::Blue: if (color) color->b = nv; break;
		case LightProp::Sun:
			scene.SetDirectionalLight(scene.GetDirectionalLight().color, nv);
			break;
		case LightProp::SunHeading:
		case LightProp::SunHeight:
		{
			float turn = 0.0f, tilt = 45.0f;
			DirToTurnTilt(scene.GetDirectionalLight().dir, turn, tilt);
			if (prop == LightProp::SunHeading)
				turn = nv;
			else
				tilt = nv;
			scene.SetSunDirection(TurnTiltToDir(turn, tilt));
			break;
		}
		case LightProp::Ambient:
		{
			const glm::vec3 c = scene.GetAmbientLight();
			const float m = Brightness(c);
			scene.SetAmbientLight((m > 1e-5f) ? c * (nv / m) : glm::vec3(nv));
			break;
		}
		case LightProp::SunRed:
		case LightProp::SunGreen:
		case LightProp::SunBlue:
		{
			glm::vec3 c = scene.GetDirectionalLight().color;
			(prop == LightProp::SunRed ? c.r : prop == LightProp::SunGreen ? c.g : c.b) = nv;
			scene.SetDirectionalLight(c, scene.GetDirectionalLight().diffuse);
			break;
		}
		case LightProp::AmbientRed:
		case LightProp::AmbientGreen:
		case LightProp::AmbientBlue:
		{
			glm::vec3 c = scene.GetAmbientLight();
			(prop == LightProp::AmbientRed ? c.r : prop == LightProp::AmbientGreen ? c.g : c.b) = nv;
			scene.SetAmbientLight(c);
			break;
		}
		default:
			break;
		}
	}
}

void Scene3DEditor::RenderInspector(Game& game, const Renderer& renderer)
{
	(void)renderer;
	const float gh = game.designHeight * Camera::MULTIPLIER;
	const ui::Rect area{ 0.0f, kBarH, kInspectorW, gh - kBarH - kStatusH };
	ui::Panel(area, ui::Colour::panel);

	// The pages, as two rows of three tabs.
	static const char* const kTabNames[6] = { "OBJECT", "SCENE", "LOOK", "MATERIAL", "CAMERA", "PROJECT" };
	const int current = (int)inspectorTab;
	for (int row = 0; row < 2; row++)
	{
		const ui::Rect r{ 12.0f, kBarH + 10.0f + row * (ui::kRowH + 4.0f), kInspectorW - 24.0f, ui::kRowH };
		ui::Choice("tabs" + std::to_string(row), r,
			{ kTabNames[row * 3], kTabNames[row * 3 + 1], kTabNames[row * 3 + 2] },
			(current / 3 == row) ? current % 3 : -1,
			[row](int i) { inspectorTab = (InspectorTab)(row * 3 + i); },
			"OBJECT: the selection.  SCENE: sky, sun, weather.  LOOK: how it renders.  "
			"MATERIAL: the selection's surface.  CAMERA: the scene's cameras.  PROJECT: every scene's defaults");
	}

	const float top = kBarH + 10.0f + 2.0f * (ui::kRowH + 4.0f) + 8.0f;
	const ui::Rect content{ 18.0f, top, kInspectorW - 24.0f, area.Bottom() - top - 8.0f };
	ui::Column col(std::string("insp.") + kTabNames[current], content, tabScroll[current]);
	switch (inspectorTab)
	{
	case InspectorTab::Object: InspectObject(game, col); break;
	case InspectorTab::Scene: InspectScene(game, col); break;
	case InspectorTab::Look: InspectLook(game, col); break;
	case InspectorTab::Material: InspectMaterial(game, col); break;
	case InspectorTab::Camera: InspectCameras(game, col); break;
	default: InspectProject(game, col); break;
	}
}

void Scene3DEditor::InspectObject(Game& game, ui::Column& col)
{
	Scene3D& scene = Scene3D::Get();
	auto commit = [this]() { CommitEdit(); };
	auto addButtons = [&]()
	{
		Buttons(col, "obj.add", {
			{ "POINT LIGHT", [this, &game]() { AddLight(game, false); }, "Add a point light above the floor at the middle of the view" },
			{ "SPOT LIGHT", [this, &game]() { AddLight(game, true); }, "Add a spot light shining down at the middle of the view" },
			{ "SLOT", [this, &game]() { AddSlot(game); }, "Add a slot: a named place where the schedule stands a character" } });
	};

	if (!HasSelection())
	{
		col.Note("Nothing selected.", ui::Colour::text);
		col.Note("Click something in the view, or in the list on the right.", ui::Colour::dim);
		col.Note("Lights are the small diamonds; slots are the cyan posts.", ui::Colour::dim);
		col.Header("ADD");
		addButtons();
		col.Note("Models: the toolbar's ADD list.", ui::Colour::dim);
		return;
	}

	auto positionRows = [&]()
	{
		const glm::vec3 p = SelectedPosition(game);
		static const char* const kNames[3] = { "POSITION X", "POSITION Y", "POSITION Z" };
		for (int a = 0; a < 3; a++)
		{
			NumberRow(col, std::string("obj.pos") + "xyz"[a], kNames[a], p[a], Spec(10.0f, false, -1e7f, 1e7f, "%.1f"),
				[this, &game, a](float v)
				{
					glm::vec3 q = SelectedPosition(game);
					q[a] = v;
					SetSelectedPosition(game, q);
				},
				commit, a == 1 ? "Height in world units (up is negative Y)" : "Position in world units");
		}
	};
	auto actionButtons = [&](bool clone)
	{
		std::vector<Action> actions;
		actions.push_back({ "ZOOM TO", [this, &game]() { ZoomToSelected(game); }, "Fly the camera to it" });
		if (clone)
			actions.push_back({ "CLONE", [this, &game]() { CloneSelected(game); }, "Make a copy one tile over (Ctrl+D)" });
		actions.push_back({ "DELETE", [this, &game]() { DeleteSelected(game); }, "Delete it (Delete key; UNDO brings it back)",
			ui::Tone::Danger });
		col.Space(8.0f);
		Buttons(col, "obj.act", actions);
	};

	// --- a model ---
	if (Scene3DModel* m = SelectedModel())
	{
		col.Header("MODEL  " + BaseName(m->objPath));
		positionRows();
		struct RotRow { const char* id; const char* name; float Scene3DModel::* field; };
		const RotRow rotations[3] = {
			{ "obj.yaw", "TURN", &Scene3DModel::yawDeg },
			{ "obj.pitch", "TILT", &Scene3DModel::pitchDeg },
			{ "obj.roll", "ROLL", &Scene3DModel::rollDeg } };
		for (const RotRow& rr : rotations)
		{
			float Scene3DModel::* field = rr.field;
			NumberRow(col, rr.id, rr.name, m->*field, Spec(15.0f, false, -36000.0f, 36000.0f, "%.1f"),
				[this, field](float v)
				{
					if (Scene3DModel* mm = SelectedModel())
					{
						mm->*field = v;
						Scene3D::Get().RecomputeModelBounds(mm);
					}
				},
				commit, "Rotation in degrees: TURN about the vertical, TILT forwards, ROLL sideways");
		}
		NumberRow(col, "obj.size", "SIZE", m->modelScale, Spec(1.1f, true, 0.02f, 1000.0f, "%.3f"),
			[this](float v)
			{
				if (Scene3DModel* mm = SelectedModel())
				{
					mm->modelScale = v;
					Scene3D::Get().RecomputeModelBounds(mm);
				}
			},
			commit, "Size: a multiplier on the whole model");
		static const char* const kStretch[3] = { "STRETCH X", "STRETCH Y", "STRETCH Z" };
		for (int a = 0; a < 3; a++)
		{
			NumberRow(col, std::string("obj.stretch") + "xyz"[a], kStretch[a], m->scaleAxis[a],
				Spec(1.1f, true, 0.02f, 1000.0f, "%.3f"),
				[this, a](float v)
				{
					if (Scene3DModel* mm = SelectedModel())
					{
						mm->scaleAxis[a] = v;
						Scene3D::Get().RecomputeModelBounds(mm);
					}
				},
				commit, "Stretch along one axis, on top of SIZE");
		}
		Buttons(col, "obj.reset", {
			{ "RESET POS", [this, &game]() { ResetTransform(game, 0); }, "Move it to the scene's origin" },
			{ "RESET ROT", [this, &game]() { ResetTransform(game, 1); }, "Clear its rotation" },
			{ "RESET SIZE", [this, &game]() { ResetTransform(game, 2); }, "Size 1, no stretch" } });

		col.Header("SURFACE AND RULES");
		if (IsGltfPath(m->objPath))
		{
			col.Note("A glTF model: it brings its own materials.", ui::Colour::dim);
		}
		else
		{
			ui::Rect c;
			if (col.Row("obj.mat", "MATERIAL", c))
			{
				const float editW = 92.0f;
				ui::Dropdown("obj.matlist", { c.x, c.y, c.w - editW - ui::kGap, c.h },
					m->materialName.empty() ? "(none)" : m->materialName,
					[]()
					{
						std::vector<std::string> items = { "(none)" };
						for (const std::string& n : MaterialLibrary::Get().Names())
							items.push_back(n);
						return items;
					},
					[this](int i)
					{
						Scene3DModel* mm = SelectedModel();
						if (mm == nullptr)
							return;
						const std::vector<std::string> names = MaterialLibrary::Get().Names();
						if (i == 0)
						{
							mm->materialName.clear();
							mm->material = nullptr;
						}
						else if (i - 1 < (int)names.size())
						{
							mm->materialName = names[i - 1];
							mm->material = MaterialLibrary::Get().Find(mm->materialName);
						}
						statusMsg = "Material: " + (mm->materialName.empty() ? std::string("none") : mm->materialName)
							+ "  (F5 saves)";
						statusFrames = 160;
						CommitEdit();
					},
					"Its material (shine, tint, glow, normal map), from data/materials.txt");
				ui::Button("obj.matedit", { c.Right() - editW, c.y, editW, c.h }, "EDIT",
					[]() { inspectorTab = InspectorTab::Material; },
					"Edit this material (the MATERIAL page)", ui::Tone::Normal, !m->materialName.empty());
			}
		}
		TextRow(col, "obj.tag", "TAG", m->interactionTag, "none", [this]() { StartNaming(PromptMode::Tag); },
			"A name the game finds it by (DB2: a clue id). Click to type it.");
		TextRow(col, "obj.guard", "GUARD", m->guard, "always there", [this]() { StartNaming(PromptMode::Guard); },
			"It's only there while this holds (the game decides): arrested, !arrested, clue:KNIFE, t>=18:00, day=2. Click to type it.");
		ToggleRow(col, "obj.solid", "SOLID", m->solid, [this]()
			{
				if (Scene3DModel* mm = SelectedModel())
				{
					mm->solid = !mm->solid;
					Scene3D::Get().RebuildSolids();
					CommitEdit();
				}
			},
			"Characters can't walk through it");

		if (m->IsWater())
		{
			col.Header("WATER");
			for (int i = 0; i < kNumWaterProps; i++)
			{
				const WaterPropDef& d = kWaterProps[i];
				NumberRow(col, "obj.water" + std::to_string(i), d.name, *WaterField(m->water, i),
					Spec(d.step, false, d.lo, d.hi, "%.2f"),
					[this, i](float v)
					{
						if (Scene3DModel* mm = SelectedModel())
							*WaterField(mm->water, i) = v;
					},
					commit, "This water surface's waves and shine (each water model has its own)");
			}
		}
		actionButtons(true);
		return;
	}

	// --- a character ---
	const std::vector<Character3D*>& chars = scene.GetCharacters();
	if (selType == SelType::Character && selIndex < (int)chars.size())
	{
		Character3D* c = chars[selIndex];
		col.Header("CHARACTER  " + c->charName);
		positionRows();
		NumberRow(col, "obj.height", "HEIGHT", c->worldHeight, Spec(1.1f, true, 20.0f, 5000.0f, "%.0f"),
			[this](float v)
			{
				const std::vector<Character3D*>& cs = Scene3D::Get().GetCharacters();
				if (selType == SelType::Character && selIndex >= 0 && selIndex < (int)cs.size())
					cs[selIndex]->worldHeight = v;
			},
			commit, "How tall the character stands, in world units");
		col.Note("Characters always face the camera.", ui::Colour::dim);
		actionButtons(false);
		return;
	}

	// --- a slot ---
	if (selType == SelType::Anchor && selIndex < (int)scene.Anchors().size())
	{
		const Scene3D::SceneAnchor& a = scene.Anchors()[selIndex];
		col.Header("SLOT  " + a.name);
		TextRow(col, "obj.slotname", "NAME", a.name, "", [this]() { StartNaming(PromptMode::SlotName); },
			"The game's schedule stands characters on slots by name. Click to rename it.");
		positionRows();
		NumberRow(col, "obj.facing", "FACING", a.yaw, Spec(15.0f, false, -36000.0f, 36000.0f, "%.0f"),
			[this](float v)
			{
				std::vector<Scene3D::SceneAnchor>& as = Scene3D::Get().Anchors();
				if (selType == SelType::Anchor && selIndex >= 0 && selIndex < (int)as.size())
					as[selIndex].yaw = v;
			},
			commit, "The way a character standing here faces, in degrees");
		col.Note("Renaming a slot breaks the game's schedule entries that use the old name.", ui::Colour::dim);
		actionButtons(false);
		return;
	}

	// --- a light ---
	ScenePointLight* pl = SelectedPointLight();
	SceneSpotLight* sl = SelectedSpotLight();
	if (pl == nullptr && sl == nullptr)
		return;
	col.Header(std::string(pl ? "POINT LIGHT  " : "SPOT LIGHT  ") + (pl ? pl->name : sl->name));
	positionRows();
	ToggleRow(col, "light.on", "ON", pl ? pl->on : sl->on, [this]()
		{
			if (ScenePointLight* p = SelectedPointLight()) { p->on = !p->on; p->fade.active = false; }
			if (SceneSpotLight* s = SelectedSpotLight()) { s->on = !s->on; s->fade.active = false; }
			CommitEdit();
		},
		"Switch it on or off (saved that way; a script can switch it later)");
	auto lightRow = [&](LightProp p, const std::string& label, const std::string& help)
	{
		const LightStepDef* d = FindLightStep(p);
		float value = 0.0f;
		std::string text;
		ReadLightValue(p, pl, sl, scene, value, text);
		NumberRow(col, std::string("light.") + d->name, label, value,
			Spec(d->step, d->mul, d->lo, d->hi, d->fmt, d->floor),
			[this, p](float v) { ApplyLightValue(p, v, SelectedPointLight(), SelectedSpotLight(), Scene3D::Get()); },
			commit, help, true, nullptr, text == "custom" ? std::string() : text);
	};
	lightRow(LightProp::Intensity, "INTENSITY", "How bright it is");
	lightRow(LightProp::Range, "RANGE", "How far its light reaches (SCALE drags it in the view too)");
	if (pl)
	{
		lightRow(LightProp::Flash, "FLASH", "Blinks this many times a second (0 = steady): sirens, alarms");
		lightRow(LightProp::FlashPhase, "FLASH PHASE", "Where in the blink it starts: two lights at 0 and 0.5 take turns");
	}
	else
	{
		lightRow(LightProp::Inner, "INNER CONE", "Full brightness inside this angle (degrees from the middle)");
		lightRow(LightProp::Outer, "OUTER CONE", "Fades to nothing by this angle");
		lightRow(LightProp::AimTurn, "AIM TURN", "Which way it points, around the vertical (ROTATE drags it too)");
		lightRow(LightProp::AimTilt, "AIM TILT", "How far down it points: 90 = straight down");
	}
	col.Header("COLOUR");
	{
		float value = 0.0f;
		std::string text;
		ReadLightValue(LightProp::Temp, pl, sl, scene, value, text);
		char buf[32];
		snprintf(buf, sizeof(buf), "%.0f K", value);
		StepperRow(col, "light.temp", "TEMPERATURE", text.empty() ? std::string(buf) : text, [this](int dir)
			{
				glm::vec3* c = SelectedPointLight() ? &SelectedPointLight()->color
					: SelectedSpotLight() ? &SelectedSpotLight()->color : nullptr;
				if (c == nullptr)
					return;
				float kelvin = 0.0f;
				*c = StepKelvin(*c, dir, kelvin);
				CommitEdit();
			},
			"Warm to cool: candle 1800 K, lamp 2700 K, daylight 6500 K, blue sky 12000 K (keeps the brightness)");
	}
	lightRow(LightProp::Red, "RED", "The light's colour");
	lightRow(LightProp::Green, "GREEN", "The light's colour");
	lightRow(LightProp::Blue, "BLUE", "The light's colour");
	col.Header("LAMP");
	if (pl)
	{
		ToggleRow(col, "light.shadow", "CASTS SHADOWS", scene.shadowCasterLight == pl->name, [this]()
			{
				if (ScenePointLight* p = SelectedPointLight())
				{
					Scene3D& s = Scene3D::Get();
					s.shadowCasterLight = (s.shadowCasterLight == p->name) ? std::string() : p->name;
					CommitEdit();
				}
			},
			"Make this the light whose shadows the scene draws (otherwise the strongest lights do)");
	}
	TextRow(col, "light.name", "NAME", pl ? pl->name : sl->name, "", [this]() { StartNaming(PromptMode::LightName); },
		"Scripts switch lights by name: scene3d light <name> on | off | fade ...  Click to rename it.");
	if (pl)
		TextRow(col, "light.guard", "GUARD", pl->guard, "always on", [this]() { StartNaming(PromptMode::Guard); },
			"It's only on while this holds (the game decides): arrested, !arrested, clue:KNIFE ...  Click to type it.");
	actionButtons(true);
}

void Scene3DEditor::InspectScene(Game& game, ui::Column& col)
{
	Scene3D& scene = Scene3D::Get();
	auto commit = [this]() { CommitEdit(); };

	col.Header("SKY");
	DropdownRow(col, "scene.sky", "PANORAMA", scene.GetAuthoredSky().empty() ? "(no sky)" : LookBaseName(scene.GetAuthoredSky()),
		[]()
		{
			std::vector<std::string> labels;
			SkyChoices(labels, listValues);
			return labels;
		},
		[this, &game](int i)
		{
			if (i < 0 || i >= (int)listValues.size())
				return;
			Scene3D::Get().SetAuthoredSky(game, listValues[i]);
			statusMsg = "Sky: " + (listValues[i].empty() ? std::string("none") : LookBaseName(listValues[i])) + "  (F5 saves)";
			statusFrames = 180;
			CommitEdit();
		},
		"The sky: an ordinary panorama image (zenith along the top edge)");
	LookRow(col, LookProp::SkySize, "SIZE", commit, scene.HasSky());

	auto lightRow = [&](LightProp p, const std::string& label, const std::string& help)
	{
		const LightStepDef* d = FindLightStep(p);
		float value = 0.0f;
		std::string text;
		ReadLightValue(p, nullptr, nullptr, scene, value, text);
		NumberRow(col, std::string("scene.") + d->name, label, value, Spec(d->step, d->mul, d->lo, d->hi, d->fmt, d->floor),
			[p](float v) { ApplyLightValue(p, v, nullptr, nullptr, Scene3D::Get()); },
			commit, help, true, nullptr, text == "custom" ? std::string() : text);
	};
	auto temperature = [&](LightProp p, const std::string& id, bool sun)
	{
		float value = 0.0f;
		std::string text;
		ReadLightValue(p, nullptr, nullptr, scene, value, text);
		char buf[32];
		snprintf(buf, sizeof(buf), "%.0f K", value);
		StepperRow(col, id, "TEMPERATURE", text.empty() ? std::string(buf) : text, [this, sun](int dir)
			{
				Scene3D& s = Scene3D::Get();
				float kelvin = 0.0f;
				if (sun)
					s.SetDirectionalLight(StepKelvin(s.GetDirectionalLight().color, dir, kelvin), s.GetDirectionalLight().diffuse);
				else
					s.SetAmbientLight(StepKelvin(s.GetAmbientLight(), dir, kelvin));
				CommitEdit();
			},
			"Warm to cool presets (keeps the brightness)");
	};

	col.Header("SUN");
	lightRow(LightProp::Sun, "STRENGTH", "How strong the sun is (0 = no sun)");
	temperature(LightProp::SunTemp, "scene.suntemp", true);
	lightRow(LightProp::SunHeading, "HEADING", "Which way the sunlight travels, around the vertical");
	lightRow(LightProp::SunHeight, "HEIGHT", "How high the sun is: low = long shadows");
	lightRow(LightProp::SunRed, "RED", "The sunlight's colour");
	lightRow(LightProp::SunGreen, "GREEN", "The sunlight's colour");
	lightRow(LightProp::SunBlue, "BLUE", "The sunlight's colour");

	col.Header("AMBIENT LIGHT");
	lightRow(LightProp::Ambient, "BRIGHTNESS", "The light that reaches everywhere");
	temperature(LightProp::AmbientTemp, "scene.ambtemp", false);
	lightRow(LightProp::AmbientRed, "RED", "The ambient light's colour");
	lightRow(LightProp::AmbientGreen, "GREEN", "The ambient light's colour");
	lightRow(LightProp::AmbientBlue, "BLUE", "The ambient light's colour");
	col.Note("A game with its own time of day may set the sun and ambient light itself.", ui::Colour::faint);

	col.Header("WEATHER AND SEASON");
	ChoiceRow(col, "scene.weather", "WEATHER", { "NONE", "RAIN", "SNOW", "STORM" }, (int)scene.GetWeather(),
		[this](int i)
		{
			Scene3D& s = Scene3D::Get();
			float intensity = s.GetWeatherIntensity();
			if (intensity <= 0.0f)
				intensity = 1.0f;
			s.SetWeather((Scene3D::WeatherType)i, intensity);
			CommitEdit();
		},
		"Rain, snow, or a storm (rain with lightning and thunder)");
	LookRow(col, LookProp::Weather, "INTENSITY", commit, scene.GetWeather() != Scene3D::WeatherType::None);
	{
		static const Scene3D::Season kSeasons[4] = { Scene3D::Season::Spring, Scene3D::Season::Summer,
			Scene3D::Season::Autumn, Scene3D::Season::Winter };
		int chosen = 0;
		for (int i = 0; i < 4; i++)
			if (kSeasons[i] == scene.GetSeason())
				chosen = i;
		ChoiceRow(col, "scene.season", "SEASON", { "SPRING", "SUMMER", "AUTUMN", "WINTER" }, chosen,
			[this, &game](int i)
			{
				Scene3D::Get().SetSeason(game, kSeasons[i]);
				CommitEdit();
			},
			"Swaps seasonal textures (grass, leaves) and bares deciduous trees in winter");
	}

	col.Header("FOUNTAIN");
	ToggleRow(col, "scene.fountain", "FOUNTAIN", scene.HasFountain(), [this]()
		{
			Scene3D& sc = Scene3D::Get();
			if (sc.HasFountain())
				sc.ClearFountain();
			else
			{
				// On top of the selected model (up is -Y, so aabbMin.y is its top).
				glm::vec3 pos(0.0f, -100.0f, 0.0f);
				if (Scene3DModel* m = SelectedModel())
					pos = glm::vec3(m->position.x, m->aabbMin.y, m->position.z);
				sc.SetFountain(pos);
			}
			CommitEdit();
		},
		"A water jet: on top of the selected model, else near the origin");
	if (scene.HasFountain())
	{
		for (int i = 0; i < kNumFountainProps; i++)
		{
			const FountainPropDef& d = kFountainProps[i];
			NumberRow(col, "scene.fountain" + std::to_string(i), d.name, GetFountainProp(scene, i),
				Spec(d.step, false, d.lo, d.hi, i == 4 ? "%.0f" : "%.1f"),
				[i](float v) { SetFountainProp(Scene3D::Get(), i, v); }, commit, "The fountain's jet");
		}
	}

	col.Header("LAMP SHADOWS");
	DropdownRow(col, "scene.caster", "CAST BY", scene.shadowCasterLight.empty() ? "the strongest lights" : scene.shadowCasterLight,
		[]()
		{
			listValues = Scene3D::Get().PointLightNames();
			listValues.insert(listValues.begin(), std::string());
			std::vector<std::string> labels = listValues;
			labels[0] = "the strongest lights";
			return labels;
		},
		[this](int i)
		{
			if (i < 0 || i >= (int)listValues.size())
				return;
			Scene3D::Get().shadowCasterLight = listValues[i];
			CommitEdit();
		},
		"Which point light's shadows the scene draws");

	col.Header("ADD");
	Buttons(col, "scene.add", {
		{ "POINT LIGHT", [this, &game]() { AddLight(game, false); }, "Add a point light above the floor at the middle of the view" },
		{ "SPOT LIGHT", [this, &game]() { AddLight(game, true); }, "Add a spot light shining down at the middle of the view" },
		{ "SLOT", [this, &game]() { AddSlot(game); }, "Add a slot: a named place where the schedule stands a character" } });
}

void Scene3DEditor::InspectLook(Game& game, ui::Column& col)
{
	(void)game;
	Scene3D& scene = Scene3D::Get();
	auto commit = [this]() { CommitEdit(); };
	if (!LinearWorkflow())
	{
		col.Note("linearLighting is off (PROJECT page): most of these", ui::Colour::warning);
		col.Note("won't show. KINJO_LINEAR=1 previews it for one run.", ui::Colour::dim);
	}
	col.Note("Grey = the project's value; click a name to go back to it.", ui::Colour::dim);

	col.Header("LIGHT");
	LookRow(col, LookProp::Exposure, "EXPOSURE", commit);
	LookRow(col, LookProp::Bloom, "BLOOM", commit);
	LookRow(col, LookProp::SkyLight, "SKY LIGHT", commit, scene.HasSky());
	LookRow(col, LookProp::SkyShine, "SKY REFLECTIONS", commit, scene.HasSky());
	LookRow(col, LookProp::Ao, "AMBIENT SHADE", commit);
	LookRow(col, LookProp::AoRadius, "SHADE RADIUS", commit);
	LookRow(col, LookProp::Shadows, "SHADOW REACH", commit);

	col.Header("FOG");
	LookRow(col, LookProp::Fog, "DENSITY", commit);
	LookRow(col, LookProp::FogFalloff, "FALLOFF", commit);
	LookRow(col, LookProp::FogGlow, "GLOW", commit);
	LookRow(col, LookProp::FogDrift, "DRIFT", commit);
	LookRow(col, LookProp::FogRed, "RED", commit);
	LookRow(col, LookProp::FogGreen, "GREEN", commit);
	LookRow(col, LookProp::FogBlue, "BLUE", commit);

	col.Header("DISTANCE FOG");
	{
		DistanceFogSettings f;
		const bool own = GetSceneDistanceFog(f);
		if (!own)
			f = DistanceFogInForce();
		std::function<void()> reset;
		if (own)
			reset = [this]() { SetSceneDistanceFog(false, DistanceFogSettings(), 0.0f); CommitEdit(); };
		ToggleRow(col, "look.distfog", "DISTANCE FOG", f.on, [this]()
			{
				// The fog in force becomes the scene's own, switched.
				DistanceFogSettings g;
				if (!GetSceneDistanceFog(g))
					g = DistanceFogInForce();
				g.on = !g.on;
				SetSceneDistanceFog(true, g, 0.0f);
				CommitEdit();
			},
			"Everything fades to one colour with its distance from the camera (any colour mode; the sky panorama doesn't)",
			own, reset);
		LookRow(col, LookProp::DistFogNear, "STARTS AT", commit, f.on);
		LookRow(col, LookProp::DistFogFar, "FULL AT", commit, f.on);
		LookRow(col, LookProp::DistFogRed, "RED", commit, f.on);
		LookRow(col, LookProp::DistFogGreen, "GREEN", commit, f.on);
		LookRow(col, LookProp::DistFogBlue, "BLUE", commit, f.on);
		if (f.on && FogActive())
			col.Note("Both fogs are on, and they add up: use one.", ui::Colour::warning);
	}

	col.Header("DEPTH OF FIELD");
	LookRow(col, LookProp::Focus, "FOCUS", commit);
	LookRow(col, LookProp::Aperture, "BLUR", commit);
	Buttons(col, "look.pick", { { lookPickFocus ? "NOW CLICK IN THE VIEW..." : "PICK THE FOCUS IN THE VIEW",
		[this]() { lookPickFocus = !lookPickFocus; },
		"Then click what the camera should focus on (turns depth of field on if it's off)",
		lookPickFocus ? ui::Tone::Accent : ui::Tone::Normal } });

	col.Header("COLOUR GRADE");
	{
		std::string path, projectPath;
		float strength = 1.0f, projectStrength = 1.0f;
		GetSceneColorGrade(path, strength);
		GetProjectColorGrade(projectPath, projectStrength);
		const std::string shown = path.empty() ? "(the project's)" : path == "none" ? "(none)" : LookBaseName(path);
		DropdownRow(col, "look.lut", "LUT", shown,
			[]()
			{
				std::vector<std::string> labels;
				LutChoices(labels, listValues, false);
				return labels;
			},
			[this](int i)
			{
				if (i < 0 || i >= (int)listValues.size())
					return;
				// Keep the strength already set when swapping one LUT for another.
				std::string current;
				float s = 1.0f;
				GetSceneColorGrade(current, s);
				if (current.empty() || current == "none")
					s = 1.0f;
				Scene3D::Get().SetColorGrade(listValues[i], s);
				CommitEdit();
			},
			"A colour look from a LUT image in data/luts (start from the neutral one and grade it)", !path.empty(),
			path.empty() ? std::function<void()>() : [this]() { Scene3D::Get().SetColorGrade("", 1.0f); CommitEdit(); });
		const bool graded = path.empty() ? !projectPath.empty() : path != "none";
		LookRow(col, LookProp::Grade, "STRENGTH", commit, graded);
	}

	col.Header("TOON AND OUTLINE");
	for (int i = 0; i < kToonRowCount; i++)
	{
		const ToonRowDef& d = kToonRows[i];
		if (d.kind == ToonKind::Back || d.kind == ToonKind::GameValues)
			continue;
		const Scene3DInternal::ToonSetting setting = d.setting;
		const bool own = Scene3DInternal::SceneOwnsToonSetting(setting);
		std::function<void()> reset;
		if (own)
			reset = [this, setting]() { Scene3DInternal::ResetToonSetting(setting); CommitEdit(); };
		const std::string id = "look.toon" + std::to_string(i);
		if (d.kind == ToonKind::OnOff)
		{
			bool on = (setting == Scene3DInternal::ToonSetting::CelShading) ? scene.celShading
				: (setting == Scene3DInternal::ToonSetting::Outline) ? scene.outlineEnabled : scene.outlineCharacters;
			const char* label = (setting == Scene3DInternal::ToonSetting::OutlineCharacters) ? "OUTLINE PEOPLE" : d.name;
			ToggleRow(col, id, label, on, [this, setting]()
				{
					Scene3D& s = Scene3D::Get();
					Scene3DInternal::OwnToonSetting(setting);
					bool& field = (setting == Scene3DInternal::ToonSetting::CelShading) ? s.celShading
						: (setting == Scene3DInternal::ToonSetting::Outline) ? s.outlineEnabled : s.outlineCharacters;
					field = !field;
					CommitEdit();
				},
				setting == Scene3DInternal::ToonSetting::CelShading ? "Banded toon lighting plus an ink outline"
				: setting == Scene3DInternal::ToonSetting::Outline ? "The ink outline (shows with cel shading on)"
				: "Outline the character sprites too",
				own, reset);
		}
		else
		{
			const std::string label = (setting == Scene3DInternal::ToonSetting::OutlineColor)
				? std::string("OUTLINE ") + d.name : (setting == Scene3DInternal::ToonSetting::OutlineWidth)
				? std::string("OUTLINE WIDTH") : std::string("OUTLINE EDGES");
			NumberRow(col, id, label, ToonValue(d), Spec(d.step, d.mul, d.lo, d.hi, d.fmt),
				[i](float v)
				{
					Scene3DInternal::OwnToonSetting(kToonRows[i].setting);
					ToonField(kToonRows[i]) = v;
				},
				commit,
				setting == Scene3DInternal::ToonSetting::OutlineWidth ? "The outline's thickness in pixels"
				: setting == Scene3DInternal::ToonSetting::OutlineDepth ? "How readily a change in depth draws a line (smaller = more lines)"
				: "The outline's colour",
				own, reset);
		}
	}
	col.Note("Grey = the game's own settings (it sets them in code).", ui::Colour::dim);

	col.Header("DEBUG VIEWS (NOT SAVED)");
	ToggleRow(col, "look.aoview", "SHOW AMBIENT SHADE", AmbientOcclusionDebugView(), []()
		{
			if (lookAoViewBefore < 0)
				lookAoViewBefore = AmbientOcclusionDebugView() ? 1 : 0;
			SetAmbientOcclusionDebugView(!AmbientOcclusionDebugView());
		},
		"Show only the ambient occlusion on lit surfaces");
	ToggleRow(col, "look.lightcount", "SHOW LIGHT COUNT", ClusterDebugView(), []()
		{
			if (lookLightCountBefore < 0)
				lookLightCountBefore = ClusterDebugView() ? 1 : 0;
			SetClusterDebugView(!ClusterDebugView());
		},
		"Colour surfaces by how many lights reach them");
}

void Scene3DEditor::InspectMaterial(Game& game, ui::Column& col)
{
	(void)game;
	Scene3D& scene = Scene3D::Get();
	auto commit = [this]() { CommitEdit(); };
	Scene3DModel* model = SelectedModel();
	if (model == nullptr)
	{
		col.Note("Select a model to edit its material.", ui::Colour::text);
		col.Note("Click one in the view, or in the list on the right.", ui::Colour::dim);
		return;
	}
	if (IsGltfPath(model->objPath))
	{
		col.Header("MATERIAL");
		col.Note(BaseName(model->objPath) + " is a glTF model:", ui::Colour::text);
		col.Note("it brings its own materials, so these don't apply.", ui::Colour::dim);
		return;
	}
	SceneMaterial* mat = model->materialName.empty() ? nullptr : MaterialLibrary::Get().FindMutable(model->materialName);
	auto current = [this]() -> SceneMaterial*
	{
		Scene3DModel* m = SelectedModel();
		return (m != nullptr && !m->materialName.empty()) ? MaterialLibrary::Get().FindMutable(m->materialName) : nullptr;
	};
	auto assign = [&]()
	{
		DropdownRow(col, "mat.assign", mat ? "USE ANOTHER" : "MATERIAL", model->materialName.empty() ? "(none)" : model->materialName,
			[]()
			{
				std::vector<std::string> items = { "(none)" };
				for (const std::string& n : MaterialLibrary::Get().Names())
					items.push_back(n);
				return items;
			},
			[this](int i)
			{
				Scene3DModel* mm = SelectedModel();
				if (mm == nullptr)
					return;
				const std::vector<std::string> names = MaterialLibrary::Get().Names();
				if (i == 0)
				{
					mm->materialName.clear();
					mm->material = nullptr;
				}
				else if (i - 1 < (int)names.size())
				{
					mm->materialName = names[i - 1];
					mm->material = MaterialLibrary::Get().Find(mm->materialName);
				}
				CommitEdit();
			},
			"Give the model a different material");
		Buttons(col, "mat.new", { { mat ? "NEW MATERIAL: A COPY OF THIS" : "NEW MATERIAL FOR IT",
			[this]() { StartNaming(PromptMode::MaterialName); },
			"A new material just for this model (a copy of the current one): then edits change only it" } });
	};
	if (mat == nullptr)
	{
		col.Header("MATERIAL");
		col.Note(BaseName(model->objPath) + " uses the plain default.", ui::Colour::text);
		assign();
		return;
	}

	int users = 0;
	for (const Scene3DModel* m : scene.GetModels())
		users += (m->materialName == mat->name) ? 1 : 0;
	col.Header("MATERIAL  " + mat->name);
	col.Note("Used by " + std::to_string(users) + (users == 1 ? " model" : " models")
		+ " here, maybe more in other scenes.", ui::Colour::dim);
	col.Note("Edits change them all. F5 saves materials.txt.", ui::Colour::dim);
	assign();

	auto matRow = [&](MatProp p, const std::string& label, const std::string& help)
	{
		const MatStepDef* d = FindMatStep(p);
		ui::NumberSpec spec = Spec(d->step, d->mul, d->lo, d->hi, d->fmt, d->floor);
		if (p == MatProp::TileU || p == MatProp::TileV)
		{
			spec.step = 0.25f;
			spec.stepFn = StepTiling;
		}
		NumberRow(col, std::string("mat.") + d->name, label, *MatField(*mat, p), spec,
			[current, p](float v)
			{
				if (SceneMaterial* m = current())
					*MatField(*m, p) = v;
			},
			commit, help);
	};

	col.Header("SURFACE");
	ChoiceRow(col, "mat.lighting", "LIGHTING", { "PHONG", "PBR", "WATER" }, (int)mat->lighting,
		[this, current](int i)
		{
			if (SceneMaterial* m = current())
			{
				m->lighting = (LightingModel)i;
				CommitEdit();
			}
		},
		"PHONG: shine and highlight size.  PBR: metal and roughness.  WATER: animated waves (tuned per water model)");
	if (mat->lighting == LightingModel::PBR)
	{
		matRow(MatProp::Metallic, "METAL", "0 = not metal, 1 = metal");
		matRow(MatProp::Roughness, "ROUGHNESS", "Low = mirror-like, high = matte");
		const std::string roughMap = MaterialLibrary::Get().RoughnessMapPath(*mat);
		DropdownRow(col, "mat.roughmap", "ROUGHNESS MAP", roughMap.empty() ? "(none)" : LookBaseName(roughMap),
			[this]()
			{
				std::vector<std::string> labels;
				Scene3DModel* m = SelectedModel();
				const SceneMaterial* sm = (m && !m->materialName.empty()) ? MaterialLibrary::Get().Find(m->materialName) : nullptr;
				if (sm != nullptr)
					MapChoices(m, MaterialLibrary::Get().RoughnessMapPath(*sm), MapKind::Roughness, labels, listValues);
				return labels;
			},
			[this, &game, current](int i)
			{
				SceneMaterial* m = current();
				if (m == nullptr || i < 0 || i >= (int)listValues.size())
					return;
				MaterialLibrary::Get().SetRoughnessMap(game, *m, listValues[i]);
				CommitEdit();
			},
			"Roughness per spot: green times ROUGHNESS, blue times METAL (dark = shinier: puddles). Names with rough");
	}
	else
	{
		matRow(MatProp::Specular, "SHINE", "How bright highlights are");
		matRow(MatProp::Shininess, "HIGHLIGHT SIZE", "Higher = smaller, sharper highlights");
	}
	matRow(MatProp::Fresnel, "RIM", "A glow at grazing angles (ice, glass)");
	matRow(MatProp::Opacity, "OPACITY", "Below 1 = see-through");

	col.Header("COLOUR");
	matRow(MatProp::TintR, "TINT RED", "Multiplies the texture's colour");
	matRow(MatProp::TintG, "TINT GREEN", "Multiplies the texture's colour");
	matRow(MatProp::TintB, "TINT BLUE", "Multiplies the texture's colour");
	matRow(MatProp::GlowR, "GLOW RED", "Light it gives off itself (above 1 blooms)");
	matRow(MatProp::GlowG, "GLOW GREEN", "Light it gives off itself (above 1 blooms)");
	matRow(MatProp::GlowB, "GLOW BLUE", "Light it gives off itself (above 1 blooms)");
	const std::string glowMap = MaterialLibrary::Get().EmissiveMapPath(*mat);
	DropdownRow(col, "mat.glowmap", "GLOW MAP", glowMap.empty() ? "(none)" : LookBaseName(glowMap),
		[this]()
		{
			std::vector<std::string> labels;
			Scene3DModel* m = SelectedModel();
			const SceneMaterial* sm = (m && !m->materialName.empty()) ? MaterialLibrary::Get().Find(m->materialName) : nullptr;
			if (sm != nullptr)
				MapChoices(m, MaterialLibrary::Get().EmissiveMapPath(*sm), MapKind::Glow, labels, listValues);
			return labels;
		},
		[this, &game, current](int i)
		{
			SceneMaterial* m = current();
			if (m == nullptr || i < 0 || i >= (int)listValues.size())
				return;
			MaterialLibrary::Get().SetEmissiveMap(game, *m, listValues[i]);
			CommitEdit();
		},
		"Where it glows: the glow colour times this image (lit windows, signs). Names with glow / emissive");

	col.Header("TEXTURE");
	matRow(MatProp::TileU, "TILE ACROSS", "How many times the texture repeats across");
	matRow(MatProp::TileV, "TILE DOWN", "How many times the texture repeats down");
	DropdownRow(col, "mat.normal", "NORMAL MAP", mat->normalMapPath.empty() ? "(none)" : LookBaseName(mat->normalMapPath),
		[this]()
		{
			std::vector<std::string> labels;
			Scene3DModel* m = SelectedModel();
			const SceneMaterial* sm = (m && !m->materialName.empty()) ? MaterialLibrary::Get().Find(m->materialName) : nullptr;
			if (sm != nullptr)
				MapChoices(m, sm->normalMapPath, MapKind::Normal, labels, listValues);
			return labels;
		},
		[this, &game, current](int i)
		{
			SceneMaterial* m = current();
			if (m == nullptr || i < 0 || i >= (int)listValues.size())
				return;
			MaterialLibrary::Get().SetNormalMap(game, *m, listValues[i]);
			CommitEdit();
		},
		"A normal map fakes surface depth (OpenGL convention: green = up the texture)");
	matRow(MatProp::NormalStrength, "NORMAL DEPTH", "How strong the normal map's bumps are");
	ChoiceRow(col, "mat.normalmode", "NORMAL MODE", { "SCREEN", "VERTEX" }, mat->normalMode == NormalMode::Vertex ? 1 : 0,
		[this, current](int i)
		{
			if (SceneMaterial* m = current())
			{
				m->normalMode = (i == 1) ? NormalMode::Vertex : NormalMode::ScreenSpace;
				CommitEdit();
			}
		},
		"SCREEN: exact, from the surface.  VERTEX: directions from the mesh's tangents");

	col.Header("RULES");
	ToggleRow(col, "mat.outline", "OUTLINE", mat->outline, [this, current]()
		{
			if (SceneMaterial* m = current())
			{
				m->outline = !m->outline;
				CommitEdit();
			}
		},
		"Draw the ink outline around it (with cel shading on)");
	ChoiceRow(col, "mat.season", "SEASONS", { "NONE", "SWAP", "BARE" }, mat->deciduous ? 2 : mat->seasonal ? 1 : 0,
		[this, current](int i)
		{
			if (SceneMaterial* m = current())
			{
				m->seasonal = i >= 1;
				m->deciduous = i == 2;
				CommitEdit();
			}
		},
		"SWAP: textures change to <texture>_<season>.png.  BARE: also leafless in winter (<model>_bare.obj)");
}

void Scene3DEditor::InspectCameras(Game& game, ui::Column& col)
{
	Scene3D& scene = Scene3D::Get();
	const std::vector<std::string>& order = scene.CameraOrder();
	col.Header("CAMERAS");
	if (order.empty())
		col.Note("No cameras yet: NEW FROM VIEW adds one.", ui::Colour::dim);
	for (size_t i = 0; i < order.size(); i++)
	{
		ui::Rect r;
		if (!col.Line(r, 36.0f))
			continue;
		const std::string name = order[i];
		ui::Item("cam." + name, r, i == 0 ? "START" : " ", name, name == currentCamName,
			[this, &game, name]()
			{
				JumpToCameraByName(game, name);
				statusMsg = "Camera: " + name;
				statusFrames = 140;
			},
			"Jump the view to this camera (and choose it for the buttons below)");
	}
	col.Space(6.0f);
	const bool chosen = !currentCamName.empty();
	if (chosen)
		TextRow(col, "cam.name", "NAME", currentCamName, "", [this]() { StartNaming(PromptMode::CameraName); },
			"Scripts cut and glide to cameras by name (scene3d cam / glide). Click to rename it.");
	Buttons(col, "cam.a", {
		{ chosen ? "SAVE VIEW TO " + currentCamName : "SAVE VIEW", [this, &game]() { CameraAction(game, 0); },
			"Put the current view into the chosen camera" },
		{ "NEW FROM VIEW", [this, &game]() { CameraAction(game, 1); }, "A new camera at the current view" } });
	Buttons(col, "cam.b", {
		{ "MAKE IT THE START", [this, &game]() { CameraAction(game, 2); }, "The scene opens on the chosen camera",
			ui::Tone::Normal, chosen },
		{ "DELETE", [this, &game]() { CameraAction(game, 3); }, "Delete the chosen camera", ui::Tone::Danger, chosen } });
	col.Note("The scene opens on its START camera.", ui::Colour::dim);
}

void Scene3DEditor::InspectProject(Game& game, ui::Column& col)
{
	(void)game;
	if (projectConfig.empty())
		projectConfig = GetMapStringsFromFile(RendererConfigPath());
	col.Note("Every scene starts from these. Each change saves at once to", ui::Colour::dim);
	col.Note(RendererConfigPath(), ui::Colour::text);
	col.Note("Grey = not in the file (the engine's default).", ui::Colour::dim);
	// The game's own values over the file's (Renderer::SetRenderSetting: a
	// player's graphics level) win while they're set
	if (!RendererSettingOverrides().empty())
	{
		std::string over;
		for (const auto& [key, value] : RendererSettingOverrides())
			over += (over.empty() ? "" : ", ") + key + " " + value;
		col.Note("The game overrides: " + over, ui::Colour::text);
	}

	auto write = [this](int row, const std::string& value)
	{
		const ProjectRowDef& d = kProjectRows[row];
		std::string message;
		if (!WriteRendererSetting(d.key, value, message))
		{
			statusMsg = "renderer.dat: " + message;
			statusFrames = 240;
			return;
		}
		projectConfig = GetMapStringsFromFile(RendererConfigPath());
		ReloadRenderSettingsLive();
		statusMsg = std::string(d.key) + " " + value + " saved";
		if (d.restart)
		{
			projectRestartKeys.insert(d.key);
			statusMsg += "  - RESTART the game to apply it";
		}
		else if (std::string(d.key) == "anisotropy")
			statusMsg += "  (textures loaded from now on; restart for all)";
		else
			statusMsg += "  (every scene, now)";
		statusFrames = 240;
	};

	static const char* const kSections[4] = { "COLOUR", "LIGHT AND SHADOW", "FOG", "QUALITY" };
	for (int section = 0; section < 4; section++)
	{
		col.Header(kSections[section]);
		for (int i = 0; i < kProjectRowCount; i++)
		{
			const ProjectRowDef& d = kProjectRows[i];
			if (d.column != section || d.kind == ProjectKind::Back)
				continue;
			const std::string id = std::string("proj.") + d.key;
			const bool own = ProjectHas(d);
			const std::string help = std::string(d.key) + " in renderer.dat" + (d.restart ? " (needs a restart)" : "");
			switch (d.kind)
			{
			case ProjectKind::OnOff:
			{
				const bool on = ProjectValue(d) == "1";
				std::string label = d.name;
				if (projectRestartKeys.count(d.key) != 0)
					label += " (RESTART)";
				ToggleRow(col, id, label, on, [write, i, on]() { write(i, on ? "0" : "1"); }, help, own);
				break;
			}
			case ProjectKind::Choice:
			{
				const std::vector<std::string> choices = ProjectChoices(d);
				std::vector<std::string> shown;
				int chosen = -1;
				for (size_t k = 0; k < choices.size(); k++)
				{
					std::string upper = choices[k];
					for (char& ch : upper)
						ch = (char)std::toupper((unsigned char)ch);
					if (upper == "AGX_PUNCHY")
						upper = "PUNCHY";
					if (std::string(d.key) == "msaa")
						upper = (choices[k] == "0") ? "OFF" : choices[k] + "X";
					shown.push_back(upper);
					if (choices[k] == ProjectValue(d))
						chosen = (int)k;
				}
				ChoiceRow(col, id, d.name, shown, chosen, [write, i, choices](int k) { write(i, choices[k]); }, help, own);
				break;
			}
			case ProjectKind::Lut:
			{
				const std::string value = ProjectValue(d);
				DropdownRow(col, id, d.name, (value.empty() || value == "none") ? "(none)" : LookBaseName(value),
					[]()
					{
						std::vector<std::string> labels;
						LutChoices(labels, listValues, true);
						return labels;
					},
					[write, i](int k)
					{
						if (k >= 0 && k < (int)listValues.size())
							write(i, listValues[k]);
					},
					help, own);
				break;
			}
			case ProjectKind::Value:
			{
				ui::NumberSpec spec = Spec(d.step, d.mul, d.lo, d.hi, d.fmt, d.floor, false);
				const char* fmt = d.fmt;
				NumberRow(col, id, d.name, ProjectNumber(d), spec,
					[write, i](float v)
					{
						std::ostringstream ss;
						ss << v;
						write(i, ss.str());
					},
					nullptr, help + "  ([-] / [+])", own);
				(void)fmt;
				break;
			}
			default:
				break;
			}
		}
	}
}

// ------------------------------------------------------------- object list

void Scene3DEditor::RenderOutliner(Game& game, const Renderer& renderer)
{
	(void)renderer;
	Scene3D& scene = Scene3D::Get();
	const float gw = game.designWidth * Camera::MULTIPLIER;
	const float gh = game.designHeight * Camera::MULTIPLIER;
	const ui::Rect area{ gw - kOutlinerW, kBarH, kOutlinerW, gh - kBarH - kStatusH };
	ui::Panel(area, ui::Colour::panel);

	struct Entry { SelType type; int index; const char* tag; std::string name; };
	std::vector<Entry> entries;
	const auto& models = scene.GetModels();
	for (size_t i = 0; i < models.size(); i++)
		entries.push_back({ SelType::Model, (int)i, "MODEL", BaseName(models[i]->objPath) });
	const auto& chars = scene.GetCharacters();
	for (size_t i = 0; i < chars.size(); i++)
		entries.push_back({ SelType::Character, (int)i, "PERSON", chars[i]->charName });
	const auto& anchors = scene.Anchors();
	for (size_t i = 0; i < anchors.size(); i++)
		entries.push_back({ SelType::Anchor, (int)i, "SLOT", anchors[i].name });
	const auto& points = scene.GetPointLights();
	for (size_t i = 0; i < points.size(); i++)
		entries.push_back({ SelType::PointLight, (int)i, "LIGHT", points[i].name });
	const auto& spots = scene.GetSpotLights();
	for (size_t i = 0; i < spots.size(); i++)
		entries.push_back({ SelType::SpotLight, (int)i, "SPOT", spots[i].name });

	ui::Label("out.title", "IN THE SCENE  (" + std::to_string(entries.size()) + ")", area.x + 18.0f, kBarH + 30.0f,
		ui::Colour::heading);
	const ui::Rect list{ area.x + 10.0f, kBarH + 58.0f, area.w - 14.0f, area.h - 66.0f };

	// Keep a newly selected object (picked in the view) in sight.
	const float pitch = 32.0f + ui::kGap;
	static std::string shownSel;
	const std::string selKey = std::to_string((int)selType) + ":" + std::to_string(selIndex);
	if (selKey != shownSel)
	{
		shownSel = selKey;
		for (size_t k = 0; k < entries.size(); k++)
		{
			if (entries[k].type == selType && entries[k].index == selIndex)
			{
				const float y = k * pitch;
				if (y < outlinerScroll)
					outlinerScroll = y;
				else if (y + pitch > outlinerScroll + list.h)
					outlinerScroll = y + pitch - list.h;
			}
		}
	}

	ui::Column col("out", list, outlinerScroll);
	for (const Entry& e : entries)
	{
		ui::Rect r;
		if (!col.Line(r, 32.0f))
			continue;
		const std::string id = "out." + std::to_string((int)e.type) + "." + std::to_string(e.index);
		const SelType type = e.type;
		const int index = e.index;
		const float zoomW = 112.0f;
		ui::Item(id, { r.x, r.y, r.w - zoomW - 4.0f, r.h }, e.tag, e.name, selType == type && selIndex == index,
			[this, type, index]() { selType = type; selIndex = index; }, "Select it");
		ui::Button(id + ".z", { r.Right() - zoomW, r.y + 2.0f, zoomW, r.h - 4.0f }, "ZOOM",
			[this, &game, type, index]()
			{
				selType = type;
				selIndex = index;
				ZoomToSelected(game);
			},
			"Select it and fly the camera to it");
	}
}

// --------------------------------------------------------------- status bar

void Scene3DEditor::RenderStatusBar(Game& game, const Renderer& renderer)
{
	(void)renderer;
	const float gw = game.designWidth * Camera::MULTIPLIER;
	const float gh = game.designHeight * Camera::MULTIPLIER;
	const ui::Rect bar{ 0.0f, gh - kStatusH, gw, kStatusH };
	ui::Panel(bar, ui::Colour::bar);

	const std::string keys = "RMB look   WASDQE fly   wheel dolly   Ctrl+Z undo   F5 save   Tab hide panels   2 exit";
	const float keysW = ui::TextWidth(keys);
	ui::Label("status.keys", keys, gw - keysW - 18.0f, bar.y + bar.h * 0.5f, ui::Colour::faint);

	std::string message;
	Color colour = ui::Colour::dim;
	if (statusFrames > 0 && !statusMsg.empty())
	{
		message = statusMsg;
		colour = ui::Colour::good;
	}
	else if (!ui::HoverHelp().empty())
	{
		message = ui::HoverHelp();
		colour = ui::Colour::text;
	}
	else if (lookPickFocus)
		message = "Click what the camera should focus on";
	else if (tileMode)
		message = "TILE mode: hover picks a tile type, click changes a tile, right-click adds / removes";
	else if (HasSelection())
	{
		static const char* const kModes[3] = { "MOVE", "ROTATE", "SCALE" };
		static const char* const kLocks[4] = { "freely", "along X", "along Y", "along Z" };
		message = std::string("Drag the selection to ") + kModes[(int)xformMode] + " it " + kLocks[lockAxis + 1]
			+ "  -  the left panel has its settings";
	}
	else
		message = "Click something in the view or the list to select it";
	ui::Label("status.msg", ui::Fit(message, gw - keysW - 70.0f), 18.0f, bar.y + bar.h * 0.5f, colour);
}
