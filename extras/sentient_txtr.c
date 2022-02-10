/**
 * Hei Platform Library
 * Copyright (C) 2017-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is licensed under MIT. See LICENSE for more details.
 */

#include <plcore/pl_image.h>

/* Sentient's TXTR format */

static bool SentientTxtrFormatCheck( PLFile *ptr ) {
	PlRewindFile( ptr );

	struct {
		char ident[ 4 ];    // RTXT
		unsigned int length;// Length of chunk
	} header;
	if ( PlReadFile( ptr, &header, sizeof( header ), 1 ) != 1 ) {
		PlReportErrorF( PL_RESULT_INVALID_PARM1, "failed to read image header" );
		return false;
	}

	if ( strncmp( "RTXT", header.ident, 4 ) != 0 ) {
		PlReportErrorF( PL_RESULT_INVALID_PARM1, "invalid identifer, %s vs RTXT", header.ident );
		return false;
	}

	if ( header.length > PlGetFileSize( ptr ) ) {
		PlReportErrorF( PL_RESULT_INVALID_PARM1, "invalid size indicated by header" );
		return false;
	}

	return true;
}

bool Sentient_TXTR_LoadFile( PLFile *ptr, PLImage *out ) {
	PlRewindFile( ptr );
	PlFileSeek( ptr, 4, PL_SEEK_CUR );

	bool status;
	unsigned int length = PlReadInt32( ptr, false, &status );
	if ( !status ) {
		PlReportErrorF( PL_RESULT_INVALID_PARM1, "failed to read length from header" );
		return false;
	}

	// HACK HACK HACK...
	unsigned int wh = sqrt( length / 3 );//plRoundUp(sqrt(length / 3), 2);
	if ( !PlIsPowerOfTwo( wh ) ) {
		if ( wh < 100 ) {
			wh = PlRoundUp( sqrt( length / 3 ), 2 );
		}
	}

	out->width = out->height = wh;
	out->colour_format = PL_COLOURFORMAT_RGB;
	out->format = PL_IMAGEFORMAT_RGB8;
	out->size = PlGetImageSize( out->format, out->width, out->height );
	out->levels = 1;
	out->data = PlCAllocA( out->levels, sizeof( uint8_t * ) );
	out->data[ 0 ] = PlCAllocA( out->size, sizeof( uint8_t ) );
	PlReadFile( ptr, out->data[ 0 ], 1, out->size );
	return true;
}
