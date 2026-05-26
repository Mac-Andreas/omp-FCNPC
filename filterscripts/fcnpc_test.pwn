/* =========================================
 *  fcnpc_test - FCNPC feature test harness (filterscript)
 *
 *  /fcnpc opens a list dialog:
 *      0  Overview          - what FCNPC is + what this harness covers
 *      1  Commands          - live demos of FCNPC features near you
 *      2  Config: Movement  - move type / mode / pathfinding / speed (+defaults)
 *      3  Config: Combat     - weapon / ammo / reloading / fighting style / accuracy
 *      4  Config: Appearance - skin / health / armour / invulnerable
 *      5  Config: Plugin     - update rate / tick rate / global move modes / crashlog
 *      6  Debug options     - on-screen NPC HUD + one-shot info dumps
 *      7  Restore defaults  - reset every tunable back to its default
 *
 *  Each player drives ONE demo NPC, spawned in front of them on first use.
 *  Config values are global (single-tester harness) and shown with [default: x].
 * ========================================= */

#include <open.mp>
#include <omp_npc.inc>   // native open.mp NPC_* (INPC) - for the ported-feature comparison
#include <FCNPC.inc>

// -------------------------------------------------------------------------
//  Dialog ids
// -------------------------------------------------------------------------
#define DLG_MAIN        (8400)
#define DLG_OVERVIEW    (8401)
#define DLG_CMDS        (8402)
#define DLG_CFG_MOVE    (8403)
#define DLG_CFG_COMBAT  (8404)
#define DLG_CFG_APP     (8405)
#define DLG_CFG_PLUGIN  (8406)
#define DLG_DEBUG       (8407)
#define DLG_RESTORE     (8408)
#define DLG_INPUT       (8409)
#define DLG_PORTED      (8410)

// colours
#define C_INFO   0x33CCFFFF
#define C_OK     0x66FF66FF
#define C_WARN   0xFFCC33FF
#define C_ERR    0xFF6666FF

// -------------------------------------------------------------------------
//  Config (global) + defaults
// -------------------------------------------------------------------------
enum E_CFG {
    cMoveType,
    cMoveMode,
    cPathfinding,
    Float:cMoveSpeed,
    cSkin,
    Float:cHealth,
    Float:cArmour,
    cWeapon,
    Float:cAccuracy,
    cFightStyle,
    bool:cInfiniteAmmo,
    bool:cReloading,
    bool:cInvuln,
    cUpdateRate,
    cTickRate,
    bool:cUseModeCA,
    bool:cUsePathRaycast,
    bool:cCrashLog
}
new gCfg[E_CFG];

// plugin-reported defaults captured at init
new gDefUpdateRate = 100;
new gDefTickRate   = 50;

// per-player demo state
new gDemoNPC[MAX_PLAYERS];
new gInpcNPC[MAX_PLAYERS];   // native-INPC test NPC (ported-feature comparison)
new Text3D:gDemoLabel[MAX_PLAYERS]; // floating "FCNPC" label (nametag-independent)
new Text3D:gInpcLabel[MAX_PLAYERS]; // floating "INPC" label
new gDemoPath[MAX_PLAYERS];
new bool:gDebug[MAX_PLAYERS];
new PlayerText:gDbgTD[MAX_PLAYERS];
new bool:gDbgInit[MAX_PLAYERS];

// pending numeric-input routing
new gInputField[MAX_PLAYERS];
new gInputBack[MAX_PLAYERS];

// input field ids
#define IN_SKIN        (1)
#define IN_HEALTH      (2)
#define IN_ARMOUR      (3)
#define IN_WEAPON      (4)
#define IN_ACCURACY    (5)
#define IN_UPDATERATE  (6)
#define IN_TICKRATE    (7)
#define IN_MOVESPEED   (8)

new gHudTimer = -1;

// forward decls: tagged returns used before definition
forward Float:CycleMoveSpeed(Float:v);
forward bool:HasInpc(playerid);

// =========================================================================
//  Lifecycle
// =========================================================================
public OnFilterScriptInit()
{
    for (new i = 0; i < MAX_PLAYERS; i++) {
        gDemoNPC[i]  = INVALID_PLAYER_ID;
        gInpcNPC[i]  = INVALID_PLAYER_ID;
        gDemoLabel[i] = Text3D:INVALID_3DTEXT_ID;
        gInpcLabel[i] = Text3D:INVALID_3DTEXT_ID;
        gDemoPath[i] = FCNPC_INVALID_MOVEPATH_ID;
        gDebug[i]    = false;
        gDbgInit[i]  = false;
    }

    // capture plugin defaults before we touch anything
    gDefUpdateRate = FCNPC_GetUpdateRate();
    gDefTickRate   = FCNPC_GetTickRate();
    if (gDefUpdateRate <= 0) gDefUpdateRate = 100;
    if (gDefTickRate   <= 0) gDefTickRate   = 50;

    ResetConfigDefaults();

    gHudTimer = SetTimer("FCNPC_HudTick", 500, true);

    new ver[48];
    FCNPC_GetPluginVersion(ver, sizeof(ver));
    printf("[fcnpc_test] loaded. FCNPC plugin v%s, include v%d.", ver, FCNPC_INCLUDE_VERSION);
    return 1;
}

public OnFilterScriptExit()
{
    if (gHudTimer != -1) { KillTimer(gHudTimer); gHudTimer = -1; }
    for (new i = 0; i < MAX_PLAYERS; i++) DestroyDemo(i);
    return 1;
}

public OnPlayerDisconnect(playerid, reason)
{
    DestroyDemo(playerid);
    gDebug[playerid]   = false;
    gDbgInit[playerid] = false;
    return 1;
}

// =========================================================================
//  Command
// =========================================================================
public OnPlayerCommandText(playerid, cmdtext[])
{
    if (!strcmp(cmdtext, "/fcnpc", true)) {
        ShowMainMenu(playerid);
        return 1;
    }
    return 0;
}

