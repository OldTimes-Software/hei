/**
 * Hei Platform Library
 * Copyright (C) 2017-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is licensed under MIT. See LICENSE for more details.
 */

#include "package_private.h"

#define PAK_MAGIC PL_MAGIC_TO_NUM( 'P', 'A', 'C', 'K' )

#define PAK_INDEX_FILENAME_LENGTH 56
#define PAK_INDEX_LENGTH          64

static PLPackage *ParsePAKFile( PLFile *file ) {
	uint32_t magic = PlReadInt32( file, false, NULL );
	if ( magic != PAK_MAGIC ) {
		PlReportErrorF( PL_RESULT_FILETYPE, "invalid magic: \"%s\"", magic );
		return NULL;
	}

	size_t fileSize = PlGetFileSize( file );
	uint32_t tocOffset = PlReadInt32( file, false, NULL );
	if ( tocOffset >= fileSize ) {
		PlReportErrorF( PL_RESULT_FILEERR, "invalid table offset: %u\n", tocOffset );
		return NULL;
	}

	uint32_t tocSize = PlReadInt32( file, false, NULL );
	if ( tocSize == 0 || tocSize + tocOffset >= fileSize ) {
		PlReportErrorF( PL_RESULT_FILEERR, "invalid table size: %u\n", tocSize );
		return NULL;
	}

	if ( !PlFileSeek( file, tocOffset, PL_SEEK_SET ) ) {
		return NULL;
	}

	unsigned int numFiles = tocSize / PAK_INDEX_LENGTH;
	PLPackage *package = PlCreatePackageHandle( PlGetFilePath( file ), numFiles, NULL );
	for ( unsigned int i = 0; i < numFiles; ++i ) {
		PlReadFile( file, package->table[ i ].fileName, sizeof( char ), PAK_INDEX_FILENAME_LENGTH );
		package->table[ i ].offset = PlReadInt32( file, false, NULL );
		package->table[ i ].fileSize = PlReadInt32( file, false, NULL );
	}

	return package;
}

PLPackage *PlLoadPAKPackage_( const char *path ) {
	PLFile *file = PlOpenFile( path, false );
	if ( file == NULL ) {
		return NULL;
	}

	PLPackage *package = ParsePAKFile( file );

	PlCloseFile( file );

	return package;
}
