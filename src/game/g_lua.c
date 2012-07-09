
#ifdef LUA_SUPPORT

/*
===========================================================================

File:	g_lua.c
Desc:	Lua version 5.1 support for the acqu-sdk

This code is based on etpub's implementation of Lua: http://www.assembla.com/code/etpub/subversion/nodes

Lua lib location rel. to the top directory: ./lua/lua5.1/lib/static
Lua lib obtained from: http://luabinaries.sourceforge.net/

Targeting bi-directional compatibility to etpro: http://wolfwiki.anime.net/index.php/ETPro:Lua_Mod_API

===========================================================================
*/

#include "g_lua.h"

lua_vm_t *lVM[LUA_NUM_VM];

// et library
static int _et_RegisterModname( lua_State *L );
static int _et_FindSelf( lua_State *L );
static int _et_FindMod( lua_State *L );
static int _et_IPCSend( lua_State *L );
static int _et_G_Print( lua_State *L );
static int _et_G_LogPrint( lua_State *L );
static int _et_ConcatArgs( lua_State *L );
static int _et_trap_Argc( lua_State *L );
static int _et_trap_Argv( lua_State *L );
static int _et_trap_Cvar_Get( lua_State *L );
static int _et_trap_Cvar_Set( lua_State *L );
static int _et_trap_GetConfigstring( lua_State *L );
static int _et_trap_SetConfigstring( lua_State *L );
static int _et_trap_SendConsoleCommand( lua_State *L );
static int _et_trap_DropClient( lua_State *L );
static int _et_trap_SendServerCommand( lua_State *L );
static int _et_G_Say( lua_State *L );
static int _et_ClientUserinfoChanged( lua_State *L );
static int _et_trap_GetUserinfo( lua_State *L );
static int _et_trap_SetUserinfo( lua_State *L );
static int _et_Info_RemoveKey( lua_State *L );
static int _et_Info_SetValueForKey( lua_State *L );
static int _et_Info_ValueForKey( lua_State *L );
static int _et_Q_CleanStr( lua_State *L );
static int _et_trap_FS_FOpenFile( lua_State *L );
static int _et_trap_FS_Read( lua_State *L );
static int _et_trap_FS_Write( lua_State *L );
static int _et_trap_FS_Rename( lua_State *L );
static int _et_trap_FS_FCloseFile( lua_State *L );
static int _et_G_SoundIndex( lua_State *L );
static int _et_G_ModelIndex( lua_State *L );
static int _et_G_globalSound( lua_State *L );
static int _et_G_Sound( lua_State *L );
static int _et_trap_Milliseconds( lua_State *L );
static int _et_G_Damage( lua_State *L );
static int _et_G_Spawn( lua_State *L );
static int _et_G_TempEntity( lua_State *L );
static int _et_G_FreeEntity( lua_State *L );
static int _et_G_GetSpawnVar( lua_State *L );
static int _et_G_SetSpawnVar( lua_State *L );
static int _et_G_SpawnGEntityFromSpawnVars( lua_State *L );
static int _et_trap_LinkEntity( lua_State *L );
static int _et_trap_UnlinkEntity( lua_State *L );
static int _et_gentity_get( lua_State *L );
static int _et_gentity_set( lua_State *L );
static int _et_G_AddEvent( lua_State *L );

// lib helper funcs
void G_LuaPushTrajectory( lua_State *L, trajectory_t *traj );
void G_LuaPushWeaponStats( lua_State *L, weapon_stat_t *ws );
void G_LuaPushVec3( lua_State *L, vec3_t vec3 );
void G_LuaSetTrajectory( lua_State *L, trajectory_t *traj );
void G_LuaSetVec3( lua_State *L, vec3_t *vec3 );
lua_vm_t *G_LuaGetVM( lua_State *L );

// lua management
void G_LuaStartVM( const char *vmname );
void G_LuaStopVM( lua_vm_t *vm );
void G_LuaRegConstants( lua_vm_t *vm );
void G_LuaError( lua_vm_t *vm, int err );

#define lua_regconstinteger(L, n) (lua_pushstring(L, #n), lua_pushinteger(L, n), lua_settable(L, -3))
#define lua_regconststring(L, n) (lua_pushstring(L, #n), lua_pushstring(L, n), lua_settable(L, -3))

#ifdef __linux__
#define HOSTARCH	"UNIX"
#elif defined WIN32
#define HOSTARCH	"WIN32"
#endif

// fields
typedef enum {	
	FT_BOOLEAN,
	FT_INT,
	FT_FLOAT,
	FT_STRING,
	FT_VECTOR,
	FT_TRAJECTORY,
	FT_WEAPONSTATS,
	FT_ENTITY,
} luafieldtypes_t;

typedef enum {
	// pointer or array
	FF_PTR			= 1,
	FF_ARR			= 2,
	// offset type
	FF_OFSENT		= 4,
	FF_OFSCLIENT	= 8,
	// access type
	FF_RO			= 16,
	FF_RW			= 32,
} luafieldflags_t;

typedef struct {
	char	*name;
	int		ofs;
	luafieldtypes_t type;
	luafieldflags_t flags;
} luafield_t;

// fields from g_spawn.c
extern field_t fields[];

/*
==============================================================
FIELDS
==============================================================
*/

