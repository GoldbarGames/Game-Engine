#include "SceneLogic.h"
#include "Game.h"
#include "Scene3D.h"
#include "CutsceneManager.h"
#include "SceneMaterial.h"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>

SceneLogic& SceneLogic::Get()
{
	static SceneLogic instance;
	return instance;
}

int SceneLogic::Val(const std::string& key) const
{
	auto it = persist.find(key);
	return (it == persist.end()) ? 0 : it->second;
}

bool SceneLogic::CondsHold(const std::vector<Cond>& conds, SceneLogicHost& host) const
{
	for (const Cond& c : conds)
	{
		switch (c.type)
		{
		case Cond::Type::Flag:         if (Val(c.key) == 0) return false; break;
		case Cond::Type::NotFlag:      if (Val(c.key) != 0) return false; break;
		case Cond::Type::Min:          if (Val(c.key) < c.value) return false; break;
		case Cond::Type::Eq:           if (Val(c.key) != c.value) return false; break;
		case Cond::Type::Chapter:      if (host.CurrentChapter() < c.value) return false; break;
		case Cond::Type::Recruited:    if (!host.IsRecruited(c.key)) return false; break;
		case Cond::Type::NotRecruited: if (host.IsRecruited(c.key)) return false; break;
		case Cond::Type::Sel:
		{
			bool ok = false;
			for (const Selector& sel : selectors)
				if (sel.name == c.key && !sel.options.empty()
					&& sel.options[sel.current].name == c.key2)
					ok = true;
			if (!ok) return false;
			break;
		}
		}
	}
	return true;
}