ShowMainMenu(playerid)
{
    ShowPlayerDialog(playerid, DLG_MAIN, DIALOG_STYLE_LIST, "FCNPC test harness",
        "Overview\n\
Commands (live demos)\n\
Ported features (INPC vs FCNPC)\n\
Config: Movement\n\
Config: Combat & Weapons\n\
Config: Appearance & Stats\n\
Config: Plugin\n\
Debug options\n\
Restore defaults",
        "Select", "Close");
}

// =========================================================================
//  Demo NPC helpers
// =========================================================================
GetPointInFront(playerid, Float:dist, &Float:x, &Float:y, &Float:z)
{
    new Float:a;
    GetPlayerPos(playerid, x, y, z);
    GetPlayerFacingAngle(playerid, a);
    x += dist * floatsin(-a, degrees);
    y += dist * floatcos(-a, degrees);
}

bool:HasDemo(playerid)
{
    return (gDemoNPC[playerid] != INVALID_PLAYER_ID && FCNPC_IsValid(gDemoNPC[playerid]));
}

EnsureDemo(playerid)
{
    if (HasDemo(playerid)) {
        ApplyConfig(gDemoNPC[playerid]);
        return gDemoNPC[playerid];
    }
    new name[24];
    format(name, sizeof(name), "Demo_NPC_%d", playerid);
    new npc = FCNPC_Create(name);
    if (npc == INVALID_PLAYER_ID) {
        SendClientMessage(playerid, C_ERR, "FCNPC_Create failed (out of NPC/bot slots? raise max_bots).");
        return INVALID_PLAYER_ID;
    }
    gDemoNPC[playerid] = npc;

    new Float:x, Float:y, Float:z;
    GetPointInFront(playerid, 3.0, x, y, z);
    FCNPC_Spawn(npc, gCfg[cSkin], x, y, z);
    // NOTE: do NOT call FCNPC_SetAngleToPlayer here - calling setRotation right
    // after spawn flips the ped upside-down (port quirk). Use the "Face me"
    // command to test it explicitly. Spawning without it stays upright.
    ApplyConfig(npc);
    return npc;
}

ApplyConfig(npc)
{
    if (!FCNPC_IsValid(npc)) return;
    FCNPC_SetSkin(npc, gCfg[cSkin]);
    FCNPC_SetHealth(npc, gCfg[cHealth]);
    FCNPC_SetArmour(npc, gCfg[cArmour]);
    FCNPC_SetWeapon(npc, gCfg[cWeapon]);
    if (gCfg[cWeapon] > 0) FCNPC_SetWeaponAccuracy(npc, gCfg[cWeapon], gCfg[cAccuracy]);
    FCNPC_SetFightingStyle(npc, gCfg[cFightStyle]);
    FCNPC_UseInfiniteAmmo(npc, gCfg[cInfiniteAmmo]);
    FCNPC_UseReloading(npc, gCfg[cReloading]);
    FCNPC_SetInvulnerable(npc, gCfg[cInvuln]);
    if (gCfg[cMoveMode] != FCNPC_MOVE_MODE_AUTO) FCNPC_SetMoveMode(npc, gCfg[cMoveMode]);
}

DestroyDemo(playerid)
{
    if (gDemoPath[playerid] != FCNPC_INVALID_MOVEPATH_ID) {
        if (FCNPC_IsValidMovePath(gDemoPath[playerid])) FCNPC_DestroyMovePath(gDemoPath[playerid]);
        gDemoPath[playerid] = FCNPC_INVALID_MOVEPATH_ID;
    }
    if (gDemoLabel[playerid] != Text3D:INVALID_3DTEXT_ID) {
        Delete3DTextLabel(gDemoLabel[playerid]);
        gDemoLabel[playerid] = Text3D:INVALID_3DTEXT_ID;
    }
    if (gDemoNPC[playerid] != INVALID_PLAYER_ID) {
        if (FCNPC_IsValid(gDemoNPC[playerid])) FCNPC_Destroy(gDemoNPC[playerid]);
        gDemoNPC[playerid] = INVALID_PLAYER_ID;
    }
    if (gInpcLabel[playerid] != Text3D:INVALID_3DTEXT_ID) {
        Delete3DTextLabel(gInpcLabel[playerid]);
        gInpcLabel[playerid] = Text3D:INVALID_3DTEXT_ID;
    }
    if (gInpcNPC[playerid] != INVALID_PLAYER_ID) {
        NPC_Destroy(gInpcNPC[playerid]);
        gInpcNPC[playerid] = INVALID_PLAYER_ID;
    }
}

GetNearestVehicle(playerid, Float:maxrange = 30.0)
{
    new Float:px, Float:py, Float:pz;
    GetPlayerPos(playerid, px, py, pz);
    new best = INVALID_VEHICLE_ID;
    new Float:bestd = maxrange;
    for (new v = 1; v < MAX_VEHICLES; v++) {
        if (GetVehicleModel(v) == 0) continue;
        new Float:vx, Float:vy, Float:vz;
        GetVehiclePos(v, vx, vy, vz);
        new Float:d = VectorSize(px - vx, py - vy, pz - vz);
        if (d < bestd) { bestd = d; best = v; }
    }
    return best;
}