luafield_t luafields[] = {
	{"activator",						FOFS(activator),							FT_ENTITY, 				FF_OFSENT | FF_RO },
	{"chain",							FOFS(chain),								FT_ENTITY, 				FF_OFSENT | FF_RW },
	{"classname",						FOFS(classname),							FT_STRING, 				FF_PTR | FF_OFSENT | FF_RW },
	{"inactivityTime",					CFOFS(inactivityTime),						FT_INT,					FF_OFSCLIENT | FF_RW },
	{"inactivityWarning",				CFOFS(inactivityWarning),					FT_BOOLEAN, 			FF_OFSCLIENT | FF_RW },
	{"clipmask",						FOFS(clipmask),								FT_INT, 				FF_OFSENT | FF_RW },
	{"closespeed",						FOFS(closespeed),							FT_FLOAT, 				FF_OFSENT | FF_RW },
	{"count",							FOFS(count),								FT_INT, 				FF_OFSENT | FF_RW },
	{"count2",							FOFS(count2),								FT_INT, 				FF_OFSENT | FF_RW },
	{"damage",							FOFS(damage),								FT_INT, 				FF_OFSENT | FF_RW },
	{"deathType",						FOFS(deathType),							FT_INT, 				FF_OFSENT | FF_RW },
	{"delay",							FOFS(delay),								FT_FLOAT, 				FF_OFSENT | FF_RW },
	{"dl_atten",						FOFS(dl_atten),								FT_INT, 				FF_OFSENT | FF_RW },
	{"dl_color",						FOFS(dl_color),								FT_VECTOR, 				FF_OFSENT | FF_RW },
	{"dl_shader",						FOFS(dl_shader),							FT_STRING, 				FF_PTR | FF_OFSENT | FF_RO },
	{"dl_stylestring",					FOFS(dl_stylestring),						FT_STRING, 				FF_PTR | FF_OFSENT | FF_RO },
	{"duration",						FOFS(duration),								FT_FLOAT, 				FF_OFSENT | FF_RW },
	{"end_size",						FOFS(end_size),								FT_INT, 				FF_OFSENT | FF_RW },
	{"enemy",							FOFS(enemy),								FT_ENTITY, 				FF_OFSENT | FF_RW },
	{"flags",							FOFS(flags),								FT_INT, 				FF_OFSENT | FF_RO },
	{"harc",							FOFS(harc),									FT_FLOAT, 				FF_OFSENT | FF_RW },
	{"health",							FOFS(health),								FT_INT, 				FF_OFSENT | FF_RW },
//	{"inuse",							FOFS(inuse),								FT_INT, 				FF_OFSENT | FF_RO }, // TODO
	{"isProp",							FOFS(isProp),								FT_BOOLEAN, 			FF_OFSENT | FF_RO },
//	{"item",							FOFS(item),									FT_STRING, 				FF_OFSENT | FF_RW }, // TODO
	{"key",								FOFS(key),									FT_INT, 				FF_OFSENT | FF_RW },
	{"message",							FOFS(message),								FT_STRING, 				FF_PTR | FF_OFSENT | FF_RW },
	{"methodOfDeath",					FOFS(methodOfDeath),						FT_INT, 				FF_OFSENT | FF_RW },
	{"mg42BaseEnt",						FOFS(mg42BaseEnt),							FT_INT, 				FF_OFSENT | FF_RW },
	{"missionLevel",					FOFS(missionLevel),							FT_INT, 				FF_OFSENT | FF_RW },
	{"model",							FOFS(model),								FT_STRING, 				FF_PTR | FF_OFSENT | FF_RO },
	{"model2",							FOFS(model2),								FT_STRING, 				FF_PTR | FF_OFSENT | FF_RO },
	{"nextTrain",						FOFS(nextTrain),							FT_ENTITY, 				FF_OFSENT | FF_RW },
	{"noise_index",						FOFS(noise_index),							FT_INT, 				FF_OFSENT | FF_RW },
//	{"origin",							FOFS(s.origin),								FT_VECTOR, 				FF_OFSENT | FF_RW }, // TODO
	{"pers.connected",					CFOFS(pers.connected),						FT_INT, 				FF_OFSCLIENT | FF_RO },
	{"pers.netname",					CFOFS(pers.netname),						FT_STRING, 				FF_ARR | FF_OFSCLIENT | FF_RO },
	{"pers.localClient",				CFOFS(pers.localClient),					FT_BOOLEAN,				FF_OFSCLIENT | FF_RO },
	{"pers.initialSpawn",				CFOFS(pers.initialSpawn),					FT_BOOLEAN,				FF_OFSCLIENT | FF_RO },
	{"pers.enterTime",					CFOFS(pers.enterTime),						FT_INT, 				FF_OFSCLIENT | FF_RO },
	{"pers.connectTime",				CFOFS(pers.connectTime),					FT_INT, 				FF_OFSCLIENT | FF_RO },
	{"pers.teamState.state",			CFOFS(pers.teamState.state),				FT_INT, 				FF_OFSCLIENT | FF_RO },
	{"pers.voteCount",					CFOFS(pers.voteCount),						FT_INT, 				FF_OFSCLIENT | FF_RW },
	{"pers.teamVoteCount",				CFOFS(pers.teamVoteCount),					FT_INT, 				FF_OFSCLIENT | FF_RW },
	{"pers.complaints",					CFOFS(pers.complaints),						FT_INT, 				FF_OFSCLIENT | FF_RO },
	{"pers.complaintClient",			CFOFS(pers.complaintClient),				FT_INT, 				FF_OFSCLIENT | FF_RO },
	{"pers.complaintEndTime",			CFOFS(pers.complaintEndTime),				FT_INT, 				FF_OFSCLIENT | FF_RO },
	{"pers.lastReinforceTime",			CFOFS(pers.lastReinforceTime),				FT_INT, 				FF_OFSCLIENT | FF_RO },
	{"pers.applicationClient",			CFOFS(pers.applicationClient),				FT_INT, 				FF_OFSCLIENT | FF_RO },
	{"pers.applicationEndTime",			CFOFS(pers.applicationEndTime),				FT_INT, 				FF_OFSCLIENT | FF_RO },
	{"pers.invitationClient",			CFOFS(pers.invitationClient),				FT_INT, 				FF_OFSCLIENT | FF_RO },
	{"pers.invitationEndTime",			CFOFS(pers.invitationEndTime),				FT_INT, 				FF_OFSCLIENT | FF_RO },
	{"pers.propositionClient",			CFOFS(pers.propositionClient),				FT_INT, 				FF_OFSCLIENT | FF_RO },
	{"pers.propositionClient2",			CFOFS(pers.propositionClient2),				FT_INT, 				FF_OFSCLIENT | FF_RO },
	{"pers.propositionEndTime",			CFOFS(pers.propositionEndTime),				FT_INT, 				FF_OFSCLIENT | FF_RO },
	{"pers.autofireteamEndTime",		CFOFS(pers.autofireteamEndTime),			FT_INT, 				FF_OFSCLIENT | FF_RO },
	{"pers.autofireteamCreateEndTime",	CFOFS(pers.autofireteamCreateEndTime),		FT_INT, 				FF_OFSCLIENT | FF_RO },
	{"pers.autofireteamJoinEndTime",	CFOFS(pers.autofireteamJoinEndTime),		FT_INT, 				FF_OFSCLIENT | FF_RO },
	{"pers.lastSpawnTime",				CFOFS(pers.lastSpawnTime),					FT_INT, 				FF_OFSCLIENT | FF_RO },
	{"pers.ready",						CFOFS(pers.ready),							FT_BOOLEAN, 			FF_OFSCLIENT | FF_RW },
	{"prevTrain	",						FOFS(prevTrain),							FT_ENTITY, 				FF_OFSENT | FF_RW },
	{"props_frame_state",				FOFS(props_frame_state),					FT_INT, 				FF_OFSENT | FF_RO },
	{"ps.stats",						CFOFS(ps.stats),							FT_INT, 				FF_ARR | FF_OFSCLIENT | FF_RW },
	{"ps.persistant",					CFOFS(ps.persistant),						FT_INT, 				FF_ARR | FF_OFSCLIENT | FF_RW },
	{"ps.ping",							CFOFS(ps.ping),								FT_INT, 				FF_OFSCLIENT | FF_RO },
	{"ps.powerups",						CFOFS(ps.powerups),							FT_INT, 				FF_ARR | FF_OFSCLIENT | FF_RW },
	{"ps.ammo",							CFOFS(ps.ammo),								FT_INT, 				FF_ARR | FF_OFSCLIENT | FF_RW },
	{"ps.ammoclip",						CFOFS(ps.ammoclip),							FT_INT,					FF_ARR | FF_OFSCLIENT | FF_RW },
	{"r.absmax",						FOFS(r.absmax),								FT_VECTOR, 				FF_OFSENT | FF_RO },
	{"r.absmin",						FOFS(r.absmin),								FT_VECTOR, 				FF_OFSENT | FF_RO },
	{"r.bmodel",						FOFS(r.bmodel),								FT_BOOLEAN, 			FF_OFSENT | FF_RO },
	{"r.contents",						FOFS(r.contents),							FT_INT, 				FF_OFSENT | FF_RW },
	{"r.currentAngles",					FOFS(r.currentAngles),						FT_VECTOR, 				FF_OFSENT | FF_RW },
	{"r.currentOrigin",					FOFS(r.currentOrigin),						FT_VECTOR, 				FF_OFSENT | FF_RW },
	{"r.eventTime",						FOFS(r.eventTime),							FT_INT, 				FF_OFSENT | FF_RW },
	{"r.linkcount",						FOFS(r.linkcount),							FT_INT, 				FF_OFSENT | FF_RO },
	{"r.linked",						FOFS(r.linked),								FT_BOOLEAN, 			FF_OFSENT | FF_RO },
	{"r.maxs",							FOFS(r.maxs),								FT_VECTOR, 				FF_OFSENT | FF_RW },
	{"r.mins",							FOFS(r.mins),								FT_VECTOR, 				FF_OFSENT | FF_RW },
	{"r.ownerNum",						FOFS(r.ownerNum),							FT_INT, 				FF_OFSENT | FF_RW },
	{"r.singleClient",					FOFS(r.singleClient),						FT_INT, 				FF_OFSENT | FF_RW },
	{"r.svFlags",						FOFS(r.svFlags),							FT_INT, 				FF_OFSENT | FF_RW },
	{"r.worldflags",					FOFS(r.worldflags),							FT_INT, 				FF_OFSENT | FF_RO },
	{"radius",							FOFS(radius),								FT_INT, 				FF_OFSENT | FF_RW },
//	{"random",							FOFS(random),								FT_FLOAT, 				FF_OFSENT | FF_RW }, // TODO
	{"rotate",							FOFS(rotate),								FT_VECTOR, 				FF_OFSENT | FF_RW },
	{"s.angles",						FOFS(s.angles),								FT_VECTOR, 				FF_OFSENT | FF_RW },
	{"s.angles2	",						FOFS(s.angles2),							FT_VECTOR, 				FF_OFSENT | FF_RW },
	{"s.apos",							FOFS(s.apos),								FT_TRAJECTORY,			FF_OFSENT | FF_RW },
	{"s.clientNum",						FOFS(s.clientNum),							FT_INT, 				FF_OFSENT | FF_RW },
	{"s.constantLight",					FOFS(s.constantLight),						FT_INT, 				FF_OFSENT | FF_RW },
	{"s.density",						FOFS(s.density),							FT_INT, 				FF_OFSENT | FF_RW },
	{"s.dl_intensity",					FOFS(s.dl_intensity),						FT_INT, 				FF_OFSENT | FF_RW },
	{"s.dmgFlags",						FOFS(s.dmgFlags),							FT_INT, 				FF_OFSENT | FF_RW },
	{"s.eFlags",						FOFS(s.eFlags),								FT_INT, 				FF_OFSENT | FF_RW },
	{"s.eType",							FOFS(s.eType),								FT_INT, 				FF_OFSENT | FF_RW },
	{"s.effect1Time",					FOFS(s.effect1Time),						FT_INT, 				FF_OFSENT | FF_RW },
	{"s.effect2Time	",					FOFS(s.effect2Time),						FT_INT, 				FF_OFSENT | FF_RW },
	{"s.effect3Time",					FOFS(s.effect3Time),						FT_INT, 				FF_OFSENT | FF_RW },
	{"s.frame",							FOFS(s.frame),								FT_INT, 				FF_OFSENT | FF_RW },
	{"s.groundEntityNum",				FOFS(s.groundEntityNum),					FT_INT, 				FF_OFSENT | FF_RO },
	{"s.loopSound",						FOFS(s.loopSound),							FT_INT, 				FF_OFSENT | FF_RW },
	{"s.modelindex",					FOFS(s.modelindex),							FT_INT, 				FF_OFSENT | FF_RW },
	{"s.modelindex2",					FOFS(s.modelindex2),						FT_INT, 				FF_OFSENT | FF_RW },
	{"s.number",						FOFS(s.number),								FT_INT, 				FF_OFSENT | FF_RO },
	{"s.onFireEnd",						FOFS(s.onFireEnd),							FT_INT, 				FF_OFSENT | FF_RW },
	{"s.onFireStart",					FOFS(s.onFireStart),						FT_INT, 				FF_OFSENT | FF_RW },
	{"s.origin",						FOFS(s.origin),								FT_VECTOR, 				FF_OFSENT | FF_RO },
	{"s.origin2",						FOFS(s.origin2),							FT_VECTOR, 				FF_OFSENT | FF_RO },
	{"s.pos",							FOFS(s.pos),								FT_TRAJECTORY,			FF_OFSENT | FF_RW },
	{"s.powerups",						FOFS(s.powerups),							FT_INT, 				FF_OFSENT | FF_RO },
	{"s.solid",							FOFS(s.solid),								FT_INT, 				FF_OFSENT | FF_RW },
	{"s.teamNum",						FOFS(s.teamNum),							FT_INT, 				FF_OFSENT | FF_RW },
	{"s.time",							FOFS(s.time),								FT_INT, 				FF_OFSENT | FF_RW },
	{"s.time2",							FOFS(s.time2),								FT_INT,					FF_OFSENT | FF_RW },
	{"s.weapon",						FOFS(s.weapon),								FT_INT, 				FF_OFSENT | FF_RO },
	{"scriptName",						FOFS(scriptName),							FT_STRING,				FF_PTR | FF_OFSENT | FF_RO },
	{"sess.sessionTeam",				CFOFS(sess.sessionTeam),					FT_INT, 				FF_OFSCLIENT | FF_RW },
	{"sess.spectatorTime",				CFOFS(sess.spectatorTime),					FT_INT, 				FF_OFSCLIENT | FF_RW },
	{"sess.spectatorState",				CFOFS(sess.spectatorState),					FT_INT, 				FF_OFSCLIENT | FF_RW },
	{"sess.spectatorClient",			CFOFS(sess.spectatorClient),				FT_INT, 				FF_OFSCLIENT | FF_RW },
//	{"sess.latchSpectatorClient",		CFOFS(sess.latchSpectatorClient),			FT_INT, 				FF_OFSCLIENT | FF_RW }, // ONLY IN ETPRO
	{"sess.playerType",					CFOFS(sess.playerType),						FT_INT, 				FF_OFSCLIENT | FF_RW },
	{"sess.playerWeapon",				CFOFS(sess.playerWeapon),					FT_INT, 				FF_OFSCLIENT | FF_RW },
	{"sess.playerWeapon2",				CFOFS(sess.playerWeapon2),					FT_INT, 				FF_OFSCLIENT | FF_RW },
	{"sess.spawnObjectiveIndex",		CFOFS(sess.spawnObjectiveIndex),			FT_INT, 				FF_OFSCLIENT | FF_RW },
	{"sess.latchPlayerType",			CFOFS(sess.latchPlayerType),				FT_INT, 				FF_OFSCLIENT | FF_RW },
	{"sess.latchPlayerWeapon",			CFOFS(sess.latchPlayerWeapon),				FT_INT, 				FF_OFSCLIENT | FF_RW },
	{"sess.latchPlayerWeapon2",			CFOFS(sess.latchPlayerWeapon2),				FT_INT, 				FF_OFSCLIENT | FF_RW },
	{"sess.damage_given",				CFOFS(sess.damage_given),					FT_INT, 				FF_OFSCLIENT | FF_RW },
	{"sess.damage_received",			CFOFS(sess.damage_received),				FT_INT, 				FF_OFSCLIENT | FF_RW },
	{"sess.deaths",						CFOFS(sess.deaths),							FT_INT, 				FF_OFSCLIENT | FF_RW },
	{"sess.game_points",				CFOFS(sess.game_points),					FT_INT, 				FF_OFSCLIENT | FF_RW },
//	{"sess.gibs",						CFOFS(sess.gibs),							FT_INT, 				FF_OFSCLIENT | FF_RW }, // ONLY IN ETPRO
	{"sess.kills",						CFOFS(sess.kills),							FT_INT, 				FF_OFSCLIENT | FF_RW },
	{"sess.medals",						CFOFS(sess.medals),							FT_INT, 				FF_ARR | FF_OFSCLIENT | FF_RW },
	{"sess.muted",						CFOFS(sess.muted),							FT_BOOLEAN,				FF_OFSCLIENT | FF_RW },
	{"sess.rank",						CFOFS(sess.rank),							FT_INT, 				FF_OFSCLIENT | FF_RW },
	{"sess.referee",					CFOFS(sess.referee),						FT_INT, 				FF_OFSCLIENT | FF_RW },
	{"sess.rounds",						CFOFS(sess.rounds),							FT_INT, 				FF_OFSCLIENT | FF_RW },
//	{"sess.semiadmin",					CFOFS(sess.semiadmin),						FT_INT, 				FF_OFSCLIENT | FF_RW }, // ONLY IN ETPRO
	{"sess.skill",						CFOFS(sess.skill),							FT_INT,					FF_ARR | FF_OFSCLIENT | FF_RW },
	{"sess.spec_invite",				CFOFS(sess.spec_invite),					FT_INT,					FF_OFSCLIENT | FF_RW },
	{"sess.spec_team",					CFOFS(sess.spec_team),						FT_INT,					FF_OFSCLIENT | FF_RW },
	{"sess.suicides",					CFOFS(sess.suicides),						FT_INT,					FF_OFSCLIENT | FF_RW },
	{"sess.team_damage",				CFOFS(sess.team_damage),					FT_INT,					FF_OFSCLIENT | FF_RW },
	{"sess.team_kills",					CFOFS(sess.team_kills),						FT_INT,					FF_OFSCLIENT | FF_RW },
//	{"sess.team_received",				CFOFS(sess.team_received),					FT_INT,					FF_OFSCLIENT | FF_RW }, // ONLY IN ETPRO
	{"sess.aWeaponStats",				CFOFS(sess.aWeaponStats),					FT_WEAPONSTATS,			FF_ARR | FF_OFSCLIENT | FF_RO },
	{"spawnflags",						FOFS(spawnflags),							FT_INT, 				FF_OFSENT | FF_RO },
	{"spawnitem",						FOFS(spawnitem),							FT_STRING, 				FF_PTR | FF_OFSENT | FF_RO },
	{"speed",							FOFS(speed),								FT_FLOAT, 				FF_OFSENT | FF_RW },
	{"splashDamage",					FOFS(splashDamage),							FT_INT, 				FF_OFSENT | FF_RW },
	{"splashMethodOfDeath",				FOFS(splashMethodOfDeath),					FT_INT, 				FF_OFSENT | FF_RW },
	{"splashRadius",					FOFS(splashRadius),							FT_INT, 				FF_OFSENT | FF_RW },
	{"start_size",						FOFS(start_size),							FT_INT, 				FF_OFSENT | FF_RW },
	{"tagName",							FOFS(tagName),								FT_STRING, 				FF_ARR | FF_OFSENT | FF_RO },
	{"tagParent",						FOFS(tagParent),							FT_ENTITY, 				FF_OFSENT | FF_RW },
	{"takedamage",						FOFS(takedamage),							FT_BOOLEAN, 			FF_OFSENT | FF_RW },
	{"tankLink",						FOFS(tankLink),								FT_ENTITY, 				FF_OFSENT | FF_RW },
	{"target",							FOFS(target),								FT_STRING, 				FF_PTR | FF_OFSENT | FF_RW },
	{"TargetAngles",					FOFS(TargetAngles),							FT_VECTOR, 				FF_OFSENT | FF_RW },
	{"TargetFlag",						FOFS(TargetFlag),							FT_INT, 				FF_OFSENT | FF_RO },
	{"targetname",						FOFS(targetname),							FT_STRING, 				FF_PTR | FF_OFSENT | FF_RO },
	{"teamchain",						FOFS(teamchain),							FT_ENTITY, 				FF_OFSENT | FF_RW },
	{"teammaster",						FOFS(teammaster),							FT_ENTITY, 				FF_OFSENT | FF_RW },
	{"track",							FOFS(track),								FT_STRING, 				FF_PTR | FF_OFSENT | FF_RO },
	{"varc",							FOFS(varc),									FT_FLOAT, 				FF_OFSENT | FF_RW },
	{"wait",							FOFS(wait),									FT_FLOAT, 				FF_OFSENT | FF_RW },
	{"waterlevel",						FOFS(waterlevel),							FT_INT, 				FF_OFSENT | FF_RO },
	{"watertype",						FOFS(watertype),							FT_INT, 				FF_OFSENT | FF_RO },
	{NULL}
};

