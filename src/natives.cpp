/* =========================================
 *
 *  FCNPC for open.mp - native API (FCNPC_*)
 *
 *  Full port. Direct natives call INPC / INPCComponent / IPlayer; natives with
 *  no open.mp equivalent are hand-rolled (see the gap table in README.md).
 *  Array-parameter natives live in natives_array.cpp (classic AMX registration).
 *
 * ========================================= */

#include <Server/Components/Pawn/Impl/pawn_natives.hpp>
#include "FCNPC.hpp"
#include "bridge.hpp"

#include <cmath>
#include <cctype>

#define PLUGIN_VERSION "2.1.0"

namespace
{
	inline INPC* GetNPC(int npcid) { return FCNPCComponent::getInstance()->getNPC(npcid); }

	// Portable case-insensitive compare (MSVC has _strnicmp, POSIX has strncasecmp).
	inline bool IEqual(StringView a, StringView b)
	{
		if (a.length() != b.length())
		{
			return false;
		}
		for (size_t i = 0; i < a.length(); ++i)
		{
			if (std::tolower(static_cast<unsigned char>(a[i])) != std::tolower(static_cast<unsigned char>(b[i])))
			{
				return false;
			}
		}
		return true;
	}

	// Find an open.mp animation index whose name part matches `name`.
	int FindAnimationId(StringView name)
	{
		for (int i = 1; i < GLM_COUNTOF(AnimationNames); ++i)
		{
			if (IEqual(splitAnimationNames(i).second, name))
			{
				return i;
			}
		}
		return -1;
	}
}

// Macro: fetch the NPC or bail with `ret`.
#define NPC_OR(ret)            \
	INPC* npc = GetNPC(npcid);  \
	if (!npc)                   \
		return ret;

// ===========================================================================
//  Plugin / global config (no INPC equivalent -> stored in component)
// ===========================================================================

SCRIPT_API(FCNPC_GetPluginVersion, bool(OutputOnlyString& version))
{
	version = StringView(PLUGIN_VERSION);
	return true;
}

SCRIPT_API(FCNPC_SetUpdateRate, bool(int rate)) { FCNPCComponent::getInstance()->updateRate() = rate; return true; }
SCRIPT_API(FCNPC_GetUpdateRate, int()) { return FCNPCComponent::getInstance()->updateRate(); }
SCRIPT_API(FCNPC_SetTickRate, bool(int rate)) { FCNPCComponent::getInstance()->tickRate() = rate; return true; }
SCRIPT_API(FCNPC_GetTickRate, int()) { return FCNPCComponent::getInstance()->tickRate(); }

SCRIPT_API(FCNPC_UseMoveMode, bool(int mode, bool use))
{
	auto& s = FCNPCComponent::getInstance()->usedMoveModes();
	if (use) s.insert(mode); else s.erase(mode);
	return true;
}
SCRIPT_API(FCNPC_IsMoveModeUsed, bool(int mode)) { return FCNPCComponent::getInstance()->usedMoveModes().count(mode) != 0; }

SCRIPT_API(FCNPC_UseMovePathfinding, bool(int pathfinding, bool use))
{
	auto& s = FCNPCComponent::getInstance()->usedPathfindings();
	if (use) s.insert(pathfinding); else s.erase(pathfinding);
	return true;
}
SCRIPT_API(FCNPC_IsMovePathfindingUsed, bool(int pathfinding)) { return FCNPCComponent::getInstance()->usedPathfindings().count(pathfinding) != 0; }

SCRIPT_API(FCNPC_UseCrashLog, bool(bool use)) { FCNPCComponent::getInstance()->crashLog() = use; return true; }
SCRIPT_API(FCNPC_IsCrashLogUsed, bool()) { return FCNPCComponent::getInstance()->crashLog(); }

// ===========================================================================
//  Lifecycle
// ===========================================================================

SCRIPT_API(FCNPC_Create, int(std::string const& name))
{
	auto npcs = FCNPCComponent::getInstance()->npcs();
	if (!npcs) return INVALID_PLAYER_ID;
	INPC* npc = npcs->create(StringView(name));
	return npc ? npc->getID() : INVALID_PLAYER_ID;
}

SCRIPT_API(FCNPC_Destroy, bool(int npcid))
{
	auto npcs = FCNPCComponent::getInstance()->npcs();
	INPC* npc = npcs ? npcs->get(npcid) : nullptr;
	if (!npc) return false;
	npcs->destroy(*npc);
	return true;
}

SCRIPT_API(FCNPC_Spawn, bool(int npcid, int skinid, float x, float y, float z))
{
	NPC_OR(false);
	npc->setSkin(skinid);
	npc->setPosition(Vector3(x, y, z), true);
	npc->spawn();
	return true;
}

SCRIPT_API(FCNPC_Respawn, bool(int npcid)) { NPC_OR(false); npc->respawn(); return true; }
SCRIPT_API(FCNPC_IsSpawned, bool(int npcid))
{
	NPC_OR(false);
	IPlayer* p = npc->getPlayer();
	return p && p->getState() != PlayerState_None && !npc->isDead();
}
SCRIPT_API(FCNPC_Kill, bool(int npcid)) { NPC_OR(false); npc->kill(nullptr, 0); return true; }
SCRIPT_API(FCNPC_IsDead, bool(int npcid)) { NPC_OR(false); return npc->isDead(); }
SCRIPT_API(FCNPC_IsValid, bool(int npcid)) { return GetNPC(npcid) != nullptr; }

SCRIPT_API(FCNPC_IsStreamedIn, bool(int npcid, int forplayerid))
{
	NPC_OR(false);
	IPlayer* p = FCNPCComponent::getInstance()->getPlayer(forplayerid);
	return p && npc->isStreamedInForPlayer(*p);
}
SCRIPT_API(FCNPC_IsStreamedInForAnyone, bool(int npcid)) { NPC_OR(false); return !npc->streamedForPlayers().empty(); }

// ===========================================================================
//  Position / orientation
// ===========================================================================

