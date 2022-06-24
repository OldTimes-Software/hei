/**
 * Hei Platform Library
 * Copyright (C) 2017-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is licensed under MIT. See LICENSE for more details.
 */

#include "package_private.h"

/* VPK as used by VTMB - doesn't appear to have any relation
 * to the VPK format introduced by Valve later on, hence the
 * 'vmtb_vpk' designation. */

#define VPK_TOC_OFFSET   5
#define VPK_TOC_NUMFILES 9

static PLPackage *ParseVPKFile( PLFile *file ) {
	size_t size = PlGetFileSize( file );

	if ( !PlFileSeek( file, size - VPK_TOC_NUMFILES, PL_SEEK_SET ) ) {
		return NULL;
	}

	uint32_t numFiles = PlReadInt32( file, false, NULL );
	if ( numFiles == 0 ) {
		PlReportErrorF( PL_RESULT_FILEERR, "no files in package\n" );
		return NULL;
	}

	uint32_t tocOffset = PlReadInt32( file, false, NULL );
	if ( tocOffset == 0 || tocOffset >= size ) {
		PlReportErrorF( PL_RESULT_FILEERR, "invalid table offset: %u\n", tocOffset );
		return NULL;
	}

	if ( !PlFileSeek( file, tocOffset, PL_SEEK_SET ) ) {
		return NULL;
	}

	PLPackage *package = PlCreatePackageHandle( PlGetFilePath( file ), numFiles, NULL );
	for ( unsigned int i = 0; i < numFiles; ++i ) {
		uint32_t nameLength = PlReadInt32( file, false, NULL );
		char *name = PL_NEW_( char, nameLength + 1 );
		PlReadFile( file, name, sizeof( char ), nameLength );
		snprintf( package->table[ i ].fileName, sizeof( package->table[ i ].fileName ), "%s", name );
		PL_DELETE( name );

		package->table[ i ].offset = PlReadInt32( file, false, NULL );
		package->table[ i ].fileSize = PlReadInt32( file, false, NULL );
	}

	return package;
}

PLPackage *PlLoadVPKPackage_( const char *path ) {
	PLFile *file = PlOpenFile( path, false );
	if ( file == NULL ) {
		return NULL;
	}

	PLPackage *package = ParseVPKFile( file );

	PlCloseFile( file );

	return package;
}
