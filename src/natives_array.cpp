/* =========================================
 *
 *  FCNPC for open.mp - array-parameter natives
 *
 *  The pawn-natives SCRIPT_API system has no clean adapter for array params,
 *  so these three are registered as classic AMX natives with manual cell
 *  marshalling. amx_GetAddr/amx_Register are defined in FCNPC.cpp (pawn_impl).
 *
 * ========================================= */

#include <Server/Components/Pawn/Impl/pawn_natives.hpp>
#include "FCNPC.hpp"

namespace
{
	// FCNPC_GetValidArray(npcs[], size) -> count
	cell AMX_NATIVE_CALL n_GetValidArray(AMX* amx, const cell* params)
	{
		cell* arr = nullptr;
		if (amx_GetAddr(amx, params[1], &arr) != AMX_ERR_NONE || !arr)
			return 0;
		int size = static_cast<int>(params[2]);
		auto npcs = FCNPCComponent::getInstance()->npcs();
		int count = 0;
		if (npcs)
		{
			for (INPC* npc : *npcs)
			{
				if (count >= size)
					break;
				arr[count++] = npc->getID();
			}
		}
		return count;
	}

	// FCNPC_AddPointsToMovePath(pathid, Float:points[][3], size)
	cell AMX_NATIVE_CALL n_AddPointsToMovePath(AMX* amx, const cell* params)
	{
		int pathid = static_cast<int>(params[1]);
		cell* outer = nullptr;
		if (amx_GetAddr(amx, params[2], &outer) != AMX_ERR_NONE || !outer)
			return 0;
		int size = static_cast<int>(params[3]);
		auto npcs = FCNPCComponent::getInstance()->npcs();
		if (!npcs)
			return 0;
		int added = 0;
		for (int i = 0; i < size; ++i)
		{
			// pawn 2D array: outer[i] is a byte offset from &outer[i] to row i.
			cell* row = reinterpret_cast<cell*>(reinterpret_cast<char*>(&outer[i]) + outer[i]);
			float x = amx_ctof(row[0]);
			float y = amx_ctof(row[1]);
			float z = amx_ctof(row[2]);
			if (npcs->addPointToPath(pathid, Vector3(x, y, z)))
				++added;
		}
		return added;
	}

	// FCNPC_AddPointsToMovePath2(pathid, Float:x[], Float:y[], Float:z[], size)
	cell AMX_NATIVE_CALL n_AddPointsToMovePath2(AMX* amx, const cell* params)
	{
		int pathid = static_cast<int>(params[1]);
		cell* xs = nullptr;
		cell* ys = nullptr;
		cell* zs = nullptr;
		if (amx_GetAddr(amx, params[2], &xs) != AMX_ERR_NONE
			|| amx_GetAddr(amx, params[3], &ys) != AMX_ERR_NONE
			|| amx_GetAddr(amx, params[4], &zs) != AMX_ERR_NONE)
			return 0;
		int size = static_cast<int>(params[5]);
		auto npcs = FCNPCComponent::getInstance()->npcs();
		if (!npcs)
			return 0;
		int added = 0;
		for (int i = 0; i < size; ++i)
		{
			if (npcs->addPointToPath(pathid, Vector3(amx_ctof(xs[i]), amx_ctof(ys[i]), amx_ctof(zs[i]))))
				++added;
		}
		return added;
	}

	const AMX_NATIVE_INFO kArrayNatives[] = {
		{ "FCNPC_GetValidArray", n_GetValidArray },
		{ "FCNPC_AddPointsToMovePath", n_AddPointsToMovePath },
		{ "FCNPC_AddPointsToMovePath2", n_AddPointsToMovePath2 },
		{ nullptr, nullptr },
	};
}

void FCNPC_RegisterArrayNatives(AMX* amx)
{
	amx_Register(amx, kArrayNatives, -1);
}
