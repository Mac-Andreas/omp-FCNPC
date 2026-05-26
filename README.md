# FCNPC for open.mp

**Fully Controllable NPC** — the classic [FCNPC](https://github.com/ziggi/FCNPC)
`FCNPC_*` Pawn API, re-implemented for [open.mp](https://github.com/openmultiplayer/open.mp).

The original FCNPC is a SA-MP plugin that drives NPCs by patching the
`samp03svr` binary at fixed memory addresses and injecting raw RakNet packets.
That approach cannot work on open.mp — a different, open-source binary with a
proper component SDK. open.mp instead ships a built-in NPC engine
(`INPCComponent`). **This component is a compatibility layer that re-exposes the
`FCNPC_*` natives on top of `INPCComponent`, plus the remaining ~5% of FCNPC that
open.mp's NPC engine does not provide.** Existing FCNPC gamemodes run largely
unmodified.

---

## Install

1. Download the [latest release](../../releases/latest).
2. Put `FCNPC.dll` (Windows) / `FCNPC.so` (Linux) into your server's `components/`.
3. Put `FCNPC.inc` into your Pawn `qawno/include/` (or `pawno/include/`).
4. Make sure the NPC component is enabled in `config.json` (ships with open.mp).

```pawn
#include <open.mp>
#include <FCNPC>

public OnGameModeInit()
{
    new id = FCNPC_Create("Bot");
    FCNPC_Spawn(id, 0, 1958.33, 1343.12, 15.36);
    return 1;
}

public OnPlayerConnect(playerid)
{
    // walk every bot to the new player
    new npcs[100], count = FCNPC_GetValidArray(npcs);
    for (new i = 0; i < count; i++)
        FCNPC_GoToPlayer(npcs[i], playerid, FCNPC_MOVE_TYPE_RUN);
    return 1;
}
```

---

## Ported status

All **182** `FCNPC_*` natives are declared in `FCNPC.inc` and implemented:
**180 ported, 2 pending**. Most call open.mp's `INPC` / `INPCComponent` /
`IPlayer` directly; the features open.mp lacks are hand-rolled in this component.

**Full per-native breakdown: [port-status.md](port-status.md).**

Notes on the non-trivial ones:

| Native | Status |
|---|---|
| `FCNPC_GetClosestEntityInBetween` | Ported - C++ scan of players/NPCs/vehicles/objects; the `MAP` flag adds a ColAndreas raycast when that plugin is loaded |
| `FCNPC_SetMoveMode` (`MAPANDREAS` / `COLANDREAS`) | Ported - the component samples ground Z each tick via the companion plugin while the NPC moves |
| `FCNPC_SetSurfingDynamicObject` / `Get...` | Ported - resolves the streamer internal object id; needs the Streamer plugin |
| `FCNPC_ShowInTabListForPlayer` / `Hide...` | Pending - no-op; open.mp exposes no scoreboard toggle (needs a raw player-list RPC) |
| `MapAndreas_*`, `CA_*` (ColAndreas) | Not included - separate plugins (below) |

The collision / streamer features call those plugins' natives at runtime (the
companion-plugin model the original FCNPC used by bundling them) - install the
open.mp builds below. Nothing falls back to SA-MP.

---

## Feature test script

[`filterscripts/fcnpc_test.pwn`](filterscripts/fcnpc_test.pwn) is a filterscript that exercises the
`FCNPC_*` API in-game (also attached to each release). Load it as a side script
and type **`/fcnpc`** to open a dialog menu:

- **Overview** - plugin + include version, what the harness covers.
- **Commands** - live demos: spawn / walk / run / sprint / follow (chase) /
  aim / shoot / melee / animation / enter+exit vehicle / move-path patrol /
  play node / invulnerable / kill / respawn / stop all actions / destroy.
- **Config** - tunables with defaults you can change live (movement type/mode/
  pathfinding/speed, weapon/ammo/reloading/fighting style/accuracy, skin/health/
  armour, plugin update+tick rate, global ColAndreas/raycast modes) + restore.
- **Ported features (INPC vs FCNPC)** - spawns a native `INPC` NPC and an FCNPC
  NPC side by side (labelled by floating 3D text) and A/Bs what the bare open.mp
  engine does vs what this port fixes - e.g. chase autoRestart (INPC stops at the
  start spot, FCNPC keeps chasing). Each test prints the intended behaviour and
  what each engine does.
- **Debug** - on-screen HUD (state, health, weapon, moving, speed, distance).

Needs `<open.mp>`, `<omp_npc>` and `<FCNPC>`; no other dependencies.

---

## Companion plugins

This port does not bundle them; install the standalone plugins (open.mp builds).
They back the collision/heightmap natives and dynamic-object surfing, exactly as
the original FCNPC relied on them. `FCNPC_SetMoveMode` constants
(`FCNPC_MOVE_MODE_MAPANDREAS`, `FCNPC_MOVE_MODE_COLANDREAS`) are kept for source
compatibility.

### MapAndreas - heightmap Z lookup
Author: **Philip (philip1337)**

[![MapAndreas](https://img.shields.io/github/v/release/philip1337/samp-plugin-mapandreas)](https://github.com/philip1337/samp-plugin-mapandreas/releases)
[![Download DLL / SO](https://img.shields.io/badge/Download-DLL%20%2F%20SO-2ea44f)](https://github.com/philip1337/samp-plugin-mapandreas/releases/latest)
[![Include](https://img.shields.io/badge/Download-mapandreas.inc-orange)](https://github.com/philip1337/samp-plugin-mapandreas/blob/master/include/mapandreas.inc)

### ColAndreas - full map collision / raycasting
Author: **Pottus** (with [uL]Chris42O, Slice)

[![ColAndreas](https://img.shields.io/github/v/release/Pottus/ColAndreas)](https://github.com/Pottus/ColAndreas/releases)
[![Download DLL / SO / EXE](https://img.shields.io/badge/Download-DLL%20%2F%20SO%20%2F%20EXE-2ea44f)](https://github.com/Pottus/ColAndreas/releases/latest)
[![Include](https://img.shields.io/badge/Download-colandreas.inc-orange)](https://github.com/Pottus/ColAndreas/blob/master/Server/include/colandreas.inc)

### Streamer - dynamic objects (for `FCNPC_*SurfingDynamicObject`)
Author: **Incognito**

[![Streamer](https://img.shields.io/github/v/release/samp-incognito/samp-streamer-plugin)](https://github.com/samp-incognito/samp-streamer-plugin/releases)
[![Download DLL / SO](https://img.shields.io/badge/Download-DLL%20%2F%20SO-2ea44f)](https://github.com/samp-incognito/samp-streamer-plugin/releases/latest)
[![Include](https://img.shields.io/badge/Download-streamer.inc-orange)](https://github.com/samp-incognito/samp-streamer-plugin/blob/master/streamer.inc)

---

## Build from source

Dependencies are git submodules (`sdk`, `pawn-natives`, `pawn`) — clone with:

```sh
git clone --recurse-submodules https://github.com/Mac-Andreas/open.mp-FCNPC
```

open.mp's Windows server is **x86** and components must use the **Microsoft C++
ABI**. Pick one:

**A. MSVC-ABI cross-build on macOS/Linux (no Visual Studio):**
```sh
brew install llvm lld msitools          # macOS (or distro equivalents)
git clone https://github.com/mstorsjo/msvc-wine /tmp/msvc-wine
python3 /tmp/msvc-wine/vsdownload.py --accept-license \
    --architecture x86 --host-arch x64 --dest /tmp/msvc
MSVC_ROOT=/tmp/msvc ./build-msvc-clang.sh    # -> FCNPC.dll
```

**B. MSVC on Windows:**
```sh
cmake -B build -A Win32 && cmake --build build --config Release
```

**C. Linux .so:** `g++ -m32` build (see `.github/workflows/build.yml`).

CI builds `FCNPC.dll` + `FCNPC.so` and attaches them, with `FCNPC.inc`, to every
tagged release.

---

## Credits

- **Original FCNPC:** OrMisicL (2013–2015), ziggi (2016–2024).
- **open.mp NPC engine (`INPCComponent`):** the open.mp team.
- **open.mp remaining port:** **Xyranaut**.
- Companion plugins: MapAndreas (Philip), ColAndreas (Pottus, [uL]Chris42O, Slice), Streamer (Incognito).
