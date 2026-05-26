# FCNPC for open.mp - port status

Per-native status of every `FCNPC_*` function in `FCNPC.inc`.

The point of this table: open.mp already ships an NPC engine (`INPCComponent` /
`INPC`) that covers most of FCNPC. This column shows **what open.mp already had**
vs **what this release contributes**:

- **open.mp INPC** - the open.mp method this maps to, or `-` if open.mp has no
  equivalent.
- **This release** - what we do:
  - *routes* = thin wrapper over the open.mp method (open.mp already did the work).
  - *adds* = open.mp did NOT have it; implemented here (the gap this port fills).
  - *pending* = declared but not yet implemented.

Totals: **182** natives. open.mp INPC already covered **~140**; **this release
adds ~40** it lacked, and routes the rest. **2 pending**.

---

## Fixes over open.mp INPC

Where open.mp's NPC engine behaves wrong or incompletely for the classic FCNPC
contract, this release does **not** just route to it - it overrides with its own
behaviour:

| Native | open.mp INPC issue | What this release does |
|---|---|---|
| `FCNPC_GoToPlayer` | `moveToPlayer` defaults `autoRestart=false` - the NPC walks to the player's *initial* spot then stops, so it never keeps chasing | Forces `autoRestart=true` and forwards the recheck delay (`stop_delay`), so the NPC continuously re-tracks the moving player |
| `FCNPC_SetMoveMode` (`MAPANDREAS`/`COLANDREAS`) | `INPC::move` does no terrain grounding - NPCs keep their spawn Z and float over lower ground / sit on roofs | Component samples ground Z each tick (MapAndreas/ColAndreas via the bridge) while the NPC moves and corrects its Z |

---

## Plugin / config

| Native | open.mp INPC | This release |
|---|---|---|
| `FCNPC_GetPluginVersion` | - | adds (component version) |
| `FCNPC_SetUpdateRate` / `GetUpdateRate` | - | adds (stored; open.mp drives sync) |
| `FCNPC_SetTickRate` / `GetTickRate` | - | adds (stored) |
| `FCNPC_UseMoveMode` / `IsMoveModeUsed` | - | adds (stored set) |
| `FCNPC_UseMovePathfinding` / `IsMovePathfindingUsed` | - | adds (stored set) |
| `FCNPC_UseCrashLog` / `IsCrashLogUsed` | - | adds (stored flag) |

## Lifecycle

| Native | open.mp INPC | This release |
|---|---|---|
| `FCNPC_Create` | `INPCComponent::create` | routes |
| `FCNPC_Destroy` | `INPCComponent::destroy` | routes |
| `FCNPC_Spawn` | `setSkin`+`setPosition`+`spawn` | routes |
| `FCNPC_Respawn` | `INPC::respawn` | routes |
| `FCNPC_IsSpawned` | player state + `isDead` | routes |
| `FCNPC_Kill` | `INPC::kill` | routes |
| `FCNPC_IsDead` | `INPC::isDead` | routes |
| `FCNPC_IsValid` | pool lookup | routes |
| `FCNPC_IsStreamedIn` | `isStreamedInForPlayer` | routes |
| `FCNPC_IsStreamedInForAnyone` | `streamedForPlayers` | routes |
| `FCNPC_GetValidArray` | - | adds (pool enumeration, classic AMX) |

## Position / orientation

| Native | open.mp INPC | This release |
|---|---|---|
| `FCNPC_SetPosition` / `GetPosition` | `set/getPosition` | routes |
| `FCNPC_GivePosition` | - | adds (get + delta + set) |
| `FCNPC_SetAngle` / `GetAngle` | `set/getRotation` | routes |
| `FCNPC_GiveAngle` | - | adds (current + delta) |
| `FCNPC_SetAngleToPos` / `SetAngleToPlayer` | - | adds (atan2) |
| `FCNPC_SetQuaternion` / `GetQuaternion` | `set/getRotation` | routes |
| `FCNPC_GiveQuaternion` | - | adds (component-wise add) |
| `FCNPC_SetVelocity` / `GetVelocity` | `set/getVelocity` | routes |
| `FCNPC_GiveVelocity` | - | adds (get + delta + set) |
| `FCNPC_SetSpeed` / `GetSpeed` | - | adds (scale / length of velocity) |
| `FCNPC_SetInterior` / `GetInterior` | `set/getInterior` | routes |
| `FCNPC_SetVirtualWorld` / `GetVirtualWorld` | `set/getVirtualWorld` | routes |

