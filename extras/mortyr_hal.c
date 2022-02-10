/**
 * Hei Platform Library
 * Copyright (C) 2017-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is licensed under MIT. See LICENSE for more details.
 */

#include <plcore/pl_package.h>

/* Mortyr's APUK package format */

PLPackage *Mortyr_HAL_LoadFile( const char *path ) {
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
	             identifier[ 0 ] == 'A' &&
	             identifier[ 1 ] == 'P' &&
	             identifier[ 2 ] == 'U' &&
	             identifier[ 3 ] == 'K' ) ) {
		PlReportErrorF( PL_RESULT_FILETYPE, "invalid package identifier, \"%s\"", identifier );
		PlCloseFile( filePtr );
		return NULL;
	}

	typedef struct FileIndex {
		uint32_t size;
		uint32_t offset;
		int32_t unknown[ 2 ];
		char name[ 16 ];
	} FileIndex;

	bool status;
	uint32_t numFiles = PlReadInt32( filePtr, false, &status );
	if ( !status ) {
		PlCloseFile( filePtr );
		return NULL;
	}

	/* make sure the file table is valid */
	size_t tableSize = sizeof( FileIndex ) * numFiles;
	if ( tableSize + 32 > PlGetFileSize( filePtr ) ) {
		PlReportErrorF( PL_RESULT_INVALID_PARM1, "invalid file table" );
		PlCloseFile( filePtr );
		return NULL;
	}

	/* 24 bytes of nothing, can't think what this was intended for... */
	if ( !PlFileSeek( filePtr, 24, PL_SEEK_CUR ) ) {
		PlCloseFile( filePtr );
		return NULL;
	}

	FileIndex *indices = PlMAllocA( sizeof( FileIndex ) * numFiles );
	for ( unsigned int i = 0; i < numFiles; ++i ) {
		indices[ i ].size = PlReadInt32( filePtr, false, &status );
		indices[ i ].offset = PlReadInt32( filePtr, false, &status );
		indices[ i ].unknown[ 0 ] = PlReadInt32( filePtr, false, &status );
		indices[ i ].unknown[ 1 ] = PlReadInt32( filePtr, false, &status );

		if ( PlReadFile( filePtr, indices[ i ].name, 1, 16 ) != 16 ) {
			status = false;
		}
	}

	PlCloseFile( filePtr );

	if ( !status ) {
		PlFree( indices );
		return NULL;
	}

	/* yay, we're finally done - now to setup the package object */

	PLPackage *package = PlCreatePackageHandle( path, numFiles, NULL );
	for ( unsigned int i = 0; i < package->table_size; ++i ) {
		PLPackageIndex *index = &package->table[ i ];
		index->offset = indices[ i ].offset;
		index->fileSize = indices[ i ].size;
		strncpy( index->fileName, indices[ i ].name, sizeof( index->fileName ) );
	}

	PlFree( indices );

	return package;
}