void SceneLogic::LoadForScene(Game& game, const std::string& scene)
{
	triggers.clear(); sequences.clear(); chains.clear(); levers.clear();
	teleporters.clear(); gates.clear(); vaults.clear(); chests.clear();
	randomKeeps.clear(); lotteries.clear(); selectors.clear();
	encounters.clear();
	roamers.clear();
	hasLastPos = false;
	loadedScene = scene;
	active = false;

	std::ifstream file("data/logic/" + scene + ".logic");
	if (!file.is_open())
		return;

	Trigger* trig = nullptr;
	Sequence* seq = nullptr;
	Chain* chn = nullptr;
	Levers* lv = nullptr;
	TeleporterPair* tp = nullptr;
	Gate* gt = nullptr;
	RandomKeep* rk = nullptr;
	Lottery* lo = nullptr;
	Selector* sel = nullptr;
	Encounters* enc = nullptr;
	Roamer* rm = nullptr;
	VaultZone* vz = nullptr;
	Chest* ch = nullptr;
	auto closeAll = [&]() { trig = nullptr; seq = nullptr; chn = nullptr; lv = nullptr; tp = nullptr; gt = nullptr; vz = nullptr; ch = nullptr; rk = nullptr; lo = nullptr; sel = nullptr; enc = nullptr; rm = nullptr; };

	// Parse a run of actions from the stream (verbs consume their own args).
	auto parseActions = [&](std::istringstream& ss, std::vector<Action>& into, const std::string& ctx)
	{
		std::string verb;
		while (ss >> verb)
		{
			Action a;
			if (verb == "cutscene") { a.type = Action::Type::Cutscene; ss >> a.arg; }
			else if (verb == "battle") { a.type = Action::Type::Battle; ss >> a.arg; }
			else if (verb == "open-gate") { a.type = Action::Type::OpenGate; ss >> a.arg; }
			else if (verb == "teleport") { a.type = Action::Type::Teleport; ss >> a.pos.x >> a.pos.y >> a.pos.z; }
			else if (verb == "teleport-slot") { a.type = Action::Type::TeleportSlot; ss >> a.arg; }
			else if (verb == "set" || verb == "add")
			{
				a.type = (verb == "set") ? Action::Type::Set : Action::Type::Add;
				ss >> a.arg;
				a.value = 1;
				// optional numeric value (peek: next token must be a number)
				std::streampos p = ss.tellg();
				std::string nxt;
				if (ss >> nxt)
				{
					char* end = nullptr;
					long v = strtol(nxt.c_str(), &end, 10);
					if (end != nullptr && *end == '\0') a.value = (int)v;
					else { ss.clear(); ss.seekg(p); }
				}
				else ss.clear();
			}
			else if (verb == "status") { a.type = Action::Type::Status; ss >> a.arg >> a.value; }
			else if (verb == "log")
			{
				a.type = Action::Type::Log;
				std::string rest;
				std::getline(ss, rest);
				size_t s0 = rest.find_first_not_of(" \t");
				a.arg = (s0 == std::string::npos) ? "" : rest.substr(s0);
				into.push_back(a);
				return;   // log consumes the rest of the line
			}
			else if (verb == "give")
			{
				a.type = Action::Type::Give;
				std::string rest;
				std::getline(ss, rest);
				size_t s0 = rest.find_first_not_of(" \t");
				a.arg = (s0 == std::string::npos) ? "" : rest.substr(s0);
				into.push_back(a);
				return;   // give consumes the rest of the line
			}
			else
			{
				std::cout << "SceneLogic: unknown action '" << verb << "' in " << ctx << std::endl;
				return;
			}
			into.push_back(a);
		}
	};

	auto parseCond = [&](std::istringstream& ss, std::vector<Cond>& into)
	{
		Cond c;
		std::string kind;
		ss >> kind;
		if (kind == "flag") { c.type = Cond::Type::Flag; ss >> c.key; }
		else if (kind == "!flag") { c.type = Cond::Type::NotFlag; ss >> c.key; }
		else if (kind == "min") { c.type = Cond::Type::Min; ss >> c.key >> c.value; }
		else if (kind == "eq") { c.type = Cond::Type::Eq; ss >> c.key >> c.value; }
		else if (kind == "chapter") { c.type = Cond::Type::Chapter; ss >> c.value; }
		else if (kind == "recruited") { c.type = Cond::Type::Recruited; ss >> c.key; }
		else if (kind == "!recruited") { c.type = Cond::Type::NotRecruited; ss >> c.key; }
		else if (kind == "sel") { c.type = Cond::Type::Sel; ss >> c.key >> c.key2; }
		else { std::cout << "SceneLogic: unknown condition '" << kind << "'" << std::endl; return; }
		into.push_back(c);
	};

	std::string line;
	while (std::getline(file, line))
	{
		if (!line.empty() && line.back() == '\r') line.pop_back();
		size_t hash = line.find('#');
		if (hash != std::string::npos) line = line.substr(0, hash);
		if (line.find_first_not_of(" \t") == std::string::npos) continue;

		std::istringstream ss(line);
		std::string key;
		ss >> key;

		if (key == "trigger") { closeAll(); triggers.push_back({}); trig = &triggers.back(); ss >> trig->name; }
		else if (key == "sequence") { closeAll(); sequences.push_back({}); seq = &sequences.back(); ss >> seq->name; }
		else if (key == "chain") { closeAll(); chains.push_back({}); chn = &chains.back(); ss >> chn->name; }
		else if (key == "levers") { closeAll(); levers.push_back({}); lv = &levers.back(); ss >> lv->name; }
		else if (key == "teleporter-pair") { closeAll(); teleporters.push_back({}); tp = &teleporters.back(); ss >> tp->name; }
		else if (key == "gate") { closeAll(); gates.push_back({}); gt = &gates.back(); ss >> gt->tag; }
		else if (key == "vault-zone") { closeAll(); vaults.push_back({}); vz = &vaults.back(); ss >> vz->name; }
		else if (key == "chest") { closeAll(); chests.push_back({}); ch = &chests.back(); ss >> ch->tag; }
		else if (key == "random-keep")
		{
			closeAll(); randomKeeps.push_back({}); rk = &randomKeeps.back();
			ss >> rk->name;
			std::string w;
			while (ss >> w)
			{
				if (w == "tag-prefix") ss >> rk->tagPrefix;
				else if (w == "keep") ss >> rk->keep;
			}
		}
		else if (key == "lottery")
		{
			closeAll(); lotteries.push_back({}); lo = &lotteries.back();
			ss >> lo->name;
			std::string w;
			while (ss >> w)
			{
				if (w == "tag-prefix") ss >> lo->tagPrefix;
				else if (w == "radius") ss >> lo->radius;
			}
		}
		else if (key == "selector") { closeAll(); selectors.push_back({}); sel = &selectors.back(); ss >> sel->name; }
		else if (key == "encounters") { closeAll(); encounters.push_back({}); enc = &encounters.back(); }
		else if (key == "roamer") { closeAll(); roamers.push_back({}); rm = &roamers.back(); ss >> rm->name; }

		else if (trig != nullptr)
		{
			if (key == "near-slot" || key == "near-tag" || key == "near-tag-prefix")
			{
				trig->kind = (key == "near-slot") ? Trigger::Kind::Slot
					: (key == "near-tag") ? Trigger::Kind::Tag : Trigger::Kind::TagPrefix;
				ss >> trig->target >> trig->radius;
				std::string p;
				while (ss >> p) if (p == "press") trig->press = true;
			}
			else if (key == "prompt")
			{
				std::string rest;
				std::getline(ss, rest);
				size_t a = rest.find_first_not_of(" \t");
				trig->prompt = (a == std::string::npos) ? "" : rest.substr(a);
			}
			else if (key == "auto") trig->kind = Trigger::Kind::Auto;
			else if (key == "once") trig->once = true;
			else if (key == "if") parseCond(ss, trig->conds);
			else if (key == "do") parseActions(ss, trig->actions, "trigger " + trig->name);
			else std::cout << "SceneLogic: unknown trigger field '" << key << "'" << std::endl;
		}
		else if (seq != nullptr)
		{
			if (key == "tags") { std::string t; while (ss >> t) seq->tags.push_back(t); }
			else if (key == "order") { std::string t; while (ss >> t) seq->order.push_back(t); }
			else if (key == "prompt")
			{
				std::string rest;
				std::getline(ss, rest);
				size_t a = rest.find_first_not_of(" \t");
				seq->prompt = (a == std::string::npos) ? "" : rest.substr(a);
			}
			else if (key == "lights") { std::string l; while (ss >> l) seq->lights.push_back(l); }
			else if (key == "lit-material") ss >> seq->litMat;
			else if (key == "unlit-material") ss >> seq->unlitMat;
			else if (key == "radius") ss >> seq->radius;
			else if (key == "persist") ss >> seq->persistKey;
			else if (key == "strict") seq->strict = true;
			else if (key == "window") ss >> seq->windowMs;
			else if (key == "if") parseCond(ss, seq->conds);
			else if (key == "on-wrong") parseActions(ss, seq->onWrong, "sequence " + seq->name);
			else if (key == "on-solved") parseActions(ss, seq->onSolved, "sequence " + seq->name);
			else std::cout << "SceneLogic: unknown sequence field '" << key << "'" << std::endl;
		}
		else if (chn != nullptr)
		{
			if (key == "persist") ss >> chn->persistKey;
			else if (key == "prompt")
			{
				std::string rest;
				std::getline(ss, rest);
				size_t a = rest.find_first_not_of(" \t");
				chn->prompt = (a == std::string::npos) ? "" : rest.substr(a);
			}
			else if (key == "radius") ss >> chn->radius;
			else if (key == "if") parseCond(ss, chn->conds);
			else if (key == "step")
			{
				ChainStep st;
				ss >> st.tag;
				parseActions(ss, st.actions, "chain " + chn->name);
				chn->steps.push_back(st);
			}
			else std::cout << "SceneLogic: unknown chain field '" << key << "'" << std::endl;
		}
		else if (lv != nullptr)
		{
			if (key == "tags") { std::string t; while (ss >> t) lv->tags.push_back(t); }
			else if (key == "tag-prefix")
			{
				ss >> lv->tagPrefix;
				std::string w;
				if (ss >> w && w == "except") { std::string t; while (ss >> t) lv->except.push_back(t); }
			}
			else if (key == "prompt")
			{
				std::string rest;
				std::getline(ss, rest);
				size_t a = rest.find_first_not_of(" \t");
				lv->prompt = (a == std::string::npos) ? "" : rest.substr(a);
			}
			else if (key == "count") ss >> lv->count;
			else if (key == "radius") ss >> lv->radius;
			else if (key == "persist") ss >> lv->persistKey;
			else if (key == "on-solved") parseActions(ss, lv->onSolved, "levers " + lv->name);
			else std::cout << "SceneLogic: unknown levers field '" << key << "'" << std::endl;
		}
		else if (tp != nullptr)
		{
			if (key == "tags") ss >> tp->tagA >> tp->tagB;
			else if (key == "radius") ss >> tp->radius;
			else if (key == "persist") ss >> tp->persistKey;
			else std::cout << "SceneLogic: unknown teleporter field '" << key << "'" << std::endl;
		}
		else if (gt != nullptr)
		{
			if (key == "if") parseCond(ss, gt->conds);
			else std::cout << "SceneLogic: unknown gate field '" << key << "'" << std::endl;
		}
		else if (vz != nullptr)
		{
			if (key == "y") ss >> vz->yMin >> vz->yMax;
			else if (key == "rope-prefix") ss >> vz->ropePrefix;
			else if (key == "reach") ss >> vz->reach;
			else std::cout << "SceneLogic: unknown vault field '" << key << "'" << std::endl;
		}
		else if (ch != nullptr)
		{
			if (key == "gives")
			{
				std::string rest;
				std::getline(ss, rest);
				size_t s0 = rest.find_first_not_of(" \t");
				ch->item = (s0 == std::string::npos) ? "" : rest.substr(s0);
			}
			else if (key == "prompt")
			{
				std::string rest;
				std::getline(ss, rest);
				size_t a = rest.find_first_not_of(" \t");
				ch->prompt = (a == std::string::npos) ? "" : rest.substr(a);
			}
			else if (key == "persist") ss >> ch->persistKey;
			else if (key == "radius") ss >> ch->radius;
			else std::cout << "SceneLogic: unknown chest field '" << key << "'" << std::endl;
		}
		else if (rm != nullptr)
		{
			if (key == "battle") ss >> rm->battleId;
			else if (key == "at") ss >> rm->slot;
			else if (key == "at-xyz") ss >> rm->home.x >> rm->home.y >> rm->home.z;
			else if (key == "material") ss >> rm->material;
			else if (key == "pattern") ss >> rm->pattern;
			else if (key == "region") ss >> rm->region;
			else if (key == "speed") ss >> rm->speed;
			else if (key == "chase") ss >> rm->chaseRadius;
			else if (key == "touch") ss >> rm->touchRadius;
			else if (key == "if") parseCond(ss, rm->conds);
			else std::cout << "SceneLogic: unknown roamer field '" << key << "'" << std::endl;
		}
		else if (enc != nullptr)
		{
			if (key == "battles") { std::string b; while (ss >> b) enc->battles.push_back(b); }
			else if (key == "steps") ss >> enc->minSteps >> enc->maxSteps;
			else if (key == "y") ss >> enc->yMin >> enc->yMax;
			else if (key == "if") parseCond(ss, enc->conds);
			else std::cout << "SceneLogic: unknown encounters field '" << key << "'" << std::endl;
		}
		else if (lo != nullptr)
		{
			if (key == "on-hit") parseActions(ss, lo->onHit, "lottery " + lo->name);
			else if (key == "on-miss") parseActions(ss, lo->onMiss, "lottery " + lo->name);
			else std::cout << "SceneLogic: unknown lottery field '" << key << "'" << std::endl;
		}
		else if (rk != nullptr)
			std::cout << "SceneLogic: unknown random-keep field '" << key << "'" << std::endl;
		else if (sel != nullptr)
		{
			if (key == "option")
			{
				Selector::Opt o;
				ss >> o.name;
				std::string w;
				if (ss >> w && w == "if") parseCond(ss, o.conds);
				sel->options.push_back(o);
			}
			else std::cout << "SceneLogic: unknown selector field '" << key << "'" << std::endl;
		}
		else
			std::cout << "SceneLogic: stray line '" << line << "'" << std::endl;
	}

	// A load of any OTHER scene invalidates a pending battle-return snapshot.
	if (!battleReturnScene.empty() && battleReturnScene != scene)
	{
		battleReturnScene.clear();
		suspendedRoamers.clear();
	}

	active = true;
	std::cout << "SceneLogic: " << scene << " - " << triggers.size() << " triggers, "
		<< sequences.size() << " sequences, " << chains.size() << " chains, "
		<< levers.size() << " levers, " << teleporters.size() << " teleporters, "
		<< vaults.size() << " vault zones, " << chests.size() << " chests, "
		<< encounters.size() << " encounter tables, "
		<< roamers.size() << " roamers" << std::endl;

	// Validation (the --checklogic guard rail, inline).
	Scene3D& sc = Scene3D::Get();
	auto hasTag = [&](const std::string& t) {
		for (Scene3DModel* m : sc.GetModels())
			if (m != nullptr && m->interactionTag == t) return true;
		return false;
	};
	auto hasSlot = [&](const std::string& s) {
		glm::vec3 p; float y;
		return sc.GetAnchor(s, p, y);
	};
	auto hasTagPrefix = [&](const std::string& p) {
		for (Scene3DModel* m : sc.GetModels())
			if (m != nullptr && m->interactionTag.rfind(p, 0) == 0) return true;
		return false;
	};
	for (const Trigger& t : triggers)
	{
		if (t.kind == Trigger::Kind::Auto) continue;
		bool ok = (t.kind == Trigger::Kind::Slot) ? hasSlot(t.target)
			: (t.kind == Trigger::Kind::TagPrefix) ? hasTagPrefix(t.target)
			: hasTag(t.target);
		if (!ok)
			std::cout << "SceneLogic: WARNING trigger '" << t.name << "' references missing "
				<< (t.kind == Trigger::Kind::Slot ? "slot '" : "tag '") << t.target << "'" << std::endl;
	}
	for (const Sequence& q : sequences)
		for (const std::string& t : q.tags)
			if (!hasTag(t))
				std::cout << "SceneLogic: WARNING sequence '" << q.name << "' missing tag '" << t << "'" << std::endl;
	for (const Chain& c : chains)
		for (const ChainStep& st : c.steps)
			if (!hasTag(st.tag))
				std::cout << "SceneLogic: WARNING chain '" << c.name << "' missing tag '" << st.tag << "'" << std::endl;
	for (const Levers& l : levers)
		for (const std::string& t : l.tags)
			if (!hasTag(t))
				std::cout << "SceneLogic: WARNING levers '" << l.name << "' missing tag '" << t << "'" << std::endl;
	for (const TeleporterPair& t : teleporters)
		if (!hasTag(t.tagA) || !hasTag(t.tagB))
			std::cout << "SceneLogic: WARNING teleporter '" << t.name << "' missing pad tag" << std::endl;
	for (const Roamer& r : roamers)
	{
		if (r.battleId.empty())
			std::cout << "SceneLogic: WARNING roamer '" << r.name << "' has no battle" << std::endl;
		if (!r.slot.empty() && !hasSlot(r.slot))
			std::cout << "SceneLogic: WARNING roamer '" << r.name
				<< "' references missing slot '" << r.slot << "'" << std::endl;
	}
	for (const Encounters& e : encounters)
		if (e.battles.empty())
			std::cout << "SceneLogic: WARNING encounters block has no battles" << std::endl;
	for (const Chest& c : chests)
		if (!hasTag(c.tag))
			std::cout << "SceneLogic: WARNING chest missing tag '" << c.tag << "'" << std::endl;
}