/*
==============================================================
ET LIBRARY
==============================================================
*/

static const luaL_Reg etlib[] = {
	// Modules
	{ "RegisterModname",					_et_RegisterModname			},
	{ "FindSelf",							_et_FindSelf				},
	{ "FindMod",							_et_FindMod					},
	{ "IPCSend",							_et_IPCSend					},
	// Printing
	{ "G_Print", 							_et_G_Print					},
	{ "G_LogPrint",							_et_G_LogPrint				},
	// Argument Handling
	{ "ConcatArgs",							_et_ConcatArgs				},
	{ "trap_Argc", 							_et_trap_Argc				},
	{ "trap_Argv", 							_et_trap_Argv				},
	// Cvars
	{ "trap_Cvar_Get",						_et_trap_Cvar_Get			},
	{ "trap_Cvar_Set",						_et_trap_Cvar_Set			},
	// Configstrings
	{ "trap_GetConfigstring",				_et_trap_GetConfigstring	},
	{ "trap_SetConfigstring",				_et_trap_SetConfigstring	},
	// Server
	{ "trap_SendConsoleCommand",			_et_trap_SendConsoleCommand	},
	// Clients
	{ "trap_DropClient",					_et_trap_DropClient			},
	{ "trap_SendServerCommand",				_et_trap_SendServerCommand	},
	{ "G_Say",								_et_G_Say					},
	{ "ClientUserinfoChanged",				_et_ClientUserinfoChanged	},
	// Userinfo
	{ "trap_GetUserinfo",					_et_trap_GetUserinfo		},
	{ "trap_SetUserinfo",					_et_trap_SetUserinfo		},
	// String Utility Functions
	{ "Info_RemoveKey",						_et_Info_RemoveKey			},
	{ "Info_SetValueForKey",				_et_Info_SetValueForKey		},
	{ "Info_ValueForKey",					_et_Info_ValueForKey		},
	{ "Q_CleanStr",							_et_Q_CleanStr				},
	// ET Filesystem
	{ "trap_FS_FOpenFile",					_et_trap_FS_FOpenFile		},
	{ "trap_FS_Read",						_et_trap_FS_Read			},
	{ "trap_FS_Write",						_et_trap_FS_Write			},
	{ "trap_FS_Rename",						_et_trap_FS_Rename			},
	{ "trap_FS_FCloseFile",					_et_trap_FS_FCloseFile		},
	// Indexes
	{ "G_SoundIndex",						_et_G_SoundIndex			},
	{ "G_ModelIndex",						_et_G_ModelIndex			},
	// Sound
	{ "G_globalSound",						_et_G_globalSound			},
	{ "G_Sound",							_et_G_Sound					},
	// Miscellaneous
	{ "trap_Milliseconds",					_et_trap_Milliseconds		},
	{ "G_Damage",							_et_G_Damage				},
	// Entities
	{ "G_Spawn",							_et_G_Spawn					},
	{ "G_TempEntity",						_et_G_TempEntity			},
	{ "G_FreeEntity",						_et_G_FreeEntity			},
	{ "G_GetSpawnVar",						_et_G_GetSpawnVar			},
	{ "G_SetSpawnVar",						_et_G_SetSpawnVar			},
	{ "G_SpawnGEntityFromSpawnVars",		_et_G_SpawnGEntityFromSpawnVars	},	// TODO
	{ "trap_LinkEntity",					_et_trap_LinkEntity			},
	{ "trap_UnlinkEntity",					_et_trap_UnlinkEntity		},
	{ "gentity_get",						_et_gentity_get				},
	{ "gentity_set",						_et_gentity_set				},
	{ "G_AddEvent",							_et_G_AddEvent				},
	{ NULL,									NULL						},
};

