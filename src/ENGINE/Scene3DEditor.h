#ifndef SCENE3DEDITOR_H
#define SCENE3DEDITOR_H
#pragma once

#include <glm/glm.hpp>
#include "CameraController.h"
#include "Scene3D.h"
#include <string>
#include <vector>

class Game;
class Renderer;
class ShaderProgram;
class Text;
struct FontInfo;

// In-game editor for DB2's 3D scenes (Scene3D). Replaces the engine's 2D
// level editor when a 3D scene is showing. Free-fly camera (Solar-System
// style: WASDQE + right-mouse look + wheel dolly), left-click to select a
// model or character, an on-screen info panel, and left-drag to move the
// selection on the ground plane or a single axis (hold X / Y / Z). Save
// writes the .scene file; Revert reloads it from disk.
//
// Toggle with the '2' key while a Scene3D is active (the 2D editor is
// suppressed for 3D games via Game::prefer3DEditor).
class KINJO_API Scene3DEditor
{
public:
	bool active = false;

	// Flip the editor on/off. No-op unless a Scene3D is currently active.
	// On enter it seeds the fly camera from the current view.
	void Toggle(Game& game);

	// Per-frame input + logic (camera, picking, dragging, save/revert keys).
	// Also owns the toggle-key edge detection, so call every frame. No-op
	// visuals when inactive.
	void Update(Game& game);

	// Draw the selection box, move gizmo, and info/help panels. Call from
	// MyGUI::Render while a Scene3D is active.
	void Render(Game& game, const Renderer& renderer);

	// Ray-pick the model/character under a screen position (window pixels) and
	// make it the selection. Public so tooling/tests can drive selection.
	void PickAt(Game& game, float sx, float sy);

	// If (sx,sy) (window pixels) falls on an object-list row, select that
	// object, zoom the camera to it, and return true. Public so tests can
	// drive it. Returns false when the click misses the list.
	bool ListClick(Game& game, float sx, float sy);

	// If (sx,sy) falls on a Move/Rotate/Scale button, switch mode and return
	// true. Public so tests can drive it.
	bool ModeButtonClick(Game& game, float sx, float sy);
	// DELETE/ADD buttons and the add-model dropdown (public for tests).
	bool ActionButtonClick(Game& game, float sx, float sy);
	bool DropdownClick(Game& game, float sx, float sy);

private:
	enum class SelType { None, Model, Character };
	SelType selType = SelType::None;
	int selIndex = -1;

	// Which transform a left-drag edits. Switched only via the on-screen
	// buttons (never a stray keypress), so you can't accidentally change the
	// wrong property.
	enum class XformMode { Move, Rotate, Scale };
	XformMode xformMode = XformMode::Move;

	FlyCameraController flyCam;

	// Axis lock, chosen with the on-screen X/Y/Z buttons (a toggle, not a held
	// key - so it can't be accidentally released mid-drag). -1 = FREE:
	//   Move  -> slide on the ground XZ plane
	//   Rotate-> yaw (about vertical)
	//   Scale -> uniform
	// 0/1/2 = lock to X/Y/Z for all three modes.
	int lockAxis = -1;

	// Drag state. dragAxis is set from lockAxis at press time.
	bool dragging = false;
	int dragAxis = -1;
	glm::vec3 dragGrabOffset = glm::vec3(0);
	// Values captured at drag-start for rotate/scale (so the edit is relative
	// to the press, not accumulated per frame).
	float dragStartYaw = 0.0f;
	float dragStartPitch = 0.0f;
	float dragStartRoll = 0.0f;
	float dragStartScale = 1.0f;
	glm::vec3 dragStartScaleAxis = glm::vec3(1.0f);
	float dragStartHeight = 0.0f;

	// Mouse edge tracking (window pixels)
	bool leftWasDown = false;
	int pressX = 0, pressY = 0;
	bool movedSincePress = false;
	int lastMouseX = 0, lastMouseY = 0;

	// Key edge tracking
	bool toggleWasDown = false;
	bool saveWasDown = false;
	bool revertWasDown = false;
	bool deleteWasDown = false;
	bool undoKeyWasDown = false;
	bool redoKeyWasDown = false;