void SceneLogic::SetTagMaterial(const std::string& tag, const std::string& mat)
{
	if (mat.empty()) return;
	for (Scene3DModel* m : Scene3D::Get().GetModels())
		if (m != nullptr && m->interactionTag == tag)
			m->material = MaterialLibrary::Get().Find(mat);
}

void SceneLogic::OpenGate(Game& game, const std::string& tag)
{
	const std::vector<Scene3DModel*>& models = Scene3D::Get().GetModels();
	for (int i = 0; i < (int)models.size(); i++)
	{
		if (models[i] != nullptr && models[i]->interactionTag == tag)
		{
			Scene3D::Get().RemoveModel(game, i);
			game.logger.Log("SceneLogic: opened gate '" + tag + "'");
			return;
		}
	}
}

bool SceneLogic::LeverDone(const Levers& lv) const
{
	return !lv.persistKey.empty() && Val(lv.persistKey) != 0;
}

void SceneLogic::ApplyState(Game& game, SceneLogicHost& host)
{
	for (Sequence& q : sequences)
	{
		q.progress.clear();
		bool solved = !q.persistKey.empty() && Val(q.persistKey) != 0;
		for (const std::string& t : q.tags)
			SetTagMaterial(t, solved ? q.litMat : q.unlitMat);
		for (const std::string& l : q.lights)
			Scene3D::Get().SetLightOn(l, solved);
		if (solved)
			for (const Action& a : q.onSolved)
				if (a.type == Action::Type::OpenGate)
					OpenGate(game, a.arg);
	}
	// Chains: re-open the gates of every completed step (only gate actions -
	// cutscenes/battles must not replay on scene re-entry).
	for (const Chain& c : chains)
	{
		int done = c.persistKey.empty() ? 0 : Val(c.persistKey);
		for (int i = 0; i < done && i < (int)c.steps.size(); i++)
			for (const Action& a : c.steps[i].actions)
				if (a.type == Action::Type::OpenGate)
					OpenGate(game, a.arg);
	}
	for (const Levers& l : levers)
		if (LeverDone(l))
			for (const Action& a : l.onSolved)
				if (a.type == Action::Type::OpenGate)
					OpenGate(game, a.arg);

	// Random-keep: keep N random members of the tag set, remove the rest
	// (re-rolled on every scene load, matching the legacy desert behavior).
	for (const RandomKeep& rk : randomKeeps)
	{
		std::vector<int> members;
		const std::vector<Scene3DModel*>& models = Scene3D::Get().GetModels();
		for (int i = 0; i < (int)models.size(); i++)
			if (models[i] != nullptr && models[i]->interactionTag.rfind(rk.tagPrefix, 0) == 0)
				members.push_back(i);
		while ((int)members.size() > rk.keep)
		{
			int drop = std::rand() % (int)members.size();
			Scene3D::Get().RemoveModel(game, members[drop]);
			// indices above the removed one shift down
			int removed = members[drop];
			members.erase(members.begin() + drop);
			for (int& m : members) if (m > removed) m--;
		}
	}
	// B3: roll each encounter table's first threshold.
	for (Encounters& e : encounters)
	{
		e.walked = 0.0f;
		float span = e.maxSteps - e.minSteps;
		e.nextAt = e.minSteps + (span > 0.0f ? (float)(std::rand() % (int)(span * 100.0f + 1)) / 100.0f : 0.0f);
	}

	// Lottery: pick this load's winning tag.
	for (Lottery& lo : lotteries)
	{
		std::vector<std::string> tags;
		for (Scene3DModel* m : Scene3D::Get().GetModels())
			if (m != nullptr && m->interactionTag.rfind(lo.tagPrefix, 0) == 0)
				tags.push_back(m->interactionTag);
		lo.picked = tags.empty() ? "" : tags[std::rand() % tags.size()];
	}

	// Declarative gates, applied on load (the per-frame pass in Update keeps
	// them enforced afterwards).
	for (const Gate& g : gates)
		if (CondsHold(g.conds, host))
			OpenGate(game, g.tag);
}

