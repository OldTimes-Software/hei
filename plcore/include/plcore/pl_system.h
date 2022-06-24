/**
 * Hei Platform Library
 * Copyright (C) 2017-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is licensed under MIT. See LICENSE for more details.
 */

#pragma once

/* This document provides platform-specific
 * headers and any additional system information.
 */

// Operating System

#define PL_SYSTEM_OS_WINDOWS 0
#define PL_SYSTEM_OS_LINUX   1
#define PL_SYSTEM_OS_MACOS   2
#define PL_SYSTEM_OS_FREEBSD 3
#define PL_SYSTEM_OS_SOLARIS 4

#if defined( _WIN32 )
#	define PL_SYSTEM_NAME              "WINDOWS"
#	define PL_SYSTEM_OS                PL_SYSTEM_OS_WINDOWS
#	define PL_SYSTEM_LIBRARY_EXTENSION ".dll"

#	if 0
#		define PL_SYSTEM_MAX_PATH 259
#	else
#		define PL_SYSTEM_MAX_PATH 256
#	endif
#	define PL_SYSTEM_MAX_USERNAME 128

#	ifdef _MSC_VER
#		pragma warning( disable : 4152 )
#		pragma warning( disable : 4800 )// 'type' : forcing value to bool 'true' or 'false' (performance warning)
#		pragma warning( disable : 4204 )// nonstandard extension used: non-constant aggregate initializer
#		pragma warning( disable : 4201 )// nonstandard extension used: nameless struct/union
#		pragma warning( disable : 4996 )// The POSIX name for this item is deprecated.

#		ifndef itoa
#			define itoa _itoa
#		endif
#		ifndef getcwd
#			define getcwd _getcwd
#		endif
#		ifndef snprintf
#			define snprintf _snprintf
#		endif
#		ifndef unlink
#			define unlink _unlink
#		endif
#		ifndef strcasecmp
#			define strcasecmp _stricmp
#		endif
#		ifndef mkdir
#			define mkdir _mkdir
#		endif
#		ifndef strncasecmp
#			define strncasecmp _str
#		endif
#		ifdef __cplusplus
#			ifndef nothrow
//#				define nothrow __nothrow
#			endif
#		endif
#	endif
#elif defined( __linux__ )// Linux
#	ifndef PL_IGNORE_SYSTEM_HEADERS
#		include <dirent.h>
#		include <unistd.h>
#		include <dlfcn.h>
#		include <strings.h>
#	endif

#	define PL_SYSTEM_NAME              "LINUX"
#	define PL_SYSTEM_OS                PL_SYSTEM_OS_LINUX
#	define PL_SYSTEM_LIBRARY_EXTENSION ".so"

#	define PL_SYSTEM_MAX_PATH     256
#	define PL_SYSTEM_MAX_USERNAME 32

#elif defined( __APPLE__ )

#	ifndef PL_IGNORE_SYSTEM_HEADERS
#		include <zconf.h>
#		include <dirent.h>
#		include <dlfcn.h>
#	endif

#	define PL_SYSTEM_NAME              "macOS"
#	define PL_SYSTEM_OS                PL_SYSTEM_OS_MACOS
#	define PL_SYSTEM_LIBRARY_EXTENSION ".so"

#	define PL_SYSTEM_MAX_PATH     512
#	define PL_SYSTEM_MAX_USERNAME 32
#else
#	error "Unsupported system type."
#endif

// (Basic) Hardware

/* endianness
 * */
#define PL_SYSTEM_LITTLE_ENDIAN 0
#define PL_SYSTEM_BIG_ENDIAN    1

/* architecture
 * */
#define PL_SYSTEM_CPU_X86   0
#define PL_SYSTEM_CPU_X64   1
#define PL_SYSTEM_CPU_ARM   2
#define PL_SYSTEM_CPU_ARM64 3

#if defined( __amd64 ) || defined( __amd64__ ) || defined( _M_X64 ) || defined( _M_AMD64 )
#	define PL_SYSTEM_CPU        PL_SYSTEM_CPU_X64
#	define PL_SYSTEM_CPU_NAME   "AMD64"
#	define PL_SYSTEM_ENDIANNESS PL_SYSTEM_LITTLE_ENDIAN
#elif defined( __arm__ ) || defined( __thumb__ ) || defined( _M_ARM ) || defined( _M_ARMT )
#	define PL_SYSTEM_CPU        PL_SYSTEM_CPU_ARM
#	define PL_SYSTEM_CPU_NAME   "ARM"
#	define PL_SYSTEM_ENDIANNESS PL_SYSTEM_LITTLE_ENDIAN
#elif defined( __aarch64__ )
#	define PL_SYSTEM_CPU        PL_SYSTEM_CPU_ARM64
#	define PL_SYSTEM_CPU_NAME   "ARM64"
#	define PL_SYSTEM_ENDIANNESS PL_SYSTEM_LITTLE_ENDIAN
#elif defined( i386 ) || defined( __i386 ) || defined( __i386__ ) || defined( _M_I86 ) || defined( _M_IX86 ) || defined( _X86_ )
#	define PL_SYSTEM_CPU        PL_SYSTEM_CPU_X86
#	define PL_SYSTEM_CPU_NAME   "INTEL86"
#	define PL_SYSTEM_ENDIANNESS PL_SYSTEM_LITTLE_ENDIAN
#else
#	error "Unsupported CPU type."
#endif

// Compiler

#if defined( _MSC_VER )
#	define PL_INSTANCE HINSTANCE
#	define PL_FARPROC  FARPROC
#	define PL_EXTERN   extern
#	define PL_CALL     __stdcall

// MSVC doesn't support __func__
#	define PL_FUNCTION __FUNCTION__// Returns the active function.

#	define PL_EXPORT __declspec( dllexport )
#	define PL_IMPORT __declspec( dllimport )

#	define PL_DEPRECATED( function ) __declspec( deprecated ) function
#	define PL_NORETURN( function )   __declspec( noreturn ) function

#	define PL_STATIC_ASSERT( a, b ) static_assert( ( a ), b )

#	define PL_PACKED_STRUCT_START( a ) \
		__pragma( pack( push, 1 ) ) typedef struct a {
#	define PL_PACKED_STRUCT_END( a ) \
		}                             \
		a;                            \
		__pragma( pack( pop ) )
#elif defined( __GNUC__ ) || defined( __GNUG__ )
#	define PL_INSTANCE void *
#	define PL_FARPROC  void *

#	define PL_EXTERN extern
#	define PL_CALL

#	define PL_FUNCTION __FUNCTION__

#	define PL_EXPORT __attribute__( ( visibility( "default" ) ) )
#	define PL_IMPORT

#	define PL_DEPRECATED( function ) function __attribute__( ( deprecated ) )
#	define PL_NORETURN( function )   function __attribute__( ( noreturn ) )

#	define PL_STATIC_ASSERT( a, b ) _Static_assert( ( a ), b )

#	define PL_PACKED_STRUCT_START( a ) typedef struct __attribute__( ( packed ) ) a {
#	define PL_PACKED_STRUCT_END( a ) \
		}                             \
		a;
#else
#	error "Unsupported compiler."
#endif
