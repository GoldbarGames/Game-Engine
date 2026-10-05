// The 3D editor's widget kit - see EditorUI.h.

#include "EditorUI.h"
#include "../Renderer.h"
#include "../Text.h"
#include "../Sprite.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <unordered_map>

namespace EditorUI
{
	namespace Colour
	{
		const Color text = { 236, 239, 243, 255 };
		const Color dim = { 138, 150, 168, 255 };
		const Color faint = { 96, 103, 116, 255 };
		const Color heading = { 118, 172, 235, 255 };
		const Color warning = { 255, 172, 64, 255 };
		const Color good = { 140, 210, 150, 255 };
		const glm::vec4 panel(0.07f, 0.075f, 0.09f, 0.985f);
		const glm::vec4 bar(0.05f, 0.055f, 0.068f, 0.99f);
		const glm::vec4 line(0.22f, 0.25f, 0.30f, 1.0f);
	}
}

namespace
{
	using namespace EditorUI;

	const glm::vec4 kControl(0.20f, 0.22f, 0.26f, 1.0f);
	const glm::vec4 kControlHot(0.28f, 0.31f, 0.36f, 1.0f);
	const glm::vec4 kAccent(0.16f, 0.44f, 0.80f, 1.0f);
	const glm::vec4 kAccentHot(0.23f, 0.53f, 0.90f, 1.0f);
	const glm::vec4 kDanger(0.52f, 0.17f, 0.17f, 1.0f);
	const glm::vec4 kDangerHot(0.66f, 0.23f, 0.23f, 1.0f);
	const glm::vec4 kDisabled(0.13f, 0.14f, 0.16f, 1.0f);
	const glm::vec4 kField(0.04f, 0.045f, 0.055f, 1.0f);
	const glm::vec4 kFieldEdge(0.29f, 0.32f, 0.37f, 1.0f);
	const glm::vec4 kFieldEdgeHot(0.52f, 0.57f, 0.65f, 1.0f);
	const glm::vec4 kRowHot(1.0f, 1.0f, 1.0f, 0.07f);
	const float kPadX = 14.0f;   // text inset in buttons and fields
	const float kSegPad = 6.0f;  // ...and in a choice's segments, which are packed tighter

	struct Hit
	{
		Rect r;
		std::string id;
		std::string help;
		std::function<void()> onPress;   // null: inert (a panel, a disabled control)
		// A draggable number field.
		bool number = false;
		float value = 0.0f;
		NumberSpec spec;
		std::function<void(float)> onSet;
		std::function<void()> onCommit;
	};
	struct ScrollArea { Rect r; float* scroll = nullptr; float max = 0.0f; };

	const Renderer* renderer = nullptr;
	FontInfo* font = nullptr;
	float guiW = 2560.0f, guiH = 1440.0f;
	std::vector<Hit> hits;            // the controls drawn last, for input
	std::vector<ScrollArea> scrolls;
	std::string hoverId, hoverHelp;

	struct Drag
	{
		bool active = false;
		std::string id;
		float startX = 0.0f, startValue = 0.0f;
		bool moved = false;
		NumberSpec spec;
		std::function<void(float)> onSet;
		std::function<void()> onCommit;
	};
	Drag drag;

	struct Popup
	{
		bool open = false;
		std::string id;
		Rect anchor;
		std::vector<std::string> items;
		std::string current;
		std::function<void(int)> onPick;
		float scroll = 0.0f;
	};
	Popup popup;

	struct Slot { ::Text* text = nullptr; std::string s; Color c = { 0, 0, 0, 0 }; float scale = 0.0f; };
	std::unordered_map<std::string, Slot> slots;
	::Text* measurer = nullptr;
	std::unordered_map<std::string, float> widths;
	std::unordered_map<std::string, std::string> fits;

	::Text* NewText()
	{
		::Text* t = new ::Text(font);
		t->isRichText = true;
		t->GetSprite()->keepPositionRelativeToCamera = true;
		t->GetSprite()->keepScaleRelativeToCamera = true;
		return t;
	}

