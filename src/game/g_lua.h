
#ifdef LUA_SUPPORT

/*
===========================================================================

File:	g_lua.h
Desc:	Lua version 5.1 support for the acqu-sdk

This code is based on etpub's implementation of Lua: http://www.assembla.com/code/etpub/subversion/nodes

Lua lib location rel. to the top directory: ./lua/lua5.1/lib/static
Lua lib obtained from: http://luabinaries.sourceforge.net/

Targeting bi-directional compatibility to etpro: http://wolfwiki.anime.net/index.php/ETPro:Lua_Mod_API

===========================================================================
*/

#include "g_local.h"

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

typedef struct {
	int			id;
	lua_State 	*L;
	char		modname[MAX_QPATH];
	char		filename[MAX_QPATH];
	char		sha1[41];
} lua_vm_t;

#define LUA_NUM_VM 16
#define LUA_MAX_FSIZE 1024*1024

extern lua_vm_t *lVM[LUA_NUM_VM];

// callbacks
void G_LuaHook_InitGame( int levelTime, int randomSeed, int restart );
void G_LuaHook_ShutdownGame( int restart );
void G_LuaHook_RunFrame( int levelTime );
void G_LuaHook_Quit( void );
qboolean G_LuaHook_ClientConnect( int clientNum, qboolean firstTime, qboolean isBot, char *reason );
void G_LuaHook_ClientDisconnect( int clientNum );
void G_LuaHook_ClientBegin( int clientNum );
void G_LuaHook_ClientUserinfoChanged( int clientNum );
void G_LuaHook_ClientSpawn( int clientNum, qboolean revived );
qboolean G_LuaHook_ClientCommand( int clientNum, char *command );
qboolean G_LuaHook_ConsoleCommand( char *command );
qboolean G_LuaHook_UpgradeSkill( int cno, skillType_t skill );
qboolean G_LuaHook_SetPlayerSkill( int cno, skillType_t skill );
void G_LuaHook_Print( char *text );
void G_LuaHook_Obituary( int victim, int killer, int meansOfDeath );

// lua management
qboolean G_LuaInit( void );
void G_LuaShutdown( void );
void G_LuaStatus(gentity_t *ent);

#endif