/*
==============================================================
ET LIBRARY CALLS
==============================================================
*/

/*
--------------------------------------------------------------
MODULES
--------------------------------------------------------------
*/

// et.RegisterModname( modname )
static int _et_RegisterModname( lua_State *L )
{
	lua_vm_t *vm;
	const char *modname = luaL_checkstring(L, 1);
	
	if (modname) {
		vm = G_LuaGetVM(L);
		if (vm) {
			Q_strncpyz(vm->modname, modname, sizeof(vm->modname));
		}
	}
	return 0;
}

// vmnumber = et.FindSelf()
static int _et_FindSelf( lua_State *L )
{
	lua_vm_t *vm = G_LuaGetVM(L);
	if (vm) {
		lua_pushinteger(L, vm->id);
	} else {
		lua_pushnil(L);
	}
	return 1;
}

// modname, signature = et.FindMod( vmnumber )
static int _et_FindMod( lua_State *L )
{
	int vmnumber = luaL_checkint(L, 1);
	lua_vm_t *vm = lVM[vmnumber];

	if(vmnumber < 0 || vmnumber >= LUA_NUM_VM) {
		G_Printf("Lua: invalid vmnumber. Must be [0, %i]\n", LUA_NUM_VM - 1);
		lua_pushnil(L);
		lua_pushnil(L);
		return 2;
	}
	
	if ( vm ) {
		lua_pushstring(L, vm->modname);
		lua_pushstring(L, vm->sha1);
	} else {
		lua_pushnil(L);
		lua_pushnil(L);
	}
	return 2;
}

// success = et.IPCSend( vmnumber, message )
static int _et_IPCSend( lua_State *L )
{
	int err;
	int vmnumber = luaL_checkint(L, 1);
	const char *message = luaL_checkstring(L, 2);
	
	lua_vm_t *sender = G_LuaGetVM(L);
	lua_vm_t *vm = lVM[vmnumber];
	
	if (!vm) {
		lua_pushinteger(L, 0);
		return 1;
	}

	if (vm->L) {
		lua_getglobal(vm->L, "et_IPCReceive");
		if (!lua_isfunction(vm->L, -1)) {
			lua_pop(vm->L, 1);
			lua_pushinteger(L, 0);
			return 1;
		}
	}
		
	// Arguments
	if (sender) {
		lua_pushinteger(vm->L, sender->id);
	} else {
		lua_pushnil(vm->L);
	}
	lua_pushstring(vm->L, message);

	err = lua_pcall(vm->L, 2, 0, 0);

	if(err) {
		G_LuaError(vm, err);
		lua_pushinteger(L, 0);
		return 1;
	}
	
	// Success
	lua_pushinteger(L, 1);
	return 1;
}

/*
--------------------------------------------------------------
PRINTING
--------------------------------------------------------------
*/

// et.G_Print( text )
static int _et_G_Print( lua_State *L )
{	
	char text[1024];
	Q_strncpyz( text, luaL_checkstring( L, 1 ), sizeof( text ) );
	trap_Printf( text );
	return 0;
}

// et.G_LogPrint( text )
static int _et_G_LogPrint( lua_State *L )
{
	char	text[1024],	s[1024];
	int		min, tens, sec;

	Q_strncpyz( text, luaL_checkstring( L, 1 ), sizeof( text ) );

	if( g_dedicated.integer ) {
		trap_Printf( text );
	}

	if( level.logFile ) {
		sec = level.time / 1000;
		min = sec / 60;
		sec -= min * 60;
		tens = sec / 10;
		sec -= tens * 10;
		Com_sprintf( s, sizeof( s ), "%i:%i%i %s", min, tens, sec, text );
		trap_FS_Write( s, strlen( s ), level.logFile );
	}	

	return 0;
}

/*
--------------------------------------------------------------
ARGUMENT HANDLING
--------------------------------------------------------------
*/

// args = et.ConcatArgs( index )
static int _et_ConcatArgs( lua_State *L )
{
	int index = luaL_checkint(L, 1);
	lua_pushstring(L, ConcatArgs(index));
	return 1;
}

// argcount = et.trap_Argc()
static int _et_trap_Argc( lua_State *L )
{
	lua_pushinteger(L, trap_Argc());
	return 1;
}

// arg = et.trap_Argv( argnum )
static int _et_trap_Argv( lua_State *L )
{
	char buff[MAX_STRING_CHARS];
	int argnum = luaL_checkint(L, 1);
	trap_Argv(argnum, buff, sizeof(buff));
	lua_pushstring(L, buff);
	return 1;
}

/*
--------------------------------------------------------------
CVARS
--------------------------------------------------------------
*/

// cvarvalue = et.trap_Cvar_Get( cvarname )
static int _et_trap_Cvar_Get( lua_State *L )
{
	char buff[MAX_CVAR_VALUE_STRING];
	const char *cvarname = luaL_checkstring(L, 1);
	trap_Cvar_VariableStringBuffer(cvarname, buff, sizeof(buff));
	lua_pushstring(L, buff);
	return 1;
}

// et.trap_Cvar_Set( cvarname, cvarvalue )
static int _et_trap_Cvar_Set( lua_State *L )
{
	const char *cvarname = luaL_checkstring(L, 1);
	const char *cvarvalue = luaL_checkstring(L, 2);
	trap_Cvar_Set(cvarname, cvarvalue);
	return 0;
}

/*
--------------------------------------------------------------
CONFIGSTRINGS
--------------------------------------------------------------
*/

// configstringvalue = et.trap_GetConfigstring( index )
static int _et_trap_GetConfigstring( lua_State *L )
{
	char buff[MAX_STRING_CHARS];
	int index = luaL_checkint(L, 1);
	trap_GetConfigstring(index, buff, sizeof(buff));
	lua_pushstring(L, buff);
	return 1;
}

// et.trap_SetConfigstring( index, configstringvalue )
static int _et_trap_SetConfigstring( lua_State *L )
{
	int index = luaL_checkint(L, 1);
	const char *csv = luaL_checkstring(L, 2);
	trap_SetConfigstring(index, csv);
	return 0;
}

/*
--------------------------------------------------------------
SERVER
--------------------------------------------------------------
*/

// et.trap_SendConsoleCommand( when, command )
static int _et_trap_SendConsoleCommand( lua_State *L )
{
	int when = luaL_checkint(L, 1);
	const char *cmd= luaL_checkstring(L, 2);
	trap_SendConsoleCommand(when, cmd);
	return 0;
}

/*
--------------------------------------------------------------
CLIENTS
--------------------------------------------------------------
*/

// et.trap_DropClient( clientnum, reason, ban_time ) 
static int _et_trap_DropClient( lua_State *L )
{
	int clientnum = luaL_checkint(L, 1);
	const char *reason = luaL_checkstring(L, 2);
	int ban = trap_Cvar_VariableIntegerValue("g_defaultBanTime");
	ban = luaL_optint(L, 3, ban);
	trap_DropClient(clientnum, reason, ban);
	return 0;
}

// et.trap_SendServerCommand( clientnum, command )
static int _et_trap_SendServerCommand( lua_State *L )
{
	int clientnum = luaL_checkint(L, 1);
	const char *cmd = luaL_checkstring(L, 2);
	trap_SendServerCommand(clientnum, cmd);
	return 0;
}

void G_Say( gentity_t *ent, gentity_t *target, int mode, const char *chatText );

// et.G_Say( clientNum, mode, text )
static int _et_G_Say( lua_State *L )
{
	int clientnum = luaL_checkint(L, 1);
	int mode = luaL_checkint(L, 2);
	const char *text = luaL_checkstring(L, 3);
	G_Say(g_entities + clientnum, NULL, mode, text);
	return 0;
}

// et.ClientUserinfoChanged( clientNum )
static int _et_ClientUserinfoChanged( lua_State *L )
{
	int clientnum = luaL_checkint(L, 1);
	ClientUserinfoChanged(clientnum);
	return 0;
}

/*
--------------------------------------------------------------
USERINFO
--------------------------------------------------------------
*/

// userinfo = et.trap_GetUserinfo( clientnum )
static int _et_trap_GetUserinfo( lua_State *L )
{
	char buff[MAX_STRING_CHARS];
	int clientnum = luaL_checkint(L, 1);
	trap_GetUserinfo(clientnum, buff, sizeof(buff));
	lua_pushstring(L, buff);
	return 1;
}

