/**
 * Hei Platform Library
 * Copyright (C) 2017-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is licensed under MIT. See LICENSE for more details.
 */

#include <plcore/pl.h>
#include <plcore/pl_filesystem.h>
#include <plcore/pl_package.h>

/* This outline is probably specific to Future Tactics,
 * which is based on RenderWare but appears to use it's
 * own package format. */

typedef struct PakIndex {
	char fileName[ 48 ];
	uint32_t offset;
	uint32_t lflag;
} PakIndex;

static PLPackage *FTactics_PAK_ParseFile( PLFile *file ) {
	bool status;

	/* first we're provided with an indication of how many files are in the package */
	unsigned int numFiles = PlReadInt32( file, false, &status );
	if ( !status ) {
		return NULL;
	}

	PakIndex *indices = PlCAllocA( numFiles, sizeof( PakIndex ) );
	if ( indices == NULL ) {
		return NULL;
	}

	PlReadFile( file, indices, sizeof( PakIndex ), numFiles );

	const char *path = PlGetFilePath( file );
	PLPackage *package = PlCreatePackageHandle( path, numFiles, NULL );
	for ( unsigned int i = 0; i < numFiles; ++i ) {
		snprintf( package->table[ i ].fileName, sizeof( package->table[ i ].fileName ), "%s", indices[ i ].fileName );

#if 0 /* this appears to be wrong, sadly, so for now just dump the compressed file */
		/* extract the flag from the end of the index */
		uint8_t flag = ( indices[ i ].lflag & 0xFF000000 ) >> 24;
		indices[ i ].lflag &= 0xFFFFFF;

		if ( flag == 0x80 ) { /* indicates the file is compressed */
			package->table[ i ].compressedSize = indices[ i ].lflag;
			package->table[ i ].compressionType = PL_COMPRESSION_ZLIB;

			if ( !gInterface->FileSeek( file, indices[ i ].offset, PL_SEEK_SET ) ) {
				continue;
			}

			package->table[ i ].fileSize = gInterface->ReadInt32( file, false, &status );
			package->table[ i ].offset += 4;
			continue;
		}
#endif

		package->table[ i ].compressedSize = 0;
		package->table[ i ].compressionType = PL_COMPRESSION_NONE;
		package->table[ i ].fileSize = indices[ i ].lflag;
		package->table[ i ].offset = indices[ i ].offset;
	}

	PlFree( indices );

	return package;
}

PLPackage *FTactics_PAK_LoadFile( const char *path ) {
	PLFile *file = PlOpenFile( path, false );
	if ( file == NULL ) {
		return NULL;
	}

	PLPackage *package = FTactics_PAK_ParseFile( file );

	PlCloseFile( file );

	return package;
}