	// Scales are absolute: Text::SetScale multiplies by the game's defaultScale.
	glm::vec2 TextScale(float s)
	{
		const glm::vec2 d = ::Text::defaultScale;
		return glm::vec2(d.x != 0.0f ? s / d.x : s, d.y != 0.0f ? s / d.y : s);
	}

	void Paint(const Rect& r, const glm::vec4& c)
	{
		if (renderer != nullptr && r.w > 0.0f && r.h > 0.0f)
			renderer->DrawRect(r.x, r.y, r.w, r.h, c);
	}

	// A dark field with an edge.
	void FieldBox(const Rect& r, bool hot, bool lit)
	{
		Paint(r, lit ? kAccent : hot ? kFieldEdgeHot : kFieldEdge);
		Paint({ r.x + 2.0f, r.y + 2.0f, r.w - 4.0f, r.h - 4.0f }, kField);
	}

	// A small downward triangle (the dropdown arrow), centred on (cx, cy).
	void Caret(float cx, float cy, const glm::vec4& c)
	{
		const float half = 9.0f;
		for (int i = 0; i < 5; i++)
		{
			const float w = half * 2.0f * (1.0f - i / 5.0f);
			Paint({ cx - w * 0.5f, cy - 5.0f + i * 2.5f, w, 2.5f }, c);
		}
	}

	bool Hot(const std::string& id)
	{
		return !id.empty() && id == hoverId && !drag.active;
	}

	Hit& AddHit(const std::string& id, const Rect& r, const std::string& help, std::function<void()> onPress)
	{
		Hit h;
		h.r = r;
		h.id = id;
		h.help = help;
		h.onPress = std::move(onPress);
		hits.push_back(std::move(h));
		return hits.back();
	}

	std::string Format(const char* fmt, float v)
	{
		char buf[64];
		snprintf(buf, sizeof(buf), fmt, v);
		return buf;
	}

	glm::vec4 ToneColour(Tone tone, bool hot, bool enabled)
	{
		if (!enabled)
			return kDisabled;
		switch (tone)
		{
		case Tone::Accent: return hot ? kAccentHot : kAccent;
		case Tone::Danger: return hot ? kDangerHot : kDanger;
		default: return hot ? kControlHot : kControl;
		}
	}

	void CentredLabel(const std::string& key, const std::string& s, const Rect& r, Color c, float scale = kTextScale,
		float padX = kPadX)
	{
		const float pad = std::min(padX, r.w * 0.12f);   // narrow buttons ([-], [+]) keep their glyph
		const std::string fitted = Fit(s, r.w - 2.0f * pad, scale);
		Label(key, fitted, r.x + (r.w - TextWidth(fitted, scale)) * 0.5f, r.y + r.h * 0.5f, c, scale);
	}

	// The value a drag across a number field gives: 10 GUI units per step
	// (adding), or one step per 30 units (multiplying).
	float DragValue(const NumberSpec& spec, float start, float dx)
	{
		float v;
		if (spec.mul)
		{
			float base = start;
			if (base <= 0.0f)
			{
				if (spec.floor <= 0.0f || dx <= 0.0f)
					return std::min(std::max(start, spec.lo), spec.hi);
				base = spec.floor;
			}
			v = base * std::pow(spec.step, dx / 30.0f);
			if (spec.floor > 0.0f && v < spec.floor * 0.999f)
				v = 0.0f;
		}
		else
		{
			const float unit = spec.step / 10.0f;
			v = std::round((start + dx * unit) / unit) * unit;
		}
		return std::min(std::max(v, spec.lo), spec.hi);
	}
}

namespace EditorUI
{
	// ----------------------------------------------------------------- frame

	void Begin(const Renderer& r, FontInfo* f, float w, float h)
	{
		renderer = &r;
		font = f;
		guiW = w;
		guiH = h;
		hits.clear();
		scrolls.clear();
	}