## Health / armour / invulnerability / skin

| Native | open.mp INPC | This release |
|---|---|---|
| `FCNPC_SetHealth` / `GetHealth` | `set/getHealth` | routes |
| `FCNPC_GiveHealth` | - | adds (get + delta + set) |
| `FCNPC_SetArmour` / `GetArmour` | `set/getArmour` | routes |
| `FCNPC_GiveArmour` | - | adds (get + delta + set) |
| `FCNPC_SetInvulnerable` / `IsInvulnerable` | `set/isInvulnerable` | routes |
| `FCNPC_SetSkin` | `setSkin` | routes |
| `FCNPC_GetSkin` / `GetCustomSkin` | `IPlayer::getSkin` | routes |

## Weapons

| Native | open.mp INPC | This release |
|---|---|---|
| `FCNPC_SetWeapon` / `GetWeapon` | `set/getWeapon` | routes |
| `FCNPC_SetAmmo` / `GetAmmo` | `set/getAmmo` | routes |
| `FCNPC_GiveAmmo` | - | adds (get + delta + set) |
| `FCNPC_SetAmmoInClip` / `GetAmmoInClip` | `set/getAmmoInClip` | routes |
| `FCNPC_GiveAmmoInClip` | - | adds (get + delta + set) |
| `FCNPC_SetWeaponSkillLevel` / `GetWeaponSkillLevel` | `set/getWeaponSkillLevel` | routes |
| `FCNPC_GiveWeaponSkillLevel` | - | adds (get + delta + set) |
| `FCNPC_SetWeaponState` / `GetWeaponState` | `set/getWeaponState` | routes |
| `FCNPC_SetWeaponReloadTime` / `GetWeaponReloadTime` | `set/getWeaponReloadTime` | routes |
| `FCNPC_GetWeaponActualReloadTime` | `getWeaponActualReloadTime` | routes |
| `FCNPC_SetWeaponShootTime` / `GetWeaponShootTime` | `set/getWeaponShootTime` | routes |
| `FCNPC_SetWeaponClipSize` / `GetWeaponClipSize` | `set/getWeaponClipSize` | routes |
| `FCNPC_GetWeaponActualClipSize` | `getWeaponActualClipSize` | routes |
| `FCNPC_SetWeaponAccuracy` / `GetWeaponAccuracy` | `set/getWeaponAccuracy` | routes |
| `FCNPC_SetWeaponInfo` / `GetWeaponInfo` | composed from the above | routes (compose) |
| `FCNPC_SetWeaponDefaultInfo` / `GetWeaponDefaultInfo` | - | adds (global table, seeded from `WeaponInfo::get`) |

## Keys / special action / animations / fighting style / reloading

| Native | open.mp INPC | This release |
|---|---|---|
| `FCNPC_SetKeys` / `GetKeys` | `set/getKeys` | routes |
| `FCNPC_SetSpecialAction` / `GetSpecialAction` | `set/getSpecialAction` | routes |
| `FCNPC_SetAnimation` | `setAnimation` | routes |
| `FCNPC_SetAnimationByName` | - | adds (name lookup over `AnimationNames[]`) |
| `FCNPC_ResetAnimation` | `resetAnimation` | routes |
| `FCNPC_GetAnimation` | `getAnimation` | routes |
| `FCNPC_ApplyAnimation` | `applyAnimation` | routes |
| `FCNPC_ClearAnimations` | `clearAnimations` | routes |
| `FCNPC_SetFightingStyle` / `GetFightingStyle` | `set/getFightingStyle` | routes |
| `FCNPC_UseReloading` / `IsReloadingUsed` | `enableReloading`/`isReloadEnabled` | routes |
| `FCNPC_UseInfiniteAmmo` / `IsInfiniteAmmoUsed` | `enableInfiniteAmmo`/`isInfiniteAmmoEnabled` | routes |