SCRIPT_API(FCNPC_SetPosition, bool(int npcid, float x, float y, float z)) { NPC_OR(false); npc->setPosition(Vector3(x, y, z), true); return true; }
SCRIPT_API(FCNPC_GivePosition, bool(int npcid, float x, float y, float z))
{
	NPC_OR(false);
	npc->setPosition(npc->getPosition() + Vector3(x, y, z), true);
	return true;
}
SCRIPT_API(FCNPC_GetPosition, bool(int npcid, float& x, float& y, float& z))
{
	NPC_OR(false);
	Vector3 p = npc->getPosition();
	x = p.x; y = p.y; z = p.z;
	return true;
}

SCRIPT_API(FCNPC_SetAngle, bool(int npcid, float angle)) { NPC_OR(false); npc->setRotation(GTAQuat(0.0f, 0.0f, angle), true); return true; }
SCRIPT_API(FCNPC_GiveAngle, float(int npcid, float angle))
{
	NPC_OR(0.0f);
	float a = npc->getRotation().ToEuler().z + angle;
	npc->setRotation(GTAQuat(0.0f, 0.0f, a), true);
	return a;
}
SCRIPT_API(FCNPC_SetAngleToPos, bool(int npcid, float x, float y))
{
	NPC_OR(false);
	Vector3 p = npc->getPosition();
	float a = atan2f(y - p.y, x - p.x) * (180.0f / 3.14159265f) - 90.0f;
	npc->setRotation(GTAQuat(0.0f, 0.0f, a), true);
	return true;
}
SCRIPT_API(FCNPC_SetAngleToPlayer, bool(int npcid, int playerid))
{
	NPC_OR(false);
	IPlayer* p = FCNPCComponent::getInstance()->getPlayer(playerid);
	if (!p) return false;
	Vector3 np = npc->getPosition();
	Vector3 pp = p->getPosition();
	float a = atan2f(pp.y - np.y, pp.x - np.x) * (180.0f / 3.14159265f) - 90.0f;
	npc->setRotation(GTAQuat(0.0f, 0.0f, a), true);
	return true;
}
SCRIPT_API(FCNPC_GetAngle, float(int npcid)) { NPC_OR(0.0f); return npc->getRotation().ToEuler().z; }

SCRIPT_API(FCNPC_SetQuaternion, bool(int npcid, float w, float x, float y, float z)) { NPC_OR(false); npc->setRotation(GTAQuat(w, x, y, z), true); return true; }
SCRIPT_API(FCNPC_GiveQuaternion, bool(int npcid, float w, float x, float y, float z))
{
	NPC_OR(false);
	GTAQuat cur = npc->getRotation();
	npc->setRotation(GTAQuat(cur.q.w + w, cur.q.x + x, cur.q.y + y, cur.q.z + z), true);
	return true;
}
SCRIPT_API(FCNPC_GetQuaternion, bool(int npcid, float& w, float& x, float& y, float& z))
{
	NPC_OR(false);
	GTAQuat q = npc->getRotation();
	w = q.q.w; x = q.q.x; y = q.q.y; z = q.q.z;
	return true;
}

SCRIPT_API(FCNPC_SetVelocity, bool(int npcid, float x, float y, float z, bool update_pos)) { NPC_OR(false); npc->setVelocity(Vector3(x, y, z), update_pos); return true; }
SCRIPT_API(FCNPC_GiveVelocity, bool(int npcid, float x, float y, float z, bool update_pos))
{
	NPC_OR(false);
	npc->setVelocity(npc->getVelocity() + Vector3(x, y, z), update_pos);
	return true;
}
SCRIPT_API(FCNPC_GetVelocity, bool(int npcid, float& x, float& y, float& z))
{
	NPC_OR(false);
	Vector3 v = npc->getVelocity();
	x = v.x; y = v.y; z = v.z;
	return true;
}
SCRIPT_API(FCNPC_SetSpeed, bool(int npcid, float speed))
{
	NPC_OR(false);
	Vector3 v = npc->getVelocity();
	float len = glm::length(v);
	if (len > 0.0001f) npc->setVelocity(v / len * speed, false);
	return true;
}
SCRIPT_API(FCNPC_GetSpeed, float(int npcid)) { NPC_OR(0.0f); return glm::length(npc->getVelocity()); }

SCRIPT_API(FCNPC_SetInterior, bool(int npcid, int interiorid)) { NPC_OR(false); npc->setInterior(interiorid); return true; }
SCRIPT_API(FCNPC_GetInterior, int(int npcid)) { NPC_OR(0); return npc->getInterior(); }
SCRIPT_API(FCNPC_SetVirtualWorld, bool(int npcid, int worldid)) { NPC_OR(false); npc->setVirtualWorld(worldid); return true; }
SCRIPT_API(FCNPC_GetVirtualWorld, int(int npcid)) { NPC_OR(0); return npc->getVirtualWorld(); }

// ===========================================================================
//  Health / armour / invulnerability / skin
// ===========================================================================

SCRIPT_API(FCNPC_SetHealth, bool(int npcid, float health)) { NPC_OR(false); npc->setHealth(health); return true; }
SCRIPT_API(FCNPC_GiveHealth, float(int npcid, float health)) { NPC_OR(0.0f); float h = npc->getHealth() + health; npc->setHealth(h); return h; }
SCRIPT_API(FCNPC_GetHealth, float(int npcid)) { NPC_OR(0.0f); return npc->getHealth(); }
SCRIPT_API(FCNPC_SetArmour, bool(int npcid, float armour)) { NPC_OR(false); npc->setArmour(armour); return true; }
SCRIPT_API(FCNPC_GiveArmour, float(int npcid, float armour)) { NPC_OR(0.0f); float a = npc->getArmour() + armour; npc->setArmour(a); return a; }
SCRIPT_API(FCNPC_GetArmour, float(int npcid)) { NPC_OR(0.0f); return npc->getArmour(); }

SCRIPT_API(FCNPC_SetInvulnerable, bool(int npcid, bool invulnerable)) { NPC_OR(false); npc->setInvulnerable(invulnerable); return true; }
SCRIPT_API(FCNPC_IsInvulnerable, bool(int npcid)) { NPC_OR(false); return npc->isInvulnerable(); }

