/**
 * Hei Platform Library
 * Copyright (C) 2017-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is licensed under MIT. See LICENSE for more details.
 */

#include <plcore/pl_console.h>
#include <plcore/pl_filesystem.h>

#include "pl_private.h"

#include <errno.h>
#if defined( _WIN32 )
#include <Windows.h>
#include <io.h>
#endif

#define CONSOLE_MAX_ARGUMENTS 8

/* Multi Console Manager */
// todo, should the console be case-sensitive?
// todo, mouse input callback
// todo, keyboard input callback

static PLConsoleCommand **_pl_commands = NULL;
static size_t _pl_num_commands = 0;
static size_t _pl_commands_size = 512;

void PlRegisterConsoleCommand( const char *name, void ( *CallbackFunction )( unsigned int argc, char *argv[] ),
                               const char *description ) {
	FunctionStart();

	if ( name == NULL || name[ 0 ] == '\0' ) {
		PlReportErrorF( PL_RESULT_COMMAND_NAME, PlGetResultString( PL_RESULT_COMMAND_NAME ) );
		return;
	}

	if ( CallbackFunction == NULL ) {
		PlReportErrorF( PL_RESULT_COMMAND_FUNCTION, PlGetResultString( PL_RESULT_COMMAND_FUNCTION ) );
		return;
	}

	// Deal with resizing the array dynamically...
	if ( ( 1 + _pl_num_commands ) > _pl_commands_size ) {
		_pl_commands = ( PLConsoleCommand ** ) PlReAllocA( _pl_commands, ( _pl_commands_size += 128 ) * sizeof( PLConsoleCommand ) );
	}

	if ( _pl_num_commands < _pl_commands_size ) {
		_pl_commands[ _pl_num_commands ] = ( PLConsoleCommand * ) PlMAllocA( sizeof( PLConsoleCommand ) );
		if ( !_pl_commands[ _pl_num_commands ] ) {
			return;
		}

		PLConsoleCommand *cmd = _pl_commands[ _pl_num_commands ];
		memset( cmd, 0, sizeof( PLConsoleCommand ) );
		cmd->Callback = CallbackFunction;
		strncpy( cmd->cmd, name, sizeof( cmd->cmd ) );
		if ( description != NULL && description[ 0 ] != '\0' ) {
			strncpy( cmd->description, description, sizeof( cmd->description ) );
		}

		_pl_num_commands++;
	}
}

void PlGetConsoleCommands( PLConsoleCommand ***cmds, size_t *num_cmds ) {
	*cmds = _pl_commands;
	*num_cmds = _pl_num_commands;
}

PLConsoleCommand *PlGetConsoleCommand( const char *name ) {
	for ( PLConsoleCommand **cmd = _pl_commands; cmd < _pl_commands + _pl_num_commands; ++cmd ) {
		if ( pl_strcasecmp( name, ( *cmd )->cmd ) == 0 ) {
			return ( *cmd );
		}
	}
	return NULL;
}

/////////////////////////////////////////////////////////////////////////////////////

static PLConsoleVariable **_pl_variables = NULL;
static size_t _pl_num_variables = 0;
static size_t _pl_variables_size = 512;

PLConsoleVariable *PlRegisterConsoleVariable( const char *name, const char *def, PLVariableType type,
                                              void ( *CallbackFunction )( const PLConsoleVariable *variable ),
                                              const char *desc ) {
	FunctionStart();

	plAssert( _pl_variables );

	if ( name == NULL || name[ 0 ] == '\0' ) {
		return NULL;
	}

	// Deal with resizing the array dynamically...
	if ( ( 1 + _pl_num_variables ) > _pl_variables_size ) {
		_pl_variables = ( PLConsoleVariable ** ) PlReAllocA( _pl_variables, ( _pl_variables_size += 128 ) * sizeof( PLConsoleVariable ) );
	}

	PLConsoleVariable *out = NULL;
	if ( _pl_num_variables < _pl_variables_size ) {
		_pl_variables[ _pl_num_variables ] = ( PLConsoleVariable * ) PlMAllocA( sizeof( PLConsoleVariable ) );
		out = _pl_variables[ _pl_num_variables ];
		out->type = type;
		snprintf( out->var, sizeof( out->var ), "%s", name );
		snprintf( out->default_value, sizeof( out->default_value ), "%s", def );
		snprintf( out->description, sizeof( out->description ), "%s", desc );

		PlSetConsoleVariable( out, out->default_value );

		// Ensure the callback is only called afterwards
		if ( CallbackFunction != NULL ) {
			out->CallbackFunction = CallbackFunction;
		}

		_pl_num_variables++;
	}

	return out;
}

