#ifdef XPSAVE_SUPPORT

#include "g_local.h"

// we put them here to avoid them being placed on stack
static char xpbuf_userinfo[MAX_INFO_STRING];
static xpsaveData_t xpbuf_xpdata;
static char xpsave_errmsg[MAX_INFO_STRING] = { 0 }; 

static void G_XPSave_LogLastError( const char *errmsg )
{
	Q_strncpyz( xpsave_errmsg, errmsg, sizeof(xpsave_errmsg) );
	//Q_strcat( xpsave_errmsg, sizeof( xpsave_errmsg ), va( " - level.time: %i", level.time ) );
}

void G_XPSave_PrintLastError( void )
{	
	G_LogPrintf( "%s\n", xpsave_errmsg );
}

static void G_XPSave_UpdateClientXP( gclient_t *cl )
{
	int i, j, cnt, oldskill, highestskill;

	if( !cl ) {
		return;
	}

	// update skills
	for( i = 0; i < SK_NUM_SKILLS; i++ ) {
		oldskill = cl->sess.skill[i];
		for( j = NUM_SKILL_LEVELS - 1; j >= 0; j-- ) {
			if( cl->sess.skillpoints[i] >= skillLevels[j] ) {
				cl->sess.skill[i] = j;
				break;
			}
		}
	}

	// update scoreboard info
	for( cl->ps.persistant[PERS_SCORE] = 0, i = 0; i < SK_NUM_SKILLS; i++ ) {
		cl->ps.persistant[PERS_SCORE] += cl->sess.skillpoints[i];
	}

	// update rank
	highestskill = 0;
	for( i = 0; i < SK_NUM_SKILLS; i++ ) {
		if( cl->sess.skill[i] > highestskill ) {
			highestskill = cl->sess.skill[i];
		}
	}

	cl->sess.rank = highestskill;

	if( cl->sess.rank >=4 ) {
		cnt = 0;

		for( i = 0; i < SK_NUM_SKILLS; i++ ) {
			if( cl->sess.skill[ i ] >= 4 ) {
				cnt++;
			}
		}

		cl->sess.rank = cnt + 3;
		if( cl->sess.rank > 10 ) {
			cl->sess.rank = 10;
		}
	}	
}

// forcecreates a new database
int G_XPSave_SVCreateNewDB( void )
{
	char		*dbpath;
	char		fsgame[MAX_QPATH];
	char		homepath[MAX_OSPATH];
	int			err;

	// close the old if there is any
	err = G_DB_DeInit();
	if( err ) {
		if( err == DB_ACCESS_NONINIT ) {
			// do nothing
		} else {
			return err;
		}
	}

	if( g_dbpath.string ) {
		dbpath = va( "%s", g_dbpath.string );
	} else {
		trap_Cvar_VariableStringBuffer( "fs_game", fsgame, sizeof( fsgame ) );
		trap_Cvar_VariableStringBuffer( "fs_homepath", homepath, sizeof( homepath ) );
		dbpath = va( "%s%c%s%c%s", homepath, PATH_SEP, fsgame, PATH_SEP, DB_STANDARD_FILENAME );
	}

	err = G_XPSave_CreateNewDB( dbpath, qtrue );
	if( err ) {
		return err;
	}

	G_LogPrintf("XPSave: new database created\n");

	err = G_DB_Init();
	if( err ) {
		return err;
	}

	return 0;
}

// creates a new xpsave database
// if force is specified, old tables are deleted if exist
int G_XPSave_CreateNewDB( char *dbpath, qboolean force )
{
	int			err;
	sqlite3		*db;

	if( dbpath == NULL || dbpath[0] == '\0' ) {
		G_XPSave_LogLastError( "G_XPSave_CreateNewDB: invalid path specified" );
		return XPSAVE_INVALID_PATH;
	}

	if( err = sqlite3_open( dbpath, &db ) ) {
		if( force ) {
			// do nothing
		} else {
			G_XPSave_LogLastError( va( "G_XPSave_CreateNewDB: sqlite3_open failed with error code %i", err ) );
			return err;
		}
	}

	if( force ) {
		if( err = sqlite3_exec( db, XPSAVE_SQLWRAP_DROP, NULL, NULL, NULL ) ) {		
			G_XPSave_LogLastError( va( "G_XPSave_CreateNewDB: sqlite3_exec drop failed with error code %i", err ) );
			sqlite3_close( db );
			return err;
		}
	}

	if( err = sqlite3_exec( db, XPSAVE_SQLWRAP_CREATE, NULL, NULL, NULL ) ) {
		G_XPSave_LogLastError( va( "G_XPSave_CreateNewDB: sqlite3_exec create failed with error code %i", err ) );
		sqlite3_close( db );
		return err;
	}

	if( err = sqlite3_close( level.database.db ) ) {
		G_XPSave_LogLastError( va( "G_XPSave_CreateNewDB: sqlite3_close failed with error code %i", err ) );
		return err;
	}

	G_LogPrintf( "XPSave: new database created at path '%s'\n", dbpath );

	return 0;
}

