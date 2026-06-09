/* =========================================
 *
 *  FCNPC for open.mp - component lifecycle
 *
 * ========================================= */

// pawn-natives macros (SCRIPT_API) + lookups (IPlayer&, etc.).
#include <Server/Components/Pawn/Impl/pawn_natives.hpp>

// Function implementations for the pawn-natives system. Include exactly once per binary.
#include <Server/Components/Pawn/Impl/pawn_impl.hpp>

#include "FCNPC.hpp"
#include "bridge.hpp"

#include <chrono>
#include <cmath>

// Defined in natives_array.cpp (classic AMX natives for array params).
void FCNPC_RegisterArrayNatives(AMX* amx);

void FCNPCComponent::onLoad(ICore* c)
{
	core_ = c;
	// pawn-natives needs the core reference for value lookups even before onInit.
	setAmxLookups(core_);
	// Tick events drive the optional MapAndreas/ColAndreas ground-Z sampling.
	core_->getEventDispatcher().addEventHandler(this);
	core_->printLn("  FCNPC for open.mp v2.1.0 loaded (INPCComponent shim).");
}

void FCNPCComponent::onInit(IComponentList* components)
{
	// The engine we drive.
	npcs_ = components->queryComponent<INPCComponent>();
	if (!npcs_)
	{
		core_->logLn(LogLevel::Error, "[FCNPC] INPCComponent not found. The NPC component must be enabled in open.mp.");
	}

	vehicles_ = components->queryComponent<IVehiclesComponent>();
	objects_ = components->queryComponent<IObjectsComponent>();

	// Pawn wiring for native registration and callback forwarding.
	pawn_ = components->queryComponent<IPawnComponent>();
	if (pawn_)
	{
		setAmxFunctions(pawn_->getAmxFunctions());
		setAmxLookups(components);
		pawn_->getEventDispatcher().addEventHandler(this);
	}
}

void FCNPCComponent::onReady()
{
}

void FCNPCComponent::onFree(IComponent* component)
{
	if (component == pawn_)
	{
		pawn_ = nullptr;
		setAmxFunctions();
		setAmxLookups();
	}
	if (component == npcs_)
	{
		npcs_ = nullptr;
	}
	if (component == vehicles_)
	{
		vehicles_ = nullptr;
	}
	if (component == objects_)
	{
		objects_ = nullptr;
	}
}

void FCNPCComponent::onTick(Microseconds elapsed, TimePoint now)
{
	// MapAndreas / ColAndreas ground-Z sampling for moving NPCs. Throttled, and a
	// no-op unless a companion plugin and the FCNPC_MOVE_MODE_MAPANDREAS /
	// _COLANDREAS mode are both present. Experimental.
	if (!npcs_ || now < nextZSample_)
	{
		return;
	}
	nextZSample_ = now + std::chrono::milliseconds(200);

	if (!fcnpc_bridge::available())
	{
		return;
	}

	for (INPC* npc : *npcs_)
	{
		auto it = extras_.find(npc->getID());
		if (it == extras_.end())
		{
			continue;
		}
		int mode = it->second.moveMode;
		if (mode != 1 /* MAPANDREAS */ && mode != 2 /* COLANDREAS */)
		{
			continue;
		}
		if (!npc->isMoving())
		{
			continue;
		}
		Vector3 pos = npc->getPosition();
		float z = pos.z;
		if (fcnpc_bridge::findGroundZ(mode, pos.x, pos.y, z))
		{
			z += fcnpc_bridge::kGroundPedZOffset;
			if (z >= it->second.minHeightPosCall && std::fabs(z - pos.z) > 0.05f)
			{
				npc->setPosition(Vector3(pos.x, pos.y, z), false);
			}
		}
	}
}

void FCNPCComponent::free()
{
	delete this;
}

void FCNPCComponent::reset()
{
	updateRate_ = 100;
	tickRate_ = 25;
	crashLog_ = false;
	usedMoveModes_.clear();
	usedPathfindings_.clear();
	weaponDefaults_.fill(FCNPCWeaponInfo {});
	extras_.clear();
}

void FCNPCComponent::onAmxLoad(IPawnScript& script)
{
	// SCRIPT_API natives auto-register here.
	pawn_natives::AmxLoad(script.GetAMX());
	// Classic array-parameter natives.
	FCNPC_RegisterArrayNatives(script.GetAMX());
}

void FCNPCComponent::onAmxUnload(IPawnScript& script)
{
}

FCNPCComponent* FCNPCComponent::getInstance()
{
	if (instance_ == nullptr)
	{
		instance_ = new FCNPCComponent();
	}
	return instance_;
}

FCNPCComponent::~FCNPCComponent()
{
	if (pawn_)
	{
		pawn_->getEventDispatcher().removeEventHandler(this);
	}
	if (core_)
	{
		core_->getEventDispatcher().removeEventHandler(this);
	}
}