SCRIPT_API(FCNPC_SetSkin, bool(int npcid, int skinid)) { NPC_OR(false); npc->setSkin(skinid); return true; }
SCRIPT_API(FCNPC_GetSkin, int(int npcid)) { NPC_OR(0); IPlayer* p = npc->getPlayer(); return p ? p->getSkin() : 0; }
SCRIPT_API(FCNPC_GetCustomSkin, int(int npcid)) { NPC_OR(0); IPlayer* p = npc->getPlayer(); return p ? p->getSkin() : 0; }

// ===========================================================================
//  Weapons
// ===========================================================================

SCRIPT_API(FCNPC_SetWeapon, bool(int npcid, int weaponid)) { NPC_OR(false); npc->setWeapon(static_cast<uint8_t>(weaponid)); return true; }
SCRIPT_API(FCNPC_GetWeapon, int(int npcid)) { NPC_OR(0); return npc->getWeapon(); }
SCRIPT_API(FCNPC_SetAmmo, bool(int npcid, int ammo)) { NPC_OR(false); npc->setAmmo(ammo); return true; }
SCRIPT_API(FCNPC_GiveAmmo, bool(int npcid, int ammo)) { NPC_OR(false); npc->setAmmo(npc->getAmmo() + ammo); return true; }
SCRIPT_API(FCNPC_GetAmmo, int(int npcid)) { NPC_OR(0); return npc->getAmmo(); }
SCRIPT_API(FCNPC_SetAmmoInClip, bool(int npcid, int ammo)) { NPC_OR(false); npc->setAmmoInClip(ammo); return true; }
SCRIPT_API(FCNPC_GiveAmmoInClip, bool(int npcid, int ammo)) { NPC_OR(false); npc->setAmmoInClip(npc->getAmmoInClip() + ammo); return true; }
SCRIPT_API(FCNPC_GetAmmoInClip, int(int npcid)) { NPC_OR(0); return npc->getAmmoInClip(); }

SCRIPT_API(FCNPC_SetWeaponSkillLevel, bool(int npcid, int skill, int level)) { NPC_OR(false); npc->setWeaponSkillLevel(static_cast<PlayerWeaponSkill>(skill), level); return true; }
SCRIPT_API(FCNPC_GiveWeaponSkillLevel, bool(int npcid, int skill, int level))
{
	NPC_OR(false);
	auto ws = static_cast<PlayerWeaponSkill>(skill);
	npc->setWeaponSkillLevel(ws, npc->getWeaponSkillLevel(ws) + level);
	return true;
}
SCRIPT_API(FCNPC_GetWeaponSkillLevel, int(int npcid, int skill)) { NPC_OR(0); return npc->getWeaponSkillLevel(static_cast<PlayerWeaponSkill>(skill)); }

SCRIPT_API(FCNPC_SetWeaponState, bool(int npcid, int weapon_state)) { NPC_OR(false); npc->setWeaponState(static_cast<PlayerWeaponState>(weapon_state)); return true; }
SCRIPT_API(FCNPC_GetWeaponState, int(int npcid)) { NPC_OR(0); return npc->getWeaponState(); }

SCRIPT_API(FCNPC_SetWeaponReloadTime, bool(int npcid, int weaponid, int time)) { NPC_OR(false); npc->setWeaponReloadTime(static_cast<uint8_t>(weaponid), time); return true; }
SCRIPT_API(FCNPC_GetWeaponReloadTime, int(int npcid, int weaponid)) { NPC_OR(-1); return npc->getWeaponReloadTime(static_cast<uint8_t>(weaponid)); }
SCRIPT_API(FCNPC_GetWeaponActualReloadTime, int(int npcid, int weaponid)) { NPC_OR(-1); return npc->getWeaponActualReloadTime(static_cast<uint8_t>(weaponid)); }
SCRIPT_API(FCNPC_SetWeaponShootTime, bool(int npcid, int weaponid, int time)) { NPC_OR(false); npc->setWeaponShootTime(static_cast<uint8_t>(weaponid), time); return true; }
SCRIPT_API(FCNPC_GetWeaponShootTime, int(int npcid, int weaponid)) { NPC_OR(-1); return npc->getWeaponShootTime(static_cast<uint8_t>(weaponid)); }
SCRIPT_API(FCNPC_SetWeaponClipSize, bool(int npcid, int weaponid, int size)) { NPC_OR(false); npc->setWeaponClipSize(static_cast<uint8_t>(weaponid), size); return true; }
SCRIPT_API(FCNPC_GetWeaponClipSize, int(int npcid, int weaponid)) { NPC_OR(-1); return npc->getWeaponClipSize(static_cast<uint8_t>(weaponid)); }
SCRIPT_API(FCNPC_GetWeaponActualClipSize, int(int npcid, int weaponid)) { NPC_OR(-1); return npc->getWeaponActualClipSize(static_cast<uint8_t>(weaponid)); }
SCRIPT_API(FCNPC_SetWeaponAccuracy, bool(int npcid, int weaponid, float accuracy)) { NPC_OR(false); npc->setWeaponAccuracy(static_cast<uint8_t>(weaponid), accuracy); return true; }
SCRIPT_API(FCNPC_GetWeaponAccuracy, float(int npcid, int weaponid)) { NPC_OR(0.0f); return npc->getWeaponAccuracy(static_cast<uint8_t>(weaponid)); }

SCRIPT_API(FCNPC_SetWeaponInfo, bool(int npcid, int weaponid, int reload_time, int shoot_time, int clip_size, float accuracy))
{
	NPC_OR(false);
	uint8_t w = static_cast<uint8_t>(weaponid);
	if (reload_time >= 0) npc->setWeaponReloadTime(w, reload_time);
	if (shoot_time >= 0) npc->setWeaponShootTime(w, shoot_time);
	if (clip_size >= 0) npc->setWeaponClipSize(w, clip_size);
	npc->setWeaponAccuracy(w, accuracy);
	return true;
}
SCRIPT_API(FCNPC_GetWeaponInfo, bool(int npcid, int weaponid, int& reload_time, int& shoot_time, int& clip_size, float& accuracy))
{
	NPC_OR(false);
	uint8_t w = static_cast<uint8_t>(weaponid);
	reload_time = npc->getWeaponReloadTime(w);
	shoot_time = npc->getWeaponShootTime(w);
	clip_size = npc->getWeaponClipSize(w);
	accuracy = npc->getWeaponAccuracy(w);
	return true;
}

