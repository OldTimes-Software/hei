// SPDX-License-Identifier: MIT
// Copyright © 2017-2023 Mark E Sowden <hogsy@oldtimes-software.com>

#include <plcore/pl.h>
#include <plcore/pl_filesystem.h>
#include <plcore/pl_package.h>

/* id Software's Doom WAD package format */

typedef struct WadIndex {
	int32_t offset;
	int32_t size;
	char name[ 8 ];
} WadIndex;

static PLPackage *ParseWADFile( PLFile *file ) {
	char magic[ 4 ];
	PlReadFile( file, magic, sizeof( char ), 4 );
	if ( memcmp( magic, "IWAD", 4 ) != 0 && memcmp( magic, "PWAD", 4 ) != 0 ) {
		PlReportErrorF( PL_RESULT_FILETYPE, "invalid magic: \"%s\"", magic );
		return NULL;
	}

	int32_t numLumps = PlReadInt32( file, false, NULL );
	if ( numLumps <= 0 ) {
		PlReportErrorF( PL_RESULT_FILEREAD, "invalid number of lumps: %d", numLumps );
		return NULL;
	}

	int32_t tableOffset = PlReadInt32( file, false, NULL );
	if ( tableOffset <= 0 ) {
		PlReportErrorF( PL_RESULT_FILEREAD, "invalid table offset: %d", tableOffset );
		return NULL;
	}

	unsigned int tableSize = sizeof( WadIndex ) * numLumps;
	if ( tableOffset + tableSize > PlGetFileSize( file ) ) {
		PlReportErrorF( PL_RESULT_FILEREAD, "invalid table offset location: %u", tableSize );
		return NULL;
	}

	PlFileSeek( file, tableOffset, PL_SEEK_SET );

	WadIndex *indices = PlMAllocA( tableSize );
	for ( int i = 0; i < numLumps; ++i ) {
		indices[ i ].offset = PlReadInt32( file, false, NULL );
		if ( indices[ i ].offset == 0 || indices[ i ].offset >= tableOffset ) {
			PlFree( indices );
			PlReportErrorF( PL_RESULT_FILEREAD, "invalid file offset for index %d", i );
			return NULL;
		}

		indices[ i ].size = PlReadInt32( file, false, NULL );
		if ( indices[ i ].size >= ( int ) PlGetFileSize( file ) ) {
			PlFree( indices );
			PlReportErrorF( PL_RESULT_FILEREAD, "invalid file size for index %d", i );
			return NULL;
		}

		if ( PlReadFile( file, indices[ i ].name, 1, 8 ) != 8 ) {
			PlFree( indices );
			return NULL;
		}
	}

	const char *path = PlGetFilePath( file );
	PLPackage *package = PlCreatePackageHandle( path, numLumps, NULL );
	for ( unsigned int i = 0; i < package->table_size; ++i ) {
		PLPackageIndex *index = &package->table[ i ];
		index->offset = indices[ i ].offset;
		index->fileSize = indices[ i ].size;
		strncpy( index->fileName, indices[ i ].name, sizeof( index->fileName ) );
		index->fileName[ 8 ] = '\0';
	}

	return package;
}

PLPackage *PlLoadIWADPackage_( const char *path ) {
	PLFile *file = PlOpenFile( path, false );
	if ( file == NULL ) {
		return NULL;
	}

	PLPackage *package = ParseWADFile( file );

	PlCloseFile( file );

	return package;
}