	void End()
	{
		if (!popup.open)
			return;

		// The open list, over everything; a press anywhere else closes it.
		AddHit("", { 0.0f, 0.0f, guiW, guiH }, "", []() { popup.open = false; });

		const float rowH = kRowH - 4.0f;
		const int count = (int)popup.items.size();
		const int visible = std::min(count, 14);
		float width = popup.anchor.w;
		for (const std::string& s : popup.items)
			width = std::max(width, TextWidth(s) + 2.0f * kPadX + 16.0f);
		width = std::min(width, guiW * 0.5f);
		const float listH = visible * rowH + 8.0f;
		float x = std::min(popup.anchor.x, guiW - width - 8.0f);
		float y = popup.anchor.Bottom() + 4.0f;
		if (y + listH > guiH - 8.0f)
			y = std::max(8.0f, popup.anchor.y - 4.0f - listH);
		const Rect list{ x, y, width, listH };
		Paint(list, kFieldEdgeHot);
		Paint({ x + 2.0f, y + 2.0f, width - 4.0f, listH - 4.0f }, Colour::bar);

		const float maxScroll = std::max(0.0f, (count - visible) * rowH);
		popup.scroll = std::min(std::max(popup.scroll, 0.0f), maxScroll);
		const int first = (int)(popup.scroll / rowH + 0.5f);
		AddHit("popup", list, "", nullptr);
		for (int k = 0; k < visible && first + k < count; k++)
		{
			const int i = first + k;
			const Rect row{ x + 4.0f, y + 4.0f + k * rowH, width - 8.0f, rowH };
			const std::string id = "popup." + std::to_string(i);
			const bool current = popup.items[i] == popup.current;
			if (current)
				Paint(row, kAccent);
			else if (Hot(id))
				Paint(row, kControlHot);
			Label("popup.row" + std::to_string(k), Fit(popup.items[i], row.w - 2.0f * kPadX),
				row.x + kPadX, row.y + rowH * 0.5f, Colour::text);
			AddHit(id, row, popup.items[i], [i]()
			{
				std::function<void(int)> pick = popup.onPick;
				popup.open = false;
				if (pick)
					pick(i);
			});
		}
		if (count > visible)
		{
			const float track = listH - 8.0f;
			const float thumb = std::max(24.0f, track * visible / (float)count);
			const float t = (maxScroll > 0.0f) ? popup.scroll / maxScroll : 0.0f;
			Paint({ list.Right() - 8.0f, y + 4.0f + t * (track - thumb), 4.0f, thumb }, kFieldEdgeHot);
		}
		scrolls.push_back({ list, &popup.scroll, maxScroll });
	}

	// ----------------------------------------------------------------- input

	Used HandleInput(const Input& in)
	{
		Used used;
		const Hit* top = nullptr;
		for (int i = (int)hits.size() - 1; i >= 0; i--)
		{
			if (hits[i].r.Contains(in.x, in.y))
			{
				top = &hits[i];
				break;
			}
		}
		hoverId = top ? top->id : std::string();
		hoverHelp = top ? top->help : std::string();

		if (drag.active)
		{
			const float dx = in.x - drag.startX;
			if (std::fabs(dx) > 4.0f)
				drag.moved = true;
			if (drag.moved && drag.onSet)
				drag.onSet(DragValue(drag.spec, drag.startValue, dx));
			if (in.released || !in.down)
			{
				std::function<void()> commit = drag.onCommit;
				const bool moved = drag.moved;
				drag = Drag();
				if (moved && commit)
					commit();
			}
			used.press = in.pressed;
		}
		else if (in.pressed && top != nullptr)
		{
			used.press = true;
			if (top->number && top->spec.drag && top->onSet)
			{
				drag.active = true;
				drag.id = top->id;
				drag.startX = in.x;
				drag.startValue = top->value;
				drag.spec = top->spec;
				drag.onSet = top->onSet;
				drag.onCommit = top->onCommit;
			}
			else if (top->onPress)
			{
				std::function<void()> f = top->onPress;   // it may clear the controls
				f();
			}
		}

		if (in.wheel != 0)
		{
			for (int i = (int)scrolls.size() - 1; i >= 0; i--)
			{
				if (scrolls[i].r.Contains(in.x, in.y))
				{
					float& s = *scrolls[i].scroll;
					s = std::min(std::max(s - in.wheel * kRowH * 2.0f, 0.0f), scrolls[i].max);
					used.wheel = true;
					break;
				}
			}
			if (!used.wheel && top != nullptr)
				used.wheel = true;   // over the interface: don't dolly the camera
		}
		return used;
	}

