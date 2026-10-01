#pragma once

// P3/P4 of docs/REFACTOR_DATA_DRIVEN.md: the per-scene declarative logic
// interpreter. Scenes with a data/logic/<scene>.logic file get their
// triggers/puzzles from data; scenes without one fall back to the legacy
// hardcoded singletons (MyGUI gates them on HandlesCurrentScene()).
//
// Grammar (line-based, # comments):
//
//   trigger <name>
//     near-slot <slot> <radius> [press]   | near-tag <tag> <radius> [press]
//     auto                                (no target: fires when conditions hold)
//     once                                (at most once per boot)
//     if <cond>                           (all conds must hold; see below)
//     do <action>
//
//   sequence <name>              free-order judged-at-end puzzle (torches)
//     tags <t...>  order <t...>  lit-material <m>  unlit-material <m>
//     lights <name...>           scene lights ON only while solved
//     on-wrong <action>  on-solved <action>  persist <key>
//
//   chain <name>                 strict-next ordered chain; off-order = no-op
//     persist <key>              (int = steps completed; gates re-open on load)
//     step <tag> [<action>...]   (actions run when that step completes)
//
//   levers <name>                interact-all-of-set, order-independent
//     tags <t...>       (or)  tag-prefix <p> [except <t...>]  count <N>
//     persist <key>              (done flag; per-tag keys <key>.<tag>)
//     on-solved <action>...
//
//   gate <tag>                   model removed whenever its conds hold
//     if <cond>                  (checked on load and every frame)
//
//   teleporter-pair <name>
//     tags <padA> <padB>  [radius <r>]  [persist <key>]
//                                (with persist: first arrival at padB
//                                 activates; without: always two-way)
//
//   random-keep <name>           tag-prefix <p>  keep <N>
//                                (scene load: keep N random members, remove rest)
//   lottery <name>               tag-prefix <p>  [radius <r>]
//     on-hit <action>...         (pressing the randomly-picked member)
//     on-miss <action>...        (pressing any other member)
//   selector <name>              TAB-cycled mode; conditions: sel <name> <opt>
//     option <opt> [if <cond>]   (option locked until its conds hold)
//
//   sequence extras: strict (wrong press = on-wrong immediately),
//     window <ms> (max time between steps), if <cond> (armed only while true)
//
//   Any trigger/sequence/chain/levers/chest may carry `prompt <text>`: the
//   on-screen hint shown while it is in reach (B2). Without one, a default
//   is derived ("Examine", "Open the chest", ...).
//
//   roamer <name>                a VISIBLE enemy that starts its battle on
//                                contact (respawns on scene entry)
//     battle <id>                which encounter it starts
//     at <slot> | at-xyz <x> <y> <z>    where it lives
//     material <mat>             body material (defaults to enemy_body)
//     pattern <wander|patrol|sentry|circle>   overrides the enemy type's own
//     region <r>  speed <s>  chase <r>  touch <r>
//     if <cond>                  optional guards
//
//   encounters                   INVISIBLE step-counter battles (legacy; the
//                                roamer block above is the default model)
//     battles <id...>            pool (one picked at random)
//     steps <min> <max>          tiles walked between fights (default 12 26)
//     y <min> <max>              optional: only on this floor
//     if <cond>                  optional guards
//
//   vault-zone <name>            y <min> <max>  rope-prefix <p>  reach <r>
//   chest <tag>                  gives <item...>  persist <key>
//
// Conditions: flag <key> | !flag <key> | min <key> <N> | eq <key> <N>
//             | chapter <N> (SaveManager::currentChapter >= N)
//             | recruited <Name> | !recruited <Name>
// Actions:    cutscene <label> | battle <id> | teleport <x> <y> <z>
//             | teleport-slot <slot> | open-gate <tag> | give <item...>
//             | set <key> [N] | add <key> [N] | fade-to <slot>
//
// Persistence: `persist` keys are ints in a generic map saved by SaveManager
// as one "sceneLogic" object; legacy per-puzzle save keys are seeded into it
// on load so old saves keep their progress.

#include "leak_check.h"
#include <glm/glm.hpp>
#include <map>
#include <string>
#include <vector>

class Game;
class Character3D;