void PlGetConsoleVariables( PLConsoleVariable ***vars, size_t *num_vars ) {
	*vars = _pl_variables;
	*num_vars = _pl_num_variables;
}

PLConsoleVariable *PlGetConsoleVariable( const char *name ) {
	for ( PLConsoleVariable **var = _pl_variables; var < _pl_variables + _pl_num_variables; ++var ) {
		if ( pl_strcasecmp( name, ( *var )->var ) == 0 ) {
			return ( *var );
		}
	}
	return NULL;
}

const char *PlGetConsoleVariableValue( const char *name ) {
	PLConsoleVariable *var = PlGetConsoleVariable( name );
	if ( var == NULL ) {
		return NULL;
	}

	return var->value;
}

const char *PlGetConsoleVariableDefaultValue( const char *name ) {
	PLConsoleVariable *var = PlGetConsoleVariable( name );
	if ( var == NULL ) {
		return NULL;
	}

	return var->default_value;
}

// Set console variable, with sanity checks...
void PlSetConsoleVariable( PLConsoleVariable *var, const char *value ) {
	plAssert( var );
	switch ( var->type ) {
		default:
			PrintWarning( "Unknown variable type %d, failed to set!\n", var->type );
			return;

		case pl_int_var:
			if ( pl_strisdigit( value ) != -1 ) {
				PrintWarning( "Unknown argument type %s, failed to set!\n", value );
				return;
			}

			var->i_value = ( int ) strtol( value, NULL, 10 );
			break;

		case pl_string_var:
			var->s_value = &var->value[ 0 ];
			break;

		case pl_float_var:
			var->f_value = strtof( value, NULL );
			break;

		case pl_bool_var:
			if ( pl_strisalnum( value ) == -1 ) {
				PrintWarning( "Unknown argument type %s, failed to set!\n", value );
				return;
			}

			if ( strcmp( value, "true" ) == 0 || strcmp( value, "1" ) == 0 ) {
				var->b_value = true;
			} else {
				var->b_value = false;
			}
			break;
	}

	strncpy( var->value, value, sizeof( var->value ) );
	if ( var->CallbackFunction != NULL ) {
		var->CallbackFunction( var );
	}
}

void PlSetConsoleVariableByName( const char *name, const char *value ) {
	PLConsoleVariable *var = PlGetConsoleVariable( name );
	if ( var == NULL ) {
		Print( "Failed to find console variable \"%s\"!\n", name );
		return;
	}

	PlSetConsoleVariable( var, value );
}

/////////////////////////////////////////////////////////////////////////////////////
// PRIVATE

IMPLEMENT_COMMAND( pwd, "Print current working directory." ) {
	PlUnused( argv );
	PlUnused( argc );
	Print( "%s\n", PlGetWorkingDirectory() );
}

IMPLEMENT_COMMAND( echo, "Prints out string to console." ) {
	for ( unsigned int i = 0; i < ( argc - 1 ); ++i ) {
		Print( "%s ", argv[ i ] );
	}
	Print( "\n" );
}

#if 0
IMPLEMENT_COMMAND(clear, "Clears the console buffer.") {
    memset(console_panes[active_console_pane].buffer, 0, 4096);
}
#endif

IMPLEMENT_COMMAND( colour, "Changes the colour of the current console." ) {
	PlUnused( argv );
	PlUnused( argc );
	//console_panes[active_console_pane].
}

IMPLEMENT_COMMAND( time, "Prints out the current time." ) {
	PlUnused( argv );
	PlUnused( argc );
	Print( "%s\n", PlGetFormattedTime() );
}

IMPLEMENT_COMMAND( mem, "Prints out current memory usage." ) {
	PlUnused( argv );
	PlUnused( argc );
}

IMPLEMENT_COMMAND( cmds, "Produces list of existing commands." ) {
	PlUnused( argv );
	PlUnused( argc );
	for ( PLConsoleCommand **cmd = _pl_commands; cmd < _pl_commands + _pl_num_commands; ++cmd ) {
		Print( " %-20s : %-20s\n", ( *cmd )->cmd, ( *cmd )->description );
	}
	Print( "%zu commands in total\n", _pl_num_commands );
}