	bool Dragging()
	{
		return drag.active;
	}

	const std::string& HoverHelp()
	{
		return hoverHelp;
	}

	void Reset()
	{
		hits.clear();
		scrolls.clear();
		hoverId.clear();
		hoverHelp.clear();
		popup.open = false;
		drag = Drag();
	}

	// ------------------------------------------------------------------ text

	float TextWidth(const std::string& s, float scale)
	{
		if (s.empty() || font == nullptr)
			return 0.0f;
		const std::string key = std::to_string((int)(scale * 1000.0f)) + "|" + s;
		auto it = widths.find(key);
		if (it != widths.end())
			return it->second;
		if (measurer == nullptr)
			measurer = NewText();
		measurer->SetText(s, Colour::text);
		measurer->SetScale(TextScale(scale));
		const float w = measurer->GetRenderedWidth();
		widths[key] = w;
		return w;
	}

	std::string Fit(const std::string& s, float width, float scale)
	{
		if (TextWidth(s, scale) <= width)
			return s;
		const std::string key = std::to_string((int)width) + "|" + std::to_string((int)(scale * 1000.0f)) + "|" + s;
		auto it = fits.find(key);
		if (it != fits.end())
			return it->second;
		std::string out = s;
		while (!out.empty() && TextWidth(out + "..", scale) > width)
			out.pop_back();
		out += "..";
		fits[key] = out;
		return out;
	}

	void Label(const std::string& key, const std::string& s, float x, float y, Color c, float scale)
	{
		if (renderer == nullptr || font == nullptr || s.empty())
			return;
		Slot& slot = slots[key];
		if (slot.text == nullptr)
			slot.text = NewText();
		if (slot.s != s || slot.scale != scale || !(slot.c == c))
		{
			slot.text->SetText(s, c);
			slot.text->SetScale(TextScale(scale));   // SetText resets the scale
			slot.s = s;
			slot.c = c;
			slot.scale = scale;
		}
		slot.text->SetPosition(x, y);
		slot.text->Render(*renderer);
	}

	// -------------------------------------------------------------- controls

	void Fill(const Rect& r, const glm::vec4& c)
	{
		Paint(r, c);
	}

	void Panel(const Rect& r, const glm::vec4& c)
	{
		Paint(r, c);
		AddHit("", r, "", nullptr);
	}

	void Button(const std::string& id, const Rect& r, const std::string& label, std::function<void()> onClick,
		const std::string& help, Tone tone, bool enabled)
	{
		Paint(r, ToneColour(tone, Hot(id), enabled));
		CentredLabel(id + ".t", label, r, enabled ? Colour::text : Colour::faint);
		AddHit(id, r, help, enabled ? std::move(onClick) : nullptr);
	}

	void Toggle(const std::string& id, const Rect& r, const std::string& label, bool on,
		std::function<void()> onClick, const std::string& help, bool enabled)
	{
		if (Hot(id) && enabled)
			Paint(r, kRowHot);
		const float box = 24.0f;
		const Rect b{ r.x + 6.0f, r.y + (r.h - box) * 0.5f, box, box };
		FieldBox(b, Hot(id) && enabled, false);
		if (on)
			Paint({ b.x + 5.0f, b.y + 5.0f, box - 10.0f, box - 10.0f }, enabled ? kAccentHot : kFieldEdgeHot);
		Label(id + ".t", Fit(label, r.w - box - 22.0f), b.Right() + 10.0f, r.y + r.h * 0.5f,
			enabled ? Colour::text : Colour::faint);
		AddHit(id, r, help, enabled ? std::move(onClick) : nullptr);
	}

