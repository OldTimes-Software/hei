/**
 * Hei Platform Library
 * Copyright (C) 2017-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is licensed under MIT. See LICENSE for more details.
 */
/****************************************
 * MEMORY MANAGEMENT
 ****************************************/

#include <stdlib.h>
#if defined( _WIN32 )
#	include <windows.h>
#	include <psapi.h>
#elif defined( __linux__ )
#	include <sys/resource.h>
#endif

#include "pl_private.h"

//#define DEBUG_MEMORY
#define TRACK_MEMORY

/* description of what's been allocated, for debugging */
#if defined( TRACK_MEMORY )
#	define MAGIC PL_MAGIC_TO_NUM( 'P', 'L', 'M', 'B' )
typedef struct PLAllocHeader {
	uint32_t magic;
	char id[ 32 ]; /* unique identifier */
	size_t length; /* allocated size in bytes */
} PLAllocHeader;
#endif
static size_t totalRAMUsage = 0;

PL_DLL PLMemoryAbortCallbackT pl_memory_abort_cb = NULL;

/**
 * Allocates the desired number of bytes, otherwise calls abort on fail if requested.
 */
void *PlCAlloc( size_t num, size_t size, bool abortOnFail ) {
	size_t totalSize = num * size;
#if defined( TRACK_MEMORY )
	totalSize += sizeof( PLAllocHeader );
#endif

	char *buf = pl_calloc( 1, totalSize );
	if ( buf == NULL ) {
		PlReportErrorF( PL_RESULT_MEMORY_ALLOCATION, "failed to allocate %lu bytes", totalSize );

		if ( abortOnFail ) {
			if ( pl_memory_abort_cb != NULL ) {
				pl_memory_abort_cb( totalSize );
			}
			abort();
		}

		return NULL;
	}

#if defined( TRACK_MEMORY )
	PLAllocHeader *header = ( PLAllocHeader * ) buf;
	header->magic = MAGIC;
	PlGenerateUniqueIdentifier( header->id, sizeof( header->id ) - 1 );
	header->length = totalSize;

	totalRAMUsage += totalSize;

#	ifdef DEBUG_MEMORY
	printf( "ALLOC: " COM_FMT_uint64 " bytes (%s)\t\t | TOTAL: " COM_FMT_double "mb\n",
	        header->length, header->id,
	        PlBytesToMegabytes( totalRAMUsage ) );
#	endif

	buf += sizeof( PLAllocHeader );
#endif

	return buf;
}

/**
 * Allocates the desired number of bytes, otherwise calls abort on fail if requested.
 * Keep in mind that the allocated buffer will be zero'd.
 */
void *PlMAlloc( size_t size, bool abortOnFail ) {
	return PlCAlloc( 1, size, abortOnFail );
}

/**
 * Allocates the desired number of bytes, otherwise calls abort on fail if requested.
 */
void *PlReAlloc( void *ptr, size_t newSize, bool abortOnFail ) {
	char *buf = ptr;
	if ( buf == NULL ) {
		return PlMAlloc( newSize, abortOnFail );
	}

#if defined( TRACK_MEMORY )
	bool isTracked = true;
	buf -= sizeof( PLAllocHeader );
	if ( ( buf != NULL ) && ( ( ( PLAllocHeader * ) buf )->magic == MAGIC ) ) {
		newSize += sizeof( PLAllocHeader ); /* maybe... ? */
	} else {
		isTracked = false;
	}
#endif

	buf = pl_realloc( buf, newSize );
	if ( buf == NULL ) {
		PlReportErrorF( PL_RESULT_MEMORY_ALLOCATION, "failed to allocate %lu bytes", newSize );

		if ( abortOnFail ) {
			if ( pl_memory_abort_cb != NULL ) {
				pl_memory_abort_cb( newSize );
			}
			abort();
		}

		return NULL;
	}

#if defined( TRACK_MEMORY )
	if ( isTracked ) {
		PLAllocHeader *header = ( PLAllocHeader * ) buf;
		size_t oldLength = header->length;
		header->length = newSize;

		totalRAMUsage -= oldLength;
		totalRAMUsage += newSize;

#	ifdef DEBUG_MEMORY
		printf( "REALLOC: " COM_FMT_uint64 " bytes (%s)\t\t | TOTAL: " COM_FMT_double "mb\n",
		        header->length, header->id,
		        PlBytesToMegabytes( totalRAMUsage ) );
#	endif

		buf += sizeof( PLAllocHeader );
	}
#endif

	return buf;
}