ResetConfigDefaults()
{
    gCfg[cMoveType]      = FCNPC_MOVE_TYPE_AUTO;
    gCfg[cMoveMode]      = FCNPC_MOVE_MODE_AUTO;
    gCfg[cPathfinding]   = FCNPC_MOVE_PATHFINDING_AUTO;
    gCfg[cMoveSpeed]     = FCNPC_MOVE_SPEED_AUTO;
    gCfg[cSkin]          = 230;
    gCfg[cHealth]        = 100.0;
    gCfg[cArmour]        = 0.0;
    gCfg[cWeapon]        = 0;
    gCfg[cAccuracy]      = 1.0;
    gCfg[cFightStyle]    = 4; // FIGHT_STYLE_NORMAL
    gCfg[cInfiniteAmmo]  = false;
    gCfg[cReloading]     = true;
    gCfg[cInvuln]        = false;
    gCfg[cUpdateRate]    = gDefUpdateRate;
    gCfg[cTickRate]      = gDefTickRate;
    gCfg[cUseModeCA]     = false;
    gCfg[cUsePathRaycast]= false;
    gCfg[cCrashLog]      = false;
}

ApplyPluginConfig()
{
    FCNPC_SetUpdateRate(gCfg[cUpdateRate]);
    FCNPC_SetTickRate(gCfg[cTickRate]);
    FCNPC_UseMoveMode(FCNPC_MOVE_MODE_COLANDREAS, gCfg[cUseModeCA]);
    FCNPC_UseMovePathfinding(FCNPC_MOVE_PATHFINDING_RAYCAST, gCfg[cUsePathRaycast]);
    FCNPC_UseCrashLog(gCfg[cCrashLog]);
}

// =========================================================================
//  Label helpers
// =========================================================================
MoveTypeLabel(v) {
    new s[12];
    switch (v) {
        case FCNPC_MOVE_TYPE_WALK:   s = "Walk";
        case FCNPC_MOVE_TYPE_RUN:    s = "Run";
        case FCNPC_MOVE_TYPE_SPRINT: s = "Sprint";
        case FCNPC_MOVE_TYPE_DRIVE:  s = "Drive";
        default:                     s = "Auto";
    }
    return s;
}
MoveModeLabel(v) {
    new s[12];
    switch (v) {
        case FCNPC_MOVE_MODE_NONE:       s = "None";
        case FCNPC_MOVE_MODE_MAPANDREAS: s = "MapAndreas";
        case FCNPC_MOVE_MODE_COLANDREAS: s = "ColAndreas";
        default:                         s = "Auto";
    }
    return s;
}
PathfindingLabel(v) {
    new s[12];
    switch (v) {
        case FCNPC_MOVE_PATHFINDING_NONE:    s = "None";
        case FCNPC_MOVE_PATHFINDING_Z:       s = "Z";
        case FCNPC_MOVE_PATHFINDING_RAYCAST: s = "Raycast";
        default:                             s = "Auto";
    }
    return s;
}
SpeedLabel(Float:v, out[], size) {
    if (v == FCNPC_MOVE_SPEED_AUTO) format(out, size, "Auto");
    else if (v == FCNPC_MOVE_SPEED_WALK) format(out, size, "Walk");
    else if (v == FCNPC_MOVE_SPEED_RUN) format(out, size, "Run");
    else if (v == FCNPC_MOVE_SPEED_SPRINT) format(out, size, "Sprint");
    else format(out, size, "%.3f", v);
}

// =========================================================================
//  Config dialog renderers
// =========================================================================
ShowCfgMove(playerid)
{
    new sp[16]; SpeedLabel(gCfg[cMoveSpeed], sp, sizeof(sp));
    new buf[512];
    format(buf, sizeof(buf),
        "Move type:\t%s\t[default: Auto]\n\
Move mode (grounding):\t%s\t[default: Auto]\n\
Pathfinding:\t%s\t[default: Auto]\n\
Move speed:\t%s\t[default: Auto]",
        MoveTypeLabel(gCfg[cMoveType]), MoveModeLabel(gCfg[cMoveMode]),
        PathfindingLabel(gCfg[cPathfinding]), sp);
    ShowPlayerDialog(playerid, DLG_CFG_MOVE, DIALOG_STYLE_TABLIST, "Config: Movement (select to cycle)",
        buf, "Cycle", "Back");
}

ShowCfgCombat(playerid)
{
    new buf[512];
    format(buf, sizeof(buf),
        "Weapon id:\t%d\t[default: 0]\n\
Weapon accuracy:\t%.2f\t[default: 1.00]\n\
Fighting style:\t%d\t[default: 4]\n\
Infinite ammo:\t%s\t[default: Off]\n\
Use reloading:\t%s\t[default: On]",
        gCfg[cWeapon], gCfg[cAccuracy], gCfg[cFightStyle],
        gCfg[cInfiniteAmmo] ? ("On") : ("Off"),
        gCfg[cReloading] ? ("On") : ("Off"));
    ShowPlayerDialog(playerid, DLG_CFG_COMBAT, DIALOG_STYLE_TABLIST, "Config: Combat & Weapons",
        buf, "Edit", "Back");
}

ShowCfgApp(playerid)
{
    new buf[512];
    format(buf, sizeof(buf),
        "Skin id:\t%d\t[default: 230]\n\
Health:\t%.0f\t[default: 100]\n\
Armour:\t%.0f\t[default: 0]\n\
Invulnerable:\t%s\t[default: Off]",
        gCfg[cSkin], gCfg[cHealth], gCfg[cArmour],
        gCfg[cInvuln] ? ("On") : ("Off"));
    ShowPlayerDialog(playerid, DLG_CFG_APP, DIALOG_STYLE_TABLIST, "Config: Appearance & Stats",
        buf, "Edit", "Back");
}

ShowCfgPlugin(playerid)
{
    new buf[640];
    format(buf, sizeof(buf),
        "Update rate (ms):\t%d\t[default: %d]\n\
Tick rate:\t%d\t[default: %d]\n\
Global ColAndreas grounding:\t%s\t[default: Off]\n\
Global raycast pathfinding:\t%s\t[default: Off]\n\
Crash log:\t%s\t[default: Off]",
        gCfg[cUpdateRate], gDefUpdateRate, gCfg[cTickRate], gDefTickRate,
        gCfg[cUseModeCA] ? ("On") : ("Off"),
        gCfg[cUsePathRaycast] ? ("On") : ("Off"),
        gCfg[cCrashLog] ? ("On") : ("Off"));
    ShowPlayerDialog(playerid, DLG_CFG_PLUGIN, DIALOG_STYLE_TABLIST, "Config: Plugin (global)",
        buf, "Edit", "Back");
}

