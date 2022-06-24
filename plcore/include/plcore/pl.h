/**
 * Hei Platform Library
 * Copyright (C) 2017-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is licensed under MIT. See LICENSE for more details.
 */

#pragma once

/*
Platform Library

This library includes standard platform headers,
gives you some standard functions to interact with
the system and includes defines for basic data-types that
you can use in your applications for easier multi-platform
support.

 Anything that uses an underscore before it's name
 is intended only for internal usage!
*/

#define PL_VERSION_MAJOR 1
#define PL_VERSION_MINOR 0
#define PL_VERSION_PATCH 0

// PL_IGNORE_SHARED_HEADERS

#define PL_INCLUDE_STD_BOOL

// Shared headers
#ifndef PL_IGNORE_SHARED_HEADERS
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdlib.h>

#ifndef __cplusplus
#ifdef PL_INCLUDE_STD_BOOL
#include <stdbool.h>
#endif
#endif

#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <setjmp.h>
#include <time.h>

#include <sys/stat.h>
#include <sys/types.h>
#endif

#ifdef __cplusplus
#define PL_EXTERN_C extern "C" {
#define PL_EXTERN_C_END }
#else
#define PL_EXTERN_C
#define PL_EXTERN_C_END
#endif

#include <plcore/pl_system.h>

typedef char PLPath[ PL_SYSTEM_MAX_PATH ];

#if !defined( NDEBUG )
#include <assert.h>

#define plAssert( a ) assert( a )
#else
#define plAssert( a )
#endif

#define PlUnused( ... ) ( void ) ( __VA_ARGS__ )

#define PL_ARRAY_ELEMENTS( a ) ( sizeof( a ) / sizeof( *( a ) ) )// Returns the number of elements within an array.
#define PL_MAX_ARRAY_INDEX( a ) ( int ) ( PL_ARRAY_ELEMENTS( a ) - 1 )
#define PL_INVALID_STRING( a ) ( a == NULL || *a == '\0' )

#define PL_ZERO( DATA, SIZE ) memset( ( DATA ), 0, ( SIZE ) )
#define PL_ZERO_( DATA )      memset( &( DATA ), 0, sizeof( ( DATA ) ) )

#define PL_MIN( A, B ) ( (A) < (B) ? (A) : (B) )
#define PL_MAX( A, B ) ( (A) > (B) ? (A) : (B) )

#define PL_STRINGIFY( num ) #num
#define PL_TOSTRING( A ) PL_STRINGIFY( A )

#define PL_MAGIC_TO_NUM( A, B, C, D ) ( ( ( D ) << 24 ) + ( ( C ) << 16 ) + ( ( B ) << 8 ) + ( A ) )

#define PL_BITFLAG( A, B ) A = ( 1U << B )

#ifndef offsetof
#define pl_offsetof( a, b ) ( ( size_t ) & ( ( ( a * ) 0 )->b ) )
#else
#define pl_offsetof( a, b ) offsetof( a, b )
#endif

typedef enum PLVariableType {
	pl_float_var,
	pl_int_var,
	pl_string_var,
	pl_bool_var,// 0,1 true,false
} PLVariableType;

//////////////////////////////////////////////////////////////////