	// GL line rendering (selection box + axis gizmo)
	ShaderProgram* lineShader = nullptr;
	unsigned int lineVAO = 0, lineVBO = 0;
	void EnsureGL();
	void DrawLines(Game& game, const Renderer& renderer,
		const std::vector<glm::vec3>& segments, const glm::vec4& color);
	// Filled 2D rectangle in GUI space (for the mode buttons' backgrounds).
	void DrawFilledRect(Game& game, const Renderer& renderer,
		float x, float y, float w, float h, const glm::vec4& color);

	// Move / Rotate / Scale button bar. Each button is sized snugly around its
	// label; the rects are cached (from the last render) for hit-testing.
	Text* modeButtonText[3] = { nullptr, nullptr, nullptr };
	float btnX[3] = { 0, 0, 0 }, btnY[3] = { 0, 0, 0 };
	float btnW[3] = { 0, 0, 0 }, btnH[3] = { 0, 0, 0 };
	bool btnLaidOut = false;
	void RenderModeButtons(Game& game, const Renderer& renderer);

	// Axis-lock button bar: FREE / X / Y / Z. Sits under the mode buttons.
	static const int kNumAxes = 4;   // FREE, X, Y, Z
	Text* axisBtnText[kNumAxes] = { nullptr, nullptr, nullptr, nullptr };
	float axisBtnX[kNumAxes] = { 0 }, axisBtnY[kNumAxes] = { 0 };
	float axisBtnW[kNumAxes] = { 0 }, axisBtnH[kNumAxes] = { 0 };
	bool axisBtnLaidOut = false;
	void RenderAxisButtons(Game& game, const Renderer& renderer);

	// Reset button bar: RESET POS / RESET ROT / RESET SCALE. Snaps the selected
	// object's position back to the scene origin, or clears its rotation / scale.
	static const int kNumResets = 3;   // POS, ROT, SCALE
	Text* resetBtnText[kNumResets] = { nullptr, nullptr, nullptr };
	float resetBtnX[kNumResets] = { 0 }, resetBtnY[kNumResets] = { 0 };
	float resetBtnW[kNumResets] = { 0 }, resetBtnH[kNumResets] = { 0 };
	bool resetBtnLaidOut = false;
	void RenderResetButtons(Game& game, const Renderer& renderer);

	// Camera-management buttons: ADD CAM / NEXT CAM / SET DEF / DEL CAM. Let the
	// designer capture the live fly-camera pose as a named scene camera, cycle
	// the view through the saved cameras, pick the startup one, and delete one.
	static const int kNumCamBtns = 4;   // ADD, NEXT, SET DEFAULT, DELETE
	Text* camBtnText[kNumCamBtns] = { nullptr, nullptr, nullptr, nullptr };
	float camBtnX[kNumCamBtns] = { 0 }, camBtnY[kNumCamBtns] = { 0 };
	float camBtnW[kNumCamBtns] = { 0 }, camBtnH[kNumCamBtns] = { 0 };
	bool camBtnLaidOut = false;
	int camCycleIndex = -1;          // last camera jumped to
	std::string currentCamName;      // camera UPDATE / SET DEF / DEL CAM act on
	void RenderCameraButtons(Game& game, const Renderer& renderer);
	void JumpToCameraByName(Game& game, const std::string& name);
	std::string NextCameraName() const;   // auto-name for SAVE CAM (cam1, cam2, ...)

	// Camera list panel (mirrors the object list): every scene camera, the
	// current one highlighted, the startup one flagged. Click a row to jump the
	// fly camera there. Row 0 is a header; camListRowCam[row] is the camera name
	// ("" for the header).
	std::vector<Text*> camListRows;
	std::vector<std::string> camListRowCam;
	float camListX = 0.0f, camListTop = 0.0f;
	std::string camListMarkerKey;
	void EnsureCameraList(Game& game);
	void BuildCameraList(Game& game);

	// Undo / Redo / Reload button bar (the last row).
	static const int kNumEditBtns = 3;   // UNDO, REDO, RELOAD
	Text* editBtnText[kNumEditBtns] = { nullptr, nullptr, nullptr };
	float editBtnX[kNumEditBtns] = { 0 }, editBtnY[kNumEditBtns] = { 0 };
	float editBtnW[kNumEditBtns] = { 0 }, editBtnH[kNumEditBtns] = { 0 };
	bool editBtnLaidOut = false;
	void RenderEditButtons(Game& game, const Renderer& renderer);