// checks if db exists, table exists, table is correct
int G_XPSave_CheckDBSanity( char *dbpath )
{
	int			err;
	sqlite3		*db;

	if( !dbpath || dbpath[0] == '\0' ) {
		G_XPSave_LogLastError( "G_XPSave_CheckDBSanity: invalid path specified" );
		return XPSAVE_INVALID_PATH;
	}

	// check if database can be opened
	err = sqlite3_open_v2( dbpath, &db, SQLITE_OPEN_READONLY, NULL );
	if( err ) {
		G_XPSave_LogLastError( va( "G_XPSave_CheckDBSanity: sqlite3_open_v2 failed with error code %i", err ) );
		return XPSAVE_DB_READ;
	} else {
		if( err = sqlite3_close( db ) ) {
			G_XPSave_LogLastError( va( "G_XPSave_CheckDBSanity: sqlite3_close failed with error code %i", err ) );
			return err;
		}
	}

	// db can be opened, now check if table exists
	if( err = sqlite3_open( dbpath, &db ) ) { // this check is most likely redundant
		G_XPSave_LogLastError( va( "G_XPSave_CheckDBSanity: sqlite3_open failed with error code %i", err ) );
		return err;
	}

	if( err = sqlite3_exec( db, XPSAVE_SQLWRAP_CHECK_IF_TABLE_EXIST, NULL, NULL, NULL ) ) {		
		G_XPSave_LogLastError( va( "G_XPSave_CheckDBSanity: sqlite3_exec failed with error code %i", err ) );
		if( err = sqlite3_close( db ) ) {
			G_XPSave_LogLastError( va( "G_XPSave_CheckDBSanity: sqlite3_close failed with error code %i", err ) );
			return err;
		}
		return XPSAVE_TABLE_NOTEXIST;
	}

	// db and table exist, now check schema
	if( err = sqlite3_exec( db, XPSAVE_SQLWRAP_CHECK_IF_TABLE_CORRECT, NULL, NULL, NULL ) ) {		
		G_XPSave_LogLastError( va( "G_XPSave_CheckDBSanity: sqlite3_exec failed with error code %i", err ) );
		if( err = sqlite3_close( db ) ) {
			G_XPSave_LogLastError( va( "G_XPSave_CheckDBSanity: sqlite3_close failed with error code %i", err ) );
			return err;
		}
		return XPSAVE_TABLE_INCORRECT;
	}

	// all ok
	if( err = sqlite3_close( db ) ) {
		G_XPSave_LogLastError( va( "G_XPSave_CheckDBSanity: sqlite3_close failed with error code %i", err ) );
		return err;
	}

	return 0;
}

