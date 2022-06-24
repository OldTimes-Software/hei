/**
 * Hei Platform Library
 * Copyright (C) 2017-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is licensed under MIT. See LICENSE for more details.
 */

#include "pl_private.h"
#include "package_private.h"
#include "filesystem_private.h"

#include "../3rdparty/miniz/miniz.h"
#include "../3rdparty/blast/blast.h"

/****************************************
 * Generic Loader
 ****************************************/

typedef struct BlstUser {
	uint8_t *buffer;
	unsigned int maxLength, length;
} BlstUser;

static unsigned int BlstCbIn( void *how, unsigned char **buf ) {
	BlstUser *user = ( BlstUser * ) how;
	if ( user->buffer == NULL ) {
		return 0;
	}

	*buf = user->buffer;
	return user->length;
}

static int BlstCbOut( void *how, unsigned char *buf, unsigned int len ) {
	BlstUser *user = ( BlstUser * ) how;
	if ( user->length >= user->maxLength ) {
		return 1;
	}
	memcpy( user->buffer + user->length, buf, len );
	user->length += len;
	return 0;
}

static int Inflate( unsigned char *dst, uint32_t *dstLength, const unsigned char *src, uint32_t srcLength, bool raw ) {
	mz_stream stream;
	PL_ZERO_( stream );
	stream.next_in = src;
	stream.avail_in = srcLength;
	stream.next_out = dst;
	stream.avail_out = *dstLength;

	int status = mz_inflateInit2( &stream, raw ? -MZ_DEFAULT_WINDOW_BITS : MZ_DEFAULT_WINDOW_BITS );
	if ( status != MZ_OK )
		return status;

	status = mz_inflate( &stream, MZ_FINISH );
	if ( status != MZ_STREAM_END ) {
		mz_inflateEnd( &stream );
		return ( ( status == MZ_BUF_ERROR ) && ( !stream.avail_in ) ) ? MZ_DATA_ERROR : status;
	}
	*dstLength = stream.total_out;

	return mz_inflateEnd( &stream );
}

/**
 * Generic loader for package files, since this is unlikely to change
 * in most cases.
 */
static uint8_t *LoadGenericPackageFile( PLFile *fh, PLPackageIndex *pi ) {
	FunctionStart();

	if ( !PlFileSeek( fh, ( signed ) pi->offset, PL_SEEK_SET ) ) {
		return NULL;
	}

	size_t size = ( pi->compressionType != PL_COMPRESSION_NONE ) ? pi->compressedSize : pi->fileSize;
	uint8_t *dataPtr = PlMAllocA( size );
	if ( PlReadFile( fh, dataPtr, sizeof( uint8_t ), size ) != size ) {
		PlFree( dataPtr );
		return NULL;
	}

	if ( pi->compressionType != PL_COMPRESSION_NONE ) {
		uint8_t *decompressedPtr = PlMAllocA( pi->fileSize );
		uint32_t uncompressedLength = ( uint32_t ) pi->fileSize;
		if ( pi->compressionType == PL_COMPRESSION_DEFLATE || pi->compressionType == PL_COMPRESSION_GZIP ) {
			int status = Inflate( decompressedPtr, &uncompressedLength, dataPtr, ( unsigned long ) pi->compressedSize, ( pi->compressionType == PL_COMPRESSION_GZIP ) );

			PlFree( dataPtr );
			dataPtr = decompressedPtr;

			if ( status != Z_OK ) {
				PlFree( dataPtr );
				PlReportErrorF( PL_RESULT_FILEREAD, "failed to decompress buffer (%s)", zError( status ) );
				return NULL;
			}
		} else if ( pi->compressionType == PL_COMPRESSION_IMPLODE ) {
			BlstUser in = {
			                 .buffer = dataPtr,
			                 .length = ( unsigned int ) size,
			         },
			         out = {
			                 .buffer = decompressedPtr,
			                 .length = 0,
			                 .maxLength = uncompressedLength,
			         };
			int status = blast( BlstCbIn, &in, BlstCbOut, &out, NULL, NULL );

			PlFree( dataPtr );
			dataPtr = decompressedPtr;

			if ( status != 0 ) {
				const char *errmsg;
				switch ( status ) {
					case 2:
						errmsg = "ran out of input before completing decompression";
						break;
					case 1:
						errmsg = "output error before completing decompression";
						break;
					case -1:
						errmsg = "literal flag not zero or one";
						break;
					case -2:
						errmsg = "dictionary size not in 4..6";
						break;
					case -3:
						errmsg = "distance is too far back";
						break;
					default:
						errmsg = "unknown error when decompressing buffer";
						break;
				}

				PlFree( dataPtr );
				PlReportErrorF( PL_RESULT_FILEREAD, "%s (%d)", errmsg, status );
				return NULL;
			}
		}
	}

	return dataPtr;
}

/****************************************
 ****************************************/

/**
 * Allocate a new package handle.
 */
PLPackage *PlCreatePackageHandle( const char *path, unsigned int tableSize, uint8_t *( *OpenFile )( PLFile *filePtr, PLPackageIndex *index ) ) {
	PLPackage *package = PlMAllocA( sizeof( PLPackage ) );

	if ( OpenFile == NULL ) {
		package->internal.LoadFile = LoadGenericPackageFile;
	} else {
		package->internal.LoadFile = OpenFile;
	}

	package->table_size = tableSize;
	package->table = PlCAllocA( tableSize, sizeof( PLPackageIndex ) );

	snprintf( package->path, sizeof( package->path ), "%s", path );

	return package;
}

/* Unloads package from memory
 */