	// --- undo / redo / dirty tracking ----------------------------------
	// Full-scene snapshots (the .scene text). Committed only when an edit
	// finishes (mouse release / button action), so a drag A->B->C is a single
	// step. baselineSnapshot = state since the last commit; savedSnapshot =
	// state at the last save/load (drives the "unsaved" indicator);
	// historyScene = which scene the stacks belong to (reset when it changes).
	std::vector<std::string> undoStack, redoStack;
	std::string baselineSnapshot, savedSnapshot, historyScene;
	Text* dirtyText = nullptr;
	void ResetHistory();       // clear stacks, re-baseline (load / new / revert)
	void CommitEdit();         // push the baseline if the scene actually changed
	void Undo(Game& game);
	void Redo(Game& game);
	void LoadSnapshot(Game& game, const std::string& snap);
	bool IsDirty() const;

	// Water-surface tuning panel, shown only when the selected model uses the
	// water material: one row per property, each  [ - ] [ + ]  NAME: VALUE.
	static const int kNumWaterProps = 7;
	float waterRowY[kNumWaterProps] = { 0 };   // per-row top (buttons share x)
	float waterMinusX = 0, waterPlusX = 0, waterBtnW = 0, waterRowH = 0;
	bool waterPanelLaidOut = false;
	Text* waterMinusText = nullptr;            // "-" (rendered once per row)
	Text* waterPlusText = nullptr;             // "+"
	Text* waterLabelText = nullptr;            // "NAME: VALUE" (re-set per row)
	Text* waterHeaderText = nullptr;
	void RenderWaterButtons(Game& game, const Renderer& renderer);
	Scene3DModel* SelectedWater(Game& game) const;   // selection if it's water, else null

	// Top of the object-details info panel; kept below the button bars so the
	// buttons never cover the text. Updated by RenderEditButtons.
	float infoPanelY = 560.0f;
public:
	// If (sx,sy) hits a water tuning button, apply it and return true.
	bool WaterButtonClick(Game& game, float sx, float sy);
	// If (sx,sy) hits UNDO/REDO/RELOAD, run it and return true.
	bool EditButtonClick(Game& game, float sx, float sy);
	// If (sx,sy) hits a FREE/X/Y/Z button, set the lock axis and return true.
	bool AxisButtonClick(Game& game, float sx, float sy);
	// If (sx,sy) hits a reset button, reset that transform and return true.
	bool ResetButtonClick(Game& game, float sx, float sy);
	// If (sx,sy) hits a camera button, run it and return true.
	bool CameraButtonClick(Game& game, float sx, float sy);
	// If (sx,sy) hits a camera-list row, jump there and return true.
	bool CameraListClick(Game& game, float sx, float sy);
	// If (sx,sy) hits the OBJECTS / CAMERAS tab, switch the panel and return true.
	bool ListTabClick(Game& game, float sx, float sy);
private:

	// Action buttons: DELETE, ADD (model dropdown), NEW (new scene), LOAD
	// (scene dropdown), TAG, MAT, SHADOW (scene-global point-light caster),
	// WEATHER (scene-global rain/snow/none).
	static const int kNumActions = 8;
	Text* actBtnText[kNumActions] = { nullptr, nullptr, nullptr, nullptr };
	float actBtnX[kNumActions] = { 0, 0, 0, 0 }, actBtnY[kNumActions] = { 0, 0, 0, 0 };
	float actBtnW[kNumActions] = { 0, 0, 0, 0 }, actBtnH[kNumActions] = { 0, 0, 0, 0 };
	bool actBtnLaidOut = false;
	// Bottom Y of the LAST action-button row (the row wraps to a second line when
	// it would reach the right-side list panel). The bars below key off this.
	float actBtnBottomY = 0.0f;
	void RenderActionButtons(Game& game, const Renderer& renderer);
	void DeleteSelected(Game& game);