// et.trap_SetUserinfo( clientnum, userinfo )
static int _et_trap_SetUserinfo( lua_State *L )
{
	int clientnum = luaL_checkint(L, 1);
	const char *userinfo = luaL_checkstring(L, 2);
	trap_SetUserinfo(clientnum, userinfo);
	return 0;
}

/*
--------------------------------------------------------------
STRING UTILITY FUNCTIONS
--------------------------------------------------------------
*/

// infostring = et.Info_RemoveKey( infostring, key )
static int _et_Info_RemoveKey( lua_State *L )
{
	char buff[MAX_INFO_STRING];
	const char *key = luaL_checkstring(L, 2);
	Q_strncpyz(buff, luaL_checkstring(L, 1), sizeof(buff));
	Info_RemoveKey(buff, key);
	lua_pushstring(L, buff);
	return 1;
}

// infostring = et.Info_SetValueForKey( infostring, key, value )
static int _et_Info_SetValueForKey( lua_State *L )
{
	char buff[MAX_INFO_STRING];
	const char *key = luaL_checkstring(L, 2);
	const char *value = luaL_checkstring(L, 3);
	Q_strncpyz(buff, luaL_checkstring(L, 1), sizeof(buff));
	Info_SetValueForKey(buff, key, value);
	lua_pushstring(L, buff);
	return 1;
}

// keyvalue = et.Info_ValueForKey( infostring, key )
static int _et_Info_ValueForKey( lua_State *L )
{
	const char *infostring = luaL_checkstring(L, 1);
	const char *key = luaL_checkstring(L, 2);
	lua_pushstring(L, Info_ValueForKey(infostring, key));
	return 1;
}

// cleanstring = et.Q_CleanStr( string )
static int _et_Q_CleanStr( lua_State *L )
{
	char buff[MAX_STRING_CHARS];
	Q_strncpyz(buff, luaL_checkstring(L, 1), sizeof(buff));
	Q_CleanStr(buff);
	lua_pushstring(L, buff);
	return 1;
}

/*
--------------------------------------------------------------
ET FILESYSTEM
--------------------------------------------------------------
*/

// fd, len = et.trap_FS_FOpenFile( filename, mode )
static int _et_trap_FS_FOpenFile( lua_State *L )
{
	fileHandle_t fd;
	int len;
	const char *filename = luaL_checkstring(L, 1);
	int mode = luaL_checkint(L, 2);
	len = trap_FS_FOpenFile(filename, &fd, mode);
	lua_pushinteger(L, fd);
	lua_pushinteger(L, len);
	return 2;
}

// filedata = et.trap_FS_Read( fd, count )
static int _et_trap_FS_Read( lua_State *L )
{
	char *filedata = "";
	fileHandle_t fd = luaL_checkint(L, 1);
	int count = luaL_checkint(L, 2);
	filedata = malloc(count + 1);
	trap_FS_Read(filedata, count, fd);
	*(filedata + count) = '\0';
	lua_pushstring(L, filedata);
	return 1;
}

// count = et.trap_FS_Write( filedata, count, fd )
static int _et_trap_FS_Write( lua_State *L )
{
	const char *filedata = luaL_checkstring(L, 1);
	int count = luaL_checkint(L, 2);
	fileHandle_t fd = luaL_checkint(L, 3);
	lua_pushinteger(L, trap_FS_Write(filedata, count, fd));
	return 1;
}

// et.trap_FS_Rename( oldname, newname )
static int _et_trap_FS_Rename( lua_State *L )
{
	const char *oldname = luaL_checkstring(L, 1);
	const char *newname = luaL_checkstring(L, 2);
	trap_FS_Rename(oldname, newname);
	return 0;
}

// et.trap_FS_FCloseFile( fd )
static int _et_trap_FS_FCloseFile( lua_State *L )
{
	fileHandle_t fd = luaL_checkint(L, 1);
	trap_FS_FCloseFile(fd);
	return 0;
}

/*
--------------------------------------------------------------
INDEXES
--------------------------------------------------------------
*/

// soundindex = et.G_SoundIndex( filename )
static int _et_G_SoundIndex( lua_State *L )
{
	const char *filename = luaL_checkstring(L, 1);
	lua_pushinteger(L, G_SoundIndex(filename));
	return 1;
}

// modelindex = et.G_ModelIndex( filename )
static int _et_G_ModelIndex( lua_State *L )
{
	const char *filename = luaL_checkstring(L, 1);
	lua_pushinteger(L, G_ModelIndex((char *)filename));
	return 1;
}

/*
--------------------------------------------------------------
SOUND
--------------------------------------------------------------
*/

// et.G_globalSound( sound )
static int _et_G_globalSound( lua_State *L )
{
	const char *sound = luaL_checkstring(L, 1);
	G_globalSound((char *)sound);
	return 0;
}

// et.G_Sound( entnum, soundindex )
static int _et_G_Sound( lua_State *L )
{
	int entnum = luaL_checkint(L, 1);
	int soundindex = luaL_checkint(L, 2);
	G_Sound(g_entities + entnum, soundindex);
	return 0;
}

/*
--------------------------------------------------------------
MISCELLANEOUS
--------------------------------------------------------------
*/

// milliseconds = et.trap_Milliseconds()
static int _et_trap_Milliseconds( lua_State *L )
{
	lua_pushinteger(L, trap_Milliseconds());
	return 1;
}

// et.G_Damage( target, inflictor, attacker, damage, dflags, mod )
static int _et_G_Damage( lua_State *L )
{
	int target = luaL_checkint(L, 1);
	int inflictor = luaL_checkint(L, 2);
	int attacker = luaL_checkint(L, 3);
	int damage = luaL_checkint(L, 4);
	int dflags = luaL_checkint(L, 5);
	int mod = luaL_checkint(L, 6);
	
	G_Damage(g_entities + target, g_entities + inflictor, g_entities + attacker,
		NULL,	NULL, damage, dflags, mod);
	
	return 0;
}

/*
--------------------------------------------------------------
ENTITIES
--------------------------------------------------------------
*/

// entnum = et.G_Spawn()
static int _et_G_Spawn( lua_State *L )
{
	gentity_t *entnum = G_Spawn();
	lua_pushinteger(L, entnum - g_entities);
	return 1;
}

// entnum = et.G_TempEntity( origin, event )
static int _et_G_TempEntity( lua_State *L )
{
	vec3_t origin;
	int event = luaL_checkint(L, 2);
	lua_pop(L, 1);
	G_LuaSetVec3(L, &origin);
	lua_pushinteger(L, G_TempEntity(origin, event) - g_entities);
	return 1;
}

// et.G_FreeEntity( entnum )
static int _et_G_FreeEntity( lua_State *L )
{
	int entnum = luaL_checkint(L, 1);
	G_FreeEntity(g_entities + entnum);
	return 0;
}

// spawnval = et.G_GetSpawnVar( entnum, key )
static int _et_G_GetSpawnVar( lua_State *L )
{
	gentity_t *ent;
	int entnum = luaL_checkint(L, 1);
	const char *key = luaL_checkstring(L, 2);
	int			index = GetFieldIndex( (char *)key );
	fieldtype_t	type = GetFieldType( (char *)key );
	int			ofs;

	// break on invalid gentity field
	if ( index == -1 ) {
		luaL_error(L, "field \"%s\" index is -1", key);
		return 0;
	}

	if ( entnum < 0 || entnum >= MAX_GENTITIES ) {
		luaL_error(L, "entnum \"%d\" is out of range", entnum);
		return 0;
	}

	ent = &g_entities[entnum];

	// If the entity is not in use, return nil
	if ( !ent->inuse ) {
		lua_pushnil(L);
		return 1;
	}

	ofs = fields[index].ofs;

	switch( type ) {
		case F_INT:
			lua_pushinteger(L, *(int *) ((byte *)ent + ofs));
			return 1;
		case F_FLOAT:
			lua_pushnumber(L, *(float *) ((byte *)ent + ofs));
			return 1;
		// TODO - etpub here checks for FIELD_FLAG_NOPTR, which (there) is only defined for the gentity_field_t structure.
		// Have to check this in near future. For now, just resort to pointer handling.
		case F_LSTRING:
		case F_GSTRING:
			lua_pushstring(L, *(char **) ((byte *)ent + ofs));
			return 1;
		case F_VECTOR:
		case F_ANGLEHACK:
			G_LuaPushVec3(L, *(vec3_t *)((byte *)ent + ofs));
			return 1;
		case F_ENTITY:
		case F_ITEM:
		case F_CLIENT:
		case F_IGNORE:
		default:
			lua_pushnil(L);
			return 1;
	}
	return 0;
}

// et.G_SetSpawnVar( entnum, key, value )
static int _et_G_SetSpawnVar( lua_State *L )
{
	gentity_t *ent;
	int entnum = luaL_checkint(L, 1);
	const char *key = luaL_checkstring(L, 2);
	int			index = GetFieldIndex( (char *)key );
	fieldtype_t	type = GetFieldType( (char *)key );
	int			ofs;
	const char *buffer;

	// break on invalid gentity field
	if ( index == -1 ) {
		luaL_error(L, "field \"%s\" index is -1", key);
		return 0;
	}

	if ( entnum < 0 || entnum >= MAX_GENTITIES ) {
		luaL_error(L, "entnum \"%d\" is out of range", entnum);
		return 0;
	}

	ent = &g_entities[entnum];

	// If the entity is not in use, return nil
	if ( !ent->inuse ) {
		lua_pushnil(L);
		return 1;
	}

	ofs = fields[index].ofs;

	switch( type ) {
		case F_INT:
			*(int *) ((byte *)ent + ofs) = luaL_checkint(L, 3);
			return 1;
		case F_FLOAT:
			*(float *) ((byte *)ent + ofs) = (float)luaL_checknumber(L, 3);
			return 1;
		// TODO - etpub here checks for FIELD_FLAG_NOPTR, which (there) is only defined for the gentity_field_t structure.
		// Have to check this in near future. For now, just resort to pointer handling.
		case F_LSTRING:
		case F_GSTRING:
			buffer = luaL_checkstring(L, 3);
			free(*(char **)((byte *)ent + ofs));
			*(char **)((byte *)ent + ofs) = malloc(strlen(buffer));
			Q_strncpyz(*(char **)((byte *)ent + ofs), buffer, strlen(buffer));
			return 1;
		case F_VECTOR:
		case F_ANGLEHACK:
			G_LuaSetVec3(L, (vec3_t *)((byte *)ent + ofs));
			return 1;
		case F_ENTITY:
			*(gentity_t **)((byte *)ent + ofs) = g_entities + luaL_checkint(L, 3);
			return 1;
		case F_ITEM:
		case F_CLIENT:
		case F_IGNORE:
		default:
			lua_pushnil(L);
			return 1;
	}

	return 0;
}

