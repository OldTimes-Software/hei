/**
 * Hei Platform Library
 * Copyright (C) 2017-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is licensed under MIT. See LICENSE for more details.
 */

#include <plcore/pl_filesystem.h>
#include <plcore/pl_image.h>

#include <errno.h>

#include "filesystem_private.h"
#include "image_private.h"

#define STBI_MALLOC( sz )        PlMAllocA( sz )
#define STBI_REALLOC( p, newsz ) PlReAllocA( p, newsz )
#define STBI_FREE( p )           PlFree( p )

#define STBIW_MALLOC( sz )        PlMAllocA( sz )
#define STBIW_REALLOC( p, newsz ) PlReAllocA( p, newsz )
#define STBIW_FREE( p )           PlFree( p )

#define STB_IMAGE_WRITE_IMPLEMENTATION
#if defined( STB_IMAGE_WRITE_IMPLEMENTATION )
#	include "stb_image_write.h"
#endif

#define STB_IMAGE_IMPLEMENTATION
#if defined( STB_IMAGE_IMPLEMENTATION )
#	define STB_IMAGE_WRITE_STATIC
#	include "stb_image.h"

static PLImage *LoadStbImage( PLFile *file ) {
	size_t s = PlGetFileSize( file );
	if ( s >= INT32_MAX ) {
		PlReportBasicError( PL_RESULT_FILESIZE );
		return NULL;
	}

	void *tmp = PlMAllocA( s );
	PlReadFile( file, tmp, sizeof( char ), s );

	int x, y, component;
	unsigned char *data = stbi_load_from_memory( tmp, ( int ) s, &x, &y, &component, 4 );

	PlFree( tmp );

	if ( data == NULL ) {
		PlReportErrorF( PL_RESULT_FILEREAD, "failed to read in image (%s)", stbi_failure_reason() );
		return NULL;
	}

	PLImage *image = PlCAllocA( 1, sizeof( PLImage ) );
	image->colour_format = PL_COLOURFORMAT_RGBA;
	image->format = PL_IMAGEFORMAT_RGBA8;
	image->width = ( unsigned int ) x;
	image->height = ( unsigned int ) y;
	image->size = PlGetImageSize( image->format, image->width, image->height );
	image->levels = 1;
	image->data = PlCAllocA( image->levels, sizeof( uint8_t * ) );
	image->data[ 0 ] = data;

	return image;
}

#endif

#define MAX_IMAGE_LOADERS 4096

typedef struct PLImageLoader {
	const char *extension;
	PLImage *( *ParseFile )( PLFile *file );
} PLImageLoader;

static PLImageLoader imageLoaders[ MAX_IMAGE_LOADERS ];
static unsigned int numImageLoaders = 0;

void PlRegisterImageLoader( const char *extension, PLImage *( *ParseFile )( PLFile *file ) ) {
	if ( numImageLoaders >= MAX_IMAGE_LOADERS ) {
		PlReportBasicError( PL_RESULT_MEMORY_EOA );
		return;
	}

	assert( ParseFile != NULL );

	imageLoaders[ numImageLoaders ].extension = extension;
	imageLoaders[ numImageLoaders ].ParseFile = ParseFile;

	numImageLoaders++;
}

void PlRegisterStandardImageLoaders( unsigned int flags ) {
	typedef struct SImageLoader {
		unsigned int flag;
		const char *extension;
		PLImage *( *LoadFunction )( PLFile *file );
	} SImageLoader;

	static const SImageLoader loaderList[] = {
	        { PL_IMAGE_FILEFORMAT_TGA, "tga", LoadStbImage },
	        { PL_IMAGE_FILEFORMAT_PNG, "png", LoadStbImage },
	        { PL_IMAGE_FILEFORMAT_JPG, "jpg", LoadStbImage },
	        { PL_IMAGE_FILEFORMAT_BMP, "bmp", LoadStbImage },
	        { PL_IMAGE_FILEFORMAT_PSD, "psd", LoadStbImage },
	        { PL_IMAGE_FILEFORMAT_GIF, "gif", LoadStbImage },
	        { PL_IMAGE_FILEFORMAT_HDR, "hdr", LoadStbImage },
	        { PL_IMAGE_FILEFORMAT_PIC, "pic", LoadStbImage },
	        { PL_IMAGE_FILEFORMAT_PNM, "pnm", LoadStbImage },
	        { PL_IMAGE_FILEFORMAT_FTX, "ftx", PlParseFtxImage },
	        { PL_IMAGE_FILEFORMAT_3DF, "3df", PlParse3dfImage },
	        { PL_IMAGE_FILEFORMAT_TIM, "tim", PlParseTimImage },
	        { PL_IMAGE_FILEFORMAT_SWL, "swl", PlParseSwlImage },
	        { PL_IMAGE_FILEFORMAT_QOI, "qoi", PlParseQoiImage },
	        { PL_IMAGE_FILEFORMAT_DDS, "dds", PlParseDdsImage },
	};

	for ( unsigned int i = 0; i < PL_ARRAY_ELEMENTS( loaderList ); ++i ) {
		if ( flags != PL_IMAGE_FILEFORMAT_ALL && !( flags & loaderList[ i ].flag ) ) {
			continue;
		}

		PlRegisterImageLoader( loaderList[ i ].extension, loaderList[ i ].LoadFunction );
	}
}

