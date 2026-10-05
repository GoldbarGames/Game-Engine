#ifndef EDITOR_UI_H
#define EDITOR_UI_H
#pragma once

// Engine-internal (not exported): the widget kit the 3D editor (Scene3DEditor)
// draws its interface with. Immediate mode: every frame, while it draws, the
// editor declares its controls; each records where it is and what it does,
// and the next frame's input runs it, so what is drawn and what is clicked
// can never drift apart.
//
// One look per kind of control, everywhere:
//   Button     an action                     flat grey; red = hard to take back
//   Toggle     on / off                      a check box
//   Choice     one of a few (MOVE|ROTATE)    joined segments, the chosen one lit
//   Dropdown   one of a list                 a dark field with a down arrow
//   Number     a value                       [-] field [+]; drag across the field
//   Stepper    one of an ordered set         [-] field [+], not draggable
//   TextField  a name / tag / guard          a dark field; clicking it edits
//   Item       a row in a list               lit when selected
// The accent colour always means "on / chosen / set here". Hovering any
// control puts its help into HoverHelp(), for a status bar.
//
// Positions are GUI units (the design resolution x Camera::MULTIPLIER).

#include "../globals.h"
#include <functional>
#include <string>
#include <vector>
#include <glm/vec4.hpp>

class Renderer;
class FontInfo;

namespace EditorUI
{
	struct Rect
	{
		float x = 0.0f, y = 0.0f, w = 0.0f, h = 0.0f;
		bool Contains(float px, float py) const { return px >= x && px < x + w && py >= y && py < y + h; }
		float Right() const { return x + w; }
		float Bottom() const { return y + h; }
	};

	constexpr float kRowH = 36.0f;        // every control's height
	constexpr float kGap = 6.0f;          // between controls
	constexpr float kTextScale = 0.13f;

	namespace Colour
	{
		extern const Color text, dim, faint, heading, warning, good;
		extern const glm::vec4 panel, bar, line;
	}

	// --- a frame (the editor's Render) ---------------------------------------
	void Begin(const Renderer& renderer, FontInfo* font, float guiWidth, float guiHeight);
	void End();   // the open dropdown list, over everything

	// --- input (the editor's Update) ------------------------------------------
	struct Input
	{
		float x = 0.0f, y = 0.0f;
		bool pressed = false, down = false, released = false;
		int wheel = 0;   // +1 = wheel up, -1 = wheel down
	};
	struct Used { bool press = false, wheel = false; };
	// Runs what's under the mouse: clicks, number drags, list scrolling. A press
	// or wheel it used shouldn't also reach the 3D view.
	Used HandleInput(const Input& in);
	bool Dragging();                  // a number is being dragged
	const std::string& HoverHelp();   // the hovered control's help ("" = none)
	void Reset();                     // forget the controls and any open list (UI hidden)

	// --- text -------------------------------------------------------------------
	float TextWidth(const std::string& s, float scale = kTextScale);
	std::string Fit(const std::string& s, float width, float scale = kTextScale);   // ".." when too long
	// Draw `s` with its left edge at x and its middle at y. `key` names the place
	// (one cached text object each, rebuilt only when what it shows changes).
	void Label(const std::string& key, const std::string& s, float x, float y, Color colour,
		float scale = kTextScale);

	// --- controls -------------------------------------------------------------
	enum class Tone { Normal, Accent, Danger };
	void Fill(const Rect& r, const glm::vec4& colour);    // paint only
	void Panel(const Rect& r, const glm::vec4& colour);   // a backdrop that swallows clicks
	void Button(const std::string& id, const Rect& r, const std::string& label, std::function<void()> onClick,
		const std::string& help, Tone tone = Tone::Normal, bool enabled = true);
	void Toggle(const std::string& id, const Rect& r, const std::string& label, bool on,
		std::function<void()> onClick, const std::string& help, bool enabled = true);
	// `chosen` -1 = none lit (a tab bar whose page is in the other row).
	void Choice(const std::string& id, const Rect& r, const std::vector<std::string>& options, int chosen,
		std::function<void(int)> onPick, const std::string& help, bool enabled = true);
	// The list is asked for when it opens, so it is always current.
	void Dropdown(const std::string& id, const Rect& r, const std::string& value,
		std::function<std::vector<std::string>()> listItems, std::function<void(int)> onPick,
		const std::string& help, bool enabled = true);

	struct NumberSpec
	{
		float step = 1.0f;       // one [-] / [+]
		bool mul = false;        // step multiplies (exposure, range...) instead of adding
		float lo = -1e9f, hi = 1e9f;
		float floor = 0.0f;      // mul: + from zero starts here; - below it drops to zero
		const char* fmt = "%.2f";
		bool drag = true;        // dragging across the field changes it
		std::function<float(float, int)> stepFn;   // a custom [-] / [+] (tiling); else step / mul
	};
	float Step(const NumberSpec& spec, float value, int dir);
	// `text` replaces the formatted value ("off", "no sky"); onSet runs while it
	// changes (a drag: every frame), onCommit once when the change is finished.
	void Number(const std::string& id, const Rect& r, float value, const std::string& text,
		const NumberSpec& spec, std::function<void(float)> onSet, std::function<void()> onCommit,
		const std::string& help, bool enabled = true);
	void Stepper(const std::string& id, const Rect& r, const std::string& text,
		std::function<void(int)> onStep, const std::string& help, bool enabled = true);
	void TextField(const std::string& id, const Rect& r, const std::string& value, const std::string& placeholder,
		std::function<void()> onClick, const std::string& help, bool enabled = true);
	// `tag` is a short dim word before the name (MODEL, LIGHT...).
	void Item(const std::string& id, const Rect& r, const std::string& tag, const std::string& label, bool selected,
		std::function<void()> onClick, const std::string& help);

	// A scrolling column of labelled rows: an inspector page. Rows scrolled out
	// of view return false (draw nothing for them); the wheel scrolls it.
	class Column
	{
	public:
		Column(const std::string& id, const Rect& area, float& scroll);
		~Column();
		void Header(const std::string& text);
		void Note(const std::string& text, Color colour);   // wraps to the column's width
		void Space(float h);
		// A labelled row: draws the label and gives the control's rect. `own`
		// false dims the label (a default value). onLabel makes the label
		// clickable (back to the default) and marks the row with an accent bar.
		bool Row(const std::string& id, const std::string& label, Rect& control, bool own = true,
			std::function<void()> onLabel = nullptr, const std::string& labelHelp = std::string());
		bool Line(Rect& r, float h = kRowH);   // a full-width row (buttons, list items)
	private:
		bool Place(float h, Rect& r);
		std::string id_;
		Rect area_;
		float* scroll_;
		float y_ = 0.0f;
		int notes_ = 0;
	};
}

#endif
