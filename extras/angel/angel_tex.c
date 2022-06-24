/* SPDX-License-Identifier: MIT */
/* Copyright © 2017-2022 Mark E Sowden <hogsy@oldtimes-software.com> */

#include <plcore/pl_image.h>

typedef struct AngelTEXHeader {
	uint16_t width;
	uint16_t height;
	uint16_t type;

	uint16_t unk0;
	uint16_t unk1;
	uint16_t unk2;
	uint16_t unk3;
} AngelTEXHeader;
PL_STATIC_ASSERT( sizeof( AngelTEXHeader ) == 14, "needs to be 14 bytes" );

typedef enum AngelTEXType {
	ANGEL_TEX_TYPE_BGRA8_PAL = 14,
} AngelTEXType;

PLImage *Angel_TEX_ParseImage( PLFile *file ) {
	AngelTEXHeader header;
	PL_ZERO_( header );

	/* there's no magic for this format, so need to do some additional awkward checks... */

	header.width = PlReadInt16( file, false, NULL );
	header.height = PlReadInt16( file, false, NULL );

	if ( !PlIsPowerOfTwo( header.width ) || !PlIsPowerOfTwo( header.height ) ) {
		PlReportBasicError( PL_RESULT_FILETYPE );
		return NULL;
	}

	unsigned int numPixels = header.width * header.height;
	if ( numPixels <= 1 ) {
		PlReportBasicError( PL_RESULT_FILETYPE );
		return NULL;
	}

	header.type = PlReadInt16( file, false, NULL );
	header.unk0 = PlReadInt16( file, false, NULL );
	header.unk1 = PlReadInt16( file, false, NULL );
	header.unk2 = PlReadInt16( file, false, NULL );
	header.unk3 = PlReadInt16( file, false, NULL );

	/* there's a hecking lot of different types supported...
	 * for now we're just supporting the more often used types */

	PLImage *image = NULL;
	switch ( header.type ) {
		default: {
			PlReportBasicError( PL_RESULT_UNSUPPORTED );
			break;
		}
		case ANGEL_TEX_TYPE_BGRA8_PAL: {
			/* sanity check */
			static const unsigned int dataOffset = 1038;
			size_t fileSize = PlGetFileSize( file );
			if ( ( fileSize - dataOffset ) != numPixels ) {
				PlReportBasicError( PL_RESULT_FILEERR );
				return NULL;
			}

			PLColour palette[ 256 ];
			if ( PlReadFile( file, palette, sizeof( PLColour ), 256 ) != 256 ) {
				return NULL;
			}

			image = PlCreateImage( NULL, header.width, header.height, PL_COLOURFORMAT_RGBA, PL_IMAGEFORMAT_RGBA8 );

			uint8_t *pixels = PL_NEW_( uint8_t, numPixels );
			PlReadFile( file, pixels, sizeof( uint8_t ), numPixels );

			uint8_t *dst = image->data[ 0 ];
			for ( int y = 0; y < header.height; ++y ) {
				uint8_t *src = pixels + ( header.height - y - 1 ) * header.width;
				for ( int x = 0; x < header.width; ++x, ++src ) {
					*dst++ = palette[ *src ].b;
					*dst++ = palette[ *src ].g;
					*dst++ = palette[ *src ].r;
					*dst++ = palette[ *src ].a;
				}
			}

			PlFree( pixels );
			break;
		}
	}

	return image;
}