// Global default table (no INPC equivalent). Seeded from the engine WeaponInfo.
SCRIPT_API(FCNPC_SetWeaponDefaultInfo, bool(int weaponid, int reload_time, int shoot_time, int clip_size, float accuracy))
{
	auto& d = FCNPCComponent::getInstance()->weaponDefault(weaponid);
	d.reloadTime = reload_time; d.shootTime = shoot_time; d.clipSize = clip_size; d.accuracy = accuracy; d.set = true;
	return true;
}
SCRIPT_API(FCNPC_GetWeaponDefaultInfo, bool(int weaponid, int& reload_time, int& shoot_time, int& clip_size, float& accuracy))
{
	auto& d = FCNPCComponent::getInstance()->weaponDefault(weaponid);
	if (d.set)
	{
		reload_time = d.reloadTime; shoot_time = d.shootTime; clip_size = d.clipSize; accuracy = d.accuracy;
	}
	else
	{
		const WeaponInfo& wi = WeaponInfo::get(static_cast<uint8_t>(weaponid));
		reload_time = wi.reloadTime; shoot_time = wi.shootTime; clip_size = wi.clipSize; accuracy = 1.0f;
	}
	return true;
}

// ===========================================================================
//  Keys / special action / animations / fighting style
// ===========================================================================

SCRIPT_API(FCNPC_SetKeys, bool(int npcid, int ud_analog, int lr_analog, int keys))
{
	NPC_OR(false);
	npc->setKeys(static_cast<uint16_t>(ud_analog), static_cast<uint16_t>(lr_analog), static_cast<uint16_t>(keys));
	return true;
}
SCRIPT_API(FCNPC_GetKeys, bool(int npcid, int& ud_analog, int& lr_analog, int& keys))
{
	NPC_OR(false);
	uint16_t ud = 0, lr = 0, k = 0;
	npc->getKeys(ud, lr, k);
	ud_analog = ud; lr_analog = lr; keys = k;
	return true;
}

SCRIPT_API(FCNPC_SetSpecialAction, bool(int npcid, int actionid)) { NPC_OR(false); npc->setSpecialAction(static_cast<PlayerSpecialAction>(actionid)); return true; }
SCRIPT_API(FCNPC_GetSpecialAction, int(int npcid)) { NPC_OR(0); return npc->getSpecialAction(); }

SCRIPT_API(FCNPC_SetAnimation, bool(int npcid, int animationid, float fDelta, int loop, int lockx, int locky, int freeze, int time))
{
	NPC_OR(false);
	npc->setAnimation(animationid, fDelta, loop != 0, lockx != 0, locky != 0, freeze != 0, time);
	return true;
}
SCRIPT_API(FCNPC_SetAnimationByName, bool(int npcid, std::string const& name, float fDelta, int loop, int lockx, int locky, int freeze, int time))
{
	NPC_OR(false);
	int id = FindAnimationId(StringView(name));
	if (id < 0) return false;
	npc->setAnimation(id, fDelta, loop != 0, lockx != 0, locky != 0, freeze != 0, time);
	return true;
}
SCRIPT_API(FCNPC_ResetAnimation, bool(int npcid)) { NPC_OR(false); npc->resetAnimation(); return true; }
SCRIPT_API(FCNPC_GetAnimation, bool(int npcid, int& animationid, float& fDelta, int& loop, int& lockx, int& locky, int& freeze, int& time))
{
	NPC_OR(false);
	bool bLoop = false, bLockX = false, bLockY = false, bFreeze = false;
	npc->getAnimation(animationid, fDelta, bLoop, bLockX, bLockY, bFreeze, time);
	loop = bLoop; lockx = bLockX; locky = bLockY; freeze = bFreeze;
	return true;
}
SCRIPT_API(FCNPC_ApplyAnimation, bool(int npcid, std::string const& animlib, std::string const& animname, float fDelta, int loop, int lockx, int locky, int freeze, int time))
{
	NPC_OR(false);
	npc->applyAnimation(AnimationData(fDelta, loop != 0, lockx != 0, locky != 0, freeze != 0, static_cast<uint32_t>(time), StringView(animlib), StringView(animname)));
	return true;
}
SCRIPT_API(FCNPC_ClearAnimations, bool(int npcid)) { NPC_OR(false); npc->clearAnimations(); return true; }

SCRIPT_API(FCNPC_SetFightingStyle, bool(int npcid, int style)) { NPC_OR(false); npc->setFightingStyle(static_cast<PlayerFightingStyle>(style)); return true; }
SCRIPT_API(FCNPC_GetFightingStyle, int(int npcid)) { NPC_OR(0); return npc->getFightingStyle(); }

SCRIPT_API(FCNPC_UseReloading, bool(int npcid, bool use)) { NPC_OR(false); npc->enableReloading(use); return true; }
SCRIPT_API(FCNPC_IsReloadingUsed, bool(int npcid)) { NPC_OR(false); return npc->isReloadEnabled(); }
SCRIPT_API(FCNPC_UseInfiniteAmmo, bool(int npcid, bool use)) { NPC_OR(false); npc->enableInfiniteAmmo(use); return true; }
SCRIPT_API(FCNPC_IsInfiniteAmmoUsed, bool(int npcid)) { NPC_OR(false); return npc->isInfiniteAmmoEnabled(); }

// ===========================================================================
//  Movement
// ===========================================================================

