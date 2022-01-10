/**
 * Hei Platform Library
 * Copyright (C) 2017-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is licensed under MIT. See LICENSE for more details.
 */

#pragma once

#include <plcore/pl.h>

#define PlBytesToKilobytes( a ) ( ( double ) ( ( a ) ) / 1000 )
#define PlBytesToMegabytes( a ) ( PlBytesToKilobytes( a ) / 1000 )
#define PlBytesToGigabytes( a ) ( PlBytesToMegabytes( a ) / 1000 )

#define PlBytesToKibibytes( a ) ( ( double ) ( ( a ) ) / 1024 )
#define PlBytesToMebibytes( a ) ( PlBytesToKibibytes( a ) / 1024 )
#define PlBytesToGibibytes( a ) ( PlBytesToMebibytes( a ) / 1024 )

typedef struct PLFile PLFile;

typedef enum PLFileSeek {
#if defined( SEEK_SET ) && defined( SEEK_CUR ) && defined( SEEK_END )
	PL_SEEK_SET = SEEK_SET,
	PL_SEEK_CUR = SEEK_CUR,
	PL_SEEK_END = SEEK_END
#else
	PL_SEEK_SET,
	PL_SEEK_CUR,
	PL_SEEK_END
#endif
} PLFileSeek;

typedef enum PLFileMemoryBufferType {
	PL_FILE_MEMORYBUFFERTYPE_COPY,      /* creates a copy of the given input and takes ownership */
	PL_FILE_MEMORYBUFFERTYPE_OWNER,     /* takes ownership of the input and frees on close */
	PL_FILE_MEMORYBUFFERTYPE_UNMANAGED, /* doesn't free on close */
} PLFileMemoryBufferType;

typedef struct PLFileSystemMount PLFileSystemMount;

PL_EXTERN_C

#if !defined( PL_COMPILE_PLUGIN )

PL_EXTERN char *PlGetUserName( char *out, size_t n );

PL_EXTERN const char *PlGetWorkingDirectory( void );
PL_EXTERN void PlSetWorkingDirectory( const char *path );

PL_EXTERN char *PlGetApplicationDataDirectory( const char *app_name, char *out, size_t n );

PL_EXTERN bool PlPathEndsInSlash( const char *p );

PL_EXTERN void PlStripExtension( char *dest, size_t length, const char *in );

PL_EXTERN const char *PlGetFileExtension( const char *in );
PL_EXTERN const char *PlGetFileName( const char *path );

PL_EXTERN bool PlLocalFileExists( const char *path );
PL_EXTERN bool PlFileExists( const char *path );
PL_EXTERN bool PlLocalPathExists( const char *path );
PL_EXTERN bool PlPathExists( const char *path );

PL_EXTERN void PlScanDirectory( const char *path, const char *extension, void ( *Function )( const char *, void * ), bool recursive, void *userData );

PL_EXTERN bool PlCreateDirectory( const char *path );
PL_EXTERN bool PlCreatePath( const char *path );

// File I/O ...

PL_EXTERN PLFile *PlCreateFileFromMemory( const char *path, void *buf, size_t bufSize, PLFileMemoryBufferType bufType );
PL_EXTERN PLFile *PlOpenLocalFile( const char *path, bool cache );
PL_EXTERN PLFile *PlOpenFile( const char *path, bool cache );
PL_EXTERN void PlCloseFile( PLFile *ptr );

PL_EXTERN bool PlCopyFile( const char *path, const char *dest );
PL_EXTERN bool PlWriteFile( const char *path, const uint8_t *buf, size_t length );
PL_EXTERN bool PlDeleteFile( const char *path );

PL_EXTERN bool PlIsFileModified( time_t oldtime, const char *path );
PL_EXTERN bool PlIsEndOfFile( const PLFile *ptr );

PL_EXTERN time_t PlGetLocalFileTimeStamp( const char *path );
PL_EXTERN size_t PlGetLocalFileSize( const char *path );

PL_EXTERN const char *PlGetFilePath( const PLFile *ptr );
PL_EXTERN const uint8_t *PlGetFileData( const PLFile *ptr );
PL_EXTERN time_t PlGetFileTimeStamp( PLFile *ptr );
PL_EXTERN size_t PlGetFileSize( const PLFile *ptr );
PL_EXTERN size_t PlGetFileOffset( const PLFile *ptr );

PL_EXTERN size_t PlReadFile( PLFile *ptr, void *dest, size_t size, size_t count );

PL_EXTERN char PlReadInt8( PLFile *ptr, bool *status );
PL_EXTERN int16_t PlReadInt16( PLFile *ptr, bool big_endian, bool *status );
PL_EXTERN int32_t PlReadInt32( PLFile *ptr, bool big_endian, bool *status );
PL_EXTERN int64_t PlReadInt64( PLFile *ptr, bool big_endian, bool *status );

PL_EXTERN float PlReadFloat32( PLFile *ptr, bool big_endian, bool *status );
PL_EXTERN double PlReadFloat64( PLFile *ptr, bool big_endian, bool *status );

PL_EXTERN char *PlReadString( PLFile *ptr, char *str, size_t size );

PL_EXTERN bool PlFileSeek( PLFile *ptr, long int pos, PLFileSeek seek );
PL_EXTERN void PlRewindFile( PLFile *ptr );

/** FS Mounting **/

PL_EXTERN PLFileSystemMount *PlMountLocalLocation( const char *path );
PL_EXTERN PLFileSystemMount *PlMountLocation( const char *path );

PL_EXTERN void PlClearMountedLocation( PLFileSystemMount *location );
PL_EXTERN void PlClearMountedLocations( void );

/****/

#endif

PL_EXTERN_C_END