void PlClearImageLoaders( void ) {
	numImageLoaders = 0;
}

PLImage *PlCreateImage( uint8_t *buf, unsigned int w, unsigned int h, PLColourFormat col, PLImageFormat dat ) {
	PLImage *image = PlMAlloc( sizeof( PLImage ), false );
	if ( image == NULL ) {
		return NULL;
	}

	image->width = w;
	image->height = h;
	image->colour_format = col;
	image->format = dat;
	image->size = PlGetImageSize( image->format, image->width, image->height );
	image->levels = 1;

	image->data = PlCAlloc( image->levels, sizeof( uint8_t * ), false );
	if ( image->data == NULL ) {
		PlDestroyImage( image );
		return NULL;
	}

	image->data[ 0 ] = PlCAlloc( image->size, sizeof( uint8_t ), false );
	if ( image->data[ 0 ] == NULL ) {
		PlDestroyImage( image );
		return NULL;
	}

	if ( buf != NULL ) {
		memcpy( image->data[ 0 ], buf, image->size );
	}

	return image;
}

void PlDestroyImage( PLImage *image ) {
	if ( image == NULL ) {
		return;
	}

	for ( unsigned int levels = 0; levels < image->levels; ++levels ) {
		PlFree( image->data[ levels ] );
	}

	PlFree( image->data );
	PlFree( image );
}

/**
 * Load an image by it's given pass.
 * Returns null on fail.
 */
PLImage *PlLoadImage( const char *path ) {
	if ( !PlFileExists( path ) ) {
		PlReportBasicError( PL_RESULT_FILEPATH );
		return NULL;
	}

	const char *extension = PlGetFileExtension( path );
	for ( unsigned int i = 0; i < numImageLoaders; ++i ) {
		if ( pl_strcasecmp( extension, imageLoaders[ i ].extension ) != 0 ) {
			continue;
		}

		PLImage *image = NULL;
		PLFile *file = PlOpenFile( path, false );
		if ( file != NULL ) {
			image = imageLoaders[ i ].ParseFile( file );
			PlCloseFile( file );
		}

		if ( image == NULL ) {
			continue;
		}

		strncpy( image->path, path, sizeof( image->path ) );
		return image;
	}

	PlReportBasicError( PL_RESULT_UNSUPPORTED );

	return NULL;
}

/**
 * Load an image by it's virtual file handle.
 */
PLImage *PlParseImage( PLFile *file ) {
	const char *extension = PlGetFileExtension( file->path );
	for ( unsigned int i = 0; i < numImageLoaders; ++i ) {
		if ( extension != NULL && pl_strcasecmp( extension, imageLoaders[ i ].extension ) != 0 ) {
			continue;
		}

		PLImage *image = imageLoaders[ i ].ParseFile( file );
		if ( image == NULL ) {
			continue;
		}

		return image;
	}

	return NULL;
}