	void Choice(const std::string& id, const Rect& r, const std::vector<std::string>& options, int chosen,
		std::function<void(int)> onPick, const std::string& help, bool enabled)
	{
		const int n = (int)options.size();
		if (n == 0)
			return;
		// Each segment as wide as its label needs, the space left shared evenly
		// (or all squeezed alike when they don't fit).
		const float gap = 3.0f;
		const float room = r.w - gap * (n - 1);
		std::vector<float> widths(n);
		auto measure = [&](float scale)
		{
			float need = 0.0f;
			for (int i = 0; i < n; i++)
			{
				widths[i] = TextWidth(options[i], scale) + 2.0f * kSegPad + 4.0f;   // what CentredLabel leaves room for
				need += widths[i];
			}
			return need;
		};
		// Long labels (SPRING SUMMER AUTUMN WINTER) get slightly smaller text.
		float scale = kTextScale;
		float need = measure(scale);
		if (need > room)
		{
			scale = std::max(kTextScale * 0.7f, kTextScale * room / need);
			need = measure(scale);
		}
		for (int i = 0; i < n; i++)
			widths[i] = (need <= room) ? widths[i] + (room - need) / n : widths[i] * room / need;
		float x = r.x;
		for (int i = 0; i < n; i++)
		{
			const Rect s{ x, r.y, widths[i], r.h };
			x += widths[i] + gap;
			const std::string sid = id + "." + std::to_string(i);
			Paint(s, ToneColour(i == chosen ? Tone::Accent : Tone::Normal, Hot(sid), enabled));
			CentredLabel(sid + ".t", options[i], s, enabled ? Colour::text : Colour::faint, scale, kSegPad);
			std::function<void()> press;
			if (enabled && onPick)
				press = [onPick, i]() { onPick(i); };
			AddHit(sid, s, help, press);
		}
	}

	void Dropdown(const std::string& id, const Rect& r, const std::string& value,
		std::function<std::vector<std::string>()> listItems, std::function<void(int)> onPick,
		const std::string& help, bool enabled)
	{
		const bool open = popup.open && popup.id == id;
		FieldBox(r, Hot(id) && enabled, open);
		Label(id + ".t", Fit(value, r.w - 2.0f * kPadX - 26.0f), r.x + kPadX, r.y + r.h * 0.5f,
			enabled ? Colour::text : Colour::faint);
		Caret(r.Right() - 22.0f, r.y + r.h * 0.5f, enabled ? glm::vec4(0.75f, 0.80f, 0.88f, 1.0f) : kFieldEdge);
		std::function<void()> press;
		if (enabled)
		{
			press = [id, r, value, listItems, onPick]()
			{
				if (popup.open && popup.id == id)
				{
					popup.open = false;
					return;
				}
				popup.open = true;
				popup.id = id;
				popup.anchor = r;
				popup.items = listItems ? listItems() : std::vector<std::string>();
				popup.current = value;
				popup.onPick = onPick;
				// Open with the current value in view.
				popup.scroll = 0.0f;
				for (size_t i = 0; i < popup.items.size(); i++)
					if (popup.items[i] == value)
						popup.scroll = std::max(0.0f, ((float)i - 6.0f) * (kRowH - 4.0f));
			};
		}
		AddHit(id, r, help, press);
	}

	float Step(const NumberSpec& spec, float v, int dir)
	{
		if (spec.stepFn)
			return std::min(std::max(spec.stepFn(v, dir), spec.lo), spec.hi);
		float nv = v;
		if (spec.mul)
		{
			if (dir > 0)
				nv = (v <= 0.0f && spec.floor > 0.0f) ? spec.floor : v * spec.step;
			else
			{
				nv = v / spec.step;
				if (spec.floor > 0.0f && nv < spec.floor * 0.999f)
					nv = 0.0f;
			}
		}
		else
		{
			// Snap to the step so repeated clicks don't drift (0.30000001).
			nv = std::round((v + (float)dir * spec.step) / spec.step) * spec.step;
		}
		return std::min(std::max(nv, spec.lo), spec.hi);
	}