IMPLEMENT_COMMAND( vars, "Produces list of existing variables." ) {
	PlUnused( argv );
	PlUnused( argc );
	for ( PLConsoleVariable **var = _pl_variables; var < _pl_variables + _pl_num_variables; ++var ) {
		Print( " %-20s : %-5s / %-15s : %-20s\n",
		       ( *var )->var, ( *var )->value, ( *var )->default_value, ( *var )->description );
	}
	Print( "%zu variables in total\n", _pl_num_variables );
}

IMPLEMENT_COMMAND( help, "Returns information regarding specified command or variable.\nUsage: help <cmd/cvar>" ) {
	if ( argc == 1 ) {
		// provide help on help, gross...
		Print( "%s\n", help_var.description );
		return;
	}

	PLConsoleVariable *var = PlGetConsoleVariable( argv[ 1 ] );
	if ( var != NULL ) {
		Print( " %-20s : %-5s / %-15s : %-20s\n",
		       var->var, var->value, var->default_value, var->description );
		return;
	}

	PLConsoleCommand *cmd = PlGetConsoleCommand( argv[ 1 ] );
	if ( cmd != NULL ) {
		Print( " %-20s : %-20s\n", cmd->cmd, cmd->description );
		return;
	}

	Print( "Unknown variable/command, %s!\n", argv[ 1 ] );
}

//////////////////////////////////////////////

void ( *ConsoleOutputCallback )( int level, const char *msg, PLColour colour );

static void InitializeDefaultLogLevels( void );
PLFunctionResult PlInitConsole( void ) {
	ConsoleOutputCallback = NULL;

	if ( ( _pl_commands = ( PLConsoleCommand ** ) PlMAllocA( sizeof( PLConsoleCommand * ) * _pl_commands_size ) ) == NULL ) {
		return PL_RESULT_MEMORY_ALLOCATION;
	}

	if ( ( _pl_variables = ( PLConsoleVariable ** ) PlMAllocA( sizeof( PLConsoleVariable * ) * _pl_variables_size ) ) == NULL ) {
		return PL_RESULT_MEMORY_ALLOCATION;
	}

	PLConsoleCommand base_commands[] = {
	        //clear_var,
	        help_var,
	        time_var,
	        mem_var,
	        colour_var,
	        cmds_var,
	        vars_var,
	        pwd_var,
	        echo_var,
	};
	for ( unsigned int i = 0; i < PL_ARRAY_ELEMENTS( base_commands ); ++i ) {
		PlRegisterConsoleCommand( base_commands[ i ].cmd, base_commands[ i ].Callback, base_commands[ i ].description );
	}

	/* initialize our internal log levels here
	 * as we depend on the console variables being setup first */
	InitializeDefaultLogLevels();

	return PL_RESULT_SUCCESS;
}

void PlShutdownConsole( void ) {
	if ( _pl_commands ) {
		for ( PLConsoleCommand **cmd = _pl_commands; cmd < _pl_commands + _pl_num_commands; ++cmd ) {
			// todo, should we return here; assume it's the end?
			if ( ( *cmd ) == NULL ) {
				continue;
			}

			PlFree( ( *cmd ) );
		}
		PlFree( _pl_commands );
	}

	if ( _pl_variables ) {
		for ( PLConsoleVariable **var = _pl_variables; var < _pl_variables + _pl_num_variables; ++var ) {
			// todo, should we return here; assume it's the end?
			if ( ( *var ) == NULL ) {
				continue;
			}

			PlFree( ( *var ) );
		}
		PlFree( _pl_variables );
	}

	ConsoleOutputCallback = NULL;
}

void PlSetConsoleOutputCallback( void ( *Callback )( int level, const char *msg, PLColour colour ) ) {
	ConsoleOutputCallback = Callback;
}

/////////////////////////////////////////////////////

/**
 * Takes a string and returns a list of possible options.
 */
const char **PlAutocompleteConsoleString( const char *string, unsigned int *numElements ) {
#define MAX_AUTO_OPTIONS 16
	static const char *options[ MAX_AUTO_OPTIONS ];
	unsigned int c = 0;
	size_t sl = strlen( string );
	/* gather all the console variables */
	for ( unsigned int i = 0; i < _pl_num_variables; ++i ) {
		if ( c >= MAX_AUTO_OPTIONS ) {
			break;
		} else if ( pl_strncasecmp( string, _pl_variables[ i ]->var, sl ) != 0 ) {
			continue;
		}
		options[ c++ ] = _pl_variables[ i ]->var;
	}
	/* gather up all the console commands */
	for ( unsigned int i = 0; i < _pl_num_commands; ++i ) {
		if ( c >= MAX_AUTO_OPTIONS ) {
			break;
		} else if ( pl_strncasecmp( string, _pl_commands[ i ]->cmd, sl ) != 0 ) {
			continue;
		}
		options[ c++ ] = _pl_commands[ i ]->cmd;
	}

	*numElements = c;
	return options;
}