SCRIPT_API(FCNPC_GoTo, bool(int npcid, float x, float y, float z, int type, float speed, int mode, int pathfinding, float radius, bool set_angle, float min_distance, int stop_delay))
{
	NPC_OR(false);
	float moveSpeed = (speed < 0.0f) ? NPC_MOVE_SPEED_AUTO : speed;
	return npc->move(Vector3(x, y, z), FCNPC_ToOmpMoveType(type), moveSpeed, radius);
}
SCRIPT_API(FCNPC_GoToPlayer, bool(int npcid, int playerid, int type, float speed, int mode, int pathfinding, float radius, bool set_angle, float min_distance, float dist_check, int stop_delay))
{
	NPC_OR(false);
	IPlayer* p = FCNPCComponent::getInstance()->getPlayer(playerid);
	if (!p) return false;
	float moveSpeed = (speed < 0.0f) ? NPC_MOVE_SPEED_AUTO : speed;
	// FCNPC semantics: GoToPlayer continuously re-tracks the player. open.mp's
	// moveToPlayer defaults autoRestart=false (walks to the initial spot then
	// stops), so force it true and forward the recheck delay.
	Milliseconds posCheck(stop_delay > 0 ? stop_delay : 500);
	float stopRange = (radius > 0.0f) ? radius : 1.0f;
	return npc->moveToPlayer(*p, FCNPC_ToOmpMoveType(type), moveSpeed, stopRange, posCheck, true);
}
SCRIPT_API(FCNPC_Stop, bool(int npcid)) { NPC_OR(false); npc->stopMove(); return true; }
SCRIPT_API(FCNPC_IsMoving, bool(int npcid)) { NPC_OR(false); return npc->isMoving(); }
SCRIPT_API(FCNPC_IsMovingAtPlayer, bool(int npcid, int playerid))
{
	NPC_OR(false);
	IPlayer* p = FCNPCComponent::getInstance()->getPlayer(playerid);
	return p && npc->isMovingToPlayer(*p);
}
SCRIPT_API(FCNPC_GetDestinationPoint, bool(int npcid, float& x, float& y, float& z))
{
	NPC_OR(false);
	Vector3 d = npc->getPositionMovingTo();
	x = d.x; y = d.y; z = d.z;
	return true;
}

// ===========================================================================
//  Aiming / attacking / shooting
// ===========================================================================

SCRIPT_API(FCNPC_AimAt, bool(int npcid, float x, float y, float z, bool shoot, int shoot_delay, bool set_angle, float ofx, float ofy, float ofz, int between_check_mode, int between_check_flags))
{
	NPC_OR(false);
	npc->aimAt(Vector3(x, y, z), shoot, shoot_delay, set_angle, Vector3(ofx, ofy, ofz), static_cast<EntityCheckType>(between_check_flags));
	return true;
}
SCRIPT_API(FCNPC_AimAtPlayer, bool(int npcid, int playerid, bool shoot, int shoot_delay, bool set_angle, float ox, float oy, float oz, float ofx, float ofy, float ofz, int between_check_mode, int between_check_flags))
{
	NPC_OR(false);
	IPlayer* p = FCNPCComponent::getInstance()->getPlayer(playerid);
	if (!p) return false;
	npc->aimAtPlayer(*p, shoot, shoot_delay, set_angle, Vector3(ox, oy, oz), Vector3(ofx, ofy, ofz), static_cast<EntityCheckType>(between_check_flags));
	return true;
}
SCRIPT_API(FCNPC_StopAim, bool(int npcid)) { NPC_OR(false); npc->stopAim(); return true; }
SCRIPT_API(FCNPC_MeleeAttack, bool(int npcid, int delay, bool fighting_style)) { NPC_OR(false); npc->meleeAttack(delay, fighting_style); return true; }
SCRIPT_API(FCNPC_StopAttack, bool(int npcid)) { NPC_OR(false); npc->stopMeleeAttack(); return true; }
SCRIPT_API(FCNPC_IsAttacking, bool(int npcid)) { NPC_OR(false); return npc->isMeleeAttacking(); }
SCRIPT_API(FCNPC_IsAiming, bool(int npcid)) { NPC_OR(false); return npc->isAiming(); }
SCRIPT_API(FCNPC_IsAimingAtPlayer, bool(int npcid, int playerid))
{
	NPC_OR(false);
	IPlayer* p = FCNPCComponent::getInstance()->getPlayer(playerid);
	return p && npc->isAimingAtPlayer(*p);
}
SCRIPT_API(FCNPC_GetAimingPlayer, int(int npcid)) { NPC_OR(INVALID_PLAYER_ID); IPlayer* p = npc->getPlayerAimingAt(); return p ? p->getID() : INVALID_PLAYER_ID; }
SCRIPT_API(FCNPC_IsShooting, bool(int npcid)) { NPC_OR(false); return npc->isShooting(); }
SCRIPT_API(FCNPC_IsReloading, bool(int npcid)) { NPC_OR(false); return npc->isReloading(); }

SCRIPT_API(FCNPC_TriggerWeaponShot, bool(int npcid, int weaponid, int hittype, int hitid, float x, float y, float z, bool is_hit, float ofx, float ofy, float ofz, int between_check_mode, int between_check_flags))
{
	NPC_OR(false);
	npc->shoot(hitid, static_cast<PlayerBulletHitType>(hittype), static_cast<uint8_t>(weaponid), Vector3(x, y, z), Vector3(ofx, ofy, ofz), is_hit, static_cast<EntityCheckType>(between_check_flags));
	return true;
}

