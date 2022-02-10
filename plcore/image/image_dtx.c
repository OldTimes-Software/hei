/**
 * Hei Platform Library
 * Copyright (C) 2017-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is licensed under MIT. See LICENSE for more details.
 */

#include "image_private.h"

/*	Monolith's DTX Format (http://www.cnblogs.com/crsky/p/4702916.html)	*/

typedef struct {
	int32_t version;// Version of the format, Lithtech used negative numbers.

	uint16_t width, height;// Width and height of the texture.
	uint16_t mipmaps;      // Number of mipmaps included.
	uint16_t sections;

	int32_t flags, userflags;
	uint8_t extra[ 12 ];
	char commandstring[ 128 ];
} DTXHeader;

#define DTX_VERSION_2 -2// Lithtech 1.0 (Shogo)
// Lithtech 1.5
// Lithtech 2.0
// Lithtech 2.2
// Lithtech 2.4
#define DTX_VERSION_5 -5// Lithtech Talon (Purge)
// Lithtech Jupiter
// Lithtech Jupiter EX
#define DTX_VERSION_MIN DTX_VERSION_2
#define DTX_VERSION_MAX DTX_VERSION_5

#define DTX_GROUP( a ) a.extra[ 0 ]
#define DTX_MIPMAP( a ) a.extra[ 1 ]
#define DTX_FORMAT( a ) a.extra[ 2 ]
#define DTX_OFFSET( a ) a.extra[ 3 ]

enum DTXFlag {
	DTX_FLAG_FULLBRIGHT = ( 1 << 0 ),
	DTX_FLAG_16BIT = ( 1 << 1 ),
	DTX_FLAG_4444 = ( 1 << 7 ),
	DTX_FLAG_5551 = ( 1 << 8 ),
	DTX_FLAG_CUBEMAP = ( 1 << 10 ),
	DTX_FLAG_NORMALMAP = ( 1 << 11 ),
} DTXFlag;

enum DTXFormat {
	DTX_FORMAT_8PALLETTE,
	DTX_FORMAT_8,
	DTX_FORMAT_16,
	DTX_FORMAT_32,

	// Compressed Formats
	DTX_FORMAT_S3TC_DXT1,
	DTX_FORMAT_S3TC_DXT3,
	DTX_FORMAT_S3TC_DXT5,
} DTXFormat;

uint8_t GetDTXFormat( DTXHeader *dtx ) {
	// This is a little hacky, DTX version 2 images don't seem to use
	// the extra[2] slot the same way as later versions. So we need to
	// basically switch it to 0 since 99.9% of textures from that period
	// use a pallette anyway.
	//
	// tl;dr this is a hack because I'm lazy!
	if ( dtx->version >= DTX_VERSION_2 ) {
		return DTX_FORMAT_8PALLETTE;
	}

	return dtx->extra[ 2 ];
}

bool PlDtxFormatCheck( PLFile *fin ) {
	PlRewindFile( fin );

	// Try reading in the type first, as Lithtech has "resource types" rather than idents.
	int type;
	if ( PlReadFile( fin, &type, sizeof( int ), 1 ) != 1 ) {
		return false;
	}

	return ( type == 0 );
}

bool PlLoadDtxImage( PLFile *fin, PLImage *out ) {
	DTXHeader header;
	if ( PlReadFile( fin, &header, sizeof( DTXHeader ), 1 ) != 1 ) {
		PlReportErrorF( PL_RESULT_FILEREAD, PlGetResultString( PL_RESULT_FILEREAD ) );
		return false;
	} else if ( ( header.version < DTX_VERSION_MAX ) || ( header.version > DTX_VERSION_MIN ) ) {
		PlReportErrorF( PL_RESULT_FILEVERSION, "invalid version: %d", header.version );
		return false;
	} else if ( ( header.width < 8 ) || ( header.height < 8 ) ) {
		PlReportErrorF( PL_RESULT_IMAGERESOLUTION, "invalid resolution: w(%d) h(%d)", header.width, header.height );
		return false;
	}

	memset( out, 0, sizeof( PLImage ) );

	out->width = header.width;
	out->height = header.height;

	switch ( GetDTXFormat( &header ) ) {
		case DTX_FORMAT_8PALLETTE:
			out->size = header.width * header.height;
			out->format = PL_IMAGEFORMAT_RGB8;
			out->colour_format = PL_COLOURFORMAT_RGB;
			break;

		case DTX_FORMAT_S3TC_DXT1:
			out->size = ( header.width * header.height ) >> 1;
			out->format = PL_IMAGEFORMAT_RGB_DXT1;
			out->colour_format = PL_COLOURFORMAT_RGB;
			break;
		case DTX_FORMAT_S3TC_DXT3:
			out->size = header.width * header.height;
			out->format = PL_IMAGEFORMAT_RGBA_DXT3;
			out->colour_format = PL_COLOURFORMAT_RGBA;
			break;
		case DTX_FORMAT_S3TC_DXT5:
			out->size = header.width * header.height;
			out->format = PL_IMAGEFORMAT_RGBA_DXT5;
			out->colour_format = PL_COLOURFORMAT_RGBA;
			break;

		case DTX_FORMAT_8:
			out->size = header.width * header.height;
			out->format = PL_IMAGEFORMAT_RGB8;
			out->colour_format = PL_COLOURFORMAT_RGB;
			break;
		default:
		case DTX_FORMAT_16:
		case DTX_FORMAT_32:
			out->size = ( unsigned int ) ( header.width * header.height * 4 );
			out->format = PL_IMAGEFORMAT_RGBA8;
			out->colour_format = PL_COLOURFORMAT_RGBA;
			break;
	}

	if ( !out->size ) {
		PlReportErrorF( PL_RESULT_FILESIZE, "invalid image size" );
		return false;
	}

#if 0
    for (int i = 0; i < header.mipmaps; i++) {
    }
#endif

	out->levels = 1;
	out->data = PlCAlloc( out->levels, sizeof( uint8_t * ), false );
	if ( out->data == NULL ) {
		PlFreeImage( out );
		return false;
	}

	out->data[ 0 ] = PlCAlloc( out->size, sizeof( uint8_t ), false );
	if ( out->data[ 0 ] == NULL ) {
		PlFreeImage( out );
		return false;
	}

	PlReadFile( fin, out->data[ 0 ], sizeof( uint8_t ), out->size );

	/*	for (unsigned int i = 0; i < (unsigned int)size; i += 4)
    {
    image[i + 0] ^= image[i + 2];
    image[i + 2] ^= image[i + 0];
    image[i + 0] ^= image[i + 2];
    }*/

	return true;
}