bool PlWriteImage( const PLImage *image, const char *path ) {
	if ( path != NULL && *path == '\0' ) {
		PlReportErrorF( PL_RESULT_FILEPATH, PlGetResultString( PL_RESULT_FILEPATH ) );
		return false;
	}

	int comp = ( int ) PlGetNumImageFormatChannels( image->format );
	if ( comp == 0 ) {
		PlReportErrorF( PL_RESULT_IMAGEFORMAT, "invalid colour format" );
		return false;
	}

	const char *extension = PlGetFileExtension( path );
	if ( extension != NULL && *extension != '\0' ) {
		if ( !pl_strncasecmp( extension, "bmp", 3 ) ) {
			if ( stbi_write_bmp( path, ( int ) image->width, ( int ) image->height, comp, image->data[ 0 ] ) == 1 ) {
				return true;
			}
		} else if ( !pl_strncasecmp( extension, "png", 3 ) ) {
			if ( stbi_write_png( path, ( int ) image->width, ( int ) image->height, comp, image->data[ 0 ], 0 ) == 1 ) {
				return true;
			}
		} else if ( !pl_strncasecmp( extension, "tga", 3 ) ) {
			if ( stbi_write_tga( path, ( int ) image->width, ( int ) image->height, comp, image->data[ 0 ] ) == 1 ) {
				return true;
			}
		} else if ( !pl_strncasecmp( extension, "jpg", 3 ) || !pl_strncasecmp( extension, "jpeg", 3 ) ) {
			if ( stbi_write_jpg( path, ( int ) image->width, ( int ) image->height, comp, image->data[ 0 ], 90 ) == 1 ) {
				return true;
			}
		} else if ( !pl_strncasecmp( extension, "qoi", 3 ) ) {
			if ( PlWriteQoiImage( image, path ) ) {
				return true;
			}
		}
	}

	PlReportErrorF( PL_RESULT_FILETYPE, PlGetResultString( PL_RESULT_FILETYPE ) );
	return false;
}

static bool RGB8toRGBA8( PLImage *image ) {
	size_t size = PlGetImageSize( PL_IMAGEFORMAT_RGBA8, image->width, image->height );
	if ( size == 0 ) {
		return false;
	}

	typedef struct RGB8 {
		uint8_t r, g, b;
	} RGB8;
	typedef struct RGBA8 {
		uint8_t r, g, b, a;
	} RGBA8;
	size_t num_pixels = image->size / 3;

	RGB8 *src = ( RGB8 * ) image->data[ 0 ];
	RGBA8 *dst = PlMAllocA( size );
	for ( size_t i = 0; i < num_pixels; ++i ) {
		dst->r = src->r;
		dst->g = src->g;
		dst->b = src->b;
		dst->a = 255;
		dst++;
		src++;
	}

	PlFree( image->data[ 0 ] );

	image->data[ 0 ] = ( uint8_t * ) ( &dst[ 0 ] );
	image->size = size;
	image->format = PL_IMAGEFORMAT_RGBA8;
	image->colour_format = PL_COLOURFORMAT_RGBA;

	return true;
}

#define scale_5to8( i ) ( ( ( ( double ) ( i ) ) / 31 ) * 255 )

static uint8_t *ImageDataRGB5A1toRGBA8( const uint8_t *src, size_t n_pixels ) {
	uint8_t *dst = PlMAlloc( n_pixels * 4, false );
	if ( dst == NULL ) {
		return NULL;
	}

	uint8_t *dp = dst;

	for ( size_t i = 0; i < n_pixels; ++i ) {
		/* Red */
		*( dp++ ) = scale_5to8( ( src[ 0 ] & 0xF8 ) >> 3 );

		/* Green */
		*( dp++ ) = scale_5to8( ( ( src[ 0 ] & 0x07 ) << 2 ) | ( ( src[ 1 ] & 0xC0 ) >> 6 ) );

		/* Blue */
		*( dp++ ) = scale_5to8( ( src[ 1 ] & 0x3E ) >> 1 );

		/* Alpha */
		*( dp++ ) = ( src[ 1 ] & 0x01 ) ? 255 : 0;

		src += 2;
	}

	return dst;
}