ShowDebug(playerid)
{
    new buf[256];
    format(buf, sizeof(buf),
        "Toggle on-screen NPC HUD:\t%s\n\
Dump all valid NPCs to chat\n\
Teleport demo NPC to me\n\
Print plugin + include version",
        gDebug[playerid] ? ("On") : ("Off"));
    ShowPlayerDialog(playerid, DLG_DEBUG, DIALOG_STYLE_TABLIST, "Debug options",
        buf, "Select", "Back");
}

ShowInput(playerid, field, back, const title[], const body[])
{
    gInputField[playerid] = field;
    gInputBack[playerid]  = back;
    ShowPlayerDialog(playerid, DLG_INPUT, DIALOG_STYLE_INPUT, title, body, "Set", "Cancel");
}

ReShow(playerid, dlg)
{
    switch (dlg) {
        case DLG_CFG_MOVE:   ShowCfgMove(playerid);
        case DLG_CFG_COMBAT: ShowCfgCombat(playerid);
        case DLG_CFG_APP:    ShowCfgApp(playerid);
        case DLG_CFG_PLUGIN: ShowCfgPlugin(playerid);
        default:             ShowMainMenu(playerid);
    }
}

// =========================================================================
//  Dialog response
// =========================================================================
public OnDialogResponse(playerid, dialogid, response, listitem, inputtext[])
{
    switch (dialogid) {
        case DLG_MAIN: {
            if (!response) return 1;
            switch (listitem) {
                case 0: ShowOverview(playerid);
                case 1: ShowCommands(playerid);
                case 2: ShowPorted(playerid);
                case 3: ShowCfgMove(playerid);
                case 4: ShowCfgCombat(playerid);
                case 5: ShowCfgApp(playerid);
                case 6: ShowCfgPlugin(playerid);
                case 7: ShowDebug(playerid);
                case 8: ShowPlayerDialog(playerid, DLG_RESTORE, DIALOG_STYLE_MSGBOX,
                            "Restore defaults",
                            "Reset every tunable (movement, combat, appearance, plugin) to its default value?\n\nThis also re-applies plugin update/tick rate and global move modes.",
                            "Reset", "Back");
            }
            return 1;
        }

        case DLG_OVERVIEW: { ShowMainMenu(playerid); return 1; }
    }

    if (dialogid == DLG_CMDS) {
        if (!response) { ShowMainMenu(playerid); return 1; }
        HandleCommandItem(playerid, listitem);
        return 1;
    }

    if (dialogid == DLG_PORTED) {
        if (!response) { ShowMainMenu(playerid); return 1; }
        switch (listitem) {
            case 0: {
                new inpc = EnsureInpcDemo(playerid);
                EnsureDemo(playerid);
                // Floating 3D text labels above each NPC - visible regardless of
                // server nametag settings (unlike SetPlayerName).
                if (inpc != INVALID_PLAYER_ID && gInpcLabel[playerid] == Text3D:INVALID_3DTEXT_ID) {
                    gInpcLabel[playerid] = Create3DTextLabel("INPC", 0x33CCFFFF, 0.0, 0.0, 0.0, 40.0, 0, false);
                    Attach3DTextLabelToPlayer(gInpcLabel[playerid], inpc, 0.0, 0.0, 0.55);
                }
                if (HasDemo(playerid) && gDemoLabel[playerid] == Text3D:INVALID_3DTEXT_ID) {
                    gDemoLabel[playerid] = Create3DTextLabel("FCNPC", 0x66FF66FF, 0.0, 0.0, 0.0, 40.0, 0, false);
                    Attach3DTextLabelToPlayer(gDemoLabel[playerid], gDemoNPC[playerid], 0.0, 0.0, 0.55);
                }
                SendClientMessage(playerid, C_INFO, "Spawned both. Floating labels mark them: INPC (blue) and FCNPC (green).");
            }
            case 1: {
                if (!HasInpc(playerid)) { SendClientMessage(playerid, C_WARN, "Spawn the test NPCs first (option 1)."); }
                else {
                    NPC_MoveToPlayer(gInpcNPC[playerid], playerid); // autoRestart defaults to false
                    SendClientMessage(playerid, C_INFO,  "INTENDED: NPC keeps chasing you as you move around.");
                    SendClientMessage(playerid, C_WARN,  "INPC (native): autoRestart defaults OFF -> walks to your CURRENT spot, then STOPS. Walk away: it won't follow.");
                    SendClientMessage(playerid, 0xAAAAAAFF, "(now run option 2 for the FCNPC version)");
                }
            }
            case 2: {
                if (!HasDemo(playerid)) { SendClientMessage(playerid, C_WARN, "Spawn the test NPCs first (option 1)."); }
                else {
                    FCNPC_GoToPlayer(gDemoNPC[playerid], playerid, gCfg[cMoveType], gCfg[cMoveSpeed], gCfg[cMoveMode], gCfg[cPathfinding], 0.0, true, 1.0);
                    SendClientMessage(playerid, C_INFO, "INTENDED: NPC keeps chasing you as you move around.");
                    SendClientMessage(playerid, C_OK,   "FCNPC (port): forces autoRestart ON -> re-tracks you continuously. Walk away: it keeps coming. (INPC could not do this.)");
                }
            }
            case 3: {
                if (!HasInpc(playerid)) { SendClientMessage(playerid, C_WARN, "Spawn the test NPCs first (option 1)."); }
                else {
                    NPC_SetAngleToPlayer(gInpcNPC[playerid], playerid);
                    SendClientMessage(playerid, C_INFO, "INTENDED: NPC turns to face you, staying upright.");
                    SendClientMessage(playerid, C_WARN, "INPC (native): NPC_SetAngleToPlayer - watch orientation vs the FCNPC one (option 4).");
                }
            }
            case 4: {
                if (!HasDemo(playerid)) { SendClientMessage(playerid, C_WARN, "Spawn the test NPCs first (option 1)."); }
                else {
                    FCNPC_SetAngleToPlayer(gDemoNPC[playerid], playerid);
                    SendClientMessage(playerid, C_INFO, "INTENDED: NPC turns to face you, staying upright.");
                    SendClientMessage(playerid, C_OK,   "FCNPC (port): FCNPC_SetAngleToPlayer - compare with INPC (option 3). (setRotation-right-after-spawn flip is the open bug.)");
                }
            }
            case 5: {
                if (HasInpc(playerid)) { NPC_Destroy(gInpcNPC[playerid]); gInpcNPC[playerid] = INVALID_PLAYER_ID; }
                DestroyDemo(playerid);
                SendClientMessage(playerid, C_INFO, "Test NPCs cleaned up.");
            }
        }
        ShowPorted(playerid);
        return 1;
    }

    if (dialogid == DLG_DEBUG) {
        if (!response) { ShowMainMenu(playerid); return 1; }
        switch (listitem) {
            case 0: { // toggle HUD
                gDebug[playerid] = !gDebug[playerid];
                if (!gDebug[playerid] && gDbgInit[playerid]) PlayerTextDrawHide(playerid, gDbgTD[playerid]);
                ShowDebug(playerid);
            }
            case 1: DumpNPCs(playerid);
            case 2: {
                if (HasDemo(playerid)) {
                    new Float:x, Float:y, Float:z;
                    GetPointInFront(playerid, 2.0, x, y, z);
                    FCNPC_SetPosition(gDemoNPC[playerid], x, y, z);
                    SendClientMessage(playerid, C_OK, "Demo NPC teleported to you.");
                } else SendClientMessage(playerid, C_WARN, "No demo NPC. Spawn one from Commands first.");
                ShowDebug(playerid);
            }
            case 3: {
                new ver[48]; FCNPC_GetPluginVersion(ver, sizeof(ver));
                new msg[96];
                format(msg, sizeof(msg), "FCNPC plugin v%s | include v%d", ver, FCNPC_INCLUDE_VERSION);
                SendClientMessage(playerid, C_INFO, msg);
                ShowDebug(playerid);
            }
        }
        return 1;
    }

    if (dialogid == DLG_RESTORE) {
        if (response) {
            ResetConfigDefaults();
            ApplyPluginConfig();
            if (HasDemo(playerid)) ApplyConfig(gDemoNPC[playerid]);
            SendClientMessage(playerid, C_OK, "All FCNPC tunables restored to defaults.");
        }
        ShowMainMenu(playerid);
        return 1;
    }

    if (dialogid == DLG_CFG_MOVE) {
        if (!response) { ShowMainMenu(playerid); return 1; }
        switch (listitem) {
            case 0: gCfg[cMoveType]    = CycleMoveType(gCfg[cMoveType]);
            case 1: gCfg[cMoveMode]    = CycleMoveMode(gCfg[cMoveMode]);
            case 2: gCfg[cPathfinding] = CyclePathfinding(gCfg[cPathfinding]);
            case 3: gCfg[cMoveSpeed]   = CycleMoveSpeed(gCfg[cMoveSpeed]);
        }
        if (HasDemo(playerid)) ApplyConfig(gDemoNPC[playerid]);
        ShowCfgMove(playerid);
        return 1;
    }

    if (dialogid == DLG_CFG_COMBAT) {
        if (!response) { ShowMainMenu(playerid); return 1; }
        switch (listitem) {
            case 0: ShowInput(playerid, IN_WEAPON, DLG_CFG_COMBAT, "Weapon id", "Enter weapon id (0-46):");
            case 1: ShowInput(playerid, IN_ACCURACY, DLG_CFG_COMBAT, "Weapon accuracy", "Enter accuracy 0.0 - 1.0:");
            case 2: { gCfg[cFightStyle] = CycleFightStyle(gCfg[cFightStyle]); if (HasDemo(playerid)) ApplyConfig(gDemoNPC[playerid]); ShowCfgCombat(playerid); }
            case 3: { gCfg[cInfiniteAmmo] = !gCfg[cInfiniteAmmo]; if (HasDemo(playerid)) ApplyConfig(gDemoNPC[playerid]); ShowCfgCombat(playerid); }
            case 4: { gCfg[cReloading] = !gCfg[cReloading]; if (HasDemo(playerid)) ApplyConfig(gDemoNPC[playerid]); ShowCfgCombat(playerid); }
        }
        return 1;
    }

    if (dialogid == DLG_CFG_APP) {
        if (!response) { ShowMainMenu(playerid); return 1; }
        switch (listitem) {
            case 0: ShowInput(playerid, IN_SKIN, DLG_CFG_APP, "Skin id", "Enter skin id (0-311):");
            case 1: ShowInput(playerid, IN_HEALTH, DLG_CFG_APP, "Health", "Enter health (1 - 100000):");
            case 2: ShowInput(playerid, IN_ARMOUR, DLG_CFG_APP, "Armour", "Enter armour (0 - 100):");
            case 3: { gCfg[cInvuln] = !gCfg[cInvuln]; if (HasDemo(playerid)) ApplyConfig(gDemoNPC[playerid]); ShowCfgApp(playerid); }
        }
        return 1;
    }

    if (dialogid == DLG_CFG_PLUGIN) {
        if (!response) { ShowMainMenu(playerid); return 1; }
        switch (listitem) {
            case 0: ShowInput(playerid, IN_UPDATERATE, DLG_CFG_PLUGIN, "Update rate", "Enter update rate in ms (e.g. 100):");
            case 1: ShowInput(playerid, IN_TICKRATE, DLG_CFG_PLUGIN, "Tick rate", "Enter tick rate (e.g. 50):");
            case 2: { gCfg[cUseModeCA] = !gCfg[cUseModeCA]; FCNPC_UseMoveMode(FCNPC_MOVE_MODE_COLANDREAS, gCfg[cUseModeCA]); ShowCfgPlugin(playerid); }
            case 3: { gCfg[cUsePathRaycast] = !gCfg[cUsePathRaycast]; FCNPC_UseMovePathfinding(FCNPC_MOVE_PATHFINDING_RAYCAST, gCfg[cUsePathRaycast]); ShowCfgPlugin(playerid); }
            case 4: { gCfg[cCrashLog] = !gCfg[cCrashLog]; FCNPC_UseCrashLog(gCfg[cCrashLog]); ShowCfgPlugin(playerid); }
        }
        return 1;
    }

    if (dialogid == DLG_INPUT) {
        new back = gInputBack[playerid];
        if (!response) { ReShow(playerid, back); return 1; }
        new Float:fv = floatstr(inputtext);
        new iv = strval(inputtext);
        switch (gInputField[playerid]) {
            case IN_SKIN:       { if (iv < 0) iv = 0; if (iv > 311) iv = 311; gCfg[cSkin] = iv; }
            case IN_HEALTH:     { if (fv < 1.0) fv = 1.0; gCfg[cHealth] = fv; }
            case IN_ARMOUR:     { if (fv < 0.0) fv = 0.0; if (fv > 100.0) fv = 100.0; gCfg[cArmour] = fv; }
            case IN_WEAPON:     { if (iv < 0) iv = 0; if (iv > 46) iv = 46; gCfg[cWeapon] = iv; }
            case IN_ACCURACY:   { if (fv < 0.0) fv = 0.0; if (fv > 1.0) fv = 1.0; gCfg[cAccuracy] = fv; }
            case IN_UPDATERATE: { if (iv < 1) iv = 1; gCfg[cUpdateRate] = iv; FCNPC_SetUpdateRate(iv); }
            case IN_TICKRATE:   { if (iv < 1) iv = 1; gCfg[cTickRate] = iv; FCNPC_SetTickRate(iv); }
        }
        if (HasDemo(playerid)) ApplyConfig(gDemoNPC[playerid]);
        ReShow(playerid, back);
        return 1;
    }

    return 0;
}

