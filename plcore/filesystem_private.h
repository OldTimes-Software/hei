/**
 * Hei Platform Library
 * Copyright (C) 2017-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is licensed under MIT. See LICENSE for more details.
 */

#pragma once

#include <plcore/pl_filesystem.h>

#ifdef _DEBUG
#	define FSLog( ... ) PlLogMessage( LOG_LEVEL_FILESYSTEM, __VA_ARGS__ )
#else
#	define FSLog( ... )
#endif

#if defined( _WIN32 )
#	define _pl_mkdir( a ) _mkdir( ( a ) )
#else
#	define _pl_mkdir( a ) mkdir( ( a ), 0777 )
#endif

#define _pl_fclose( a ) \
	fclose( ( a ) );    \
	( a ) = NULL

typedef struct PLFile {
	char path[ PL_SYSTEM_MAX_PATH ];
	uint8_t *data;
	uint8_t *pos;
	size_t size;
	time_t timeStamp;
	void *fptr;
	bool isUnmanaged;
} PLFile;

const char *PlResolveVirtualPath_( const char *path, char *dest, size_t size );
