#ifdef XPSAVE_SUPPORT

#include "g_local.h"
#include "sqlite3.h"

#define XPSAVE_DB_FILENAME "et.db"

#define XPSAVE_SQLWRAP_CREATE "CREATE TABLE etdb_table " \
	"( "						\
	"guid text primary key, "	\
	"B integer, "				\
	"E integer, "				\
	"F integer, "				\
	"S integer, "				\
	"L integer, "				\
	"H integer, "				\
	"I integer );"


#define XPSAVE_SQLWRAP_INSERT "INSERT INTO etdb_table " \
	"( "						\
	"guid, "					\
	"B, "						\
	"E, "						\
	"F, "						\
	"S, "						\
	"L, "						\
	"H, "						\
	"I "						\
	") "						\
	"VALUES ('%s', '%i', '%i', '%i', '%i', '%i', '%i', '%i');"

#define XPSAVE_SQLWRAP_UPDATE "UPDATE etdb_table " \
	"SET "						\
	"B = '%i', "				\
	"E = '%i', "				\
	"F = '%i', "				\
	"S = '%i', "				\
	"L = '%i', "				\
	"H = '%i', "				\
	"I = '%i' "					\
	"WHERE "					\
	"guid = '%s';"

#define XPSAVE_SQLWRAP_SELECT "SELECT * FROM etdb_table WHERE guid = '%s';"

static char dbospath[MAX_OSPATH];
static qboolean dbinitialized = qfalse;

int G_XPSave_InitDatabase( void )
{
	int		err;
	char	*dbpath;
	char	fsgame[MAX_QPATH];
	char	homepath[MAX_OSPATH];
	sqlite3	*db;

	if( dbinitialized ) {
		return 0;
	}

	trap_Cvar_VariableStringBuffer( "fs_game", fsgame, sizeof( fsgame ) );
	trap_Cvar_VariableStringBuffer( "fs_homepath", homepath, sizeof( homepath ) );
	dbpath = va( "%s%c%s%c%s", homepath, PATH_SEP, fsgame, PATH_SEP, XPSAVE_DB_FILENAME );

	Q_strncpyz( dbospath, dbpath, MAX_OSPATH );

	err = sqlite3_open_v2(dbospath, &db, SQLITE_OPEN_READONLY, NULL);
	sqlite3_close(db);

	if( !err ) {
		dbinitialized = qtrue;
		return 0;
	}

	if( err = sqlite3_open(dbospath, &db) ) {
		G_Printf("XPSave: can not open database (err: %i)\n", err);
		return 1;
	}

	if( err = sqlite3_exec(db, XPSAVE_SQLWRAP_CREATE, NULL, NULL, NULL) ) {
		G_Printf("XPSave: sqlite3_exec (err: %i)\n", err);
		sqlite3_close(db);
		return 1;
	}

	if( err = sqlite3_close(db) ) {
		G_Printf("XPSave: sqlite3_close (err: %i)\n", err);
		return 1;
	}

	dbinitialized = qtrue;
	G_Printf("XPSave: database created on path '%s'\n", dbospath);
	return 0;
}

void G_XPSave_ReadClient( gentity_t *ent, const char *userinfo )
{
	char			*guid, *sqlstmt;
	int				i, j, cnt, err, oldskill, highestskill;
	sqlite3			*db;
	sqlite3_stmt	*sqlitestmt;
	gclient_t		*cl = ent->client;

	if( !dbinitialized ) {
		if( err = G_XPSave_InitDatabase() ) {
			G_Printf("XPSave: can not initialize database\n", err);
			return;
		}
	}

	if( err = sqlite3_open( dbospath, &db ) ) {
		G_Printf("XPSave: can not open database (err: %i)\n", err);
		return;
	}

	guid = Info_ValueForKey ( userinfo, "cl_guid" );
	G_Printf("XPSave: Reading data for guid %s\n", guid);

	sqlstmt = va( XPSAVE_SQLWRAP_SELECT, guid );
	if( err = sqlite3_prepare( db, sqlstmt, strlen(sqlstmt), &sqlitestmt, NULL ) ) {
		G_Printf("XPSave: sqlite3_prepare (err: %i)\n", err);
		sqlite3_close(db);
	}

	if( err = sqlite3_step(sqlitestmt) != SQLITE_DONE ) {
		cl->sess.skillpoints[SK_BATTLE_SENSE] = (float)sqlite3_column_int(sqlitestmt, 1);
		cl->sess.skillpoints[SK_EXPLOSIVES_AND_CONSTRUCTION] = (float)sqlite3_column_int(sqlitestmt, 2);
		cl->sess.skillpoints[SK_FIRST_AID] = (float)sqlite3_column_int(sqlitestmt, 3);
		cl->sess.skillpoints[SK_SIGNALS] = (float)sqlite3_column_int(sqlitestmt, 4);
		cl->sess.skillpoints[SK_LIGHT_WEAPONS] = (float)sqlite3_column_int(sqlitestmt, 5);
		cl->sess.skillpoints[SK_HEAVY_WEAPONS] = (float)sqlite3_column_int(sqlitestmt, 6);
		cl->sess.skillpoints[SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS] = (float)sqlite3_column_int(sqlitestmt, 7);
	} else {
		G_Printf("XPSave: sqlite3_step (err: %i)\n", err);
	}

	if( err = sqlite3_finalize(sqlitestmt) ) {
		G_Printf("XPSave: sqlite3_finalize (err: %i)\n", err);
	}

	sqlite3_close(db);

	// update skill levels
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

	//ClientUserinfoChanged( ent-g_entities );
}