void PlParseConsoleString( const char *string ) {
	if ( string == NULL || *string == '\0' ) {
		DebugPrint( "Invalid string passed to ParseConsoleString!\n" );
		return;
	}

	size_t l = strlen( string ) + 2;
	char *buf = PlMAllocA( l );
	snprintf( buf, l, "%s\n", string );
	PlLogMessage( LOG_LEVEL_LOW, buf );
	PlFree( buf );

	static char **argv = NULL;
	if ( argv == NULL ) {
		if ( ( argv = ( char ** ) PlMAllocA( sizeof( char * ) * CONSOLE_MAX_ARGUMENTS ) ) == NULL ) {
			return;
		}
		for ( char **arg = argv; arg < argv + CONSOLE_MAX_ARGUMENTS; ++arg ) {
			( *arg ) = ( char * ) PlMAllocA( sizeof( char ) * 1024 );
			if ( ( *arg ) == NULL ) {
				break;// continue to our doom... ?
			}
		}
	}

	unsigned int argc = 0;
	for ( const char *pos = string; *pos; ) {
		size_t arglen = strcspn( pos, " " );
		if ( arglen > 0 ) {
			strncpy( argv[ argc ], pos, arglen );
			argv[ argc ][ arglen ] = '\0';
			++argc;
		}
		pos += arglen;
		pos += strspn( pos, " " );
	}

	PLConsoleVariable *var;
	PLConsoleCommand *cmd;

	if ( ( var = PlGetConsoleVariable( argv[ 0 ] ) ) != NULL ) {
		// todo, should the var not be set by defacto here?

		if ( argc > 1 ) {
			PlSetConsoleVariable( var, argv[ 1 ] );
		} else {
			Print( "    %s\n", var->var );
			Print( "    %s\n", var->description );
			Print( "    %-10s : %s\n", var->value, var->default_value );
		}
	} else if ( ( cmd = PlGetConsoleCommand( argv[ 0 ] ) ) != NULL ) {
		if ( cmd->Callback != NULL ) {
			cmd->Callback( argc, argv );
		} else {
			Print( "    Invalid command, no callback provided!\n" );
			Print( "    %s\n", cmd->cmd );
			Print( "    %s\n", cmd->description );
		}
	} else {
		Print( "Unknown variable/command, %s!\n", argv[ 0 ] );
	}
}

/*	Log System	*/

#define MAX_LOG_LEVELS 512

typedef struct LogLevel {
	bool isReserved;
	char prefix[ 64 ];// e.g. 'warning, 'error'
	PLColour colour;
	PLConsoleVariable *var;
} LogLevel;
static LogLevel levels[ MAX_LOG_LEVELS ];

int LOG_LEVEL_LOW = 0;
int LOG_LEVEL_MEDIUM = 0;
int LOG_LEVEL_HIGH = 0;
int LOG_LEVEL_DEBUG = 0;
int LOG_LEVEL_FILESYSTEM = 0;

static void InitializeDefaultLogLevels( void ) {
	memset( levels, 0, sizeof( LogLevel ) * MAX_LOG_LEVELS );

	LOG_LEVEL_LOW = PlAddLogLevel( "plcore", ( PLColour ){ 255, 255, 255, 255 }, true );
	LOG_LEVEL_MEDIUM = PlAddLogLevel( "plcore/warning", ( PLColour ){ 255, 255, 0, 255 }, true );
	LOG_LEVEL_HIGH = PlAddLogLevel( "plcore/error", ( PLColour ){ 255, 0, 0, 255 }, true );
	LOG_LEVEL_DEBUG = PlAddLogLevel( "plcore/debug", ( PLColour ){ 255, 255, 255, 255 },
#if !defined( NDEBUG )
	                                 true
#else
	                                 false
#endif
	);
	LOG_LEVEL_FILESYSTEM = PlAddLogLevel( "plcore/filesystem", ( PLColour ){ 0, 255, 255, 255 }, true );
}

/**
 * Converts an external log level id into an
 * internal one.
 */
