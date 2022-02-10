/**
 * Hei Platform Library
 * Copyright (C) 2017-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is licensed under MIT. See LICENSE for more details.
 */

#include <plcore/pl.h>
#include <plcore/pl_filesystem.h>
#include <plcore/pl_package.h>

/* some information pulled from
 * http://wiki.xentax.com/index.php/Galleon_-_Island_Of_Mystery_(XBox)
 * should handle packages from both the retail game and prototype */

#define PKG_MAGIC   PL_MAGIC_TO_NUM( 'C', 'R', 'S', 'R' )
#define PKG_MAX_DIR 64

/* terminators indicate different things,
 * only two of these are documented but
 * there are others as well */
#define PKG_TERM_NEW_EXTENSION 130
#define PKG_TERM_END_FILENAME  129
#define PKG_TERM_END_TABLE     255

typedef struct PkgHeader {
	uint32_t magic; /* CRSR */
	uint16_t version; /* ? */
	uint16_t unknown;
	uint32_t length;
	uint32_t unused;
	uint32_t startOffset;
} PkgHeader;

typedef struct PkgIndex {
	char fileName[ 64 ];
	unsigned int fileNameLength;
	uint32_t offset;
	uint32_t length;
} PkgIndex;

static PLPackage *PKG_ReadFile( PLFile *file ) {
	bool status;

	/* first load in the header */
	PkgHeader header;
	header.magic = PlReadInt32( file, false, &status );
	header.version = PlReadInt16( file, false, &status );
	header.unknown = PlReadInt16( file, false, &status );
	header.length = PlReadInt32( file, false, &status );
	header.unused = PlReadInt32( file, false, &status );
	header.startOffset = PlReadInt32( file, false, &status );
	if ( !status ) {
		return NULL;
	}

	/* verify it */
	if ( header.magic != PKG_MAGIC ) {
		PlReportError( PL_RESULT_FILETYPE, "magic was \"%s\", expected \"%s\"", header.magic, PKG_MAGIC );
		return NULL;
	}

	/* read in the directory name */
	uint16_t dirLength = PlReadInt16( file, false, &status );
	if ( !status ) {
		return NULL;
	} else if ( dirLength >= PKG_MAX_DIR ) {
		PlReportError( PL_RESULT_FILETYPE, "unexpected directory length (%d vs max %d)", dirLength, PKG_MAX_DIR );
		return NULL;
	}
	/* was originally gonna allocate a buffer instead, but don't currently think
	 * this is necessary - if we hit the limit above, i'll switch this over */
	char dirName[ 64 ];
	if ( PlReadFile( file, dirName, sizeof( char ), dirLength ) != dirLength ) {
		return NULL;
	}

	unsigned int maxFiles = 2048;
	PkgIndex *fileTable = PlMAlloc( sizeof( PkgIndex ) * maxFiles, true );
	unsigned int numFiles = 0;

	char fileExtension[ 16 ];
	uint8_t rule;
	while ( ( rule = PlReadInt8( file, &status ) ) != PKG_TERM_END_TABLE ) {
		if ( !status ) {
			PlFree( fileTable );
			return NULL;
		}

		if ( numFiles >= maxFiles ) {
			maxFiles += 16;
			fileTable = PlReAlloc( fileTable, sizeof( PkgIndex ) * maxFiles, true );
		}

		PkgIndex *curIndex = &fileTable[ numFiles ];
		memset( curIndex, 0, sizeof( PkgIndex ) );

		/* read in the name of the file */
		if ( rule > 0 ) {
			PkgIndex *lastFile = &fileTable[ numFiles - 1 ];
			curIndex->fileNameLength = rule - lastFile->fileNameLength;
			strncpy( curIndex->fileName, lastFile->fileName, curIndex->fileNameLength );
		}
		for ( ; curIndex->fileNameLength < sizeof( curIndex->fileName ); ++curIndex->fileNameLength ) {
			uint8_t c = PlReadInt8( file, &status );
			if ( c == PKG_TERM_END_FILENAME ) {
				break;
			}

			curIndex->fileName[ curIndex->fileNameLength ] = c;
		}
		curIndex->fileName[ curIndex->fileNameLength ] = '\0';

		/* now check for and read in the extension */
		if ( numFiles == 0 ) {
		}

		/* documentation on xentax says that terminator is indicated
		 * by '243' - however packages from the prototype do not respect
		 * this rule. instead will just iterate until we hit a non-ascii
		 * character. */

		numFiles++;
	}
}

PLPackage *PKG_LoadFile( const char *path ) {
	PLFile *file = PlOpenFile( path, false );
	if ( file == NULL ) {
		return NULL;
	}

	PLPackage *package = PKG_ReadFile( file );

	PlCloseFile( file );

	return package;
}
