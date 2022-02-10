/**
 * Hei Platform Library
 * Copyright (C) 2017-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is licensed under MIT. See LICENSE for more details.
 */

#pragma once

#include <plcore/pl.h>

#if defined( _MSC_VER )
#pragma warning( disable : 4204 )
#pragma warning( disable : 4820 )
#pragma warning( disable : 4668 )
#endif

// OS Specific Headers
#if defined( _WIN32 )
//#define WIN32_LEAN_AND_MEAN 1
//#include <windows.h>
//#include <afxres.h>
//#define STRSAFE_NO_DEPRECATE 1
//#include <strsafe.h>
#undef far
#undef near
#endif

/* Hard Limits */

#define MAX_OBJECT_INTERFACES 512

/*********************************/

extern int LOG_LEVEL_LOW, LOG_LEVEL_MEDIUM, LOG_LEVEL_HIGH, LOG_LEVEL_DEBUG;
extern int LOG_LEVEL_FILESYSTEM;

#define Print( FORMAT, ... )        PlLogMessage( LOG_LEVEL_LOW, FORMAT, ##__VA_ARGS__ )
#define PrintWarning( FORMAT, ... ) PlLogWFunction( LOG_LEVEL_MEDIUM, FORMAT, ##__VA_ARGS__ )
#define PrintError( FORMAT, ... )   PlLogWFunction( LOG_LEVEL_HIGH, FORMAT, ##__VA_ARGS__ )
#if !defined( NDEBUG )
#define debug_printf( ... )       printf( __VA_ARGS__ )
#define DebugPrint( FORMAT, ... ) PlLogWFunction( LOG_LEVEL_DEBUG, FORMAT, ##__VA_ARGS__ )
#else
#define debug_printf( ... )
#define DebugPrint( ... ) Print( __VA_ARGS__ )
#endif

#define FunctionStart() PlClearError()

/* * * * * * * * * * * * * * * * * * * */
/* Sub Systems                         */

PLFunctionResult PlInitFileSystem( void );
void PlShutdownFileSystem( void );

PLFunctionResult PlInitConsole( void );
void PlShutdownConsole( void );

void PlInitPackageSubSystem( void );

/* * * * * * * * * * * * * * * * * * * */

#ifdef _WIN32

const char *GetLastError_strerror( uint32_t errnum );

#else

int GetLastError( void );
const char *GetLastError_strerror( int errnum );

#define WSAGetLastError()                  GetLastError()
#define WSAGetLastError_strerror( errnum ) GetLastError_strerror( errnum )

#endif

/* * * * * * * * * * * * * * * * * * * */
/* Console Utilities                   */

#define IMPLEMENT_COMMAND( NAME, DESC )                                \
	static void NAME##_func( unsigned int argc, char *argv[] );        \
	static PLConsoleCommand NAME##_var = { #NAME, NAME##_func, DESC }; \
	static void NAME##_func( unsigned int argc, char *argv[] )