void PlFree( void *ptr ) {
	char *buf = ptr;
#if defined( TRACK_MEMORY )
	if ( buf != NULL ) {
		buf -= sizeof( PLAllocHeader );
		PLAllocHeader *header = ( PLAllocHeader * ) buf;
		if ( ( header != NULL ) && (header->magic == MAGIC) ) {
			totalRAMUsage -= header->length;
#	ifdef DEBUG_MEMORY
			printf( "FREE: %p | " COM_FMT_uint64 " bytes (%s)\t\t | TOTAL: " COM_FMT_double "mb\n",
			        ptr, header->length, header->id,
			        PlBytesToMegabytes( totalRAMUsage ) );
#	endif
		} else {
			/* not allocated by us,
			 * someone is mixing crap likely */
			buf = ptr;
		}
	}
#endif

	pl_free( buf );
}

/* lower-level function pointers */
PL_DLL void *( *pl_malloc )( size_t size ) = malloc;
PL_DLL void *( *pl_calloc )( size_t num, size_t size ) = calloc;
PL_DLL void *( *pl_realloc )( void *ptr, size_t newSize ) = realloc;
PL_DLL void ( *pl_free )( void *ptr ) = free;

/**
 * Returns total memory allocated locally by
 * the library (obviously doesn't include memory
 * allocated outside of scope).
 */
size_t PlGetTotalAllocatedMemory( void ) {
	return totalRAMUsage;
}

/**
 * Returns the total amount of system memory in bytes.
 */
uint64_t PlGetTotalSystemMemory( void ) {
#if defined( __linux__ ) || defined( __APPLE__ )
	long pages = sysconf( _SC_PHYS_PAGES );
	long pageSize = sysconf( _SC_PAGE_SIZE );
	return pages * pageSize;
#elif defined( _WIN32 )
	MEMORYSTATUSEX stat;
	stat.dwLength = sizeof( stat );
	GlobalMemoryStatusEx( &stat );
	return stat.ullTotalPageFile;
#else
#	error "Missing implementation!"
#endif
}

/**
 * Returns the total amount of available system memory in bytes.
 */
uint64_t PlGetTotalAvailableSystemMemory( void ) {
#if defined( __linux__ ) || defined( __APPLE__ )
	/* unfortunately, on OSX, we don't have AVPHYS_PAGES... */
#	if defined( __APPLE__ )
	long pages = sysconf( _SC_PHYS_PAGES );
#	else
	long pages = sysconf( _SC_AVPHYS_PAGES );
#	endif
	long pageSize = sysconf( _SC_PAGE_SIZE );
	return pages * pageSize;
#elif defined( _WIN32 )
	MEMORYSTATUSEX stat;
	stat.dwLength = sizeof( stat );
	GlobalMemoryStatusEx( &stat );
	return stat.ullAvailPhys;
#else
#	error "Missing implementation!"
#endif
}

/**
 * Returns the memory usage of the current process in bytes.
 */
uint64_t PlGetCurrentMemoryUsage( void ) {
#if defined( __linux__ ) || defined( __APPLE__ )
	/* this is probably the only thing close to an api
	 * that provides this info on linux, without having
	 * to parse shit...but it returns kb and isn't
	 * supported everywhere... */
	struct rusage usage;
	getrusage( RUSAGE_SELF, &usage );
	return usage.ru_maxrss * 1000;
#elif defined( _WIN32 )
	PROCESS_MEMORY_COUNTERS pmc;
	GetProcessMemoryInfo( GetCurrentProcess(), &pmc, sizeof( pmc ) );
	return pmc.WorkingSetSize;
#else
#	error "Missing implementation!"
#endif
}
