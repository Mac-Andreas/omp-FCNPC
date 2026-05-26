/* =========================================
 *
 *  FCNPC for open.mp - companion-plugin bridge (implementation)
 *
 * ========================================= */

#include <Server/Components/Pawn/Impl/pawn_natives.hpp>
#include "FCNPC.hpp"
#include "bridge.hpp"

#include <cstdint>

namespace
{
	AMX* MainAMX()
	{
		auto self = FCNPCComponent::getInstance();
		return self ? self->mainAMX() : nullptr;
	}

	// Resolve a registered native to its C function pointer via the AMX native
	// table. nullptr if not present (plugin not loaded).
	AMX_NATIVE FindNative(AMX* amx, const char* name)
	{
		int index = -1;
		if (amx_FindNative(amx, name, &index) != AMX_ERR_NONE || index < 0)
		{
			return nullptr;
		}
		AMX_HEADER* hdr = reinterpret_cast<AMX_HEADER*>(amx->base);
		auto* stub = reinterpret_cast<AMX_FUNCSTUBNT*>(amx->base + hdr->natives + index * hdr->defsize);
		if (stub->address == 0)
		{
			return nullptr;
		}
		return reinterpret_cast<AMX_NATIVE>(static_cast<uintptr_t>(stub->address));
	}
}

namespace fcnpc_bridge
{
	bool available()
	{
		return MainAMX() != nullptr;
	}

	bool mapAndreasFindZ(float x, float y, float& z)
	{
		AMX* amx = MainAMX();
		if (!amx)
		{
			return false;
		}
		AMX_NATIVE fn = FindNative(amx, "MapAndreas_FindZ_For2DCoord");
		if (!fn)
		{
			return false;
		}
		cell amxAddr = 0;
		cell* phys = nullptr;
		if (amx_Allot(amx, 1, &amxAddr, &phys) != AMX_ERR_NONE)
		{
			return false;
		}
		float fx = x, fy = y;
		cell params[4];
		params[0] = 3 * sizeof(cell);
		params[1] = amx_ftoc(fx);
		params[2] = amx_ftoc(fy);
		params[3] = amxAddr;
		cell ret = fn(amx, params);
		cell zc = *phys;
		z = amx_ctof(zc);
		amx_Release(amx, amxAddr);
		return ret != 0;
	}

	bool colAndreasRayDown(float x, float y, float& z)
	{
		AMX* amx = MainAMX();
		if (!amx)
		{
			return false;
		}
		AMX_NATIVE fn = FindNative(amx, "CA_RayCastLine");
		if (!fn)
		{
			return false;
		}
		cell amxAddr = 0;
		cell* phys = nullptr;
		if (amx_Allot(amx, 3, &amxAddr, &phys) != AMX_ERR_NONE)
		{
			return false;
		}
		float sx = x, sy = y, sz = z + 1000.0f;
		float ex = x, ey = y, ez = z - 1000.0f;
		cell params[10];
		params[0] = 9 * sizeof(cell);
		params[1] = amx_ftoc(sx);
		params[2] = amx_ftoc(sy);
		params[3] = amx_ftoc(sz);
		params[4] = amx_ftoc(ex);
		params[5] = amx_ftoc(ey);
		params[6] = amx_ftoc(ez);
		params[7] = amxAddr;
		params[8] = amxAddr + static_cast<cell>(sizeof(cell));
		params[9] = amxAddr + static_cast<cell>(2 * sizeof(cell));
		cell ret = fn(amx, params);
		cell zc = phys[2];
		z = amx_ctof(zc);
		amx_Release(amx, amxAddr);
		return ret != 0;
	}

	bool colAndreasRayLine(float sx, float sy, float sz, float ex, float ey, float ez,
		float& hx, float& hy, float& hz)
	{
		AMX* amx = MainAMX();
		if (!amx)
		{
			return false;
		}
		AMX_NATIVE fn = FindNative(amx, "CA_RayCastLine");
		if (!fn)
		{
			return false;
		}
		cell amxAddr = 0;
		cell* phys = nullptr;
		if (amx_Allot(amx, 3, &amxAddr, &phys) != AMX_ERR_NONE)
		{
			return false;
		}
		float fsx = sx, fsy = sy, fsz = sz, fex = ex, fey = ey, fez = ez;
		cell params[10];
		params[0] = 9 * sizeof(cell);
		params[1] = amx_ftoc(fsx);
		params[2] = amx_ftoc(fsy);
		params[3] = amx_ftoc(fsz);
		params[4] = amx_ftoc(fex);
		params[5] = amx_ftoc(fey);
		params[6] = amx_ftoc(fez);
		params[7] = amxAddr;
		params[8] = amxAddr + static_cast<cell>(sizeof(cell));
		params[9] = amxAddr + static_cast<cell>(2 * sizeof(cell));
		cell ret = fn(amx, params);
		cell cx = phys[0], cy = phys[1], cz = phys[2];
		hx = amx_ctof(cx);
		hy = amx_ctof(cy);
		hz = amx_ctof(cz);
		amx_Release(amx, amxAddr);
		return ret != 0;
	}

	bool findGroundZ(int moveMode, float x, float y, float& z)
	{
		if (moveMode == 2 /* COLANDREAS */)
		{
			return colAndreasRayDown(x, y, z);
		}
		if (moveMode == 1 /* MAPANDREAS */)
		{
			return mapAndreasFindZ(x, y, z);
		}
		return false;
	}

	int streamerInternalObject(int playerid, int dynamicid)
	{
		AMX* amx = MainAMX();
		if (!amx)
		{
			return -1;
		}
		AMX_NATIVE fn = FindNative(amx, "Streamer_GetItemInternalID");
		if (!fn)
		{
			return -1;
		}
		cell params[4];
		params[0] = 3 * sizeof(cell);
		params[1] = playerid;
		params[2] = 0; // STREAMER_TYPE_OBJECT
		params[3] = dynamicid;
		return static_cast<int>(fn(amx, params));
	}
}