	void Number(const std::string& id, const Rect& r, float value, const std::string& text,
		const NumberSpec& spec, std::function<void(float)> onSet, std::function<void()> onCommit,
		const std::string& help, bool enabled)
	{
		const float bw = kRowH;
		const Rect minus{ r.x, r.y, bw, r.h };
		const Rect plus{ r.Right() - bw, r.y, bw, r.h };
		const Rect field{ minus.Right() + 3.0f, r.y, r.w - 2.0f * bw - 6.0f, r.h };
		auto step = [=](int dir)
		{
			return [=]()
			{
				if (!onSet)
					return;
				onSet(Step(spec, value, dir));
				if (onCommit)
					onCommit();
			};
		};
		Paint(minus, ToneColour(Tone::Normal, Hot(id + ".-"), enabled));
		Paint(plus, ToneColour(Tone::Normal, Hot(id + ".+"), enabled));
		CentredLabel(id + ".-t", "-", minus, enabled ? Colour::text : Colour::faint);
		CentredLabel(id + ".+t", "+", plus, enabled ? Colour::text : Colour::faint);
		const bool dragging = drag.active && drag.id == id;
		FieldBox(field, Hot(id) && enabled && spec.drag, dragging);
		CentredLabel(id + ".t", text.empty() ? Format(spec.fmt, value) : text, field,
			enabled ? Colour::text : Colour::faint);

		AddHit(id + ".-", minus, help, enabled ? std::function<void()>(step(-1)) : std::function<void()>());
		AddHit(id + ".+", plus, help, enabled ? std::function<void()>(step(+1)) : std::function<void()>());
		Hit& h = AddHit(id, field, spec.drag && enabled ? help + "  (drag left / right to change it)" : help, nullptr);
		if (enabled)
		{
			h.number = true;
			h.value = value;
			h.spec = spec;
			h.onSet = onSet;
			h.onCommit = onCommit;
		}
	}

	void Stepper(const std::string& id, const Rect& r, const std::string& text,
		std::function<void(int)> onStep, const std::string& help, bool enabled)
	{
		const float bw = kRowH;
		const Rect minus{ r.x, r.y, bw, r.h };
		const Rect plus{ r.Right() - bw, r.y, bw, r.h };
		const Rect field{ minus.Right() + 3.0f, r.y, r.w - 2.0f * bw - 6.0f, r.h };
		Paint(minus, ToneColour(Tone::Normal, Hot(id + ".-"), enabled));
		Paint(plus, ToneColour(Tone::Normal, Hot(id + ".+"), enabled));
		CentredLabel(id + ".-t", "-", minus, enabled ? Colour::text : Colour::faint);
		CentredLabel(id + ".+t", "+", plus, enabled ? Colour::text : Colour::faint);
		FieldBox(field, false, false);
		CentredLabel(id + ".t", text, field, enabled ? Colour::text : Colour::faint);
		std::function<void()> down, up;
		if (enabled && onStep)
		{
			down = [onStep]() { onStep(-1); };
			up = [onStep]() { onStep(+1); };
		}
		AddHit(id + ".-", minus, help, down);
		AddHit(id + ".+", plus, help, up);
		AddHit(id, field, help, nullptr);
	}

	void TextField(const std::string& id, const Rect& r, const std::string& value, const std::string& placeholder,
		std::function<void()> onClick, const std::string& help, bool enabled)
	{
		FieldBox(r, Hot(id) && enabled, false);
		const bool empty = value.empty();
		Label(id + ".t", Fit(empty ? placeholder : value, r.w - 2.0f * kPadX), r.x + kPadX, r.y + r.h * 0.5f,
			(empty || !enabled) ? Colour::faint : Colour::text);
		AddHit(id, r, help, enabled ? std::move(onClick) : nullptr);
	}

	void Item(const std::string& id, const Rect& r, const std::string& tag, const std::string& label, bool selected,
		std::function<void()> onClick, const std::string& help)
	{
		if (selected)
			Paint(r, kAccent);
		else if (Hot(id))
			Paint(r, kRowHot);
		const float tagW = tag.empty() ? 0.0f : 96.0f;
		if (!tag.empty())
			Label(id + ".g", Fit(tag, tagW - 10.0f, kTextScale * 0.8f), r.x + 10.0f, r.y + r.h * 0.5f,
				selected ? Colour::text : Colour::faint, kTextScale * 0.8f);
		Label(id + ".t", Fit(label, r.w - tagW - 20.0f), r.x + 10.0f + tagW, r.y + r.h * 0.5f, Colour::text);
		AddHit(id, r, help, std::move(onClick));
	}