// =========================================================================
//  Overview
// =========================================================================
ShowOverview(playerid)
{
    new ver[48]; FCNPC_GetPluginVersion(ver, sizeof(ver));
    new buf[1024];
    format(buf, sizeof(buf),
        "FCNPC for open.mp - plugin v%s, include v%d\n\n\
This harness drives ONE demo NPC in front of you and lets you exercise the\n\
FCNPC API by category:\n\
 - Lifecycle: create / spawn / respawn / kill / destroy\n\
 - Movement: GoTo, GoToPlayer (chase), move paths, road nodes, stop\n\
 - Combat: aim, shoot, melee, weapons, accuracy, fighting style\n\
 - Animations and special actions\n\
 - Vehicles: enter / exit\n\
 - Tunable config (movement / combat / appearance / plugin) with defaults\n\
 - Debug HUD + info dumps\n\n\
Open Commands to spawn the demo NPC, then tune it from the Config screens.",
        ver, FCNPC_INCLUDE_VERSION);
    ShowPlayerDialog(playerid, DLG_OVERVIEW, DIALOG_STYLE_MSGBOX, "FCNPC - Overview", buf, "Back", "");
}

// =========================================================================
//  Commands (live demos)
// =========================================================================
ShowCommands(playerid)
{
    ShowPlayerDialog(playerid, DLG_CMDS, DIALOG_STYLE_LIST, "FCNPC - Commands (live demos)",
        "Spawn demo NPC (in front of me)\n\
Face me (test rotation)\n\
Walk to me\n\
Run to me\n\
Sprint to me\n\
Follow me (chase)\n\
Aim at me\n\
Aim + shoot at me\n\
Melee attack me\n\
Play animation (wave)\n\
Enter nearest vehicle\n\
Exit vehicle\n\
Patrol square (move path)\n\
Play nearest road node\n\
Toggle invulnerable\n\
Kill demo NPC\n\
Respawn demo NPC\n\
Stop all actions\n\
Destroy demo NPC",
        "Run", "Back");
}

