#include "g_local.h"

#ifdef DATABASE_SUPPORT

// TODO in-memory database

static char database_errmsg[MAX_INFO_STRING] = { 0 };

static void G_DB_LogLastError( const char *errmsg )
{
	Q_strncpyz( database_errmsg, errmsg, sizeof(database_errmsg) );
	//Q_strcat( database_errmsg, sizeof( database_errmsg ), va( " - level.time: %i", level.time ) );
}

void G_DB_PrintLastError( void )
{	
	G_LogPrintf( "%s\n", database_errmsg );
}

// called each map to initialize level.database struct
// once initialized it must be deinitialized again to free resources associated with the database
int G_DB_Init( void )
{
	char		*dbpath;
	char		fsgame[MAX_QPATH];
	char		homepath[MAX_OSPATH];
	int			err;
#ifdef XPSAVE_SUPPORT
	qboolean	xpsave_initialize = qfalse;
#endif
	qboolean	initialize = qfalse;

	if( level.database.initialized ) {
		G_DB_LogLastError( "G_DB_Init: attempt to initialize already initialized database" );
		return DB_INIT_INIT;
	}

	if( g_dbpath.string ) {		
		dbpath = va( "%s", g_dbpath.string );
	} else {
		trap_Cvar_VariableStringBuffer( "fs_game", fsgame, sizeof( fsgame ) );
		trap_Cvar_VariableStringBuffer( "fs_homepath", homepath, sizeof( homepath ) );
		dbpath = va( "%s%c%s%c%s", homepath, PATH_SEP, fsgame, PATH_SEP, DB_STANDARD_FILENAME );
	}

#ifdef XPSAVE_SUPPORT
	// perform xpsave related sanity check on database
	if( g_xpsave.integer == 1 ) {
		err = G_XPSave_CheckDBSanity( dbpath );
		if( err ) {
			switch( err ) {
			case XPSAVE_DB_READ:
				G_DB_LogLastError( "G_DB_Init: XPSave failed to read database" );
				break;
			case XPSAVE_TABLE_NOTEXIST:
				err = G_XPSave_CreateNewDB( dbpath, qfalse );
				break;
			case XPSAVE_TABLE_INCORRECT:
				G_DB_LogLastError( "G_DB_Init: XPSave failed to read schema" );
				break;
			default:
				G_DB_LogLastError( va( "G_DB_Init: XPSave failed to initialize database default with error code %i", err ) );
				break;
			}
			
			// if it still fails
			if( err ) {
				G_DB_LogLastError( va( "G_DB_Init: XPSave failed to initialize database with error code %i", err ) );
			} else {
				xpsave_initialize = qtrue;
			}
		} else {
			xpsave_initialize = qtrue;
		}
	}	
#endif

	// if one of the modules passes sanity tests, the database will be initialized
#ifdef XPSAVE_SUPPORT
	// this only really makes sense if there are multiple modules operating on db, not just XPSave
	if( xpsave_initialize ) {
		initialize = qtrue;
	}
#endif 

	if( !initialize ) {
		G_DB_LogLastError( "No module initialized\n" );
		return DB_INIT_MODFAIL;
	}

	// database can be initialized!
	Q_strncpyz( level.database.path, dbpath, MAX_OSPATH );

	if( err = sqlite3_open( level.database.path, &level.database.db ) ) { // keep it open until deinit
		G_DB_LogLastError( va( "G_DB_Init: sqlite3_open failed with error code %i", err ) );
		return err;
	}

#ifdef XPSAVE_SUPPORT
	if( xpsave_initialize ) {
		level.database.initialized &= XPSAVE_INITIALZE_FLAG;
	}	
#endif 

	G_LogPrintf( "Database: %s initialized\n", xpsave_initialize ? "XPSave" : "XPSave not" );

	return 0;
}

// frees all resources associated with the database
int G_DB_DeInit( void )
{
	int		err;

	if( !level.database.initialized ) {
		G_DB_LogLastError( "G_DB_DeInit: access to non-initialized database" );
		return DB_ACCESS_NONINIT;
	}

	if( err = sqlite3_close( level.database.db ) ) {
		G_DB_LogLastError( va( "G_DB_DeInit: sqlite3_close failed with error code %i", err ) );
		return err;
	}

	level.database.db = NULL;
	level.database.path[0] = '\0';
	level.database.initialized = 0;

	G_LogPrintf( "Database deinitialized\n" );

	return 0;
}

#endif // DATABASE_SUPPORT