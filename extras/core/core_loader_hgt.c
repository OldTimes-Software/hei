/* SPDX-License-Identifier: MIT */
/* Copyright © 2017-2022 Mark E Sowden <hogsy@oldtimes-software.com> */

#include <plcore/pl.h>
#include <plcore/pl_image.h>

/* Core Design, HGT format
 *
 * Usages:
 * 	- Herdy Gerdy (2002, PS2)
 *
 * Notes:
 *  It's just a BMP with an extra header at the start.
 */

#define HGT_HEADER_MAGIC PL_MAGIC_TO_NUM( 'T', 'X', 'T', 'R' )
#define HGT_HEADER_SIZE  44

PLImage *Core_HGT_ParseImage( PLFile *file ) {
	/* validate it first */
	PlFileSeek( file, 4, PL_SEEK_CUR );
	uint32_t magic = PlReadInt32( file, false, NULL );
	if ( magic != HGT_HEADER_MAGIC ) {
		PlReportBasicError( PL_RESULT_FILETYPE );
		return NULL;
	}

	/* now seek to the bitmap */
	if ( !PlFileSeek( file, HGT_HEADER_SIZE, PL_SEEK_SET ) ) {
		return NULL;
	}

	/* load the file into a memory, as it's just a bmp */
	const void *buf = PlCacheFile( file );
	if ( buf == NULL ) {
		return NULL;
	}

	/* create a temporary file handle, so we can pass it back to our ParseImage function */
	PLPath tmpPath;
	const char *path = PlGetFilePath( file );
	snprintf( tmpPath, sizeof( PLPath ), "%s.bmp", ( path != NULL ) ? path : "tmp" );
	PLFile *tmp = PlCreateFileFromMemory( tmpPath, ( void * ) buf, ( PlGetFileSize( file ) - HGT_HEADER_SIZE ), PL_FILE_MEMORYBUFFERTYPE_UNMANAGED );

	PLImage *image = PlParseImage( tmp );

	PlCloseFile( tmp );

	return image;
}