	// A single dropdown, listing either model types (to add) or scenes (to
	// load). Only one open at a time; it hangs under its anchor button.
	enum class DropKind { None, AddModel, LoadScene, MatSelect };
	DropKind openDropdown = DropKind::None;
	int dropdownAnchor = 1;
	std::vector<Scene3D::ModelDef> addPalette;
	std::vector<std::string> sceneList;
	std::vector<std::string> matList;   // "(none)" + material names
	std::vector<Text*> dropdownRows;
	float dropdownX = 0.0f, dropdownTop = 0.0f;
	void OpenAddDropdown(Game& game);
	void OpenLoadDropdown(Game& game);
	void OpenMatDropdown(Game& game);
	int DropdownCount() const;
	void RenderDropdown(Game& game, const Renderer& renderer);

	// Typed-text prompt (SDL key edges). Shared by NEW-scene naming and CLUE
	// tagging; promptMode selects what confirming does. namingScene stays the
	// "prompt is active" flag the rest of the editor already checks.
	enum class PromptMode { NewScene, Tag, CameraName };
	bool namingScene = false;
	PromptMode promptMode = PromptMode::NewScene;
	std::string nameBuffer;
	Text* namePromptText = nullptr;
	Uint8 prevKeys[SDL_NUM_SCANCODES] = { 0 };
	void StartNaming(PromptMode mode);
	void UpdateNaming(const Uint8* keys, Game& game);
	void RenderNamePrompt(Game& game, const Renderer& renderer);

	// Info + help text (created lazily). editorFont is a dedicated font sized
	// for a legible overlay (the VN's own fonts are huge and scaled down).
	FontInfo* editorFont = nullptr;
	Text* infoText = nullptr;
	Text* helpText = nullptr;
	Text* statusText = nullptr;
	FontInfo* EnsureFont(Game& game);
	std::string lastInfo;
	std::string lastStatus;
	std::string statusMsg;
	int statusFrames = 0;

	// Object-list panel: a clickable list of every model + character. Row 0 is
	// a non-clickable header; each following row maps to listEntries[row]. Each
	// row is its own Text placed at an explicit Y, so hit-testing uses the
	// exact same row positions as rendering (no reliance on internal line
	// spacing, which the text metrics can't predict reliably).
	// The object list and camera list share one right-side panel, switched by two
	// tabs at the top, so a long object list (e.g. campus) can't push the camera
	// list off-screen. Only the active tab's list renders + is hit-tested.
	enum class ListTab { Objects, Cameras };
	ListTab listTab = ListTab::Objects;
	Text* tabBtnText[2] = { nullptr, nullptr };
	float tabBtnX[2] = { 0, 0 }, tabBtnY[2] = { 0, 0 };
	float tabBtnW[2] = { 0, 0 }, tabBtnH[2] = { 0, 0 };
	bool tabBtnLaidOut = false;
	void RenderListTabs(Game& game, const Renderer& renderer);

	struct ListEntry { SelType type; int index; };
	std::vector<ListEntry> listEntries;
	std::vector<Text*> listRows;
	float listX = 0.0f;      // GUI-space left edge (depends on resolution)
	int listBuiltCount = -1;
	std::string lastListMarkerKey;
	void EnsureObjectList(Game& game);
	void BuildObjectList(Game& game);
	// Per-row "zoom" button drawn at each object row's right edge (a separate
	// affordance so a name-click only selects). Reuses one marker Text.
	Text* zoomMarkerText = nullptr;
	void RenderListZoomButtons(Game& game, const Renderer& renderer);

	// Camera zoom-to-object: a short editor-owned glide (kept separate from the
	// scene's own glides, which the fly camera would fight). Active while
	// camGliding; suppresses fly-camera control until it settles or the user
	// grabs the camera (movement key / right-drag).
	bool camGliding = false;
	float camGlideElapsed = 0.0f, camGlideDur = 0.4f;
	glm::vec3 glideFromPos = glm::vec3(0), glideToPos = glm::vec3(0);
	float glideFromPitch = 0, glideToPitch = 0, glideFromYaw = 0, glideToYaw = 0;
	void ZoomToSelected(Game& game);

	// Selection helpers
	bool HasSelection() const { return selType != SelType::None && selIndex >= 0; }
	glm::vec3 SelectedPosition(Game& game) const;
	void SetSelectedPosition(Game& game, const glm::vec3& p);
	void ClearSelection();
	void RefreshInfoText(Game& game);
	void Deselect(Game& game);
	bool SelectedAABB(Game& game, glm::vec3& outMin, glm::vec3& outMax) const;
};

#endif