// Everything game-specific the interpreter needs, supplied by the game
// (P6 of Eggwhite's docs/REFACTOR_DATA_DRIVEN.md: the interpreter is engine
// code; input keys, battles, items, statuses, chapters and party are not).
// The *Pressed() calls must return per-frame EDGES (host owns the was-down
// state).
class KINJO_API SceneLogicHost
{
public:
	virtual ~SceneLogicHost() {}
	virtual bool ActionPressed() = 0;
	virtual bool TogglePressed() = 0;
	virtual Character3D* PlayerVisual() = 0;
	virtual float PlayerFacingYaw() = 0;
	virtual bool PlayerAirborne() = 0;
	virtual void Vault(const glm::vec3& dirXZ) = 0;
	// advantage: +1 = the player struck first (approached from behind),
	// -1 = the roamer ambushed the player, 0 = neither.
	virtual void StartBattle(Game& game, const std::string& id, int advantage = 0) = 0;
	// Roamer bodies: the game owns what an enemy LOOKS like (Eggwhite builds
	// the same blocky humanoid it uses for the party/ghosts). Handles are
	// opaque ints; -1 means "spawn failed".
	virtual int SpawnBody(const glm::vec3& pos, float yaw, const std::string& material) = 0;
	virtual void MoveBody(int body, const glm::vec3& pos, float yaw) = 0;
	virtual void DespawnBody(int body) = 0;
	// Per-enemy-type default wander pattern (Eggwhite reads enemies.txt's
	// `roam` field for the battle's first enemy). Empty = "wander".
	virtual std::string DefaultRoamPattern(const std::string& battleId) = 0;
	virtual void GiveItem(const std::string& item) = 0;
	virtual void ApplyStatus(const std::string& name, int ms) = 0;
	// Move the player to a slot behind a screen fade (doorways into interiors
	// that live in the SAME scene - cheaper than loading a scene per room).
	virtual void FadeMoveTo(const std::string& slot) = 0;
	virtual int CurrentChapter() = 0;
	virtual bool IsRecruited(const std::string& name) = 0;
};

class KINJO_API SceneLogic
{
public:
	static SceneLogic& Get();

	// Generic persisted flags/counters (the game's save system owns
	// serializing this map).
	std::map<std::string, int> persist;

	// Call every frame while a Scene3D may be active.
	void Update(Game& game, SceneLogicHost& host);

	bool HandlesCurrentScene() const { return active; }

	// Battle-arena round trip: the game calls this before loading the arena
	// and the state is reapplied when the SAME scene loads again, so coming
	// back from a fight is not a "scene re-entry" (the enemy you just fought
	// stays gone, the rest keep their spots). Any other scene clears it.
	void SuspendForBattle();

	// B2 interact prompts: what the player could press ACTION on right now
	// ("Light the torch"), recomputed every Update. Empty when nothing is in
	// reach. The game renders it (engine has no HUD opinions).
	const std::string& CurrentPrompt() const { return prompt; }

private:
	SceneLogic() = default;

	struct Action
	{
		enum class Type { Cutscene, Battle, Teleport, TeleportSlot, OpenGate, Give, Set, Add, Log, Status, FadeTo };
		Type type = Type::Cutscene;
		std::string arg;
		glm::vec3 pos = glm::vec3(0.0f);
		int value = 1;
	};

	struct Cond
	{
		enum class Type { Flag, NotFlag, Min, Eq, Chapter, Recruited, NotRecruited, Sel };
		Type type = Type::Flag;
		std::string key;
		std::string key2;   // sel: option name
		int value = 0;
	};

	struct Trigger
	{
		std::string name;
		std::string prompt;   // B2 override; default derives from the actions
		enum class Kind { Slot, Tag, TagPrefix, Auto };
		Kind kind = Kind::Slot;
		std::string target;
		float radius = 150.0f;
		bool press = false;
		bool once = false;
		bool fired = false;
		std::vector<Cond> conds;
		std::vector<Action> actions;
	};

	struct Sequence
	{
		std::string name;
		std::string prompt;
		std::vector<std::string> tags, order, progress;
		std::string litMat, unlitMat, persistKey;
		// Scene lights that are ON only while the puzzle is solved (the
		// torch-room glow: darkened torches used to keep glowing).
		std::vector<std::string> lights;
		float radius = 150.0f;
		bool strict = false;        // wrong-tag press = immediate on-wrong+reset
		float windowMs = 0.0f;      // max ms between steps (0 = untimed)
		float sinceStepMs = 0.0f;
		std::vector<Cond> conds;    // sequence only reacts while these hold
		std::vector<Action> onWrong, onSolved;
	};

	struct ChainStep
	{
		std::string tag;
		std::vector<Action> actions;
	};
	struct Chain
	{
		std::string name;
		std::string prompt;
		std::string persistKey;   // int progress
		float radius = 150.0f;
		std::vector<Cond> conds;
		std::vector<ChainStep> steps;
	};

	struct Levers
	{
		std::string name;
		std::string prompt;
		std::vector<std::string> tags;      // explicit set...
		std::string tagPrefix;              // ...or prefix
		std::vector<std::string> except;
		int count = 0;                      // required with tag-prefix
		std::string persistKey;
		float radius = 150.0f;
		std::vector<Action> onSolved;
	};

	struct TeleporterPair
	{
		std::string name;
		std::string tagA, tagB;
		float radius = 90.0f;
		std::string persistKey;   // empty = always active
		bool nearA = false, nearB = false;
	};