// gets xp from database based on guid, outputs filled xpdata
static int G_XPSave_GetXP( char *guid, xpsaveData_t *xpdata )
{
	char			*sqlstmt;
	int				err;
	sqlite3_stmt	*sqlitestmt;

	if( !(level.database.initialized & XPSAVE_INITIALZE_FLAG) ) {
		G_XPSave_LogLastError( "G_XPSave_GetXP: access to non-initialized database" );
		return DB_ACCESS_NONINIT;
	}

	if( !G_CheckGUIDSanity( guid, qtrue ) ) {
		G_XPSave_LogLastError( "G_XPSave_GetXP: guid failed" );
		return XPSAVE_INVALID_GUID;
	}

	sqlstmt = va( XPSAVE_SQLWRAP_SELECT, guid );
	if( err = sqlite3_prepare( level.database.db, sqlstmt, strlen(sqlstmt), &sqlitestmt, NULL ) ) {		
		G_XPSave_LogLastError( va( "G_XPSave_GetXP: sqlite3_prepare failed with error code %i", err ) );
		return err;
	}

	err = sqlite3_step(sqlitestmt);
	if( err == SQLITE_ROW ) {
		xpdata->skillpoints[SK_BATTLE_SENSE] = (float)sqlite3_column_int(sqlitestmt, 1);
		xpdata->skillpoints[SK_EXPLOSIVES_AND_CONSTRUCTION] = (float)sqlite3_column_int(sqlitestmt, 2);
		xpdata->skillpoints[SK_FIRST_AID] = (float)sqlite3_column_int(sqlitestmt, 3);
		xpdata->skillpoints[SK_SIGNALS] = (float)sqlite3_column_int(sqlitestmt, 4);
		xpdata->skillpoints[SK_LIGHT_WEAPONS] = (float)sqlite3_column_int(sqlitestmt, 5);
		xpdata->skillpoints[SK_HEAVY_WEAPONS] = (float)sqlite3_column_int(sqlitestmt, 6);
		xpdata->skillpoints[SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS] = (float)sqlite3_column_int(sqlitestmt, 7);
	} else {
		// two cases: either no db entry found (legit), or fail of other reason
		if( err == SQLITE_DONE ) {
			xpdata->skillpoints[SK_BATTLE_SENSE] = 0;
			xpdata->skillpoints[SK_EXPLOSIVES_AND_CONSTRUCTION] = 0;
			xpdata->skillpoints[SK_FIRST_AID] = 0;
			xpdata->skillpoints[SK_SIGNALS] = 0;
			xpdata->skillpoints[SK_LIGHT_WEAPONS] = 0;
			xpdata->skillpoints[SK_HEAVY_WEAPONS] = 0;
			xpdata->skillpoints[SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS] = 0;
		} else {
			sqlite3_finalize(sqlitestmt);
			G_XPSave_LogLastError( va( "G_XPSave_GetXP: sqlite3_step failed with error code %i", err )  );
			return err;
		}
	}

	if( err = sqlite3_finalize(sqlitestmt) ) {
		G_XPSave_LogLastError( va( "G_XPSave_GetXP: sqlite3_finalize failed with error code %i", err ) );
		return err;
	}

	return 0;
}

// sets xp in database based on guid and xpdata
static int G_XPSave_SetXP( char *guid, xpsaveData_t *xpdata )
{
	char			*sqlstmt;
	int				err;
	sqlite3_stmt	*sqlitestmt;

	if( !(level.database.initialized & XPSAVE_INITIALZE_FLAG) ) {
		G_XPSave_LogLastError( "G_XPSave_SetXP: access to non-initialized database" );
		return DB_ACCESS_NONINIT;
	}

	if( !G_CheckGUIDSanity( guid, qtrue ) ) {
		G_XPSave_LogLastError( "G_XPSave_SetXP: guid failed" );
		return XPSAVE_INVALID_GUID;
	}

	sqlstmt = va( XPSAVE_SQLWRAP_SELECT, guid );
	sqlite3_prepare( level.database.db, sqlstmt, strlen(sqlstmt), &sqlitestmt, NULL );

	// TODO check statements
	err = sqlite3_step(sqlitestmt);
	if( err == SQLITE_DONE ) {
		sqlstmt = va( XPSAVE_SQLWRAP_INSERT,
			guid,
			(int)xpdata->skillpoints[SK_BATTLE_SENSE],
			(int)xpdata->skillpoints[SK_EXPLOSIVES_AND_CONSTRUCTION],
			(int)xpdata->skillpoints[SK_FIRST_AID],
			(int)xpdata->skillpoints[SK_SIGNALS],
			(int)xpdata->skillpoints[SK_LIGHT_WEAPONS],
			(int)xpdata->skillpoints[SK_HEAVY_WEAPONS],
			(int)xpdata->skillpoints[SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS]);

		err = sqlite3_exec(level.database.db, sqlstmt, NULL, NULL, NULL);
		if( err ) {
			G_XPSave_LogLastError( va( "G_XPSave_SetXP: sqlite3_exec::1 failed with error code %i", err ) );
			return err;
		}
	} else {
		sqlstmt = va( XPSAVE_SQLWRAP_UPDATE,
			(int)xpdata->skillpoints[SK_BATTLE_SENSE],
			(int)xpdata->skillpoints[SK_EXPLOSIVES_AND_CONSTRUCTION],
			(int)xpdata->skillpoints[SK_FIRST_AID],
			(int)xpdata->skillpoints[SK_SIGNALS],
			(int)xpdata->skillpoints[SK_LIGHT_WEAPONS],
			(int)xpdata->skillpoints[SK_HEAVY_WEAPONS],
			(int)xpdata->skillpoints[SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS],
			guid );

		err = sqlite3_exec(level.database.db, sqlstmt, NULL, NULL, NULL);
		if( err ) {			
			G_XPSave_LogLastError( va( "G_XPSave_SetXP: sqlite3_exec::2 failed with error code %i", err ) );
			return err;
		}
	}

	err = sqlite3_finalize(sqlitestmt);
	if( err ) {
		G_XPSave_LogLastError( va( "G_XPSave_SetXP: sqlite3_finalize failed with error code %i", err ) );
		return err;
	}

	return 0;
}

