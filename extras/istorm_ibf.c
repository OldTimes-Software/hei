/**
 * Hei Platform Library
 * Copyright (C) 2017-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is licensed under MIT. See LICENSE for more details.
 */

#include <plcore/pl_package.h>

/* 4X Technologies Package Format
 * These come in both an IBF and LST format;
 * the LST file lists all of the contents
 * of the IBF.
 *
 * There doesn't appear to be any form of
 * compression used on these packages. */

PLPackage *IStorm_LST_LoadFile( const char *path ) {
	PLFile *fh = PlOpenFile( path, false );
	if ( fh == NULL ) {
		return NULL;
	}

	PLPackage *package = NULL;

	/* any files we're going to load will be from the IBF, not the LST */
	char ibf_path[ PL_SYSTEM_MAX_PATH + 1 ];
	strncpy( ibf_path, path, strlen( path ) - 3 );
	strncat( ibf_path, "ibf", PL_SYSTEM_MAX_PATH );
	if ( !PlFileExists( ibf_path ) ) {
		PlReportErrorF( PL_RESULT_FILEPATH, "failed to open ibf package at \"%s\", aborting", ibf_path );
		goto ABORT;
	}

	//DebugPrint("LST %s\n", path);
	//DebugPrint("IBF %s\n", ibf_path);

	/* grab the IBF size so we can do some sanity checking later */
	size_t ibf_size = PlGetLocalFileSize( ibf_path );
	if ( ibf_size == 0 ) {
		PlReportErrorF( PL_RESULT_FILESIZE, "invalid ibf \"%s\" size of 0, aborting", ibf_path );
		goto ABORT;
	}

	/* read in the ident */
	char ident[ 8 ];
	if ( PlReadFile( fh, ident, sizeof( char ), 8 ) != 8 ) {
		PlReportErrorF( PL_RESULT_FILETYPE, "failed to read identification, aborting" );
		goto ABORT;
	}

	if ( strncmp( ident, "_TSL1.0V", 8 ) != 0 ) {
		PlReportErrorF( PL_RESULT_FILETYPE, "invalid file ident, \"%s\", aborting", ident );
		goto ABORT;
	}

	/* sanity checking */
	uint32_t num_indices;
	if ( PlReadFile( fh, &num_indices, sizeof( unsigned int ), 1 ) != 1 ) {
		PlReportErrorF( PL_RESULT_FILEREAD, "failed to read in indices count from lst, aborting" );
		goto ABORT;
	}

	if ( num_indices > 4096 ) {
		PlReportErrorF( PL_RESULT_FILESIZE, "larger than expected package, aborting" );
		goto ABORT;
	}

	//DebugPrint("LST INDICES %u\n", num_indices);

	struct {
		char name[ 64 ];
		uint32_t data_offset;
		uint32_t data_length;
	} index;

	package = PlCreatePackageHandle( path, num_indices, NULL );
	for ( unsigned int i = 0; i < num_indices; ++i ) {
		if ( PlIsEndOfFile( fh ) != 0 ) {
			printf( "Unexpected end of package in %s, ignoring!\n", path );
			break;
		}

		if ( PlReadFile( fh, &index, sizeof( index ), 1 ) != 1 ) {
			PlReportErrorF( PL_RESULT_FILEREAD, "failed to read index at %d, aborting", PlGetFileOffset( fh ) );
			goto ABORT;
		}

		//DebugPrint("LST INDEX %s\n", index.name);

		if ( index.data_offset >= ibf_size || ( uint64_t ) ( index.data_offset ) + ( uint64_t ) ( index.data_length ) > ibf_size ) {
			PlReportErrorF( PL_RESULT_FILESIZE, "offset/length falls beyond IBF size, aborting" );
			goto ABORT;
		}

		strncpy( package->table[ i ].fileName, index.name, sizeof( package->table[ i ].fileName ) );
		package->table[ i ].fileName[ sizeof( package->table[ i ].fileName ) - 1 ] = '\0';
		package->table[ i ].fileSize = index.data_length;
		package->table[ i ].offset = index.data_offset;
	}

	PlCloseFile( fh );

	return package;

ABORT:

	if ( package != NULL ) {
		PlDestroyPackage( package );
	}

	PlCloseFile( fh );

	return NULL;
}