	// Declarative gate: the tagged model is removed whenever the conditions
	// hold (checked on scene load AND every frame) - replaces the legacy
	// EnsureGatesOpenIfSolved re-apply pattern.
	struct Gate
	{
		std::string tag;
		std::vector<Cond> conds;
	};

	// B3 wandering encounters: walking accumulates distance; once past a
	// random threshold in [minSteps, maxSteps] a random battle from the
	// table fires. `steps` are 100-unit tiles. Suppressed while airborne /
	// in cutscenes (the caller only ticks during free roam).
	struct Encounters
	{
		std::vector<std::string> battles;
		float minSteps = 12.0f, maxSteps = 26.0f;
		float yMin = -1e9f, yMax = 1e9f;   // optional height band (a floor)
		std::vector<Cond> conds;
		float walked = 0.0f;               // tiles since the last fight
		float nextAt = 0.0f;               // rolled threshold
	};

	// B3 (rev.2): a VISIBLE enemy that wanders the scene and starts its
	// battle on contact - the Paper Mario model, replacing the old invisible
	// step-counter encounters. Touch from behind decides who strikes first.
	// Roamers respawn on every scene entry (no persistence by design); one
	// that has been touched is consumed for the rest of that visit.
	struct Roamer
	{
		std::string name, battleId, material, slot, pattern;
		glm::vec3 home = glm::vec3(0.0f);
		float region = 500.0f;      // wander/patrol extent around home
		float speed = 130.0f;       // units/sec while idling
		float chaseRadius = 650.0f; // start chasing inside this
		float chaseMult = 1.5f;     // speed multiplier while chasing
		float touchRadius = 95.0f;  // contact = battle
		std::vector<Cond> conds;

		// runtime (per scene visit)
		int body = -1;
		bool alive = false;
		glm::vec3 pos = glm::vec3(0.0f);
		glm::vec3 target = glm::vec3(0.0f);
		float yaw = 0.0f;
		float phase = 0.0f;         // circle pattern angle
		int dir = 1;                // patrol direction
	};

	struct VaultZone
	{
		std::string name;
		float yMin = 0.0f, yMax = 0.0f;
		std::string ropePrefix = "rope_swing_";
		float reach = 200.0f;
	};

	struct Chest
	{
		std::string tag, item, persistKey;
		std::string prompt;
		float radius = 150.0f;
	};

	// Pick `keep` random members of a tag set on every scene (re)load and
	// REMOVE the rest (the desert's wandering old-man shop).
	struct RandomKeep
	{
		std::string name, tagPrefix;
		int keep = 1;
	};

	// Pick ONE random member of a tag set per scene load; pressing on the
	// winner runs on-hit, any other member runs on-miss (the crate trapdoor).
	struct Lottery
	{
		std::string name, tagPrefix;
		float radius = 150.0f;
		std::string picked;         // session state, chosen at scene load
		std::vector<Action> onHit, onMiss;
	};

	// A cycled "equipped mode" (TAB): options may require a flag to select.
	// Conditions test it with `sel <selector> <option>`.
	struct Selector
	{
		std::string name;
		struct Opt { std::string name; std::vector<Cond> conds; };
		std::vector<Opt> options;
		int current = 0;
	};

	std::vector<Trigger> triggers;
	std::vector<Sequence> sequences;
	std::vector<Chain> chains;
	std::vector<Levers> levers;
	std::vector<TeleporterPair> teleporters;
	std::vector<Gate> gates;
	std::vector<Encounters> encounters;
	std::vector<Roamer> roamers;
	std::vector<VaultZone> vaults;
	std::vector<Chest> chests;
	std::vector<RandomKeep> randomKeeps;
	std::vector<Lottery> lotteries;
	std::vector<Selector> selectors;

	std::string loadedScene;
	bool active = false;
	std::string prompt;   // B2: this frame's interact hint (see CurrentPrompt)
	// see SuspendForBattle
	std::string battleReturnScene;
	std::vector<Roamer> suspendedRoamers;
	glm::vec3 lastPos = glm::vec3(0.0f);   // B3: for step accumulation
	bool hasLastPos = false;

	int Val(const std::string& key) const;
	bool CondsHold(const std::vector<Cond>& conds, SceneLogicHost& host) const;
	void LoadForScene(Game& game, const std::string& scene);
	void ApplyState(Game& game, SceneLogicHost& host);
	void RunActions(Game& game, SceneLogicHost& host, const std::vector<Action>& actions);
	// Roamer lifecycle + per-frame movement/contact (see the Roamer struct).
	void SpawnRoamers(Game& game, SceneLogicHost& host);
	void UpdateRoamers(Game& game, SceneLogicHost& host, const glm::vec3& playerPos, float dtSec);
	void SetTagMaterial(const std::string& tag, const std::string& mat);
	void OpenGate(Game& game, const std::string& tag);
	bool LeverDone(const Levers& lv) const;
};