// Error return values
typedef enum PLFunctionResult {
	PL_RESULT_SUCCESS,
	PL_RESULT_FAIL,
	PL_RESULT_UNSUPPORTED,

	PL_RESULT_INVALID_PARM1,// Invalid function parameter 1
	PL_RESULT_INVALID_PARM2,// Invalid function parameter 2
	PL_RESULT_INVALID_PARM3,// Invalid function parameter 3
	PL_RESULT_INVALID_PARM4,// Invalid function parameter 4

	// FILE I/O
	PL_RESULT_FILEREAD,   // Failed to read file!
	PL_RESULT_FILEWRITE,  // Failed to write file!
	PL_RESULT_FILETYPE,   // Unexpected file type!
	PL_RESULT_FILEVERSION,// Unsupported version!
	PL_RESULT_FILESIZE,   // Invalid file size!
	PL_RESULT_FILEPATH,   // Invalid path!
	PL_RESULT_FILEERR,    // Generic Filesystem error

	// CONSOLE
	PL_RESULT_COMMAND_NAME,    // Invalid name provided for command!
	PL_RESULT_COMMAND_FUNCTION,// Invalid callback provided for command!

	// GRAPHICS
	PL_RESULT_GRAPHICSINIT,            // Graphics failed to initialise!
	PL_RESULT_INVALID_SHADER_TYPE,     /* shader type passed is not a valid type */
	PL_RESULT_UNSUPPORTED_SHADER_TYPE, /* shader type not supported by hardware */
	PL_RESULT_SHADER_COMPILE,          // Failed to compile shader!
	PL_RESULT_SHADER_LINK,             /* shader program failed link */
	PL_RESULT_DRAW_MODE,               // Invalid mesh draw mode!
	PL_RESULT_DRAW_PRIMITIVE,          // Invalid mesh primitive!

	// IMAGE
	PL_RESULT_IMAGERESOLUTION,// Invalid image resolution!
	PL_RESULT_IMAGEFORMAT,    // Unsupported image format!

	// MEMORY
	PL_RESULT_MEMORY_ALLOCATION,// Ran out of memory!
	PL_RESULT_MEMORY_UNDERFLOW,
	PL_RESULT_MEMORY_EOA,// End of array

	PL_RESULT_SYSERR,// Generic system error
} PLFunctionResult;

//////////////////////////////////////////////////////////////////

enum {
	PL_BITFLAG( PL_SUBSYSTEM_GRAPHICS, 0 ),
	PL_BITFLAG( PL_SUBSYSTEM_IO, 1 ),
};

#if defined( PL_INTERNAL )
#define PL_DLL PL_EXPORT
#else
#define PL_DLL PL_IMPORT
#endif

typedef void PLLibrary; /* handle to dll/module */

PL_EXTERN_C

#if !defined( PL_COMPILE_PLUGIN )

PL_EXTERN PLFunctionResult PlInitialize( int argc, char **argv );
PL_EXTERN PLFunctionResult PlInitializeSubSystems( unsigned int subsystems );
PL_EXTERN void PlShutdown( void );

/******************************************************************/
/* ERROR HANDLING */

PL_EXTERN void PlClearError( void );// Resets the error message to "null", so you can ensure you have the correct message from the library.

PL_EXTERN PLFunctionResult PlGetFunctionResult( void );
PL_EXTERN const char *PlGetResultString( PLFunctionResult result );
PL_EXTERN const char *PlGetError( void );// Returns the last recorded error.

PL_EXTERN void PlReportError( PLFunctionResult result, const char *function, const char *message, ... );
#define PlReportErrorF( type, ... ) PlReportError( type, PL_FUNCTION, __VA_ARGS__ )
#define PlReportBasicError( type ) PlReportErrorF( ( type ), PlGetResultString( ( type ) ) )

/******************************************************************/

PL_EXTERN const char *PlGenerateUniqueIdentifier( char *dest, size_t destLength );

// CL Arguments
PL_EXTERN const char *PlGetCommandLineArgumentValue( const char *arg );
PL_EXTERN bool PlHasCommandLineArgument( const char *arg );

//////////////////////////////////////////////////////////////////

PL_EXTERN void pl_crc32( const void *data, uint32_t n_bytes, uint32_t *crc );

//////////////////////////////////////////////////////////////////

bool PlIsSubSystemActive( unsigned int subsystem );

//////////////////////////////////////////////////////////////////

/* standard headers */
#include <plcore/pl_string.h>
#include <plcore/pl_memory.h>

PL_EXTERN const char *PlGetFormattedTime( void );
PL_EXTERN time_t PlStringToTime( const char *ts );

//////////////////////////////////////////////////////////////////

PL_EXTERN PLLibrary *PlLoadLibrary( const char *path, bool appendPath );
PL_EXTERN void *PlGetLibraryProcedure( PLLibrary *library, const char *procedureName );
PL_EXTERN void PlUnloadLibrary( PLLibrary *library );

/**
 * Plugin Interface
 **/

PL_EXTERN const struct PLPluginExportTable *PlGetExportTable( void );
PL_EXTERN bool PlRegisterPlugin( const char *path );
PL_EXTERN void PlRegisterPlugins( const char *pluginDir );
PL_EXTERN void PlInitializePlugins( void );

#endif

PL_EXTERN_C_END
