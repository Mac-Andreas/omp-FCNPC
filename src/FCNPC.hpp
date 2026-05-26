/* =========================================
 *
 *  FCNPC for open.mp - Fully Controllable NPC
 *  ------------------------------------------
 *
 *  Compatibility shim that re-exposes the classic SA-MP `FCNPC_*` native API
 *  on top of open.mp's built-in `INPCComponent`. Existing FCNPC gamemodes are
 *  meant to run unmodified.
 *
 *  Direct mappings call INPC / INPCComponent / IPlayer. The handful of FCNPC
 *  features open.mp has no equivalent for are hand-rolled here (per-NPC extras,
 *  global weapon-default table, config stores).
 *
 *  Original FCNPC: OrMisicL (2013-2015), ziggi (2016-present).
 *  open.mp remaining port: Xyranaut.
 *
 * ========================================= */

#pragma once

#include <sdk.hpp>

#include <Server/Components/NPCs/npcs.hpp>
#include <Server/Components/Pawn/pawn.hpp>
#include <Server/Components/Vehicles/vehicles.hpp>
#include <Server/Components/Objects/objects.hpp>

#include <array>
#include <chrono>
#include <string>
#include <unordered_map>
#include <unordered_set>

using namespace Impl;

// ---- FCNPC <-> open.mp constant maps -------------------------------------

enum FCNPCMoveType
{
	FCNPC_MOVE_TYPE_AUTO = -1,
	FCNPC_MOVE_TYPE_WALK = 0,
	FCNPC_MOVE_TYPE_RUN = 1,
	FCNPC_MOVE_TYPE_SPRINT = 2,
	FCNPC_MOVE_TYPE_DRIVE = 3,
};

inline NPCMoveType FCNPC_ToOmpMoveType(int fcnpcType)
{
	switch (fcnpcType)
	{
	case FCNPC_MOVE_TYPE_WALK: return NPCMoveType_Walk;
	case FCNPC_MOVE_TYPE_RUN: return NPCMoveType_Jog;
	case FCNPC_MOVE_TYPE_SPRINT: return NPCMoveType_Sprint;
	case FCNPC_MOVE_TYPE_DRIVE: return NPCMoveType_Drive;
	default: return NPCMoveType_Auto;
	}
}

// Per-weapon info bundle (reload/shoot/clip/accuracy), used for the global
// default table that FCNPC_*WeaponDefaultInfo manage (no INPC equivalent).
struct FCNPCWeaponInfo
{
	int reloadTime = -1;
	int shootTime = -1;
	int clipSize = -1;
	float accuracy = 1.0f;
	bool set = false;
};

// Per-NPC state that INPC does not track itself.
struct FCNPCExtra
{
	int moveMode = -1; // FCNPC_MOVE_MODE_AUTO
	float minHeightPosCall = 0.0f;
	std::string playbackPath;
	int surfingDynamicObject = -1; // streamer dynamic object id (INVALID_OBJECT_ID = none)
};

class FCNPCComponent final
	: public IComponent
	, public PawnEventHandler
	, public CoreEventHandler
{
private:
	ICore* core_ = nullptr;
	IPawnComponent* pawn_ = nullptr;
	INPCComponent* npcs_ = nullptr;
	IVehiclesComponent* vehicles_ = nullptr;
	IObjectsComponent* objects_ = nullptr;
	TimePoint nextZSample_ = TimePoint::min();

	// Global config stores (FCNPC globals with no INPC equivalent).
	int updateRate_ = 100;
	int tickRate_ = 25;
	bool crashLog_ = false;
	std::unordered_set<int> usedMoveModes_;
	std::unordered_set<int> usedPathfindings_;
	std::array<FCNPCWeaponInfo, 64> weaponDefaults_ {};

	// Per-NPC extras keyed by npc id.
	std::unordered_map<int, FCNPCExtra> extras_;

	inline static FCNPCComponent* instance_ = nullptr;

public:
	PROVIDE_UID(UID(0x46434E50435F6F6D));

	ICore* getCore() const { return core_; }
	INPCComponent* npcs() const { return npcs_; }
	IVehiclesComponent* vehicles() const { return vehicles_; }
	IPawnComponent* pawn() const { return pawn_; }

	// FCNPC ids == open.mp NPC/player ids (claimed on create), so a pool lookup
	// is the identity mapping.
	INPC* getNPC(int id) const { return npcs_ ? npcs_->get(id) : nullptr; }
	IPlayer* getPlayer(int id) const { return core_ ? core_->getPlayers().get(id) : nullptr; }
	IVehicle* getVehicle(int id) const { return vehicles_ ? vehicles_->get(id) : nullptr; }
	IObjectsComponent* objects() const { return objects_; }

	// Main gamemode AMX, used to call companion-plugin natives (ColAndreas /
	// MapAndreas / Streamer) at runtime. Null until a script is loaded.
	AMX* mainAMX() const
	{
		if (!pawn_)
		{
			return nullptr;
		}
		IPawnScript* s = pawn_->mainScript();
		return s ? s->GetAMX() : nullptr;
	}

	int& updateRate() { return updateRate_; }
	int& tickRate() { return tickRate_; }
	bool& crashLog() { return crashLog_; }
	std::unordered_set<int>& usedMoveModes() { return usedMoveModes_; }
	std::unordered_set<int>& usedPathfindings() { return usedPathfindings_; }
	FCNPCWeaponInfo& weaponDefault(int weapon)
	{
		static FCNPCWeaponInfo dummy;
		return (weapon >= 0 && weapon < 64) ? weaponDefaults_[weapon] : dummy;
	}
	FCNPCExtra& extra(int npcid) { return extras_[npcid]; }

	// --- IComponent ---
	StringView componentName() const override { return "FCNPC"; }
	SemanticVersion componentVersion() const override { return SemanticVersion(2, 1, 0, 0); }
	void onLoad(ICore* c) override;
	void onInit(IComponentList* components) override;
	void onReady() override;
	void onFree(IComponent* component) override;
	void free() override;
	void reset() override;

	// --- PawnEventHandler ---
	void onAmxLoad(IPawnScript& script) override;
	void onAmxUnload(IPawnScript& script) override;

	// --- CoreEventHandler ---
	// Samples ground Z (MapAndreas/ColAndreas) for NPCs whose move mode requests
	// it, while they are moving. Throttled; no-op if the plugin is absent.
	void onTick(Microseconds elapsed, TimePoint now) override;

	static FCNPCComponent* getInstance();
	~FCNPCComponent();
};
