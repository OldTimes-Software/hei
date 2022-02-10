/**
 * Hei Platform Library
 * Copyright (C) 2017-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is licensed under MIT. See LICENSE for more details.
 */

#include <plcore/pl_package.h>

/* Outcast OPK format */

static PLPackage *Outcast_OPK_ParseFile( PLFile *file ) {
	static const int32_t opkMagic = 0x6e71;
	int32_t magic = PlReadInt32( file, false, NULL );
	if ( magic != opkMagic ) {
		PlReportErrorF( PL_RESULT_FILETYPE, "unexpected magic: %d", magic );
		return NULL;
	}

	/* these seem to provide some sort of metadata for
	 * identifying the file type? noticed other files
	 * feature the same magic, but then these same bytes
	 * are different depending on the type. interesting. */
	if ( !PlFileSeek( file, 12, PL_SEEK_CUR ) ) {
		PlReportErrorF( PL_RESULT_FILEREAD, "failed to seek to table" );
		return NULL;
	}

	int32_t numFiles = PlReadInt32( file, false, NULL );
	if ( numFiles <= 0 ) {
		PlReportErrorF( PL_RESULT_FILEREAD, "no files in package" );
		return NULL;
	}

	/* read in toc */
	PLPackage *package = PlCreatePackageHandle( PlGetFilePath( file ), numFiles, NULL );
	for ( int32_t i = 0; i < numFiles; ++i ) {
		PLPackageIndex *index = &package->table[ i ];
		int32_t nameLength = PlReadInt32( file, false, NULL );
		if ( nameLength >= sizeof( index->fileName ) || nameLength <= 0 ) {
			PlReportErrorF( PL_RESULT_FILEREAD, "invalid index name length, %d", i );
			PlDestroyPackage( package );
			return NULL;
		}

		if ( PlReadFile( file, index->fileName, sizeof( char ), nameLength ) != nameLength ) {
			PlReportErrorF( PL_RESULT_FILEREAD, "failed to read in file name for index %d", i );
			PlDestroyPackage( package );
			return NULL;
		}

		bool status;
		index->offset = PlReadInt32( file, false, &status );
		index->compressedSize = PlReadInt32( file, false, &status );
		index->fileSize = PlReadInt32( file, false, &status );
		index->compressionType = PL_COMPRESSION_IMPLODE;
	}

	return package;
}

PLPackage *Outcast_OPK_LoadFile( const char *path ) {
	PLFile *file = PlOpenFile( path, false );
	if ( file == NULL ) {
		return NULL;
	}

	PLPackage *package = Outcast_OPK_ParseFile( file );

	PlCloseFile( file );

	return package;
}
