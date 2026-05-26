/* =========================================
 *
 *  FCNPC for open.mp - companion-plugin bridge
 *
 *  Calls ColAndreas / MapAndreas / Streamer Pawn natives from C++ at runtime.
 *  These are legacy SA-MP plugins (no open.mp C++ interface), so the only way to
 *  reach them is through the main script's AMX native table. Every call is a
 *  no-op returning false/-1 when the plugin is not loaded.
 *
 *  EXPERIMENTAL: written to the documented native signatures but not verifiable
 *  without the companion plugins on a live server.
 *
 * ========================================= */

#pragma once

namespace fcnpc_bridge
{
	// True if a main script AMX is available to route native calls through.
	bool available();

	// MapAndreas_FindZ_For2DCoord(x, y, &z). Returns false if unavailable.
	bool mapAndreasFindZ(float x, float y, float& z);

	// CA_RayCastLine straight down through (x, y) to find ground z.
	bool colAndreasRayDown(float x, float y, float& z);

	// CA_RayCastLine from start to end; on hit, writes the hit point. False if
	// no hit or ColAndreas is unavailable.
	bool colAndreasRayLine(float sx, float sy, float sz, float ex, float ey, float ez,
		float& hx, float& hy, float& hz);

	// Dispatch by FCNPC move mode: 1 = MapAndreas, 2 = ColAndreas.
	bool findGroundZ(int moveMode, float x, float y, float& z);

	// Streamer_GetItemInternalID(playerid, STREAMER_TYPE_OBJECT, dynamicid).
	// Returns the per-player internal object id, or -1 if unavailable.
	int streamerInternalObject(int playerid, int dynamicid);
}