// Entity-in-between: pure-C++ proximity scan of players/NPCs/vehicles/objects
// against the ray (npc + offset) -> (x,y,z). The map flag additionally raycasts
// world geometry via ColAndreas if it is loaded.
SCRIPT_API(FCNPC_GetClosestEntityInBetween, int(int npcid, float x, float y, float z, float range, int between_check_mode, int between_check_flags, float ofx, float ofy, float ofz, int& entity_id, int& entity_type, int& object_owner_id, float& point_x, float& point_y, float& point_z))
{
	entity_id = -1; entity_type = -1; object_owner_id = INVALID_PLAYER_ID;
	point_x = point_y = point_z = 0.0f;

	auto self = FCNPCComponent::getInstance();
	INPC* npc = self->getNPC(npcid);
	if (!npc)
	{
		return 0;
	}

	const Vector3 start = npc->getPosition() + Vector3(ofx, ofy, ofz);
	const Vector3 end(x, y, z);
	const Vector3 dir = end - start;
	const float segLen2 = glm::dot(dir, dir);
	if (segLen2 < 0.0001f)
	{
		return 0;
	}

	float bestT = 1.01f; // fraction along the segment of the closest hit so far

	auto consider = [&](const Vector3& p, float radius, int type, int id, int owner) {
		float t = glm::dot(p - start, dir) / segLen2;
		if (t < 0.0f || t > 1.0f)
		{
			return;
		}
		Vector3 closest = start + dir * t;
		if (glm::distance(p, closest) <= radius && t < bestT)
		{
			bestT = t;
			entity_id = id;
			entity_type = type;
			object_owner_id = owner;
			point_x = closest.x; point_y = closest.y; point_z = closest.z;
		}
	};

	const int flags = between_check_flags;
	// Players (1) and NPCs (2) both live in the player pool; isBot() splits them.
	if (flags & (1 | 2))
	{
		for (IPlayer* p : self->getCore()->getPlayers().entries())
		{
			if (p->getID() == npcid)
			{
				continue;
			}
			bool bot = p->isBot();
			int bit = bot ? 2 : 1;
			if (flags & bit)
			{
				consider(p->getPosition(), 1.0f, bit, p->getID(), INVALID_PLAYER_ID);
			}
		}
	}
	if ((flags & 8) && self->vehicles()) // VEHICLE
	{
		for (IVehicle* v : *self->vehicles())
		{
			consider(v->getPosition(), 3.0f, 8, v->getID(), INVALID_PLAYER_ID);
		}
	}
	if ((flags & 16) && self->objects()) // OBJECT
	{
		for (IObject* o : *self->objects())
		{
			consider(o->getPosition(), 2.0f, 16, o->getID(), INVALID_PLAYER_ID);
		}
	}

	// Map geometry via ColAndreas, if requested and loaded. A map hit closer than
	// the nearest entity wins.
	if (flags & 128) // MAP
	{
		float hx, hy, hz;
		if (fcnpc_bridge::colAndreasRayLine(start.x, start.y, start.z, end.x, end.y, end.z, hx, hy, hz))
		{
			Vector3 hit(hx, hy, hz);
			float t = glm::dot(hit - start, dir) / segLen2;
			if (t >= 0.0f && t <= 1.0f && t < bestT)
			{
				bestT = t;
				entity_id = -1;
				entity_type = 128;
				object_owner_id = INVALID_PLAYER_ID;
				point_x = hx; point_y = hy; point_z = hz;
			}
		}
	}

	return (bestT <= 1.0f) ? 1 : 0;
}

// ===========================================================================
//  Vehicles
// ===========================================================================

SCRIPT_API(FCNPC_EnterVehicle, bool(int npcid, int vehicleid, int seatid, int type))
{
	NPC_OR(false);
	IVehicle* v = FCNPCComponent::getInstance()->getVehicle(vehicleid);
	if (!v) return false;
	npc->enterVehicle(*v, static_cast<uint8_t>(seatid), FCNPC_ToOmpMoveType(type));
	return true;
}
SCRIPT_API(FCNPC_ExitVehicle, bool(int npcid)) { NPC_OR(false); npc->exitVehicle(); return true; }
SCRIPT_API(FCNPC_PutInVehicle, bool(int npcid, int vehicleid, int seatid))
{
	NPC_OR(false);
	IVehicle* v = FCNPCComponent::getInstance()->getVehicle(vehicleid);
	if (!v) return false;
	return npc->putInVehicle(*v, static_cast<uint8_t>(seatid));
}
SCRIPT_API(FCNPC_RemoveFromVehicle, bool(int npcid)) { NPC_OR(false); return npc->removeFromVehicle(); }
SCRIPT_API(FCNPC_GetVehicleID, int(int npcid)) { NPC_OR(INVALID_VEHICLE_ID); IVehicle* v = npc->getVehicle(); return v ? v->getID() : INVALID_VEHICLE_ID; }
SCRIPT_API(FCNPC_GetVehicleSeat, int(int npcid)) { NPC_OR(-1); return npc->getVehicleSeat(); }
SCRIPT_API(FCNPC_UseVehicleSiren, bool(int npcid, bool use)) { NPC_OR(false); npc->useVehicleSiren(use); return true; }
SCRIPT_API(FCNPC_IsVehicleSirenUsed, bool(int npcid)) { NPC_OR(false); return npc->isVehicleSirenUsed(); }
SCRIPT_API(FCNPC_SetVehicleHealth, bool(int npcid, float health)) { NPC_OR(false); npc->setVehicleHealth(health); return true; }
SCRIPT_API(FCNPC_GetVehicleHealth, float(int npcid)) { NPC_OR(0.0f); return npc->getVehicleHealth(); }
SCRIPT_API(FCNPC_SetVehicleHydraThrusters, bool(int npcid, int direction)) { NPC_OR(false); npc->setVehicleHydraThrusters(direction); return true; }
SCRIPT_API(FCNPC_GetVehicleHydraThrusters, int(int npcid)) { NPC_OR(0); return npc->getVehicleHydraThrusters(); }
SCRIPT_API(FCNPC_SetVehicleGearState, bool(int npcid, int gear_state)) { NPC_OR(false); npc->setVehicleGearState(gear_state); return true; }
SCRIPT_API(FCNPC_GetVehicleGearState, int(int npcid)) { NPC_OR(0); return npc->getVehicleGearState(); }
SCRIPT_API(FCNPC_SetVehicleTrainSpeed, bool(int npcid, float speed)) { NPC_OR(false); npc->setVehicleTrainSpeed(speed); return true; }
SCRIPT_API(FCNPC_GetVehicleTrainSpeed, float(int npcid)) { NPC_OR(0.0f); return npc->getVehicleTrainSpeed(); }

// ===========================================================================
//  Surfing (mapped onto PlayerSurfingData)
// ===========================================================================