HandleCommandItem(playerid, item)
{
    // item 0 = spawn; everything else needs a live demo
    if (item != 0 && !HasDemo(playerid)) {
        SendClientMessage(playerid, C_WARN, "No demo NPC yet - choose 'Spawn demo NPC' first.");
        ShowCommands(playerid);
        return;
    }
    new npc = gDemoNPC[playerid];
    new Float:x, Float:y, Float:z;

    switch (item) {
        case 0: {
            EnsureDemo(playerid);
            if (HasDemo(playerid)) SendClientMessage(playerid, C_OK, "Demo NPC spawned in front of you (upright).");
        }
        case 1: {
            FCNPC_SetAngleToPlayer(npc, playerid);
            SendClientMessage(playerid, C_WARN, "Called FCNPC_SetAngleToPlayer - watch if the ped flips (rotation test).");
        }
        case 2: { GetPlayerPos(playerid, x, y, z); FCNPC_GoTo(npc, x, y, z, FCNPC_MOVE_TYPE_WALK,   gCfg[cMoveSpeed], gCfg[cMoveMode], gCfg[cPathfinding], 0.0, true, 1.0); }
        case 3: { GetPlayerPos(playerid, x, y, z); FCNPC_GoTo(npc, x, y, z, FCNPC_MOVE_TYPE_RUN,    gCfg[cMoveSpeed], gCfg[cMoveMode], gCfg[cPathfinding], 0.0, true, 1.0); }
        case 4: { GetPlayerPos(playerid, x, y, z); FCNPC_GoTo(npc, x, y, z, FCNPC_MOVE_TYPE_SPRINT, gCfg[cMoveSpeed], gCfg[cMoveMode], gCfg[cPathfinding], 0.0, true, 1.0); }
        case 5: { FCNPC_GoToPlayer(npc, playerid, gCfg[cMoveType], gCfg[cMoveSpeed], gCfg[cMoveMode], gCfg[cPathfinding], 0.0, true, 1.0); SendClientMessage(playerid, C_OK, "NPC chasing you (move around)."); }
        case 6: { FCNPC_AimAtPlayer(npc, playerid); }
        case 7: {
            if (gCfg[cWeapon] == 0) { gCfg[cWeapon] = 24; FCNPC_SetWeapon(npc, 24); FCNPC_SetAmmo(npc, 999); }
            FCNPC_UseInfiniteAmmo(npc, true);
            FCNPC_AimAtPlayer(npc, playerid, true);
            SendClientMessage(playerid, C_WARN, "NPC shooting at you.");
        }
        case 8: { FCNPC_MeleeAttack(npc); }
        case 9: { FCNPC_ApplyAnimation(npc, "PED", "WAVE_loop", 4.1, 1, 0, 0, 0, 0); }
        case 10: {
            new v = GetNearestVehicle(playerid);
            if (v == INVALID_VEHICLE_ID) SendClientMessage(playerid, C_WARN, "No vehicle within 30m.");
            else { FCNPC_EnterVehicle(npc, v, 0); SendClientMessage(playerid, C_OK, "NPC entering nearest vehicle."); }
        }
        case 11: { FCNPC_ExitVehicle(npc); }
        case 12: {
            if (gDemoPath[playerid] != FCNPC_INVALID_MOVEPATH_ID && FCNPC_IsValidMovePath(gDemoPath[playerid]))
                FCNPC_DestroyMovePath(gDemoPath[playerid]);
            new p = FCNPC_CreateMovePath();
            gDemoPath[playerid] = p;
            GetPlayerPos(playerid, x, y, z);
            FCNPC_AddPointToMovePath(p, x + 8.0, y + 8.0, z);
            FCNPC_AddPointToMovePath(p, x + 8.0, y - 8.0, z);
            FCNPC_AddPointToMovePath(p, x - 8.0, y - 8.0, z);
            FCNPC_AddPointToMovePath(p, x - 8.0, y + 8.0, z);
            FCNPC_GoByMovePath(npc, p, 0, FCNPC_MOVE_TYPE_RUN, gCfg[cMoveSpeed], gCfg[cMoveMode], gCfg[cPathfinding]);
            SendClientMessage(playerid, C_OK, "NPC patrolling a square around you.");
        }
        case 13: {
            FCNPC_OpenNode(0);
            FCNPC_PlayNode(npc, 0, FCNPC_MOVE_TYPE_RUN);
            SendClientMessage(playerid, C_WARN, "Playing road node 0 (roams the road graph - feature demo).");
        }
        case 14: {
            gCfg[cInvuln] = !gCfg[cInvuln];
            FCNPC_SetInvulnerable(npc, gCfg[cInvuln]);
            SendClientMessage(playerid, C_INFO, gCfg[cInvuln] ? ("NPC invulnerable: ON") : ("NPC invulnerable: OFF"));
        }
        case 15: { FCNPC_Kill(npc); }
        case 16: { FCNPC_Respawn(npc); ApplyConfig(npc); }
        case 17: {
            // Stop ALL actions: movement, attacking, aiming, animations.
            FCNPC_Stop(npc);
            FCNPC_StopAttack(npc);
            FCNPC_StopAim(npc);
            FCNPC_ClearAnimations(npc);
            SendClientMessage(playerid, C_INFO, "Stopped all actions (move + attack + aim + anim).");
        }
        case 18: { DestroyDemo(playerid); SendClientMessage(playerid, C_INFO, "Demo NPC destroyed."); }
    }
    ShowCommands(playerid);
}