bool PlConvertPixelFormat( PLImage *image, PLImageFormat new_format ) {
	if ( image->format == new_format ) {
		return true;
	}

	switch ( image->format ) {
		case PL_IMAGEFORMAT_RGB8: {
			if ( new_format == PL_IMAGEFORMAT_RGBA8 ) { return RGB8toRGBA8( image ); }
		}

		case PL_IMAGEFORMAT_RGB5A1: {
			if ( new_format == PL_IMAGEFORMAT_RGBA8 ) {
				uint8_t **levels = PlCAllocA( image->levels, sizeof( uint8_t * ) );

				/* Make a new copy of each detail level in the new format. */

				unsigned int lw = image->width;
				unsigned int lh = image->height;

				for ( unsigned int l = 0; l < image->levels; ++l ) {
					levels[ l ] = ImageDataRGB5A1toRGBA8( image->data[ l ], lw * lh );
					if ( levels[ l ] == NULL ) {
						/* Memory allocation failed, ditch any buffers we've created so far. */
						for ( unsigned int m = 0; m < l; ++m ) {
							PlFree( levels[ m ] );
						}

						PlFree( levels );

						PlReportErrorF( PL_RESULT_MEMORY_ALLOCATION, "couldn't allocate memory for image data" );
						return false;
					}

					lw /= 2;
					lh /= 2;
				}

				/* Now that all levels have been converted, free and replace the old buffers. */

				for ( unsigned int l = 0; l < image->levels; ++l ) {
					PlFree( image->data[ l ] );
				}

				PlFree( image->data );
				image->data = levels;

				image->format = new_format;
				/* TODO: Update colour_format */

				return true;
			}
		} break;

		default:
			break;
	}

	PlReportErrorF( PL_RESULT_IMAGEFORMAT, "unsupported image format conversion" );
	return false;
}

unsigned int PlGetImageSize( PLImageFormat format, unsigned int width, unsigned int height ) {
	switch ( format ) {
		case PL_IMAGEFORMAT_RGB_DXT1:
			return ( width * height ) >> 1;
		default: {
			unsigned int bytes = PlGetImageFormatPixelSize( format );
			return width * height * bytes;
		}
	}
}

/* Returns the number of BYTES per pixel for the given PLImageFormat.
 *
 * If the format doesn't have a predictable size or the size isn't a multiple
 * of one byte, returns ZERO. */
unsigned int PlGetImageFormatPixelSize( PLImageFormat format ) {
	switch ( format ) {
		case PL_IMAGEFORMAT_RGBA4:
		case PL_IMAGEFORMAT_RGB5A1:
		case PL_IMAGEFORMAT_RGB565:
			return 2;
		case PL_IMAGEFORMAT_RGB8:
			return 3;
		case PL_IMAGEFORMAT_RGBA_DXT1:
		case PL_IMAGEFORMAT_RGBA8:
			return 4;
		case PL_IMAGEFORMAT_RGBA12:
			return 6;
		case PL_IMAGEFORMAT_RGBA16:
		case PL_IMAGEFORMAT_RGBA16F:
			return 8;
		default:
			return 0;
	}
}

/**
 * Returns the number of channels for the given format.
 */
unsigned int PlGetNumImageFormatChannels( PLImageFormat format ) {
	switch ( format ) {
		case PL_IMAGEFORMAT_RGB4:
		case PL_IMAGEFORMAT_RGB5:
		case PL_IMAGEFORMAT_RGB565:
		case PL_IMAGEFORMAT_RGB8:
		case PL_IMAGEFORMAT_RGB_DXT1:
		case PL_IMAGEFORMAT_RGB_FXT1:
			return 3;
		case PL_IMAGEFORMAT_RGBA4:
		case PL_IMAGEFORMAT_RGB5A1:
		case PL_IMAGEFORMAT_RGBA8:
		case PL_IMAGEFORMAT_RGBA12:
		case PL_IMAGEFORMAT_RGBA16:
		case PL_IMAGEFORMAT_RGBA16F:
			return 4;
		default:
			return 0;
	}
}

bool PlImageHasAlpha( const PLImage *image ) {
	return ( PlGetNumImageFormatChannels( image->format ) >= 4 );
}

void PlInvertImageColour( PLImage *image ) {
	unsigned int num_colours = PlGetNumImageFormatChannels( image->format );
	switch ( image->format ) {
		case PL_IMAGEFORMAT_RGB8:
		case PL_IMAGEFORMAT_RGBA8: {
			for ( size_t i = 0; i < image->size; i += num_colours ) {
				uint8_t *pixel = &image->data[ 0 ][ i ];
				pixel[ 0 ] = ~pixel[ 0 ];
				pixel[ 1 ] = ~pixel[ 1 ];
				pixel[ 2 ] = ~pixel[ 2 ];
			}
		} break;

		default:
			break;
	}

	PlReportErrorF( PL_RESULT_IMAGEFORMAT, "unsupported image format" );
}