int G_XPSave_LoadClientXP( gclient_t *cl )
{
	char			*guid;
	int				i, err, clientNum;

	if( !(level.database.initialized & XPSAVE_INITIALZE_FLAG) ) {
		G_XPSave_LogLastError( "G_XPSave_LoadClientXP: access to non-initialized database" );
		return DB_ACCESS_NONINIT;
	}

	if( !cl ) {
		G_XPSave_LogLastError( "G_XPSave_LoadClientXP: null client" );
		return XPSAVE_CLIENT_NULL;
	}

	if( cl->pers.xpsave_loaded == qtrue ) {
		G_XPSave_LogLastError( "G_XPSave_LoadClientXP: already loaded" );
		return XPSAVE_LOAD_LOAD;
	}

	clientNum = cl - level.clients;

	trap_GetUserinfo( clientNum, xpbuf_userinfo, sizeof( xpbuf_userinfo ) );
	guid = Info_ValueForKey ( xpbuf_userinfo, "cl_guid" );

	err = G_XPSave_GetXP( guid, &xpbuf_xpdata );
	if( err ) {		
		return err;
	} else {
		for( i = 0; i < SK_NUM_SKILLS; i++ ) {
			cl->sess.skillpoints[i] = xpbuf_xpdata.skillpoints[i];
		}
		G_XPSave_UpdateClientXP( cl );
		ClientUserinfoChanged( clientNum );
	}

	cl->pers.xpsave_loaded = qtrue;

	G_LogPrintf( "XPSave: loaded client: %i %s\n", clientNum, cl->pers.netname );

	return 0;
}

int G_XPSave_SaveClientXP( gclient_t *cl )
{
	char			*guid;
	int				i, err, clientNum;

	if( !(level.database.initialized & XPSAVE_INITIALZE_FLAG) ) {
		G_XPSave_LogLastError( "G_XPSave_SaveClientXP: access to non-initialized database" );
		return DB_ACCESS_NONINIT;
	}

	if( !cl ) {
		G_XPSave_LogLastError( "G_XPSave_SaveClientXP: null client" );
		return XPSAVE_CLIENT_NULL;
	}

	if( cl->pers.xpsave_loaded == qfalse ) {
		G_XPSave_LogLastError( "G_XPSave_SaveClientXP: not loaded" );
		return XPSAVE_SAVE_UNLOAD;
	}

	clientNum = cl - level.clients;

	trap_GetUserinfo( clientNum, xpbuf_userinfo, sizeof( xpbuf_userinfo ) );
	guid = Info_ValueForKey ( xpbuf_userinfo, "cl_guid" );

	for( i = 0; i < SK_NUM_SKILLS; i++ ) {
		xpbuf_xpdata.skillpoints[i] = cl->sess.skillpoints[i];
	}	

	err = G_XPSave_SetXP( guid, &xpbuf_xpdata );
	if( err ) {
		return err;
	}

	G_LogPrintf( "XPSave: saved client: %i %s\n", clientNum, cl->pers.netname );

	return 0;
}

void G_XPSave_WriteSessionData( void )
{
	int				i, err;
	gentity_t		*ent;

	if( !(level.database.initialized & XPSAVE_INITIALZE_FLAG) ) {
		G_LogPrintf( "G_XPSave_WriteSessionData: access to non-initialized database" );
		return;
	}

	for( i = 0; i < level.numConnectedClients; i++ ) {
		ent = &g_entities[level.sortedClients[i]];
		if( !ent ) {
			continue;
		}

		err = G_XPSave_SaveClientXP( ent->client );
		if( err ) {
			G_XPSave_PrintLastError();
		}
	}

	G_LogPrintf( "XPSave: Wrote Session Data\n" );
}

// TODO:

// suppress save on bots error message
// size on database, impose limited db space
// clean up function based on age
// check prepare statements
// more server console administration tools
// quit on server console does not saved xp

#endif // XPSAVE_SUPPORT