// integer entnum = et.G_SpawnGEntityFromSpawnVars( string spawnvar, string spawnvalue, ... )
static int _et_G_SpawnGEntityFromSpawnVars( lua_State *L )
{
	// TODO
	G_Printf("_et_G_SpawnGEntityFromSpawnVars\n");
	return 0;
}

// et.trap_LinkEntity( entnum )
static int _et_trap_LinkEntity( lua_State *L )
{
	int entnum = luaL_checkint(L, 1);
	trap_LinkEntity(g_entities + entnum);
	return 0;
}

// et.trap_UnlinkEntity( entnum )
static int _et_trap_UnlinkEntity( lua_State *L )
{
	int entnum = luaL_checkint(L, 1);
	trap_UnlinkEntity(g_entities + entnum);
	return 0;
}

// (variable) = et.gentity_get ( entnum, Fieldname ,arrayindex )
static int _et_gentity_get( lua_State *L )
{
	int i;
	const char *fieldname;
	byte	*b;
	gentity_t *ent;	
	luafield_t *f = NULL;	

	ent = g_entities + luaL_checkint(L, 1);
	fieldname = luaL_checkstring(L, 2);

	if( !ent ) {
		lua_pushnil(L);
		return 1;
	}

	for( i = 0; luafields[i].name; i++ ) {
		if( !Q_stricmp( fieldname, luafields[i].name ) ) {
			f = (luafield_t *)&luafields[i];
		}
	}

	if( !f ) {
		G_Printf("Lua: cannot verify fieldname %s.\n", fieldname);
		return 0;
	}

	if ( f->flags & FF_OFSENT ) {
		b = (byte *)ent;
	} else {		
		if( !ent->client ) {
			lua_pushnil(L);
			return 1;
		}
		b = (byte *)ent->client;
	}

	switch( f->type ) {
		case FT_BOOLEAN:
			lua_pushboolean (L, *(int *)(b+f->ofs));
			return 1;
		case FT_INT:
			if ( f->flags & FF_ARR )
				lua_pushinteger(L, *((int *)(b+f->ofs) + luaL_checkint(L, 3)));
			else
				lua_pushinteger(L, *(int *)(b+f->ofs));
			return 1;
		case FT_FLOAT:
			lua_pushnumber(L, *(float *)(b+f->ofs));
			return 1;
		case FT_STRING:
			if ( f->flags & FF_PTR )
				lua_pushstring(L, *(char **)(b+f->ofs));
			else
				lua_pushstring(L, (char *)(b+f->ofs));
			return 1;
		case FT_VECTOR:
			G_LuaPushVec3(L, *(vec3_t *)(b+f->ofs));
			return 1;
		case FT_TRAJECTORY:
			G_LuaPushTrajectory(L, (trajectory_t *)(b+f->ofs));
			return 1;
		case FT_WEAPONSTATS:
			G_LuaPushWeaponStats(L, (weapon_stat_t *)(b+f->ofs) + luaL_checkint(L, 3));
			return 1;
		case FT_ENTITY:
			lua_pushinteger(L, (*(gentity_t **)(b+f->ofs)) - g_entities);
			return 1;
	}

	return 0;
}

// et.gentity_set( entnum, Fieldname, arrayindex, (value) )
static int _et_gentity_set( lua_State *L )
{
	int i;
	const char *fieldname, *buf;
	byte	*b;
	gentity_t *ent;	
	luafield_t *f = NULL;

	ent = g_entities + luaL_checkint(L, 1);
	fieldname = luaL_checkstring(L, 2);

	if( !ent ) {
		lua_pushnil(L);
		return 1;
	}

	for( i = 0; luafields[i].name; i++ ) {
		if( !Q_stricmp( fieldname, luafields[i].name ) ) {
			f = (luafield_t *)&luafields[i];
		}
	}
	
	// break on invalid gentity field
	if ( !f ) {
		G_Printf("Lua: tried to set invalid gentity field %s.\n", fieldname);
		return 0;
	}

	// break on read-only gentity field
	if ( f->flags & FF_RO ) {
		G_Printf("Lua: tried to set read-only field %s.\n", fieldname);
		return 0;
	}
	
	if ( f->flags & FF_OFSENT ) {
		b = (byte *)ent;
	} else {		
		if( !ent->client ) {
			lua_pushnil(L);
			return 1;
		}
		b = (byte *)ent->client;
	}

	switch( f->type ) {
		case FT_BOOLEAN:
			*(qboolean *)(b+f->ofs) = luaL_checkint(L, 3);
			break;
		case FT_INT:
			if ( f->flags & FF_ARR )
				*((int *)(b+f->ofs) + luaL_checkint(L, 3)) = luaL_checkint(L, 4);
			else
				*(int *)(b+f->ofs) = luaL_checkint(L, 3);
			break;
		case FT_FLOAT:
			*(float *)(b+f->ofs) = (float)luaL_checknumber(L, 3);
			break;
		case FT_STRING:
			buf = luaL_checkstring(L, 3);
			if ( f->flags & FF_PTR ) {
				free(*(char **)(b+f->ofs));
				*(char **)(b+f->ofs) = malloc(strlen(buf));
				Q_strncpyz(*(char **)(b+f->ofs), buf, strlen(buf));
			} else {
				Q_strncpyz((char *)(b+f->ofs), buf, strlen((char *)(b+f->ofs)));
			}
			break;
		case FT_VECTOR:
			G_LuaSetVec3(L, (vec3_t *)(b+f->ofs));
			break;
		case FT_TRAJECTORY:
			G_LuaSetTrajectory(L, (trajectory_t *)(b+f->ofs));
			break;
		case FT_ENTITY:
			*(gentity_t **)(b+f->ofs) = g_entities + luaL_checkint(L, 3);
			break;
	}

	return 0;
}

// et.G_AddEvent( ent, event, eventparm )
static int _et_G_AddEvent( lua_State *L )
{
	int ent = luaL_checkint(L, 1);
	int event = luaL_checkint(L, 2);
	int eventparm = luaL_checkint(L, 3);
	G_AddEvent(g_entities + ent, event, eventparm);
	return 0;
}

/*
==============================================================
HELPER FUNCS
==============================================================
*/

void G_LuaPushTrajectory( lua_State *L, trajectory_t *traj )
{
	int index;

	lua_newtable(L);
	index = lua_gettop(L);
	lua_pushstring(L, "trTime");
	lua_pushinteger(L, traj->trTime);
	lua_settable(L, -3);
	lua_pushstring(L, "trType");
	lua_pushinteger(L, traj->trType);
	lua_settable(L, -3);
	lua_pushstring(L, "trDuration");
	lua_pushinteger(L, traj->trDuration);
	lua_settable(L, -3);
	lua_settop(L, index);
	lua_pushstring(L, "trBase");
	G_LuaPushVec3(L, traj->trBase);
	lua_settable(L, -3);
	lua_settop(L, index);
	lua_pushstring(L, "trDelta");
	G_LuaPushVec3(L, traj->trDelta);
	lua_settable(L, -3);
}

void G_LuaPushWeaponStats( lua_State *L, weapon_stat_t *ws )
{
	lua_newtable(L);
	lua_pushstring(L, "atts");
	lua_pushinteger(L, ws->atts);
	lua_settable(L, -3);
	lua_pushstring(L, "deaths");
	lua_pushinteger(L, ws->deaths);
	lua_settable(L, -3);
	lua_pushstring(L, "headshots");
	lua_pushinteger(L, ws->headshots);
	lua_settable(L, -3);
	lua_pushstring(L, "hits");
	lua_pushinteger(L, ws->hits);
	lua_settable(L, -3);
	lua_pushstring(L, "kills");
	lua_pushinteger(L, ws->kills);
	lua_settable(L, -3);
}

void G_LuaPushVec3( lua_State *L, vec3_t vec3 )
{
	lua_newtable(L);
	lua_pushnumber(L, vec3[0]);
	lua_rawseti(L, -2, 1);
	lua_pushnumber(L, vec3[1]);
	lua_rawseti(L, -2, 2);
	lua_pushnumber(L, vec3[2]);
	lua_rawseti(L, -2, 3);
}

void G_LuaSetTrajectory( lua_State *L, trajectory_t *traj )
{
	lua_pushstring(L, "trType");
	lua_gettable(L, -2);
	traj->trType = (float)lua_tonumber(L, -1);
	lua_pop(L, 1);
	lua_pushstring(L, "trTime");
	lua_gettable(L, -2);
	traj->trTime = (float)lua_tonumber(L, -1);
	lua_pop(L, 1);
	lua_pushstring(L, "trDuration");
	lua_gettable(L, -2);
	traj->trDuration = (float)lua_tonumber(L, -1);
	lua_pop(L, 1);
	lua_pushstring(L, "trBase");
	lua_gettable(L, -2);
	G_LuaSetVec3(L, (vec3_t *)traj->trBase);
	lua_pop(L, 1);
	lua_pushstring(L, "trDelta");
	lua_gettable(L, -2);
	G_LuaSetVec3(L, (vec3_t *)traj->trDelta);
	lua_pop(L, 1);
}

