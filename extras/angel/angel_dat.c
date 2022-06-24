/* SPDX-License-Identifier: MIT */
/* Copyright © 2017-2022 Mark E Sowden <hogsy@oldtimes-software.com> */

#include <plcore/pl.h>
#include <plcore/pl_package.h>

#define ANGEL_DAT_MAGIC  PL_MAGIC_TO_NUM( 'D', 'A', 'V', 'E' ) /* oni2 */
#define ANGEL_DAT_MAGIC2 PL_MAGIC_TO_NUM( 'D', 'a', 'v', 'e' ) /* tsnw, rdr, sh2 */

typedef struct AngelDATHeader {
	uint32_t magic;// 'DAVE'/'Dave'
	uint32_t tocIndices;
	uint32_t tocLength;
	uint32_t stringTableLength;
} AngelDATHeader;

typedef struct AngelDATIndex {
	uint32_t nameOffset;    // offset into string table
	uint32_t offset;        // absolute offset
	uint32_t size;          // size in memory
	uint32_t compressedSize;// size on disk - if same as memory, not compressed
} AngelDATIndex;

static PLPackage *ParseDATFile( PLFile *file ) {
	AngelDATHeader header;
	PL_ZERO_( header );

	header.magic = PlReadInt32( file, false, NULL );
	if ( header.magic != ANGEL_DAT_MAGIC && header.magic != ANGEL_DAT_MAGIC2 ) {
		PlReportBasicError( PL_RESULT_FILETYPE );
		return NULL;
	}

	header.tocIndices = PlReadInt32( file, false, NULL );
	header.tocLength = PlReadInt32( file, false, NULL );
	header.stringTableLength = PlReadInt32( file, false, NULL );

	size_t fileSize = PlGetFileSize( file );
	if ( header.tocLength >= fileSize || header.stringTableLength >= fileSize ) {
		PlReportBasicError( PL_RESULT_FILEERR );
		return NULL;
	}

	/* toc is always padded to 2048 bytes */
	PlFileSeek( file, 2048, PL_SEEK_SET );

	AngelDATIndex *indices = PL_NEW_( AngelDATIndex, header.tocIndices );
	for ( unsigned int i = 0; i < header.tocIndices; ++i ) {
		indices[ i ].nameOffset = PlReadInt32( file, false, NULL );
		indices[ i ].offset = PlReadInt32( file, false, NULL );
		indices[ i ].size = PlReadInt32( file, false, NULL );
		indices[ i ].compressedSize = PlReadInt32( file, false, NULL );
	}

	/* and now seek to the string table */
	PlFileSeek( file, 2048 + header.tocLength, PL_SEEK_SET );

	char *names = PL_NEW_( char, header.stringTableLength );
	PlReadFile( file, names, sizeof( char ), header.stringTableLength );

	bool encodedStrings = false;
	if ( header.magic == ANGEL_DAT_MAGIC2 ) {
		encodedStrings = true;
	}

	const char *path = PlGetFilePath( file );
	PLPackage *package = PlCreatePackageHandle( path, header.tocIndices, NULL );
	for ( unsigned int i = 0; i < package->table_size; ++i ) {
		package->table[ i ].offset = indices[ i ].offset;
		package->table[ i ].compressedSize = indices[ i ].compressedSize;
		package->table[ i ].fileSize = indices[ i ].size;
		if ( indices[ i ].compressedSize != indices[ i ].size ) {
			package->table[ i ].compressionType = PL_COMPRESSION_GZIP;
		}

		/* no idea right now, so just spit by index */
		if ( encodedStrings ) {
			snprintf( package->table[ i ].fileName, sizeof( package->table[ i ].fileName ), "%d", i );
			continue;
		}

		strncpy( package->table[ i ].fileName, &names[ indices[ i ].nameOffset ], sizeof( package->table[ i ].fileName ) - 1 );
	}

	PlFree( indices );
	PlFree( names );

	return package;
}

PLPackage *Angel_DAT_LoadPackage( const char *path ) {
	PLFile *file = PlOpenFile( path, false );
	if ( file == NULL ) {
		return NULL;
	}

	PLPackage *package = ParseDATFile( file );

	PlCloseFile( file );

	return package;
}