// =========================================================================
//  Ported features: INPC (native open.mp) vs FCNPC v2.x
//  Shows what the bare INPC engine does vs what the port fixes.
// =========================================================================
EnsureInpcDemo(playerid)
{
    if (gInpcNPC[playerid] != INVALID_PLAYER_ID) return gInpcNPC[playerid];
    new name[24];
    format(name, sizeof(name), "INPC_Test_%d", playerid);
    new id = NPC_Create(name);
    if (id == INVALID_PLAYER_ID) {
        SendClientMessage(playerid, C_ERR, "NPC_Create (native INPC) failed - raise max_bots.");
        return INVALID_PLAYER_ID;
    }
    gInpcNPC[playerid] = id;
    new Float:x, Float:y, Float:z;
    GetPointInFront(playerid, 5.0, x, y, z);
    NPC_SetSkin(id, 287);
    NPC_SetPos(id, x, y, z);
    NPC_Spawn(id);
    return id;
}

ShowPorted(playerid)
{
    ShowPlayerDialog(playerid, DLG_PORTED, DIALOG_STYLE_LIST, "Ported features: INPC vs FCNPC v2.x",
        "Spawn both test NPCs (INPC + FCNPC) in front of me\n\
Chase: INPC native (autoRestart off - stops at your start spot)\n\
Chase: FCNPC (forced autoRestart - keeps chasing)\n\
Rotation: INPC NPC_SetAngleToPlayer\n\
Rotation: FCNPC_SetAngleToPlayer\n\
Clean up test NPCs",
        "Run", "Back");
}

bool:HasInpc(playerid)
{
    return (gInpcNPC[playerid] != INVALID_PLAYER_ID);
}

