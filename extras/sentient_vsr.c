/**
 * Hei Platform Library
 * Copyright (C) 2017-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is licensed under MIT. See LICENSE for more details.
 */

#include <plcore/pl_package.h>

/* Loader for Sentient's VSR package format */

typedef struct VSRChunkHeader {
	char identifier[ 4 ];
	uint32_t length;
} VSRChunkHeader;

typedef struct VSRDirectoryIndex {
	uint32_t offset; /* start offset for data */
	uint32_t length; /* length of the file in bytes */
	uint32_t unknown0[ 8 ];
} VSRDirectoryIndex;
PL_STATIC_ASSERT( sizeof( VSRDirectoryIndex ) == 40, "needs to be 40 bytes" );

typedef struct VSRDirectoryChunk {
	VSRChunkHeader header; /* DIRC */
	uint32_t num_indices;  /* number of indices in this chunk */
} VSRDirectoryChunk;
PL_STATIC_ASSERT( sizeof( VSRDirectoryChunk ) == 12, "needs to be 12 bytes" );

typedef struct VSRStringIndex {
	char file_name[ 256 ];
} VSRStringIndex;

typedef struct VSRStringChunk {
	VSRChunkHeader header; /* STRT */
	uint32_t num_indices;
} VSRStringChunk;

typedef struct VSRHeader {
	VSRChunkHeader header; /* VSR1 */
	uint32_t unknown0;
	uint32_t unknown1; /* always 4 */
	uint32_t unknown2;
	uint32_t length_data;  /* length of file, minus header */
	uint32_t num_indices;  /* number of files in the package */
	uint32_t num_indices2; /* ditto to the above? */
} VSRHeader;
PL_STATIC_ASSERT( sizeof( VSRHeader ) == 32, "needs to be 32 bytes" );

PLPackage *Sentient_VSR_LoadFile( const char *path ) {
	PLFile *fp = PlOpenFile( path, false );
	if ( fp == NULL ) {
		return NULL;
	}

	VSRHeader chunk_header;
	VSRDirectoryChunk chunk_directory;
	VSRStringChunk chunk_strings;

	VSRDirectoryIndex *directories = NULL;
	VSRStringIndex *strings = NULL;

	/* load in all the file data first */

	PlReadFile( fp, &chunk_header, sizeof( VSRHeader ), 1 );
	if ( strncmp( chunk_header.header.identifier, "1RSV", 4 ) == 0 ) {
		PlReadFile( fp, &chunk_directory, sizeof( VSRDirectoryChunk ), 1 );
		if ( strncmp( chunk_directory.header.identifier, "CRID", 4 ) == 0 ) {
			directories = PlMAllocA( sizeof( VSRDirectoryIndex ) * chunk_directory.num_indices );
			PlReadFile( fp, directories, sizeof( VSRDirectoryIndex ), chunk_directory.num_indices );

			/* skip VSRN chunk, seems to be unused? */
			PlFileSeek( fp, 12, PL_SEEK_CUR );

			PlReadFile( fp, &chunk_strings, sizeof( VSRStringChunk ), 1 );
			if ( strncmp( chunk_strings.header.identifier, "TRTS", 4 ) == 0 ) {
				/* read in all the string offsets, which in turn helps us determine
				 * the length of each string */

				/* fuck this, let's do this the lazy way */
				PlFileSeek( fp, sizeof( uint32_t ) * chunk_strings.num_indices, PL_SEEK_CUR );
				strings = PlCAllocA( sizeof( VSRStringIndex ), chunk_strings.num_indices );
				for ( unsigned int i = 0; i < chunk_strings.num_indices; ++i ) {
					for ( unsigned int j = 0; j < 256; ++j ) {
						strings[ i ].file_name[ j ] = PlReadInt8( fp, NULL );
						if ( strings[ i ].file_name[ j ] == '\0' ) {
							break;
						}
					}
				}
			} else {
				PlReportErrorF( PL_RESULT_FILETYPE, "failed to read STRS header" );
			}
		} else {
			PlReportErrorF( PL_RESULT_FILETYPE, "failed to read DIRC header" );
		}
	} else {
		PlReportErrorF( PL_RESULT_FILETYPE, "failed to read VSR1 header" );
	}

	PlCloseFile( fp );

	if ( PlGetFunctionResult() != PL_RESULT_SUCCESS ) {
		PlFree( directories );
		PlFree( strings );
		return NULL;
	}

	/* now create our package handle */

	PLPackage *package = PlCreatePackageHandle( path, chunk_directory.num_indices, NULL );
	for ( unsigned int i = 0; i < package->table_size; ++i ) {
		PLPackageIndex *index = &package->table[ i ];
		index->offset = directories[ i ].offset;
		index->fileSize = directories[ i ].length;
		strncpy( index->fileName, strings[ i ].file_name, sizeof( index->fileName ) );
	}

	PlFree( directories );
	PlFree( strings );

	if ( PlGetFunctionResult() != PL_RESULT_SUCCESS ) {
		PlDestroyPackage( package );
		package = NULL;
	}

	return package;
}