static LogLevel *GetLogLevelForId( int id ) {
	if ( id < 0 || id >= MAX_LOG_LEVELS ) {
		PlReportErrorF( PL_RESULT_MEMORY_EOA, "failed to find slot for log level %d", id );
		return NULL;
	}

	return &levels[ id ];
}

/**
 * Fetches the next unreserved slot.
 */
static int GetNextFreeLogLevel( void ) {
	for ( int i = 0; i < MAX_LOG_LEVELS; ++i ) {
		if ( !levels[ i ].isReserved ) {
			return i;
		}
	}

	return -1;
}

/////////////////////////////////////////////////////////////////////////////////////
// public

static char logOutputPath[ PL_SYSTEM_MAX_PATH ] = { '\0' };

void PlSetupLogOutput( const char *path ) {
	if ( path == NULL || path[ 0 ] == '\0' ) {
		return;
	}

	strncpy( logOutputPath, path, sizeof( logOutputPath ) );
	if ( PlFileExists( logOutputPath ) ) {
		unlink( logOutputPath );
	}
}

int PlAddLogLevel( const char *prefix, PLColour colour, bool status ) {
	int i = GetNextFreeLogLevel();
	if ( i == -1 ) {
		return -1;
	}

	LogLevel *l = &levels[ i ];
	l->colour = colour;
	l->isReserved = true;

	if ( prefix != NULL && prefix[ 0 ] != '\0' ) {
		snprintf( l->prefix, sizeof( l->prefix ), "%s", prefix );
	}

	char var[ 32 ];
	snprintf( var, sizeof( var ), "log.%s", prefix );
	l->var = PlRegisterConsoleVariable( var, status ? "1" : "0", pl_bool_var, NULL, "Console output level." );

	return i;
}

/**
 * Removes the specified log levels.
 * Keep in mind that this will not remove any
 * console variables generated from adding those
 * levels.
 */
void PlRemoveLogLevel( int id ) {
	LogLevel *l = GetLogLevelForId( id );
	if ( l == NULL ) {
		return;
	}

	PL_ZERO( l, sizeof( LogLevel ) );
}

void PlSetLogLevelStatus( int id, bool status ) {
	LogLevel *l = GetLogLevelForId( id );
	if ( l == NULL ) {
		return;
	}

	PlSetConsoleVariable( l->var, status ? "1" : "0" );
}

void PlLogMessage( int id, const char *msg, ... ) {
	LogLevel *l = GetLogLevelForId( id );
	if ( l == NULL || !l->var->b_value ) {
		return;
	}

	va_list args;
	va_start( args, msg );

	int length = pl_vscprintf( msg, args ) + 1;
	if ( length <= 0 )
		return;

	char *buf = PlCAllocA( length, sizeof( char ) );
	vsnprintf( buf, length, msg, args );

	va_end( args );

#if defined( _WIN32 )
	OutputDebugString( buf );
#endif

	printf( "%s", buf );

	if ( ConsoleOutputCallback != NULL ) {
		// todo: pass back level
		ConsoleOutputCallback( id, buf, l->colour );
	}

	static bool avoid_recursion = false;
	if ( !avoid_recursion ) {
		if ( logOutputPath[ 0 ] != '\0' ) {
			FILE *file = fopen( logOutputPath, "a" );
			if ( file != NULL ) {
				// add the prefix to the start
				char prefix[ 128 ];
				if ( l->prefix[ 0 ] != '\0' ) {
					snprintf( prefix, sizeof( prefix ), "[%s] %s: ", PlGetFormattedTime(), l->prefix );
				} else {
					snprintf( prefix, sizeof( prefix ), "[%s]: ", PlGetFormattedTime() );
				}

				size_t nl = strlen( prefix ) + length;
				char *logBuf = PlCAllocA( nl, sizeof( char ) );
				snprintf( logBuf, nl, "%s%s", prefix, buf );

				if ( fwrite( logBuf, sizeof( char ), nl, file ) != nl ) {
					avoid_recursion = true;
					PlReportErrorF( PL_RESULT_FILEERR, "failed to write to log, %s\n%s", logOutputPath, strerror( errno ) );
				}
				fclose( file );

				PlFree( logBuf );
			} else {
				// todo, needs to be more appropriate; return details on exact issue
				avoid_recursion = true;
				PlReportErrorF( PL_RESULT_FILEREAD, "failed to open %s", logOutputPath );
			}
		}
	}

	PlFree( buf );
}
