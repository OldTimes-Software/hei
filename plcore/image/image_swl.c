/**
 * Hei Platform Library
 * Copyright (C) 2017-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is licensed under MIT. See LICENSE for more details.
 */

#include "image_private.h"

/* Ritual Entertainment's SWL Format, used by SiN */

typedef struct SWLHeader {
	char path[ 64 ]; /* file path, kinda useless for us */
	uint32_t width;
	uint32_t height;
} SWLHeader;

PLImage *PlParseSwlImage( PLFile *fin ) {
	SWLHeader header;
	if ( PlReadFile( fin, &header, sizeof( header ), 1 ) != 1 ) {
		return NULL;
	}

	if ( header.width > 512 || header.width == 0 ||
	     header.height > 512 || header.height == 0 ) {
		PlReportBasicError( PL_RESULT_IMAGERESOLUTION );
		return NULL;
	}

	struct {
		uint8_t r, g, b, a;
	} palette[ 256 ];
	if ( PlReadFile( fin, palette, 4, 256 ) != 256 ) {
		PlReportBasicError( PL_RESULT_FILEREAD );
		return false;
	}

	/* according to sources, this is a collection of misc data that's
   	 * specific to SiN itself, so we'll skip it. */
	if ( !PlFileSeek( fin, 0x4D4, PL_SEEK_SET ) ) {
		PlReportBasicError( PL_RESULT_FILEREAD );
		return false;
	}

	PLImage *out = PlMAlloc( sizeof( PLImage ), true );
	out->width = header.width;
	out->height = header.height;
	out->levels = 4;
	out->colour_format = PL_COLOURFORMAT_RGBA;
	out->format = PL_IMAGEFORMAT_RGBA8;
	out->size = PlGetImageSize( out->format, out->width, out->height );

	unsigned int mip_w = out->width;
	unsigned int mip_h = out->height;
	for ( unsigned int i = 0; i < out->levels; ++i ) {
		if ( i > 0 ) {
			mip_w = out->width >> ( i + 1 );
			mip_h = out->height >> ( i + 1 );
		}

		size_t buf_size = mip_w * mip_h;
		uint8_t *buf = PlMAllocA( buf_size );
		if ( PlReadFile( fin, buf, 1, buf_size ) != buf_size ) {
			PlDestroyImage( out );
			PlFree( buf );
			return false;
		}

		out->data = PlCAllocA( out->levels, sizeof( uint8_t * ) );

		size_t level_size = PlGetImageSize( out->format, mip_w, mip_h );
		out->data[ i ] = PlCAllocA( level_size, sizeof( uint8_t ) );

		/* now we fill in the buf we just allocated,
    	 * by using the palette */
		for ( size_t j = 0, k = 0; j < buf_size; ++j, k += 4 ) {
			out->data[ i ][ k ] = palette[ buf[ j ] ].r;
			out->data[ i ][ k + 1 ] = palette[ buf[ j ] ].g;
			out->data[ i ][ k + 2 ] = palette[ buf[ j ] ].b;

			/* the alpha channel appears to be used more like
       * a flag to say "yes this texture will be transparent",
       * rather than actual levels of alpha for this pixel.
       *
       * because of that we'll just ignore it */
			out->data[ i ][ k + 3 ] = 255; /*(uint8_t) (255 - palette[buf[j]].a);*/
		}

		PlFree( buf );
	}

	return out;
}
