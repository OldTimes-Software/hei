/* SPDX-License-Identifier: MIT */
/* Copyright © 2017-2022 Mark E Sowden <hogsy@oldtimes-software.com> */

#include <plcore/pl.h>
#include <plcore/pl_filesystem.h>
#include <plcore/pl_package.h>

/* Core Design, CLU (cluster) format
 *
 * Usages:
 * 	- Herdy Gerdy (2002, PS2)
 *
 * Notes:
 * 	Levels use the original level name as the description,
 * 	while the stored file uses a hashed name instead, hence
 * 	why the CLU names for levels appear somewhat random.
 *
 * 	Table of contents is sorted by hash, hence why the last
 * 	32-bit value increments (was pretty confusing at first!)
 * 	This is also why the table of contents doesn't match with
 * 	the order of the data.
 */

#define CLU_MAGIC   PL_MAGIC_TO_NUM( 'C', 'L', 'U', '\0' )
#define CLU_VERSION 2

typedef struct CLUHeader {
	uint32_t magic;   /* 'CLU ' */
	uint32_t version; /* always appears to be 2 - changed to 1 in later version */
	uint32_t headerLength;
	uint32_t numIndices;
	uint32_t hash;
	uint32_t unused;
	char description[ 60 ];
} CLUHeader;
PL_STATIC_ASSERT( sizeof( CLUHeader ) == 84U, "Invalid struct size!" );

typedef struct CLUIndex {
	uint32_t nameOffset; /* always resolves to 4 bytes before data, because it's unused */
	uint32_t size;
	uint32_t offset;
	uint32_t hash;
} CLUIndex;
PL_STATIC_ASSERT( sizeof( CLUIndex ) == 16U, "Invalid struct size!" );

/**
 * Seems to provide the correct result as far as I've been
 * able to determine...
 */
static uint32_t GenerateCoreHash( const char *string ) {
	uint32_t hash = 0;
	char c;
	while ( ( c = *string ) != '\0' ) {
		hash += ( hash << 7 ) + ( hash << 1 ) + ( uint32_t ) c;
		string++;
	}

	return hash;
}

static const char *GetStringForHash( uint32_t hash ) {
	static const char *sampleStringTable[] = {
#include "core_hg_files.h"
	};

	for ( unsigned int i = 0; i < PL_ARRAY_ELEMENTS( sampleStringTable ); ++i ) {
		uint32_t sHash = GenerateCoreHash( sampleStringTable[ i ] );
		if ( sHash != hash ) {
			continue;
		}

		return sampleStringTable[ i ];
	}

	return NULL;
}

static PLPackage *ParseCLUFile( PLFile *file ) {
	CLUHeader header;
	PlReadFile( file, &header, sizeof( CLUHeader ), 1 );

	if ( header.magic != CLU_MAGIC ) {
		PlReportBasicError( PL_RESULT_FILETYPE );
		return NULL;
	}

	if ( header.version != CLU_VERSION ) {
		PlReportBasicError( PL_RESULT_FILEVERSION );
		return NULL;
	}

	if ( header.headerLength >= PlGetFileSize( file ) ) {
		PlReportErrorF( PL_RESULT_FILEERR, "invalid data offset provided in header" );
		return NULL;
	}

	size_t tableSize = ( sizeof( CLUIndex ) * header.numIndices );
	if ( tableSize >= header.headerLength ) {
		PlReportErrorF( PL_RESULT_FILEERR, "invalid relative table size to data offset" );
		return NULL;
	}

	CLUIndex *indicies = PlMAllocA( tableSize );
	if ( PlReadFile( file, indicies, sizeof( CLUIndex ), header.numIndices ) != header.numIndices ) {
		PlFree( indicies );
		return NULL;
	}

	PLPackage *package = PlCreatePackageHandle( PlGetFilePath( file ), header.numIndices, NULL );
	for ( unsigned int i = 0; i < header.numIndices; ++i ) {
		package->table[ i ].fileSize = indicies[ i ].size;
		package->table[ i ].offset = indicies[ i ].offset;

		const char *fileName = GetStringForHash( indicies[ i ].hash );
		if ( fileName == NULL ) {
			snprintf( package->table[ i ].fileName, sizeof( PLPath ), "%X", indicies[ i ].hash );
		} else {
			snprintf( package->table[ i ].fileName, sizeof( PLPath ), "%s", fileName );
		}
	}

	PlFree( indicies );

	return package;
}

PLPackage *Core_CLU_LoadPackage( const char *path ) {
	PLFile *file = PlOpenFile( path, false );
	if ( file == NULL ) {
		return NULL;
	}

	PLPackage *package = ParseCLUFile( file );

	PlCloseFile( file );

	return package;
}
