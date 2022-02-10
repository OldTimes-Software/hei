/**
 * Hei Platform Library
 * Copyright (C) 2017-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is licensed under MIT. See LICENSE for more details.
 */

#include <plcore/pl_package.h>

/* Eradicator's BDIR package format */

typedef struct BdirIndex {
	char name[ 12 ];
	int32_t offset;
	int32_t size;
} BdirIndex;

PLPackage *Eradicator_RID_LoadFile( const char *path ) {
	PLFile *filePtr = PlOpenFile( path, false );
	if ( filePtr == NULL ) {
		return NULL;
	}

	/* read in the header */

	char identifier[ 4 ];
	if ( PlReadFile( filePtr, identifier, 1, sizeof( identifier ) ) != sizeof( identifier ) ) {
		PlCloseFile( filePtr );
		return NULL;
	}

	if ( !(
	             identifier[ 0 ] == 'R' &&
	             identifier[ 1 ] == 'I' &&
	             identifier[ 2 ] == 'D' &&
	             identifier[ 3 ] == 'B' ) ) {
		PlReportErrorF( PL_RESULT_FILETYPE, "invalid bdir header, \"%s\"", identifier );
		PlCloseFile( filePtr );
		return NULL;
	}

	bool status;

	int32_t numLumps = PlReadInt32( filePtr, false, &status );
	int32_t tableOffset = PlReadInt32( filePtr, false, &status );
	size_t tableSize = sizeof( BdirIndex ) * numLumps;
	if ( tableOffset + tableSize > PlGetFileSize( filePtr ) ) {
		PlReportErrorF( PL_RESULT_INVALID_PARM1, "invalid table offset" );
		PlCloseFile( filePtr );
		return NULL;
	}

	if ( !status ) {
		PlCloseFile( filePtr );
		return NULL;
	}

	/* and now read in the file table */

	PlFileSeek( filePtr, tableOffset, PL_SEEK_SET );

	BdirIndex *indices = PlMAllocA( tableSize );
	for ( unsigned int i = 0; i < numLumps; ++i ) {
#define cleanup()       \
	PlFree( indices ); \
	PlCloseFile( filePtr )
		if ( PlReadFile( filePtr, indices[ i ].name, 1, 12 ) != 12 ) {
			cleanup();
			return NULL;
		}
		indices[ i ].name[ 11 ] = '\0';

		indices[ i ].offset = PlReadInt32( filePtr, false, &status );
		if ( indices[ i ].offset >= tableOffset ) {
			PlReportErrorF( PL_RESULT_INVALID_PARM1, "invalid file offset for index %d", i );
			cleanup();
			return NULL;
		}

		indices[ i ].size = PlReadInt32( filePtr, false, &status );
		if ( indices[ i ].size >= PlGetFileSize( filePtr ) ) {
			PlReportErrorF( PL_RESULT_INVALID_PARM1, "invalid file size for index %d", i );
			cleanup();
			return NULL;
		}
	}

	PlCloseFile( filePtr );

	if ( !status ) {
		PlFree( indices );
		return NULL;
	}

	/* yay, we're finally done - now to setup the package object */

	PLPackage *package = PlCreatePackageHandle( path, numLumps, NULL );
	for ( unsigned int i = 0; i < package->table_size; ++i ) {
		PLPackageIndex *index = &package->table[ i ];
		index->offset = indices[ i ].offset;
		index->fileSize = indices[ i ].size;
		strncpy( index->fileName, indices[ i ].name, sizeof( index->fileName ) );
	}

	PlFree( indices );

	return package;
}
