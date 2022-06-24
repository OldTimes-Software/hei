/* SPDX-License-Identifier: MIT */
/* Copyright © 2017-2022 Mark E Sowden <hogsy@oldtimes-software.com> */

#include <plcore/pl.h>
#include <plcore/pl_filesystem.h>
#include <plcore/pl_package.h>

/* not exactly my most elegant attempt at a loader, but hey
 * I did my best given the circumstances. FITD was a helpful
 * reference for this.
 *
 * side thought, maybe the TOC begins and ends with 0 as an
 * indicator for where the TOC literally begins and ends?
 */

static PLPackage *ParsePAKFile( PLFile *file ) {
	bool status;
	/* first four bytes are expected to be 0 */
	if ( PlReadInt32( file, false, &status ) != 0 ) {
		PlReportBasicError( PL_RESULT_FILETYPE );
		return NULL;
	}

	/* probably just a general IO error */
	if ( !status ) {
		return NULL;
	}

	/* need to figure out where the file listing ends */
	uint32_t fileOffset = PlReadInt32( file, false, &status );
	if ( !status ) {
		return NULL;
	}

	/* and now we can determine the number of files in the pak */
	uint32_t numFiles = ( fileOffset / 4 ) - 2;

	/* out of paranoia, let's get the last offset and just be sure that's
	 * within the file boundary */
	PlFileSeek( file, -8, PL_SEEK_CUR );
	fileOffset = PlReadInt32( file, false, &status );
	if ( !status ) return NULL;
	size_t fileSize = PlGetFileSize( file );
	if ( fileOffset >= fileSize ) {
		PlReportErrorF( PL_RESULT_FILEERR, "last file index is >= to file size" );
		return NULL;
	}

	/* seek back to the start of the file table */
	PlFileSeek( file, 4, PL_SEEK_SET );

	PLPackage *package = PlCreatePackageHandle( PlGetFilePath( file ), numFiles, NULL );
	PLPackageIndex *indices = package->table;
	for ( unsigned int i = 0; i < numFiles; ++i ) {
		PLFileOffset oldPos = PlGetFileOffset( file );

		/* we start off in the file table, read in the offset, jump to it
		 * and leap ahead by 4 bytes from the offset due to an unknown int */
		int32_t offset = PlReadInt32( file, false, &status );
		PlFileSeek( file, offset + 4, PL_SEEK_SET );

		indices[ i ].compressedSize = PlReadInt32( file, false, &status );
		indices[ i ].fileSize = PlReadInt32( file, false, &status );

		/* read in the compression type */
		uint8_t flags = PlReadInt8( file, &status );
		switch( flags ) {
			default:
			case 0:
				indices[ i ].compressionType = PL_COMPRESSION_NONE;
				break;
			case 1:
				indices[ i ].compressionType = PL_COMPRESSION_IMPLODE;
				break;
			case 4:
				indices[ i ].compressionType = PL_COMPRESSION_GZIP;
				break;
		}

		PlReadInt8( file, &status );

		uint16_t nameLength = PlReadInt16( file, false, &status );
		if ( nameLength > 0 ) {
			PlReadFile( file, indices[ i ].fileName, sizeof( char ), nameLength );
		} else {
			snprintf( indices[ i ].fileName, sizeof( indices[ i ].fileName ), "%u.bin", i );
		}

		indices[ i ].offset = PlGetFileOffset( file );

		/* seek to the next slot */
		PlFileSeek( file, oldPos + 4, PL_SEEK_SET );
	}

	return package;
}

PLPackage *AITD_PAK_LoadPackage( const char *path ) {
	PLFile *file = PlOpenFile( path, false );
	if ( file == NULL ) {
		return NULL;
	}

	PLPackage *package = ParsePAKFile( file );

	PlCloseFile( file );

	return package;
}