void G_LuaSetVec3( lua_State *L, vec3_t *vec3 )
{
	lua_pushnumber(L, 1);
	lua_gettable(L, -2);
	(*vec3)[0] = (float)lua_tonumber(L, -1);
	lua_pop(L, 1);
	lua_pushnumber(L, 2);
	lua_gettable(L, -2);
	(*vec3)[1] = (float)lua_tonumber(L, -1);
	lua_pop(L, 1);
	lua_pushnumber(L, 3);
	lua_gettable(L, -2);
	(*vec3)[2] = (float)lua_tonumber(L, -1);
	lua_pop(L, 1);
}

lua_vm_t *G_LuaGetVM( lua_State *L )
{
	int i;
	for (i = 0; i < LUA_NUM_VM; i++)
		if (lVM[i] && lVM[i]->L == L)
			return lVM[i];
	return NULL;
}

/*
==============================================================
CALLBACKS
==============================================================
*/

/*
--------------------------------------------------------------
QAGAME EXECUTION
--------------------------------------------------------------
*/

// et_InitGame( levelTime, randomSeed, restart )
void G_LuaHook_InitGame( int levelTime, int randomSeed, int restart )
{
	int i, err;
	lua_vm_t* vm;

	for (i = 0; i < LUA_NUM_VM; i++) {
		if (!lVM[i])
			continue;

		vm = lVM[i];
		lua_getglobal(vm->L, "et_InitGame");

		if (!lua_isfunction(vm->L, -1)) {
			lua_pop(vm->L, 1);
			continue;
		}

		// Arguments
		lua_pushinteger(vm->L, levelTime);
		lua_pushinteger(vm->L, randomSeed);
		lua_pushinteger(vm->L, restart);

		// Call
		err = lua_pcall(vm->L, 3, 0, 0);
		if(err)	G_LuaError(vm, err);
	}
}

// et_ShutdownGame( restart )
void G_LuaHook_ShutdownGame( int restart )
{
	int i, err;
	lua_vm_t* vm;

	for (i = 0; i < LUA_NUM_VM; i++) {
		if (!lVM[i])
			continue;

		vm = lVM[i];
		lua_getglobal(vm->L, "et_ShutdownGame");

		if (!lua_isfunction(vm->L, -1)) {
			lua_pop(vm->L, 1);
			continue;
		}

		// Arguments
		lua_pushinteger(vm->L, restart);

		// Call
		err = lua_pcall(vm->L, 1, 0, 0);
		if(err)	G_LuaError(vm, err);
	}
}

// et_RunFrame( levelTime )
void G_LuaHook_RunFrame( int levelTime )
{
	int i, err;
	lua_vm_t* vm;

	for (i = 0; i < LUA_NUM_VM; i++) {
		if (!lVM[i])
			continue;

		vm = lVM[i];
		lua_getglobal(vm->L, "et_RunFrame");

		if (!lua_isfunction(vm->L, -1)) {
			lua_pop(vm->L, 1);
			continue;
		}

		// Arguments
		lua_pushinteger(vm->L, levelTime);

		// Call
		err = lua_pcall(vm->L, 1, 0, 0);
		if(err)	G_LuaError(vm, err);
	}
}

// et_Quit()
void G_LuaHook_Quit( void )
{
	G_Printf("G_LuaHook_Quit\n");
}

/*
--------------------------------------------------------------
CLIENT MANAGEMENT
--------------------------------------------------------------
*/

// rejectreason = et_ClientConnect( clientNum, firstTime, isBot )
qboolean G_LuaHook_ClientConnect( int clientNum, qboolean firstTime, qboolean isBot, char *reason )
{
	int i, err;
	lua_vm_t* vm;

	for (i = 0; i < LUA_NUM_VM; i++) {
		if (!lVM[i])
			continue;

		vm = lVM[i];
		lua_getglobal(vm->L, "et_ClientConnect");

		if (!lua_isfunction(vm->L, -1)) {
			lua_pop(vm->L, 1);
			continue;
		}

		// Arguments
		lua_pushinteger(vm->L, clientNum);
		lua_pushinteger(vm->L, (int)firstTime);
		lua_pushinteger(vm->L, (int)isBot);

		// Call
		err = lua_pcall(vm->L, 3, 1, 0);
		if(err)	G_LuaError(vm, err);

		// Return values
		if (lua_isstring(vm->L, -1)) {
			Q_strncpyz(reason, lua_tostring(vm->L, -1), MAX_STRING_CHARS);
			return qtrue;
		}
	}

	return qfalse;
}

// et_ClientDisconnect( clientNum )
void G_LuaHook_ClientDisconnect( int clientNum )
{
	int i, err;
	lua_vm_t* vm;

	for (i = 0; i < LUA_NUM_VM; i++) {
		if (!lVM[i])
			continue;

		vm = lVM[i];
		lua_getglobal(vm->L, "et_ClientDisconnect");

		if (!lua_isfunction(vm->L, -1)) {
			lua_pop(vm->L, 1);
			continue;
		}

		// Arguments
		lua_pushinteger(vm->L, clientNum);

		// Call
		err = lua_pcall(vm->L, 1, 0, 0);
		if(err) G_LuaError(vm, err);
	}
}

// et_ClientBegin( clientNum )
void G_LuaHook_ClientBegin( int clientNum )
{
	int i, err;
	lua_vm_t* vm;

	for (i = 0; i < LUA_NUM_VM; i++) {
		if (!lVM[i])
			continue;

		vm = lVM[i];
		lua_getglobal(vm->L, "et_ClientBegin");

		if (!lua_isfunction(vm->L, -1)) {
			lua_pop(vm->L, 1);
			continue;
		}

		// Arguments
		lua_pushinteger(vm->L, clientNum);

		// Call
		err = lua_pcall(vm->L, 1, 0, 0);
		if(err)	G_LuaError(vm, err);
	}
}

// et_ClientUserinfoChanged( clientNum )
void G_LuaHook_ClientUserinfoChanged( int clientNum )
{
	int i, err;
	lua_vm_t* vm;

	for (i = 0; i < LUA_NUM_VM; i++) {
		if (!lVM[i])
			continue;

		vm = lVM[i];
		lua_getglobal(vm->L, "et_ClientUserinfoChanged");

		if (!lua_isfunction(vm->L, -1)) {
			lua_pop(vm->L, 1);
			continue;
		}

		// Arguments
		lua_pushinteger(vm->L, clientNum);

		// Call
		err = lua_pcall(vm->L, 1, 0, 0);
		if(err) G_LuaError(vm, err);
	}
}

// et_ClientSpawn( clientNum, revived )
void G_LuaHook_ClientSpawn( int clientNum, qboolean revived )
{
	int i, err;
	lua_vm_t* vm;

	for (i = 0; i < LUA_NUM_VM; i++) {
		if (!lVM[i])
			continue;

		vm = lVM[i];
		lua_getglobal(vm->L, "et_ClientSpawn");

		if (!lua_isfunction(vm->L, -1)) {
			lua_pop(vm->L, 1);
			continue;
		}

		// Arguments
		lua_pushinteger(vm->L, clientNum);
		lua_pushinteger(vm->L, (int)revived);

		// Call
		err = lua_pcall(vm->L, 2, 0, 0);
		if(err) G_LuaError(vm, err);
	}
}

/*
--------------------------------------------------------------
COMMANDS
--------------------------------------------------------------
*/

// intercepted = et_ClientCommand( clientNum, command )
qboolean G_LuaHook_ClientCommand( int clientNum, char *command )
{
	int i, err;
	lua_vm_t* vm;

	for (i = 0; i < LUA_NUM_VM; i++) {
		if (!lVM[i])
			continue;

		vm = lVM[i];
		lua_getglobal(vm->L, "et_ClientCommand");

		if (!lua_isfunction(vm->L, -1)) {
			lua_pop(vm->L, 1);
			continue;
		}

		// Arguments
		lua_pushinteger(vm->L, clientNum);
		lua_pushstring(vm->L, command);

		// Call
		err = lua_pcall(vm->L, 2, 1, 0);
		if(err) G_LuaError(vm, err);		

		// Return values
		if (lua_isnumber(vm->L, -1)) {
			if (lua_tointeger(vm->L, -1) == 1) {
				return qtrue;
			}
		}
	}

	return qfalse;
}

// intercepted = et_ConsoleCommand()
qboolean G_LuaHook_ConsoleCommand( char *command )
{
	int i, err;
	lua_vm_t* vm;

	for (i = 0; i < LUA_NUM_VM; i++) {
		if (!lVM[i])
			continue;

		vm = lVM[i];
		lua_getglobal(vm->L, "et_ConsoleCommand");

		if (!lua_isfunction(vm->L, -1)) {
			lua_pop(vm->L, 1);
			continue;
		}

		// Arguments
		lua_pushstring(vm->L, command);

		// Call
		err = lua_pcall(vm->L, 1, 1, 0);
		if(err) G_LuaError(vm, err);

		// Return values
		if (lua_isnumber(vm->L, -1)) {
			if (lua_tointeger(vm->L, -1) == 1) {
				return qtrue;
			}
		}
	}

	return qfalse;
}

/*
--------------------------------------------------------------
XP
--------------------------------------------------------------
*/