void PlDestroyPackage( PLPackage *package ) {
	if ( package == NULL ) {
		return;
	}

	PlFree( package->table );
	PlFree( package );
}
#if 0// todo
void plWritePackage(PLPackage *package) {

}
#endif
/////////////////////////////////////////////////////////////////

typedef struct PLPackageLoader {
	const char *ext;
	PLPackage *( *LoadFunction )( const char *path );
} PLPackageLoader;

static PLPackageLoader package_loaders[ MAX_OBJECT_INTERFACES ];
static unsigned int num_package_loaders = 0;

void PlInitPackageSubSystem( void ) {
	PlClearPackageLoaders();
}

/**
 * Returns a list of file extensions representing all
 * the formats supported by the package loader.
 */
const char **PlGetSupportedPackageFormats( unsigned int *numElements ) {
	static const char *formats[ MAX_OBJECT_INTERFACES ];
	for ( unsigned int i = 0; i < num_package_loaders; ++i ) {
		formats[ i ] = package_loaders[ i ].ext;
	}

	*numElements = num_package_loaders;

	return formats;
}

void PlClearPackageLoaders( void ) {
	memset( package_loaders, 0, sizeof( PLPackageLoader ) * MAX_OBJECT_INTERFACES );
	num_package_loaders = 0;
}

void PlRegisterPackageLoader( const char *ext, PLPackage *( *LoadFunction )( const char *path ) ) {
	package_loaders[ num_package_loaders ].ext = ext;
	package_loaders[ num_package_loaders ].LoadFunction = LoadFunction;
	num_package_loaders++;
}

void PlRegisterStandardPackageLoaders( void ) {
	PlRegisterPackageLoader( "wad", PlLoadIWADPackage_ );
	PlRegisterPackageLoader( "wad", PlLoadWAD2Package_ );
	PlRegisterPackageLoader( "pak", PlLoadPAKPackage_ );
	PlRegisterPackageLoader( "vpk", PlLoadVPKPackage_ );
	/* hogs of war */
	PlRegisterPackageLoader( "mad", PlLoadMadPackage );
	PlRegisterPackageLoader( "mtd", PlLoadMadPackage );
	/* starfox adventures */
	PlRegisterPackageLoader( "tab", PlLoadTabPackage );
}

PLPackage *PlLoadPackage( const char *path ) {
	FunctionStart();

	const char *ext = PlGetFileExtension( path );
	for ( unsigned int i = 0; i < num_package_loaders; ++i ) {
		if ( package_loaders[ i ].LoadFunction == NULL ) {
			break;
		}

		if ( !PL_INVALID_STRING( ext ) && !PL_INVALID_STRING( package_loaders[ i ].ext ) ) {
			if ( pl_strncasecmp( ext, package_loaders[ i ].ext, sizeof( package_loaders[ i ].ext ) ) == 0 ) {
				PLPackage *package = package_loaders[ i ].LoadFunction( path );
				if ( package != NULL ) {
					strncpy( package->path, path, sizeof( package->path ) );
					return package;
				}
			}
		} else if ( PL_INVALID_STRING( ext ) && PL_INVALID_STRING( package_loaders[ i ].ext ) ) {
			PLPackage *package = package_loaders[ i ].LoadFunction( path );
			if ( package != NULL ) {
				strncpy( package->path, path, sizeof( package->path ) );
				return package;
			}
		}
	}

	return NULL;
}

PLFile *PlLoadPackageFile( PLPackage *package, const char *path ) {
	if ( package->internal.LoadFile == NULL ) {
		PlReportErrorF( PL_RESULT_FILEREAD, "package has not been initialized, no LoadFile function assigned, aborting" );
		return NULL;
	}

	for ( unsigned int i = 0; i < package->table_size; ++i ) {
		if ( strcmp( path, package->table[ i ].fileName ) != 0 ) {
			continue;
		}

		/* load in the package */
		PLFile *packageFile = PlOpenFile( package->path, true );
		if ( packageFile == NULL ) {
			return NULL;
		}

		PLFile *file = NULL;

		uint8_t *dataPtr = package->internal.LoadFile( packageFile, &( package->table[ i ] ) );
		if ( dataPtr != NULL ) {
			file = PlMAllocA( sizeof( PLFile ) );
			snprintf( file->path, sizeof( file->path ), "%s", package->table[ i ].fileName );
			file->size = package->table[ i ].fileSize;
			file->data = dataPtr;
			file->pos = file->data;
		}

		PlCloseFile( packageFile );

		return file;
	}

	PlReportErrorF( PL_RESULT_INVALID_PARM2, "failed to find file in package" );
	return NULL;
}

PLFile *PlLoadPackageFileByIndex( PLPackage *package, unsigned int index ) {
	if ( index >= package->table_size ) {
		PlReportBasicError( PL_RESULT_INVALID_PARM2 );
		return NULL;
	}

	return PlLoadPackageFile( package, package->table[ index ].fileName );
}

const char *PlGetPackagePath( const PLPackage *package ) {
	return package->path;
}

const char *PlGetPackageFileName( const PLPackage *package, unsigned int index ) {
	if ( index >= package->table_size ) {
		PlReportBasicError( PL_RESULT_INVALID_PARM2 );
		return NULL;
	}

	return package->table[ index ].fileName;
}

unsigned int PlGetPackageTableSize( const PLPackage *package ) {
	return package->table_size;
}

int PlGetPackageTableIndex( const PLPackage *package, const char *indexName ) {
	FunctionStart();

	for ( unsigned int i = 0; i < package->table_size; ++i ) {
		if ( strcmp( indexName, package->table[ i ].fileName ) != 0 ) {
			continue;
		}

		return i;
	}

	PlReportBasicError( PL_RESULT_INVALID_PARM2 );

	return -1;
}