// =========================================================================
//  Cyclers
// =========================================================================
CycleMoveType(v) {
    switch (v) {
        case FCNPC_MOVE_TYPE_AUTO:   return FCNPC_MOVE_TYPE_WALK;
        case FCNPC_MOVE_TYPE_WALK:   return FCNPC_MOVE_TYPE_RUN;
        case FCNPC_MOVE_TYPE_RUN:    return FCNPC_MOVE_TYPE_SPRINT;
        default:                     return FCNPC_MOVE_TYPE_AUTO;
    }
}
CycleMoveMode(v) {
    switch (v) {
        case FCNPC_MOVE_MODE_AUTO:       return FCNPC_MOVE_MODE_NONE;
        case FCNPC_MOVE_MODE_NONE:       return FCNPC_MOVE_MODE_MAPANDREAS;
        case FCNPC_MOVE_MODE_MAPANDREAS: return FCNPC_MOVE_MODE_COLANDREAS;
        default:                         return FCNPC_MOVE_MODE_AUTO;
    }
}
CyclePathfinding(v) {
    switch (v) {
        case FCNPC_MOVE_PATHFINDING_AUTO: return FCNPC_MOVE_PATHFINDING_NONE;
        case FCNPC_MOVE_PATHFINDING_NONE: return FCNPC_MOVE_PATHFINDING_Z;
        case FCNPC_MOVE_PATHFINDING_Z:    return FCNPC_MOVE_PATHFINDING_RAYCAST;
        default:                          return FCNPC_MOVE_PATHFINDING_AUTO;
    }
}
Float:CycleMoveSpeed(Float:v) {
    if (v == FCNPC_MOVE_SPEED_AUTO)   return FCNPC_MOVE_SPEED_WALK;
    if (v == FCNPC_MOVE_SPEED_WALK)   return FCNPC_MOVE_SPEED_RUN;
    if (v == FCNPC_MOVE_SPEED_RUN)    return FCNPC_MOVE_SPEED_SPRINT;
    return FCNPC_MOVE_SPEED_AUTO;
}
CycleFightStyle(v) {
    // 4=normal,5=boxing,6=kungfu,7=kneehead,15=grabkick,16=elbow
    switch (v) {
        case 4:  return 5;
        case 5:  return 6;
        case 6:  return 7;
        case 7:  return 15;
        case 15: return 16;
        default: return 4;
    }
}

// =========================================================================
//  Debug
// =========================================================================
DumpNPCs(playerid)
{
    new ids[64];
    new count = FCNPC_GetValidArray(ids, sizeof(ids));
    new msg[128];
    format(msg, sizeof(msg), "Valid FCNPC NPCs: %d", count);
    SendClientMessage(playerid, C_INFO, msg);
    for (new i = 0; i < count && i < sizeof(ids); i++) {
        new npc = ids[i];
        if (!FCNPC_IsValid(npc)) continue;
        new Float:x, Float:y, Float:z;
        FCNPC_GetPosition(npc, x, y, z);
        format(msg, sizeof(msg), " NPC %d: hp=%.0f wpn=%d moving=%s pos=%.1f,%.1f,%.1f",
            npc, FCNPC_GetHealth(npc), FCNPC_GetWeapon(npc),
            FCNPC_IsMoving(npc) ? ("yes") : ("no"), x, y, z);
        SendClientMessage(playerid, C_INFO, msg);
    }
    ShowDebug(playerid);
}

forward FCNPC_HudTick();
public FCNPC_HudTick()
{
    for (new i = 0; i < MAX_PLAYERS; i++) {
        if (!gDebug[i] || !IsPlayerConnected(i)) continue;
        if (!gDbgInit[i]) {
            gDbgTD[i] = CreatePlayerTextDraw(i, 500.0, 300.0, "_");
            PlayerTextDrawFont(i, gDbgTD[i], TEXT_DRAW_FONT_1);
            PlayerTextDrawLetterSize(i, gDbgTD[i], 0.22, 1.1);
            PlayerTextDrawColour(i, gDbgTD[i], 0xFFFFFFFF);
            PlayerTextDrawSetOutline(i, gDbgTD[i], 1);
            PlayerTextDrawSetShadow(i, gDbgTD[i], 0);
            PlayerTextDrawUseBox(i, gDbgTD[i], true);
            PlayerTextDrawBoxColour(i, gDbgTD[i], 0x00000099);
            PlayerTextDrawTextSize(i, gDbgTD[i], 632.0, 0.0);
            gDbgInit[i] = true;
        }

        new buf[256];
        if (!HasDemo(i)) {
            format(buf, sizeof(buf), "~y~FCNPC debug~n~~w~no demo NPC (Commands > Spawn)");
        } else {
            new npc = gDemoNPC[i];
            new Float:x, Float:y, Float:z, Float:px, Float:py, Float:pz;
            FCNPC_GetPosition(npc, x, y, z);
            GetPlayerPos(i, px, py, pz);
            new Float:dist = VectorSize(px - x, py - y, pz - z);
            format(buf, sizeof(buf),
                "~y~FCNPC debug  ~w~npc %d~n~\
hp %.0f  arm %.0f  wpn %d~n~\
moving %s  spd %.2f  veh %d~n~\
dist %.1fm  streamed %s",
                npc, FCNPC_GetHealth(npc), FCNPC_GetArmour(npc), FCNPC_GetWeapon(npc),
                FCNPC_IsMoving(npc) ? ("Y") : ("N"), FCNPC_GetSpeed(npc), FCNPC_GetVehicleID(npc),
                dist, FCNPC_IsStreamedIn(npc, i) ? ("Y") : ("N"));
        }
        PlayerTextDrawSetString(i, gDbgTD[i], buf);
        PlayerTextDrawShow(i, gDbgTD[i]);
    }
}