void SceneLogic::RunActions(Game& game, SceneLogicHost& host, const std::vector<Action>& actions)
{
	for (const Action& a : actions)
	{
		switch (a.type)
		{
		case Action::Type::Cutscene:
			game.cutsceneManager.PlayCutscene(a.arg.c_str());
			break;
		case Action::Type::Battle:
			host.StartBattle(game, a.arg, 0);
			break;
		case Action::Type::Teleport:
			if (host.PlayerVisual() != nullptr)
				host.PlayerVisual()->SetPosition(a.pos);
			break;
		case Action::Type::TeleportSlot:
		{
			glm::vec3 p; float y = 0.0f;
			if (Scene3D::Get().GetAnchor(a.arg, p, y) && host.PlayerVisual() != nullptr)
				host.PlayerVisual()->SetPosition(p);
			break;
		}
		case Action::Type::OpenGate:
			OpenGate(game, a.arg);
			break;
		case Action::Type::Give:
			host.GiveItem(a.arg);
			game.logger.Log("SceneLogic: gave " + a.arg);
			break;
		case Action::Type::Set:
			persist[a.arg] = a.value;
			break;
		case Action::Type::Add:
			persist[a.arg] += a.value;
			break;
		case Action::Type::Log:
			game.logger.Log("SceneLogic: " + a.arg);
			break;
		case Action::Type::Status:
			host.ApplyStatus(a.arg, a.value);
			break;
		}
	}
}

