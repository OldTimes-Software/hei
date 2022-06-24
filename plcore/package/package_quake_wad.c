/**
 * Hei Platform Library
 * Copyright (C) 2017-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is licensed under MIT. See LICENSE for more details.
 */

#include <plcore/pl.h>
#include <plcore/pl_filesystem.h>
#include <plcore/pl_package.h>

/* id Software's Quake WAD package format */

#define WAD2_MAGIC       PL_MAGIC_TO_NUM( 'W', 'A', 'D', '2' )
#define WAD2_NAME_LENGTH 16

#define WAD3_MAGIC PL_MAGIC_TO_NUM( 'W', 'A', 'D', '3' )

typedef struct WAD2Index {
	int32_t offset;
	int32_t cSize;
	int32_t size;
	uint8_t type;
	uint8_t compression;
	uint16_t unused;
	char name[ WAD2_NAME_LENGTH ];
} WAD2Index;

static PLPackage *ParseWAD2File( PLFile *file ) {
	uint32_t magic = PlReadInt32( file, false, NULL );
	if ( magic != WAD2_MAGIC && magic != WAD3_MAGIC ) {
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

	unsigned int tableSize = sizeof( WAD2Index ) * numLumps;
	if ( tableOffset + tableSize > PlGetFileSize( file ) ) {
		PlReportErrorF( PL_RESULT_FILEREAD, "invalid table offset location: %u", tableSize );
		return NULL;
	}

	PlFileSeek( file, tableOffset, PL_SEEK_SET );

	const char *path = PlGetFilePath( file );
	PLPackage *package = PlCreatePackageHandle( path, numLumps, NULL );
	for ( unsigned int i = 0; i < package->table_size; ++i ) {
		PLPackageIndex *index = &package->table[ i ];
		index->offset = PlReadInt32( file, false, NULL );
		index->compressedSize = PlReadInt32( file, false, NULL );
		index->fileSize = PlReadInt32( file, false, NULL );

		const char *hint;
		uint8_t type = PlReadInt8( file, NULL ); /* type */
		switch ( type ) {
			default:
				hint = ".none";
				break;
			case 1:
				hint = ".label";
				break;
			case 64:
				hint = ".palette";
				break;
			case 65:
				hint = ".colormap";
				break;
			case 66:
				hint = ".qpic";
				break;
			case 67:
				hint = ".miptex";
				break;
			case 68:
				hint = ".raw";
				break;
			case 69:
				hint = ".colormap2";
				break;
			case 70:
				hint = ".font";
				break;
		}

		PlReadInt8( file, NULL );         /* compression (afaik, never used) */
		PlReadInt16( file, false, NULL ); /* unused */

		PlReadFile( file, index->fileName, sizeof( char ), WAD2_NAME_LENGTH );
		strcat( index->fileName, hint );
	}

	return package;
}

PLPackage *PlLoadWAD2Package_( const char *path ) {
	PLFile *file = PlOpenFile( path, false );
	if ( file == NULL ) {
		return NULL;
	}

	PLPackage *package = ParseWAD2File( file );

	PlCloseFile( file );

	return package;
}