## Movement

| Native | open.mp INPC | This release |
|---|---|---|
| `FCNPC_GoTo` | `move` | routes |
| `FCNPC_GoToPlayer` | `moveToPlayer` | routes + fix (forces `autoRestart=true`; see Fixes below) |
| `FCNPC_Stop` | `stopMove` | routes |
| `FCNPC_IsMoving` | `isMoving` | routes |
| `FCNPC_IsMovingAtPlayer` | `isMovingToPlayer` | routes |
| `FCNPC_GetDestinationPoint` | `getPositionMovingTo` | routes |

## Aiming / attacking / shooting

| Native | open.mp INPC | This release |
|---|---|---|
| `FCNPC_AimAt` | `aimAt` | routes |
| `FCNPC_AimAtPlayer` | `aimAtPlayer` | routes |
| `FCNPC_StopAim` | `stopAim` | routes |
| `FCNPC_MeleeAttack` | `meleeAttack` | routes |
| `FCNPC_StopAttack` | `stopMeleeAttack` | routes |
| `FCNPC_IsAttacking` | `isMeleeAttacking` | routes |
| `FCNPC_IsAiming` | `isAiming` | routes |
| `FCNPC_IsAimingAtPlayer` | `isAimingAtPlayer` | routes |
| `FCNPC_GetAimingPlayer` | `getPlayerAimingAt` | routes |
| `FCNPC_IsShooting` | `isShooting` | routes |
| `FCNPC_IsReloading` | `isReloading` | routes |
| `FCNPC_TriggerWeaponShot` | `shoot` | routes |
| `FCNPC_GetClosestEntityInBetween` | - | adds (C++ entity scan; `MAP` flag via ColAndreas) |

## Vehicles

| Native | open.mp INPC | This release |
|---|---|---|
| `FCNPC_EnterVehicle` | `enterVehicle` | routes |
| `FCNPC_ExitVehicle` | `exitVehicle` | routes |
| `FCNPC_PutInVehicle` | `putInVehicle` | routes |
| `FCNPC_RemoveFromVehicle` | `removeFromVehicle` | routes |
| `FCNPC_GetVehicleID` | `getVehicle()->getID()` | routes |
| `FCNPC_GetVehicleSeat` | `getVehicleSeat` | routes |
| `FCNPC_UseVehicleSiren` / `IsVehicleSirenUsed` | `useVehicleSiren`/`isVehicleSirenUsed` | routes |
| `FCNPC_SetVehicleHealth` / `GetVehicleHealth` | `set/getVehicleHealth` | routes |
| `FCNPC_SetVehicleHydraThrusters` / `GetVehicleHydraThrusters` | `set/getVehicleHydraThrusters` | routes |
| `FCNPC_SetVehicleGearState` / `GetVehicleGearState` | `set/getVehicleGearState` | routes |
| `FCNPC_SetVehicleTrainSpeed` / `GetVehicleTrainSpeed` | `set/getVehicleTrainSpeed` | routes |

## Surfing