	// ---------------------------------------------------------------- column

	Column::Column(const std::string& id, const Rect& area, float& scroll)
		: id_(id), area_(area), scroll_(&scroll)
	{
	}

	Column::~Column()
	{
		const float maxScroll = std::max(0.0f, y_ - area_.h);
		*scroll_ = std::min(std::max(*scroll_, 0.0f), maxScroll);
		if (maxScroll > 0.0f)
		{
			const float thumb = std::max(30.0f, area_.h * area_.h / y_);
			const float t = *scroll_ / maxScroll;
			Paint({ area_.Right() - 6.0f, area_.y, 4.0f, area_.h }, glm::vec4(1.0f, 1.0f, 1.0f, 0.04f));
			Paint({ area_.Right() - 6.0f, area_.y + t * (area_.h - thumb), 4.0f, thumb }, kFieldEdgeHot);
		}
		scrolls.push_back({ area_, scroll_, maxScroll });
	}

	bool Column::Place(float h, Rect& r)
	{
		r = { area_.x, area_.y + y_ - *scroll_, area_.w - 12.0f, h };
		y_ += h + kGap;
		return r.y >= area_.y - 0.5f && r.Bottom() <= area_.Bottom() + 0.5f;
	}

	void Column::Header(const std::string& text)
	{
		y_ += 8.0f;
		Rect r;
		if (Place(28.0f, r))
		{
			Label(id_ + ".h." + text, text, r.x, r.y + 13.0f, Colour::heading);
			Paint({ r.x, r.Bottom() - 2.0f, r.w, 2.0f }, Colour::line);
		}
	}

	void Column::Note(const std::string& text, Color colour)
	{
		// Greedy word wrap to the column's width.
		const float width = area_.w - 12.0f;
		std::vector<std::string> lines;
		std::string line, word;
		auto flushWord = [&]()
		{
			if (word.empty())
				return;
			const std::string joined = line.empty() ? word : line + " " + word;
			if (!line.empty() && TextWidth(joined) > width)
			{
				lines.push_back(line);
				line = word;
			}
			else
				line = joined;
			word.clear();
		};
		for (char c : text)
		{
			if (c == ' ')
				flushWord();
			else
				word += c;
		}
		flushWord();
		if (!line.empty())
			lines.push_back(line);
		const int note = notes_++;
		for (size_t i = 0; i < lines.size(); i++)
		{
			Rect r;
			if (Place(24.0f, r))
				Label(id_ + ".n" + std::to_string(note) + "." + std::to_string(i), Fit(lines[i], r.w), r.x,
					r.y + 12.0f, colour);
		}
	}

	void Column::Space(float h)
	{
		y_ += h;
	}

	bool Column::Row(const std::string& id, const std::string& label, Rect& control, bool own,
		std::function<void()> onLabel, const std::string& labelHelp)
	{
		Rect r;
		if (!Place(kRowH, r))
			return false;
		const float labelW = std::floor(r.w * 0.40f);
		const Rect lr{ r.x, r.y, labelW, r.h };
		control = { r.x + labelW, r.y, r.w - labelW, r.h };
		if (own && onLabel)   // set here, and can go back to the default
			Paint({ r.x, r.y + 8.0f, 4.0f, r.h - 16.0f }, kAccentHot);
		const std::string lid = id + ".label";
		const bool hot = onLabel && Hot(lid);
		Label(lid, Fit(label, labelW - 22.0f), r.x + 14.0f, r.y + r.h * 0.5f,
			hot ? Colour::heading : own ? Colour::text : Colour::dim);
		if (onLabel)
			AddHit(lid, lr, labelHelp, std::move(onLabel));
		return true;
	}

	bool Column::Line(Rect& r, float h)
	{
		return Place(h, r);
	}
}