// result = et_UpgradeSkill( cno, skill )
qboolean G_LuaHook_UpgradeSkill( int cno, skillType_t skill )
{
	int i, err;
	lua_vm_t* vm;

	for (i = 0; i < LUA_NUM_VM; i++) {
		if (!lVM[i])
			continue;

		vm = lVM[i];
		lua_getglobal(vm->L, "et_UpgradeSkill");

		if (!lua_isfunction(vm->L, -1)) {
			lua_pop(vm->L, 1);
			continue;
		}

		// Arguments
		lua_pushinteger(vm->L, cno);
		lua_pushinteger(vm->L, (int)skill);

		// Call
		err = lua_pcall(vm->L, 2, 1, 0);
		if(err) G_LuaError(vm, err);

		// Return values
		if (lua_isnumber(vm->L, -1)) {
			if (lua_tointeger(vm->L, -1) == -1) {
				return qtrue;
			}
		}
	}

	return qfalse;
}

// result = et_SetPlayerSkill( cno, skill )
qboolean G_LuaHook_SetPlayerSkill( int cno, skillType_t skill )
{
	int i, err;
	lua_vm_t* vm;

	for (i = 0; i < LUA_NUM_VM; i++) {
		if (!lVM[i])
			continue;

		vm = lVM[i];
		lua_getglobal(vm->L, "et_SetPlayerSkill");

		if (!lua_isfunction(vm->L, -1)) {
			lua_pop(vm->L, 1);
			continue;
		}

		// Arguments
		lua_pushinteger(vm->L, cno);
		lua_pushinteger(vm->L, skill);

		// Call
		err = lua_pcall(vm->L, 2, 1, 0);
		if(err) G_LuaError(vm, err);

		// Return values
		if (lua_isnumber(vm->L, -1)) {
			if (lua_tointeger(vm->L, -1) == -1) {
				return qtrue;
			}
		}
	}

	return qfalse;
}

/*
--------------------------------------------------------------
MISCELLANEOUS
--------------------------------------------------------------
*/

// et_Print( text )
void G_LuaHook_Print( char *text )
{
	int i, err;
	lua_vm_t *vm;
	for (i = 0; i < LUA_NUM_VM; i++) {
		if (!lVM[i])
			continue;

		vm = lVM[i];

		lua_getglobal(vm->L, "et_Print");

		if (!lua_isfunction(vm->L, -1)) {
			lua_pop(vm->L, 1);
			continue;
		}

		// Arguments
		lua_pushstring(vm->L, text);

		// Call
		err = lua_pcall(vm->L, 1, 0, 0);
		if(err) G_LuaError(vm, err);
	}
}

// et_Obituary( victim, killer, meansOfDeath )
void G_LuaHook_Obituary( int victim, int killer, int meansOfDeath )
{
	int i, err;
	lua_vm_t *vm;

	for (i = 0; i < LUA_NUM_VM; i++) {
		if (!lVM[i])
			continue;

		vm = lVM[i];
		lua_getglobal(vm->L, "et_Obituary");

		if (!lua_isfunction(vm->L, -1)) {
			lua_pop(vm->L, 1);
			continue;
		}

		// Arguments
		lua_pushinteger(vm->L, victim);
		lua_pushinteger(vm->L, killer);
		lua_pushinteger(vm->L, meansOfDeath);

		// Call
		err = lua_pcall(vm->L, 3, 0, 0);
		if(err) G_LuaError(vm, err);
	}
}

/*
==============================================================
LUA MANAGEMENT
==============================================================
*/

qboolean G_LuaInit( void )
{
	int i, len;
	char buf[MAX_CVAR_VALUE_STRING], *vmname;

	if (!lua_modules.string[0])
		return qtrue;

	for (i = 0; i < LUA_NUM_VM; i++)
		lVM[i] = NULL;

	Q_strncpyz(buf, lua_modules.string, sizeof(buf));
	len = strlen(buf);
	vmname = buf;

	for (i = 0; i <= len; i++) {
		if (buf[i] == ' ' || buf[i] == '\0') {
			buf[i] = '\0';

			G_LuaStartVM(vmname);

			// prepare for next iteration
			if( i + 1 < len)
				vmname = buf + i + 1;
		}
	}

	return qtrue;
}

void G_LuaShutdown( void )
{
	int i;	

	for(i = 0; i < LUA_NUM_VM; i++) {
		if(!lVM[i])
			continue;
		G_LuaStopVM(lVM[i]);
	}
}

void G_LuaStartVM( const char *vmname )
{
	int i, err, len = 0;
	char *sha1, *code;	
	fileHandle_t f;
	lua_vm_t *vm;

	// find next free slot
	for(i = 0; lVM[i] && i < LUA_NUM_VM; i++);

	if(i >= LUA_NUM_VM) {
		G_Printf("Lua: no free slots for lua module %s.\n", vmname);
		return;
	}

	// check for boundaries and add sha1 signature
	len = trap_FS_FOpenFile(vmname, &f, FS_READ);
	if (len < 0) {
		G_Printf("Lua: cannot open file %s\n", vmname);
		return;
	} else if (len > LUA_MAX_FSIZE) {		
		G_Printf("Lua: ignoring file %s (too big)\n", vmname);
		trap_FS_FCloseFile(f);
		return;
	} else {
		code = malloc(len + 1);
		trap_FS_Read(code, len, f);
		*(code + len) = '\0';
		trap_FS_FCloseFile(f);
		sha1 = G_SHA1(code);
		free(code);
	}

	// don't load disallowed lua modules
	if ( Q_stricmp(lua_allowedModules.string, "") &&
		 !strstr(lua_allowedModules.string, sha1) ) {		
		G_Printf("Lua: disallowed lua module [%s] [%s]\n", vmname, sha1);
		return;
	}

	// allocate some space
	vm = (lua_vm_t *) malloc(sizeof(lua_vm_t));
	if(vm) {
		lVM[i] = vm;
	} else {
		G_Printf("Lua: malloc failed. Cannot load %s.\n", vmname);
		return;
	}

	// constructor
	vm->L = luaL_newstate();
	vm->id = i;
	Q_strncpyz(vm->modname, vmname, sizeof(vm->modname));
	Q_strncpyz(vm->filename, vmname, sizeof(vm->filename));
	Q_strncpyz(vm->sha1, sha1, sizeof(vm->sha1));

	// open standard libs
	luaL_openlibs(vm->L);
	
	// load lua chunk
	err = luaL_loadfile(vm->L, vmname);
	if (err) {
		G_Printf("Lua: cannot load file %s.\n", lua_tostring(vm->L, -1));
		G_LuaStopVM(vm);
		return;
	}

	// pre-compile lua chunk
	err = lua_pcall(vm->L, 0, 0, 0);
	if (err) {
		G_Printf("Lua: cannot pre-compile file %s (err. %i).\n", vmname, err );
		G_LuaStopVM(vm);
		return;
	}

	// register predefined constants
	G_LuaRegConstants(vm);

	// register library
	luaL_register(vm->L, "et", etlib);

	G_Printf("Lua: successful init of module %s.\n", vmname);
	return;
}

void G_LuaStopVM( lua_vm_t *vm )
{
	if (!vm)
		return;

	if (vm->L) {
		lua_close(vm->L);
		vm->L = NULL;
	}

	lVM[vm->id] = NULL;
	free(vm);
}

void G_LuaRegConstants( lua_vm_t *vm )
{	
	lua_newtable( vm->L );
	lua_regconstinteger( vm->L, CS_PLAYERS );
	lua_regconstinteger( vm->L, EXEC_NOW );
	lua_regconstinteger( vm->L, EXEC_INSERT );
	lua_regconstinteger( vm->L, EXEC_APPEND );
	lua_regconstinteger( vm->L, FS_READ );
	lua_regconstinteger( vm->L, FS_WRITE );
	lua_regconstinteger( vm->L, FS_APPEND );
	lua_regconstinteger( vm->L, FS_APPEND_SYNC );
	lua_regconstinteger( vm->L, SAY_ALL );
	lua_regconstinteger( vm->L, SAY_TEAM );
	lua_regconstinteger( vm->L, SAY_BUDDY );
	lua_regconstinteger( vm->L, SAY_TEAMNL );
	lua_regconststring(vm->L, HOSTARCH );
	lua_setglobal( vm->L, "et" );
}

void G_LuaError( lua_vm_t *vm, int err )
{
	if (err == LUA_ERRRUN) {		
		G_Printf("Lua: error running lua script: %s\n", lua_tostring(vm->L, -1));
		lua_pop(vm->L, 1);
	} else if (err == LUA_ERRMEM) {
		G_Printf("Lua: memory allocation error #2 ( %s )\n", vm->filename);
	} else if (err == LUA_ERRERR) {
		G_Printf("Lua: traceback error ( %s )\n", vm->filename);
	}
}

void G_LuaStatus(gentity_t *ent)
{
	int i, cnt = 0;
	for (i=0; i<LUA_NUM_VM; i++)
		if (lVM[i])
			cnt++;
	
	if (cnt == 0) {
		G_refPrintf(ent, "Lua: no scripts loaded.");
		return;
	} else if (cnt == 1) {
		G_refPrintf(ent, "Lua: showing lua information ( 1 module loaded )");
	} else {
		G_refPrintf(ent, "Lua: showing lua information ( %d modules loaded )", cnt);
	}
	G_refPrintf(ent, "--------------------------------------------------------");
	for (i=0; i<LUA_NUM_VM; i++) {
		if (lVM[i]) {			
			G_refPrintf(ent, "VM slot\t\t%d\nModname\t\t%s\nFilename\t%s\nSignature\t%s",
				lVM[i]->id,
				lVM[i]->modname,
				lVM[i]->filename,
				lVM[i]->sha1);
			G_refPrintf(ent, "--------------------------------------------------------");
		}
	}	
}

#endif