void SceneLogic::SuspendForBattle()
{
	battleReturnScene = loadedScene;
	suspendedRoamers = roamers;   // alive flags + positions, bodies ignored
}

void SceneLogic::SpawnRoamers(Game& game, SceneLogicHost& host)
{
	// Returning from a battle in this same scene: restore what was on the
	// field instead of repopulating it (see SuspendForBattle).
	bool resuming = (!battleReturnScene.empty() && battleReturnScene == loadedScene
		&& !suspendedRoamers.empty());

	for (Roamer& r : roamers)
	{
		// Respawn fresh on every scene entry (deliberate: cleared rooms
		// refill, matching the request).
		if (r.body >= 0)
			host.DespawnBody(r.body);
		r.body = -1;
		r.alive = false;

		if (!CondsHold(r.conds, host))
			continue;

		if (resuming)
		{
			const Roamer* prev = nullptr;
			for (const Roamer& sr : suspendedRoamers)
				if (sr.name == r.name) { prev = &sr; break; }
			if (prev != nullptr)
			{
				if (!prev->alive)
					continue;              // stays defeated
				r.home = prev->home;
				r.pos = prev->pos;
				r.target = prev->target;
				r.yaw = prev->yaw;
				r.phase = prev->phase;
				r.dir = prev->dir;
				r.alive = true;
				if (r.pattern.empty())
					r.pattern = host.DefaultRoamPattern(r.battleId);
				if (r.pattern.empty())
					r.pattern = "wander";
				r.body = host.SpawnBody(r.pos, r.yaw,
					r.material.empty() ? std::string("enemy_body") : r.material);
				continue;
			}
		}

		glm::vec3 home = r.home;
		if (!r.slot.empty())
		{
			float y = 0.0f;
			if (!Scene3D::Get().GetAnchor(r.slot, home, y))
				continue;
		}
		r.home = home;
		r.pos = home;
		r.target = home;
		r.phase = (float)(std::rand() % 628) / 100.0f;
		r.dir = 1;
		r.alive = true;
		if (r.pattern.empty())
			r.pattern = host.DefaultRoamPattern(r.battleId);
		if (r.pattern.empty())
			r.pattern = "wander";
		r.body = host.SpawnBody(r.pos, r.yaw,
			r.material.empty() ? std::string("enemy_body") : r.material);
	}
}

void SceneLogic::UpdateRoamers(Game& game, SceneLogicHost& host,
	const glm::vec3& playerPos, float dtSec)
{
	if (dtSec <= 0.0f || dtSec > 0.5f)
		dtSec = 0.016f;   // first frame / hitch guard

	for (Roamer& r : roamers)
	{
		if (!r.alive)
			continue;

		float distToPlayer = glm::distance(glm::vec2(r.pos.x, r.pos.z),
			glm::vec2(playerPos.x, playerPos.z));
		bool chasing = distToPlayer <= r.chaseRadius;
		float speed = r.speed * (chasing ? r.chaseMult : 1.0f);

		if (chasing)
		{
			r.target = playerPos;
		}
		else if (r.pattern == "sentry")
		{
			r.target = r.home;
		}
		else if (r.pattern == "circle")
		{
			r.phase += dtSec * (speed / std::max(50.0f, r.region * 0.6f));
			r.target = r.home + glm::vec3(cosf(r.phase), 0.0f, sinf(r.phase)) * (r.region * 0.6f);
		}
		else if (r.pattern == "patrol")
		{
			// ping-pong along X across the region
			glm::vec3 endA = r.home - glm::vec3(r.region, 0.0f, 0.0f);
			glm::vec3 endB = r.home + glm::vec3(r.region, 0.0f, 0.0f);
			r.target = (r.dir > 0) ? endB : endA;
			if (glm::distance(glm::vec2(r.pos.x, r.pos.z), glm::vec2(r.target.x, r.target.z)) < 40.0f)
				r.dir = -r.dir;
		}
		else   // wander: drift to random points inside the region
		{
			if (glm::distance(glm::vec2(r.pos.x, r.pos.z), glm::vec2(r.target.x, r.target.z)) < 40.0f)
			{
				float a = (float)(std::rand() % 628) / 100.0f;
				float d = (float)(std::rand() % 100) / 100.0f * r.region;
				r.target = r.home + glm::vec3(cosf(a), 0.0f, sinf(a)) * d;
			}
		}

		glm::vec3 delta(r.target.x - r.pos.x, 0.0f, r.target.z - r.pos.z);
		float len = glm::length(delta);
		if (len > 1.0f)
		{
			glm::vec3 step = delta / len * std::min(speed * dtSec, len);
			glm::vec3 next = r.pos + step;
			// Stay on walkable ground and out of walls, like the player does.
			Scene3D::Get().ResolveAgainstSolids(next, 35.0f, 120.0f, 75.0f);
			float gy;
			if (Scene3D::Get().HasWalkableGround())
			{
				if (Scene3D::Get().GetGroundHeight(next, 75.0f, gy))
					next.y = gy;
				else
					next = r.pos;   // refuse to walk off the world
			}
			if (glm::distance(glm::vec2(next.x, next.z), glm::vec2(r.pos.x, r.pos.z)) > 0.01f)
				r.yaw = glm::degrees(atan2f(next.x - r.pos.x, -(next.z - r.pos.z)));
			r.pos = next;
		}

		if (r.body >= 0)
			host.MoveBody(r.body, r.pos, r.yaw);

		// --- contact -> battle, with the first-strike rule ----------------
		if (distToPlayer > r.touchRadius)
			continue;

		const float DEG2RAD = 3.14159265f / 180.0f;
		glm::vec2 toPlayer = glm::vec2(playerPos.x - r.pos.x, playerPos.z - r.pos.z);
		float tlen = glm::length(toPlayer);
		if (tlen > 0.001f) toPlayer /= tlen;
		float ry = r.yaw * DEG2RAD;
		glm::vec2 enemyFacing(sinf(ry), -cosf(ry));
		float py = host.PlayerFacingYaw() * DEG2RAD;
		glm::vec2 playerFacing(sinf(py), -cosf(py));

		int advantage = 0;
		if (glm::dot(enemyFacing, toPlayer) < -0.3f)
			advantage = 1;    // player came at its back -> first strike
		else if (glm::dot(playerFacing, -toPlayer) < -0.3f)
			advantage = -1;   // it reached the player's back -> ambush

		game.logger.Log("SceneLogic: roamer '" + r.name + "' contact (advantage "
			+ std::to_string(advantage) + ")");
		r.alive = false;
		if (r.body >= 0)
		{
			host.DespawnBody(r.body);
			r.body = -1;
		}
		host.StartBattle(game, r.battleId, advantage);
		return;
	}
}