SCRIPT_API(FCNPC_SetSurfingOffsets, bool(int npcid, float x, float y, float z))
{
	NPC_OR(false);
	PlayerSurfingData d = npc->getSurfingData();
	d.offset = Vector3(x, y, z);
	npc->setSurfingData(d);
	return true;
}
SCRIPT_API(FCNPC_GiveSurfingOffsets, bool(int npcid, float x, float y, float z))
{
	NPC_OR(false);
	PlayerSurfingData d = npc->getSurfingData();
	d.offset += Vector3(x, y, z);
	npc->setSurfingData(d);
	return true;
}
SCRIPT_API(FCNPC_GetSurfingOffsets, bool(int npcid, float& x, float& y, float& z))
{
	NPC_OR(false);
	PlayerSurfingData d = npc->getSurfingData();
	x = d.offset.x; y = d.offset.y; z = d.offset.z;
	return true;
}
SCRIPT_API(FCNPC_SetSurfingVehicle, bool(int npcid, int vehicleid))
{
	NPC_OR(false);
	PlayerSurfingData d = npc->getSurfingData();
	d.type = PlayerSurfingData::Type::Vehicle; d.ID = vehicleid;
	npc->setSurfingData(d);
	return true;
}
SCRIPT_API(FCNPC_GetSurfingVehicle, int(int npcid))
{
	NPC_OR(INVALID_VEHICLE_ID);
	PlayerSurfingData d = npc->getSurfingData();
	return d.type == PlayerSurfingData::Type::Vehicle ? d.ID : INVALID_VEHICLE_ID;
}
SCRIPT_API(FCNPC_SetSurfingObject, bool(int npcid, int objectid))
{
	NPC_OR(false);
	PlayerSurfingData d = npc->getSurfingData();
	d.type = PlayerSurfingData::Type::Object; d.ID = objectid;
	npc->setSurfingData(d);
	return true;
}
SCRIPT_API(FCNPC_GetSurfingObject, int(int npcid))
{
	NPC_OR(INVALID_OBJECT_ID);
	PlayerSurfingData d = npc->getSurfingData();
	return d.type == PlayerSurfingData::Type::Object ? d.ID : INVALID_OBJECT_ID;
}
SCRIPT_API(FCNPC_SetSurfingPlayerObject, bool(int npcid, int objectid))
{
	NPC_OR(false);
	PlayerSurfingData d = npc->getSurfingData();
	d.type = PlayerSurfingData::Type::PlayerObject; d.ID = objectid;
	npc->setSurfingData(d);
	return true;
}
SCRIPT_API(FCNPC_GetSurfingPlayerObject, int(int npcid))
{
	NPC_OR(INVALID_OBJECT_ID);
	PlayerSurfingData d = npc->getSurfingData();
	return d.type == PlayerSurfingData::Type::PlayerObject ? d.ID : INVALID_OBJECT_ID;
}
SCRIPT_API(FCNPC_StopSurfing, bool(int npcid)) { NPC_OR(false); npc->resetSurfingData(); return true; }

// Dynamic (streamer) object surfing. Streamer dynamic objects have a per-player
// internal id; surfing data is a single broadcast value, so this resolves the
// internal id via a representative connected player. Needs the Streamer plugin.
SCRIPT_API(FCNPC_SetSurfingDynamicObject, bool(int npcid, int objectid))
{
	auto self = FCNPCComponent::getInstance();
	INPC* npc = self->getNPC(npcid);
	if (!npc)
	{
		return false;
	}
	self->extra(npcid).surfingDynamicObject = objectid;

	int internalId = -1;
	for (IPlayer* p : self->getCore()->getPlayers().entries())
	{
		if (p->isBot())
		{
			continue;
		}
		internalId = fcnpc_bridge::streamerInternalObject(p->getID(), objectid);
		if (internalId >= 0)
		{
			break;
		}
	}

	PlayerSurfingData d = npc->getSurfingData();
	d.type = PlayerSurfingData::Type::PlayerObject;
	d.ID = (internalId >= 0) ? internalId : objectid;
	npc->setSurfingData(d);
	return true;
}
SCRIPT_API(FCNPC_GetSurfingDynamicObject, int(int npcid))
{
	auto self = FCNPCComponent::getInstance();
	if (!self->getNPC(npcid))
	{
		return INVALID_OBJECT_ID;
	}
	return self->extra(npcid).surfingDynamicObject;
}

// ===========================================================================
//  Playback
// ===========================================================================

SCRIPT_API(FCNPC_StartPlayingPlayback, bool(int npcid, std::string const& file, int recordid, bool auto_unload, float dx, float dy, float dz, float dqw, float dqx, float dqy, float dqz))
{
	NPC_OR(false);
	Vector3 point(dx, dy, dz);
	GTAQuat rot(dqw, dqx, dqy, dqz);
	if (!file.empty())
	{
		auto& path = FCNPCComponent::getInstance()->extra(npcid).playbackPath;
		std::string full = path.empty() ? file : (path + file);
		return npc->startPlayback(StringView(full), auto_unload, point, rot);
	}
	return npc->startPlayback(recordid, auto_unload, point, rot);
}
SCRIPT_API(FCNPC_StopPlayingPlayback, bool(int npcid)) { NPC_OR(false); npc->stopPlayback(); return true; }
SCRIPT_API(FCNPC_PausePlayingPlayback, bool(int npcid)) { NPC_OR(false); npc->pausePlayback(true); return true; }
SCRIPT_API(FCNPC_ResumePlayingPlayback, bool(int npcid)) { NPC_OR(false); npc->pausePlayback(false); return true; }
SCRIPT_API(FCNPC_LoadPlayingPlayback, int(std::string const& file))
{
	auto npcs = FCNPCComponent::getInstance()->npcs();
	return npcs ? npcs->loadRecord(StringView(file)) : -1;
}
SCRIPT_API(FCNPC_UnloadPlayingPlayback, bool(int recordid))
{
	auto npcs = FCNPCComponent::getInstance()->npcs();
	return npcs && npcs->unloadRecord(recordid);
}
SCRIPT_API(FCNPC_SetPlayingPlaybackPath, bool(int npcid, std::string const& path))
{
	FCNPCComponent::getInstance()->extra(npcid).playbackPath = path;
	return true;
}
SCRIPT_API(FCNPC_GetPlayingPlaybackPath, bool(int npcid, OutputOnlyString& path))
{
	path = StringView(FCNPCComponent::getInstance()->extra(npcid).playbackPath);
	return true;
}

// ===========================================================================
//  Nodes
// ===========================================================================

