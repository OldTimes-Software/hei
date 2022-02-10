/*
MIT License

Copyright (c) 2017-2021 Mark E Sowden <hogsy@oldtimes-software.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once

#include <plcore/pl.h>
#include <plcore/pl_math.h>

PL_EXTERN_C

/* todo: make this structure private */
typedef struct PLConsoleVariable {
	char var[ 32 ];
	char description[ 256 ];

	PLVariableType type;

	void ( *CallbackFunction )( const struct PLConsoleVariable *variable );

	/////////////////////////////

#define PL_VAR_VALUE_LENGTH 512

	union {
		float f_value;
		int i_value;
		const char *s_value;
		bool b_value;
	};
	char value[ PL_VAR_VALUE_LENGTH ];
	char default_value[ PL_VAR_VALUE_LENGTH ];

	/////////////////////////////

	bool archive;
} PLConsoleVariable;

#if !defined( PL_COMPILE_PLUGIN )

void PlGetConsoleVariables( PLConsoleVariable ***vars, size_t *num_vars );

PLConsoleVariable *PlGetConsoleVariable( const char *name );
const char *PlGetConsoleVariableValue( const char *name );
const char *PlGetConsoleVariableDefaultValue( const char *name );
void PlSetConsoleVariable( PLConsoleVariable *var, const char *value );
void PlSetConsoleVariableByName( const char *name, const char *value );

PLConsoleVariable *PlRegisterConsoleVariable( const char *name, const char *def, PLVariableType type,
                                              void ( *CallbackFunction )( const PLConsoleVariable *variable ),
                                              const char *desc );

/* fetch and cache console var for trivial lookup */
#define PL_GET_CVAR( NAME, STORE )                    \
	static PLConsoleVariable *( STORE ) = NULL;       \
	if ( ( STORE ) == NULL ) {                        \
		( STORE ) = PlGetConsoleVariable( ( NAME ) ); \
		assert( ( STORE ) != NULL );                  \
	}

#endif /* !defined( PL_COMPILE_PLUGIN ) */

/////////////////////////////////////////////////////////////////////////////////////

typedef struct PLConsoleCommand {
	char cmd[ 24 ];

	void ( *Callback )( unsigned int argc, char *argv[] );

	char description[ 512 ];
} PLConsoleCommand;

#if !defined( PL_COMPILE_PLUGIN )

PL_EXTERN void PlGetConsoleCommands( PLConsoleCommand ***cmds, size_t *num_cmds );
PL_EXTERN void PlRegisterConsoleCommand( const char *name, void ( *CallbackFunction )( unsigned int argc, char *argv[] ), const char *description );
PL_EXTERN PLConsoleCommand *PlGetConsoleCommand( const char *name );

PL_EXTERN void PlSetConsoleOutputCallback( void ( *Callback )( int level, const char *msg, PLColour colour ) );

PL_EXTERN const char **PlAutocompleteConsoleString( const char *string, unsigned int *numElements );
PL_EXTERN void PlParseConsoleString( const char *string );

/////////////////////////////////////////////////////////////////////////////////////

extern void PlSetupLogOutput( const char *path );
extern int PlAddLogLevel( const char *prefix, PLColour colour, bool status );
extern void PlSetLogLevelStatus( int id, bool status );
extern void PlLogMessage( int id, const char *msg, ... );

#define PlLogWFunction( ID, FORMAT, ... ) PlLogMessage( ( ID ), "(%s) " FORMAT, PL_FUNCTION, ## __VA_ARGS__ )

#endif /* !defined( PL_COMPILE_PLUGIN ) */

PL_EXTERN_C_END