void SceneLogic::Update(Game& game, SceneLogicHost& host)
{
	Scene3D& scene = Scene3D::Get();
	if (!scene.active)
	{
		loadedScene.clear();
		active = false;
		return;
	}
	if (scene.currentScene != loadedScene)
	{
		LoadForScene(game, scene.currentScene);
		if (active)
		{
			ApplyState(game, host);
			SpawnRoamers(game, host);
		}
	}
	if (!active)
		return;

	Character3D* visual = host.PlayerVisual();
	if (visual == nullptr || game.cutsceneManager.watchingCutscene)
		return;

	bool actionPressed = host.ActionPressed();

	// --- B2: what could ACTION do right now? ----------------------------
	prompt.clear();

	// Selector cycling (TAB), skipping options whose conditions don't hold.
	{
		if (host.TogglePressed())
		{
			for (Selector& sel : selectors)
			{
				for (int step = 1; step <= (int)sel.options.size(); step++)
				{
					int next = (sel.current + step) % (int)sel.options.size();
					if (CondsHold(sel.options[next].conds, host))
					{
						sel.current = next;
						game.logger.Log("SceneLogic: " + sel.name + " -> "
							+ sel.options[next].name);
						break;
					}
				}
			}
		}
	}

	// Sequence step-timers (strict/windowed sequences time out to on-wrong).
	for (Sequence& q : sequences)
	{
		if (q.windowMs <= 0.0f || q.progress.empty()) continue;
		q.sinceStepMs += game.dt;
		if (q.sinceStepMs > q.windowMs)
		{
			q.progress.clear();
			for (const std::string& t : q.tags) SetTagMaterial(t, q.unlitMat);
			game.logger.Log("SceneLogic: " + q.name + " timed out, reset");
			RunActions(game, host, q.onWrong);
		}
	}

	glm::vec3 pos = visual->GetPosition();

	auto tagPos = [&](const std::string& tag, glm::vec3& out) -> bool {
		for (Scene3DModel* m : scene.GetModels())
			if (m != nullptr && m->interactionTag == tag) { out = m->GetPosition(); return true; }
		return false;
	};

	// --- roamers (visible enemies; contact starts their battle) ---------
	if (!roamers.empty())
	{
		UpdateRoamers(game, host, pos, game.dt / 1000.0f);
		if (!active) return;
	}

	// --- B3 wandering encounters ----------------------------------------
	if (hasLastPos && !encounters.empty() && !host.PlayerAirborne())
	{
		float moved = glm::distance(glm::vec2(pos.x, pos.z), glm::vec2(lastPos.x, lastPos.z));
		if (moved > 0.5f && moved < 500.0f)   // ignore teleports
		{
			for (Encounters& e : encounters)
			{
				if (pos.y < e.yMin || pos.y > e.yMax) continue;
				if (!CondsHold(e.conds, host)) continue;
				e.walked += moved / 100.0f;   // 100 units = one tile
				if (e.walked < e.nextAt || e.battles.empty()) continue;

				e.walked = 0.0f;
				float span = e.maxSteps - e.minSteps;
				e.nextAt = e.minSteps + (span > 0.0f
					? (float)(std::rand() % (int)(span * 100.0f + 1)) / 100.0f : 0.0f);
				const std::string& id = e.battles[std::rand() % e.battles.size()];
				game.logger.Log("SceneLogic: wandering encounter -> " + id);
				host.StartBattle(game, id, 0);
				lastPos = pos;
				return;
			}
		}
	}
	lastPos = pos;
	hasLastPos = true;

	// B2 prompt scan: mirrors the press-driven checks below (same targets,
	// same conditions), but only records the hint - it never fires anything.
	{
		auto nearTag = [&](const std::string& tag, float r) {
			for (Scene3DModel* m : scene.GetModels())
				if (m != nullptr && m->interactionTag == tag
					&& glm::distance(pos, m->GetPosition()) <= r) return true;
			return false;
		};
		auto nearPrefix = [&](const std::string& pfx, float r, std::string& hitTag) {
			for (Scene3DModel* m : scene.GetModels())
				if (m != nullptr && m->interactionTag.rfind(pfx, 0) == 0
					&& glm::distance(pos, m->GetPosition()) <= r)
				{ hitTag = m->interactionTag; return true; }
			return false;
		};

		for (const Trigger& t : triggers)
		{
			if (!t.press || (t.once && t.fired)) continue;
			if (!CondsHold(t.conds, host)) continue;
			bool in = false;
			std::string hit;
			if (t.kind == Trigger::Kind::Slot)
			{
				glm::vec3 sp; float sy = 0.0f;
				in = scene.GetAnchor(t.target, sp, sy) && glm::distance(pos, sp) <= t.radius;
			}
			else if (t.kind == Trigger::Kind::Tag) in = nearTag(t.target, t.radius);
			else if (t.kind == Trigger::Kind::TagPrefix) in = nearPrefix(t.target, t.radius, hit);
			if (!in) continue;
			prompt = t.prompt.empty() ? std::string("Examine") : t.prompt;
			break;
		}
		if (prompt.empty())
			for (const Chest& c : chests)
			{
				if (!c.persistKey.empty() && Val(c.persistKey) != 0) continue;
				if (!nearTag(c.tag, c.radius)) continue;
				prompt = c.prompt.empty() ? std::string("Open the chest") : c.prompt;
				break;
			}
		if (prompt.empty())
			for (const Sequence& q : sequences)
			{
				if (!q.persistKey.empty() && Val(q.persistKey) != 0) continue;
				if (!CondsHold(q.conds, host)) continue;
				bool in = false;
				for (const std::string& t : q.tags) if (nearTag(t, q.radius)) { in = true; break; }
				if (!in) continue;
				prompt = q.prompt.empty() ? std::string("Light it") : q.prompt;
				break;
			}
		if (prompt.empty())
			for (const Chain& c : chains)
			{
				if (!CondsHold(c.conds, host)) continue;
				int done = c.persistKey.empty() ? 0 : Val(c.persistKey);
				if (done >= (int)c.steps.size()) continue;
				if (!nearTag(c.steps[done].tag, c.radius)) continue;
				prompt = c.prompt.empty() ? std::string("Use it") : c.prompt;
				break;
			}
		if (prompt.empty())
			for (const Levers& l : levers)
			{
				if (LeverDone(l)) continue;
				bool in = false;
				std::string hit;
				if (!l.tags.empty())
				{
					for (const std::string& t : l.tags)
						if (nearTag(t, l.radius) && Val(l.persistKey + "." + t) == 0) { in = true; break; }
				}
				else if (!l.tagPrefix.empty())
					in = nearPrefix(l.tagPrefix, l.radius, hit) && Val(l.persistKey + "." + hit) == 0;
				if (!in) continue;
				prompt = l.prompt.empty() ? std::string("Pull it") : l.prompt;
				break;
			}
		if (prompt.empty() && !host.PlayerAirborne())
			for (const VaultZone& v : vaults)
			{
				bool inZone = pos.y >= v.yMin && pos.y <= v.yMax;
				bool nearRope = false;
				if (!inZone)
					for (Scene3DModel* m : scene.GetModels())
						if (m != nullptr && m->interactionTag.rfind(v.ropePrefix, 0) == 0
							&& glm::distance(pos, m->GetPosition()) <= v.reach) { nearRope = true; break; }
				if (inZone || nearRope) { prompt = "Jump"; break; }
			}
	}

	// --- triggers -------------------------------------------------------
	for (Trigger& t : triggers)
	{
		if (t.once && t.fired) continue;
		if (t.press && !actionPressed) continue;
		if (!CondsHold(t.conds, host)) continue;

		if (t.kind != Trigger::Kind::Auto)
		{
			bool have = false;
			glm::vec3 tp2; float ty = 0.0f;
			if (t.kind == Trigger::Kind::Slot)
				have = scene.GetAnchor(t.target, tp2, ty);
			else if (t.kind == Trigger::Kind::Tag)
				have = tagPos(t.target, tp2);
			else   // TagPrefix: nearest matching tag within radius
			{
				for (Scene3DModel* m : scene.GetModels())
				{
					if (m == nullptr || m->interactionTag.rfind(t.target, 0) != 0) continue;
					if (glm::distance(pos, m->GetPosition()) <= t.radius) { have = true; tp2 = m->GetPosition(); break; }
				}
				if (!have) continue;
			}
			if (!have || glm::distance(pos, tp2) > t.radius) continue;
		}

		t.fired = true;
		RunActions(game, host, t.actions);
		return;
	}

	// --- declarative gates ----------------------------------------------
	for (const Gate& g : gates)
		if (CondsHold(g.conds, host))
			OpenGate(game, g.tag);

	// --- teleporter pairs (proximity, no press) -------------------------
	for (TeleporterPair& t : teleporters)
	{
		glm::vec3 pa, pb;
		if (!tagPos(t.tagA, pa) || !tagPos(t.tagB, pb)) continue;
		bool atA = glm::distance(pos, pa) <= t.radius;
		bool atB = glm::distance(pos, pb) <= t.radius;
		bool activated = t.persistKey.empty() || Val(t.persistKey) != 0;

		if (atB && !t.nearB)
		{
			if (!activated) persist[t.persistKey] = 1;
			visual->SetPosition(pa);
			t.nearA = true; t.nearB = false;
			game.logger.Log("SceneLogic: teleporter " + t.name);
			return;
		}
		if (atA && !t.nearA && activated)
		{
			visual->SetPosition(pb);
			t.nearB = true; t.nearA = false;
			game.logger.Log("SceneLogic: teleporter " + t.name);
			return;
		}
		t.nearA = atA;
		t.nearB = atB;
	}

	// --- chests ---------------------------------------------------------
	if (actionPressed)
	{
		for (Chest& c : chests)
		{
			if (!c.persistKey.empty() && Val(c.persistKey) != 0) continue;
			glm::vec3 cp;
			if (!tagPos(c.tag, cp) || glm::distance(pos, cp) > c.radius) continue;
			if (!c.persistKey.empty()) persist[c.persistKey] = 1;
			if (!c.item.empty())
			{
				host.GiveItem(c.item);
				game.logger.Log("SceneLogic: chest '" + c.tag + "' gave " + c.item);
			}
			return;
		}
	}

	// --- lotteries --------------------------------------------------------
	if (actionPressed)
	{
		for (Lottery& lo : lotteries)
		{
			if (lo.picked.empty()) continue;
			for (Scene3DModel* m : scene.GetModels())
			{
				if (m == nullptr || m->interactionTag.rfind(lo.tagPrefix, 0) != 0) continue;
				if (glm::distance(pos, m->GetPosition()) > lo.radius) continue;
				bool hit = (m->interactionTag == lo.picked);
				game.logger.Log("SceneLogic: lottery " + lo.name + " "
					+ m->interactionTag + (hit ? " HIT" : " miss"));
				RunActions(game, host, hit ? lo.onHit : lo.onMiss);
				return;
			}
		}
	}

	// --- chains (strict-next ordered) -----------------------------------
	if (actionPressed)
	{
		for (Chain& c : chains)
		{
			if (!CondsHold(c.conds, host)) continue;
			int done = c.persistKey.empty() ? 0 : Val(c.persistKey);
			if (done >= (int)c.steps.size()) continue;
			const ChainStep& st = c.steps[done];
			glm::vec3 sp;
			if (!tagPos(st.tag, sp) || glm::distance(pos, sp) > c.radius) continue;
			if (!c.persistKey.empty()) persist[c.persistKey] = done + 1;
			game.logger.Log("SceneLogic: " + c.name + " step " + std::to_string(done + 1)
				+ "/" + std::to_string(c.steps.size()));
			RunActions(game, host, st.actions);
			return;
		}
	}

	// --- levers (all-of-set) --------------------------------------------
	if (actionPressed)
	{
		for (Levers& l : levers)
		{
			if (LeverDone(l)) continue;
			for (Scene3DModel* m : scene.GetModels())
			{
				if (m == nullptr || m->interactionTag.empty()) continue;
				const std::string& tag = m->interactionTag;
				bool member = false;
				if (!l.tags.empty())
				{
					for (const std::string& t : l.tags) if (t == tag) member = true;
				}
				else if (!l.tagPrefix.empty() && tag.rfind(l.tagPrefix, 0) == 0)
				{
					member = true;
					for (const std::string& t : l.except) if (t == tag) member = false;
				}
				if (!member) continue;
				if (glm::distance(pos, m->GetPosition()) > l.radius) continue;

				std::string perTag = l.persistKey + "." + tag;
				if (Val(perTag) != 0) return;   // already thrown
				persist[perTag] = 1;

				int need = !l.tags.empty() ? (int)l.tags.size() : l.count;
				int have = 0;
				for (const auto& kv : persist)
					if (kv.second != 0 && kv.first.rfind(l.persistKey + ".", 0) == 0) have++;
				game.logger.Log("SceneLogic: " + l.name + " " + tag + " ("
					+ std::to_string(have) + "/" + std::to_string(need) + ")");
				if (have >= need)
				{
					persist[l.persistKey] = 1;
					RunActions(game, host, l.onSolved);
					game.logger.Log("SceneLogic: " + l.name + " COMPLETE");
				}
				return;
			}
		}
	}

	// --- sequences (free-order, judged at full length) ------------------
	if (actionPressed)
	{
		for (Sequence& q : sequences)
		{
			if (!q.persistKey.empty() && Val(q.persistKey) != 0) continue;
			if (!CondsHold(q.conds, host)) continue;
			for (Scene3DModel* m : scene.GetModels())
			{
				if (m == nullptr || m->interactionTag.empty()) continue;
				bool inSeq = false;
				for (const std::string& t : q.tags) if (t == m->interactionTag) inSeq = true;
				if (!inSeq) continue;
				if (glm::distance(pos, m->GetPosition()) > q.radius) continue;

				// Strict mode: only the exact next-in-order tag advances;
				// anything else = immediate wrong (reset + penalty).
				if (q.strict && m->interactionTag != q.order[q.progress.size()])
				{
					q.progress.clear();
					for (const std::string& t : q.tags) SetTagMaterial(t, q.unlitMat);
					for (const std::string& l : q.lights) Scene3D::Get().SetLightOn(l, false);
					game.logger.Log("SceneLogic: " + q.name + " wrong step, reset");
					RunActions(game, host, q.onWrong);
					return;
				}

				bool already = false;
				for (const std::string& t : q.progress) if (t == m->interactionTag) already = true;
				if (already) return;

				q.sinceStepMs = 0.0f;
				q.progress.push_back(m->interactionTag);
				SetTagMaterial(m->interactionTag, q.litMat);
				game.logger.Log("SceneLogic: " + q.name + " " + m->interactionTag
					+ " (" + std::to_string(q.progress.size()) + "/" + std::to_string(q.order.size()) + ")");
				if (q.progress.size() < q.order.size())
					return;

				if (q.progress == q.order)
				{
					if (!q.persistKey.empty()) persist[q.persistKey] = 1;
					for (const std::string& l : q.lights)
						Scene3D::Get().SetLightOn(l, true);
					RunActions(game, host, q.onSolved);
					game.logger.Log("SceneLogic: " + q.name + " SOLVED");
				}
				else
				{
					q.progress.clear();
					for (const std::string& t : q.tags) SetTagMaterial(t, q.unlitMat);
					for (const std::string& l : q.lights) Scene3D::Get().SetLightOn(l, false);
					game.logger.Log("SceneLogic: " + q.name + " wrong order, reset");
					RunActions(game, host, q.onWrong);
				}
				return;
			}
		}
	}

	// --- vault zones ----------------------------------------------------
	if (actionPressed && !host.PlayerAirborne())
	{
		for (const VaultZone& v : vaults)
		{
			bool inZone = pos.y >= v.yMin && pos.y <= v.yMax;
			bool nearRope = false;
			if (!inZone)
			{
				for (Scene3DModel* m : scene.GetModels())
				{
					if (m == nullptr || m->interactionTag.rfind(v.ropePrefix, 0) != 0) continue;
					if (glm::distance(pos, m->GetPosition()) <= v.reach) { nearRope = true; break; }
				}
			}
			if (inZone || nearRope)
			{
				const float DEG2RAD = 3.14159265f / 180.0f;
				float f = host.PlayerFacingYaw() * DEG2RAD;
				host.Vault(glm::vec3(sinf(f), 0.0f, -cosf(f)));
				game.logger.Log("SceneLogic: vaulted (" + v.name + ")");
				return;
			}
		}
	}
}