SCRIPT_API(FCNPC_OpenNode, bool(int nodeid)) { auto n = FCNPCComponent::getInstance()->npcs(); return n && n->openNode(nodeid); }
SCRIPT_API(FCNPC_CloseNode, bool(int nodeid)) { auto n = FCNPCComponent::getInstance()->npcs(); if (n) n->closeNode(nodeid); return true; }
SCRIPT_API(FCNPC_IsNodeOpen, bool(int nodeid)) { auto n = FCNPCComponent::getInstance()->npcs(); return n && n->isNodeOpen(nodeid); }
SCRIPT_API(FCNPC_GetNodeType, int(int nodeid)) { auto n = FCNPCComponent::getInstance()->npcs(); return n ? n->getNodeType(nodeid) : -1; }
SCRIPT_API(FCNPC_SetNodePoint, bool(int nodeid, int pointid)) { auto n = FCNPCComponent::getInstance()->npcs(); return n && n->setNodePoint(nodeid, static_cast<uint16_t>(pointid)); }
SCRIPT_API(FCNPC_GetNodePointPosition, bool(int nodeid, float& x, float& y, float& z))
{
	auto n = FCNPCComponent::getInstance()->npcs();
	if (!n) return false;
	Vector3 pos;
	if (!n->getNodePointPosition(nodeid, pos)) return false;
	x = pos.x; y = pos.y; z = pos.z;
	return true;
}
SCRIPT_API(FCNPC_GetNodePointCount, int(int nodeid)) { auto n = FCNPCComponent::getInstance()->npcs(); return n ? n->getNodePointCount(nodeid) : 0; }
SCRIPT_API(FCNPC_GetNodeInfo, bool(int nodeid, int& vehnodes, int& pednodes, int& navinode))
{
	auto n = FCNPCComponent::getInstance()->npcs();
	if (!n) return false;
	uint32_t v = 0, p = 0, na = 0;
	bool ok = n->getNodeInfo(nodeid, v, p, na);
	vehnodes = v; pednodes = p; navinode = na;
	return ok;
}
SCRIPT_API(FCNPC_PlayNode, bool(int npcid, int nodeid, int type, float speed, int mode, float radius, bool set_angle))
{
	NPC_OR(false);
	float moveSpeed = (speed < 0.0f) ? NPC_MOVE_SPEED_AUTO : speed;
	return npc->playNode(nodeid, FCNPC_ToOmpMoveType(type), moveSpeed, radius, set_angle);
}
SCRIPT_API(FCNPC_StopPlayingNode, bool(int npcid)) { NPC_OR(false); npc->stopPlayingNode(); return true; }
SCRIPT_API(FCNPC_PausePlayingNode, bool(int npcid)) { NPC_OR(false); npc->pausePlayingNode(); return true; }
SCRIPT_API(FCNPC_ResumePlayingNode, bool(int npcid)) { NPC_OR(false); npc->resumePlayingNode(); return true; }
SCRIPT_API(FCNPC_IsPlayingNode, bool(int npcid)) { NPC_OR(false); return npc->isPlayingNode(); }
SCRIPT_API(FCNPC_IsPlayingNodePaused, bool(int npcid)) { NPC_OR(false); return npc->isPlayingNodePaused(); }

// ===========================================================================
//  Move paths
// ===========================================================================

SCRIPT_API(FCNPC_CreateMovePath, int()) { auto n = FCNPCComponent::getInstance()->npcs(); return n ? n->createPath() : -1; }
SCRIPT_API(FCNPC_DestroyMovePath, bool(int pathid)) { auto n = FCNPCComponent::getInstance()->npcs(); return n && n->destroyPath(pathid); }
SCRIPT_API(FCNPC_IsValidMovePath, bool(int pathid)) { auto n = FCNPCComponent::getInstance()->npcs(); return n && n->isValidPath(pathid); }
SCRIPT_API(FCNPC_AddPointToMovePath, bool(int pathid, float x, float y, float z)) { auto n = FCNPCComponent::getInstance()->npcs(); return n && n->addPointToPath(pathid, Vector3(x, y, z)); }
SCRIPT_API(FCNPC_RemovePointFromMovePath, bool(int pathid, int pointid)) { auto n = FCNPCComponent::getInstance()->npcs(); return n && n->removePointFromPath(pathid, pointid); }
SCRIPT_API(FCNPC_IsValidMovePathPoint, bool(int pathid, int pointid))
{
	auto n = FCNPCComponent::getInstance()->npcs();
	return n && pointid >= 0 && static_cast<size_t>(pointid) < n->getPathPointCount(pathid);
}
SCRIPT_API(FCNPC_GetMovePathPoint, bool(int pathid, int pointid, float& x, float& y, float& z))
{
	auto n = FCNPCComponent::getInstance()->npcs();
	if (!n) return false;
	Vector3 pos; float range;
	if (!n->getPathPoint(pathid, pointid, pos, range)) return false;
	x = pos.x; y = pos.y; z = pos.z;
	return true;
}
SCRIPT_API(FCNPC_GetNumberMovePathPoint, int(int pathid)) { auto n = FCNPCComponent::getInstance()->npcs(); return n ? static_cast<int>(n->getPathPointCount(pathid)) : 0; }
SCRIPT_API(FCNPC_GoByMovePath, bool(int npcid, int pathid, int pointid, int type, float speed, int mode, int pathfinding, float radius, bool set_angle, float min_distance))
{
	NPC_OR(false);
	float moveSpeed = (speed < 0.0f) ? NPC_MOVE_SPEED_AUTO : speed;
	return npc->moveByPath(pathid, FCNPC_ToOmpMoveType(type), moveSpeed, false);
}

// ===========================================================================
//  Per-NPC move mode / min-height (stored; INPC has no equivalent)
// ===========================================================================

SCRIPT_API(FCNPC_SetMoveMode, bool(int npcid, int mode)) { FCNPCComponent::getInstance()->extra(npcid).moveMode = mode; return true; }
SCRIPT_API(FCNPC_GetMoveMode, int(int npcid)) { return FCNPCComponent::getInstance()->extra(npcid).moveMode; }
SCRIPT_API(FCNPC_SetMinHeightPosCall, bool(int npcid, float height)) { FCNPCComponent::getInstance()->extra(npcid).minHeightPosCall = height; return true; }
SCRIPT_API(FCNPC_GetMinHeightPosCall, float(int npcid)) { return FCNPCComponent::getInstance()->extra(npcid).minHeightPosCall; }

// ===========================================================================
//  Tab list (no direct INPC/IPlayer API; no-op stubs returning success)
// ===========================================================================

SCRIPT_API(FCNPC_ShowInTabListForPlayer, bool(int npcid, int forplayerid)) { return GetNPC(npcid) != nullptr; }
SCRIPT_API(FCNPC_HideInTabListForPlayer, bool(int npcid, int forplayerid)) { return GetNPC(npcid) != nullptr; }