void G_XPSave_WriteBackClient( gentity_t *ent )
{
	char			*guid, *sqlstmt;
	char			userinfo[MAX_INFO_STRING];
	int				i, err;
	sqlite3			*db;
	sqlite3_stmt	*sqlitestmt;
	gclient_t		*cl;

	if( !dbinitialized ) {
		if( err = G_XPSave_InitDatabase() ) {
			G_Printf("XPSave: can not initialize database\n", err);
			return;
		}
	}

	sqlite3_open( dbospath, &db );

	cl = ent->client;

	trap_GetUserinfo( cl->ps.clientNum, userinfo, sizeof( userinfo ) );
	guid = Info_ValueForKey ( userinfo, "cl_guid" );

	// TODO sanity check on guid

	sqlstmt = va( XPSAVE_SQLWRAP_SELECT, guid );
	sqlite3_prepare( db, sqlstmt, strlen(sqlstmt), &sqlitestmt, NULL );

	if(sqlite3_step(sqlitestmt) == SQLITE_DONE) {
		sqlstmt = va( XPSAVE_SQLWRAP_INSERT,
			guid,
			(int)cl->sess.skillpoints[SK_BATTLE_SENSE],
			(int)cl->sess.skillpoints[SK_EXPLOSIVES_AND_CONSTRUCTION],
			(int)cl->sess.skillpoints[SK_FIRST_AID],
			(int)cl->sess.skillpoints[SK_SIGNALS],
			(int)cl->sess.skillpoints[SK_LIGHT_WEAPONS],
			(int)cl->sess.skillpoints[SK_HEAVY_WEAPONS],
			(int)cl->sess.skillpoints[SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS]);

		err = sqlite3_exec(db, sqlstmt, NULL, NULL, NULL);
		G_Printf("Xpsave Write: insert on client %s with errcode %i\n", cl->pers.netname, err);
	} else {
		sqlstmt = va( XPSAVE_SQLWRAP_UPDATE,
			(int)cl->sess.skillpoints[SK_BATTLE_SENSE],
			(int)cl->sess.skillpoints[SK_EXPLOSIVES_AND_CONSTRUCTION],
			(int)cl->sess.skillpoints[SK_FIRST_AID],
			(int)cl->sess.skillpoints[SK_SIGNALS],
			(int)cl->sess.skillpoints[SK_LIGHT_WEAPONS],
			(int)cl->sess.skillpoints[SK_HEAVY_WEAPONS],
			(int)cl->sess.skillpoints[SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS],
			guid );

		err = sqlite3_exec(db, sqlstmt, NULL, NULL, NULL);
		G_Printf("Xpsave Write: update on client %s with errcode %i\n", cl->pers.netname, err);
	}

	sqlite3_finalize(sqlitestmt);
	sqlite3_close(db);
}

void G_XPSave_WriteSession( void )
{
	char			*guid, *sqlstmt;
	char			userinfo[MAX_INFO_STRING];
	int				i, err;
	sqlite3			*db;
	sqlite3_stmt	*sqlitestmt;
	gclient_t		*cl;

	if( !dbinitialized ) {
		if( err = G_XPSave_InitDatabase() ) {
			G_Printf("XPSave: can not initialize database\n", err);
			return;
		}
	}

	sqlite3_open( dbospath, &db );

	for( i = 0; i < level.numConnectedClients; i++ ) {
		cl = &level.clients[level.sortedClients[i]];

		trap_GetUserinfo( cl->ps.clientNum, userinfo, sizeof( userinfo ) );
		guid = Info_ValueForKey ( userinfo, "cl_guid" );

		// TODO sanity check on guid

		sqlstmt = va( XPSAVE_SQLWRAP_SELECT, guid );
		sqlite3_prepare( db, sqlstmt, strlen(sqlstmt), &sqlitestmt, NULL );

		if(sqlite3_step(sqlitestmt) == SQLITE_DONE) {
			sqlstmt = va( XPSAVE_SQLWRAP_INSERT,
				guid,
				(int)cl->sess.skillpoints[SK_BATTLE_SENSE],
				(int)cl->sess.skillpoints[SK_EXPLOSIVES_AND_CONSTRUCTION],
				(int)cl->sess.skillpoints[SK_FIRST_AID],
				(int)cl->sess.skillpoints[SK_SIGNALS],
				(int)cl->sess.skillpoints[SK_LIGHT_WEAPONS],
				(int)cl->sess.skillpoints[SK_HEAVY_WEAPONS],
				(int)cl->sess.skillpoints[SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS] );

			err = sqlite3_exec(db, sqlstmt, NULL, NULL, NULL);
			G_Printf("Xpsave Write: insert on client %s with errcode %i\n", cl->pers.netname, err);
		} else {
			sqlstmt = va( XPSAVE_SQLWRAP_UPDATE,
				(int)cl->sess.skillpoints[SK_BATTLE_SENSE],
				(int)cl->sess.skillpoints[SK_EXPLOSIVES_AND_CONSTRUCTION],
				(int)cl->sess.skillpoints[SK_FIRST_AID],
				(int)cl->sess.skillpoints[SK_SIGNALS],
				(int)cl->sess.skillpoints[SK_LIGHT_WEAPONS],
				(int)cl->sess.skillpoints[SK_HEAVY_WEAPONS],
				(int)cl->sess.skillpoints[SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS],
				guid );

			err = sqlite3_exec(db, sqlstmt, NULL, NULL, NULL);
			G_Printf("Xpsave Write: update on client %s with errcode %i\n", cl->pers.netname, err);
		}

		sqlite3_finalize(sqlitestmt);
	}

	sqlite3_close(db);
}

#endif // XPSAVE_SUPPORT