| Native | open.mp INPC | This release |
|---|---|---|
| `FCNPC_SetSurfingOffsets` / `GetSurfingOffsets` | `set/getSurfingData` offset | routes |
| `FCNPC_GiveSurfingOffsets` | - | adds (offset + delta) |
| `FCNPC_SetSurfingVehicle` / `GetSurfingVehicle` | `set/getSurfingData` (Vehicle) | routes |
| `FCNPC_SetSurfingObject` / `GetSurfingObject` | `set/getSurfingData` (Object) | routes |
| `FCNPC_SetSurfingPlayerObject` / `GetSurfingPlayerObject` | `set/getSurfingData` (PlayerObject) | routes |
| `FCNPC_SetSurfingDynamicObject` / `GetSurfingDynamicObject` | - | adds (resolves streamer internal id; needs Streamer plugin) |
| `FCNPC_StopSurfing` | `resetSurfingData` | routes |

## Playback

| Native | open.mp INPC | This release |
|---|---|---|
| `FCNPC_StartPlayingPlayback` | `startPlayback` | routes |
| `FCNPC_StopPlayingPlayback` | `stopPlayback` | routes |
| `FCNPC_PausePlayingPlayback` | `pausePlayback(true)` | routes |
| `FCNPC_ResumePlayingPlayback` | `pausePlayback(false)` | routes |
| `FCNPC_LoadPlayingPlayback` | `INPCComponent::loadRecord` | routes |
| `FCNPC_UnloadPlayingPlayback` | `INPCComponent::unloadRecord` | routes |
| `FCNPC_SetPlayingPlaybackPath` / `GetPlayingPlaybackPath` | - | adds (per-NPC base dir) |

## Nodes

| Native | open.mp INPC | This release |
|---|---|---|
| `FCNPC_OpenNode` / `CloseNode` / `IsNodeOpen` | `INPCComponent::openNode`/`closeNode`/`isNodeOpen` | routes |
| `FCNPC_GetNodeType` / `GetNodePointPosition` / `GetNodePointCount` / `GetNodeInfo` | matching `INPCComponent` getters | routes |
| `FCNPC_SetNodePoint` | `setNodePoint` | routes |
| `FCNPC_PlayNode` | `INPC::playNode` | routes |
| `FCNPC_StopPlayingNode` / `PausePlayingNode` / `ResumePlayingNode` | matching `INPC` methods | routes |
| `FCNPC_IsPlayingNode` / `IsPlayingNodePaused` | `isPlayingNode`/`isPlayingNodePaused` | routes |

## Move paths

| Native | open.mp INPC | This release |
|---|---|---|
| `FCNPC_CreateMovePath` / `DestroyMovePath` / `IsValidMovePath` | `INPCComponent::createPath`/`destroyPath`/`isValidPath` | routes |
| `FCNPC_AddPointToMovePath` | `addPointToPath` | routes |
| `FCNPC_AddPointsToMovePath` / `AddPointsToMovePath2` | - | adds (bulk loop, classic AMX) |
| `FCNPC_RemovePointFromMovePath` | `removePointFromPath` | routes |
| `FCNPC_IsValidMovePathPoint` | index vs `getPathPointCount` | routes |
| `FCNPC_GetMovePathPoint` | `getPathPoint` | routes |
| `FCNPC_GetNumberMovePathPoint` | `getPathPointCount` | routes |
| `FCNPC_GoByMovePath` | `moveByPath` | routes |

## Per-NPC move mode / min-height / tab list

| Native | open.mp INPC | This release |
|---|---|---|
| `FCNPC_SetMoveMode` / `GetMoveMode` | - | adds (stored; samples ground Z via MapAndreas/ColAndreas each tick while moving) |
| `FCNPC_SetMinHeightPosCall` / `GetMinHeightPosCall` | - | adds (stored per-NPC) |
| `FCNPC_ShowInTabListForPlayer` / `HideInTabListForPlayer` | - | pending (no open.mp scoreboard API; needs a player-list RPC) |

---

## Not declared (separate plugins, not NPC functions)

| Native(s) | Reason |
|---|---|
| `MapAndreas_*` | Separate plugin - see [MapAndreas](https://github.com/philip1337/samp-plugin-mapandreas). |
| `CA_*` (ColAndreas) | Separate plugin - see [ColAndreas](https://github.com/Pottus/ColAndreas). |