/* utility function */
void PlGenerateStipplePattern( PLImage *image, unsigned int depth ) {
#if 0
    unsigned int p = 0;
    unsigned int num_colours = plGetSamplesPerPixel(image->colour_format);
    switch(image->format) {
        case PL_IMAGEFORMAT_RGB8: break;
        case PL_IMAGEFORMAT_RGBA8: {
            for(unsigned int i = 0; i < image->size; i += num_colours) {
                uint8_t *pixel = &image->data[0][i];
                if(num_colours == 4) {
                    //if()
                }
            }
        } break;

        default:break;
    }

    ReportError(PL_RESULT_IMAGEFORMAT, "unsupported image format");
#else
	/* todo */
#endif
}

void PlReplaceImageColour( PLImage *image, PLColour target, PLColour dest ) {
	unsigned int num_colours = PlGetNumImageFormatChannels( image->format );
	switch ( image->format ) {
		case PL_IMAGEFORMAT_RGB8:
		case PL_IMAGEFORMAT_RGBA8: {
			for ( unsigned int i = 0; i < image->size; i += num_colours ) {
				uint8_t *pixel = &image->data[ 0 ][ i ];
				if ( num_colours == 4 ) {
					if ( PlCompareColour( PLColour( pixel[ 0 ], pixel[ 1 ], pixel[ 2 ], pixel[ 3 ] ), target ) ) {
						pixel[ 0 ] = dest.r;
						pixel[ 1 ] = dest.g;
						pixel[ 2 ] = dest.b;
						pixel[ 3 ] = dest.a;
					}
				} else {
					if ( PlCompareColour( PLColourRGB( pixel[ 0 ], pixel[ 1 ], pixel[ 2 ] ), target ) ) {
						pixel[ 0 ] = dest.r;
						pixel[ 1 ] = dest.g;
						pixel[ 2 ] = dest.b;
					}
				}
			}
		} break;

		default:
			break;
	}

	PlReportErrorF( PL_RESULT_IMAGEFORMAT, "unsupported image format" );
}

void PlFreeImage( PLImage *image ) {
	FunctionStart();

	if ( image == NULL || image->data == NULL ) {
		return;
	}

	for ( unsigned int levels = 0; levels < image->levels; ++levels ) {
		PlFree( image->data[ levels ] );
	}

	PlFree( image->data );
}

bool PlFlipImageVertical( PLImage *image ) {
	unsigned int width = image->width;
	unsigned int height = image->height;

	unsigned int bytes_per_pixel = PlGetImageFormatPixelSize( image->format );
	if ( bytes_per_pixel == 0 ) {
		PlReportErrorF( PL_RESULT_IMAGEFORMAT, "cannot flip images in this format" );
		return false;
	}

	unsigned int bytes_per_row = width * bytes_per_pixel;

	unsigned char *swap = PlMAlloc( bytes_per_row, false );
	if ( swap == NULL ) {
		return false;
	}

	for ( unsigned int l = 0; l < image->levels; ++l ) {
		for ( unsigned int r = 0; r < height / 2; ++r ) {
			unsigned char *tr = image->data[ l ] + ( r * bytes_per_row );
			unsigned char *br = image->data[ l ] + ( ( ( height - 1 ) - r ) * bytes_per_row );

			memcpy( swap, tr, bytes_per_row );
			memcpy( tr, br, bytes_per_row );
			memcpy( br, swap, bytes_per_row );
		}

		bytes_per_row /= 2;
		height /= 2;
	}

	PlFree( swap );

	return true;
}

/**
 * Returns a list of file extensions representing all
 * the formats supported by the image loader.
 */
const char **PlGetSupportedImageFormats( unsigned int *numElements ) {
	static const char *imageFormats[ MAX_IMAGE_LOADERS ];
	for ( unsigned int i = 0; i < numImageLoaders; ++i ) {
		imageFormats[ i ] = imageLoaders[ i ].extension;
	}

	*numElements = numImageLoaders;

	return imageFormats;
}

/* getters */
unsigned int PlGetImageWidth( const PLImage *image ) { return image->width; }
unsigned int PlGetImageHeight( const PLImage *image ) { return image->height; }
PLImageFormat PlGetImageFormat( const PLImage *image ) { return image->format; }

const uint8_t *PlGetImageData( const PLImage *image, unsigned int level ) {
	if ( level >= image->levels ) {
		return NULL;
	}

	return image->data[ level ];
}

unsigned int PlGetImageDataSize( const PLImage *image ) {
	return PlGetImageSize( image->format, image->width, image->height );
}
