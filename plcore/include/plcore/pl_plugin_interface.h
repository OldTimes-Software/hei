/*
MIT License

Copyright (c) 2017-2021 Mark E Sowden <hogsy@oldtimes-software.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once

#include <plcore/pl.h>
#include <plcore/pl_console.h>
#include <plcore/pl_package.h>
#include <plcore/pl_image.h>

typedef uint16_t PLPluginInterfaceVersion[ 2 ];

typedef struct PLPluginDescription {
	PLPluginInterfaceVersion interfaceVersion;
	unsigned int pluginVersion[ 3 ]; /* Major, minor and patch. */
	const char *description;
} PLPluginDescription;

/**
 * Functions exported from the platform library.
 */
typedef struct PLPluginExportTable {
	PLPluginInterfaceVersion version;

	void ( *ReportError )( PLFunctionResult resultType, const char *function, const char *message, ... );
	const char *( *GetError )( void );

	void *( *MAlloc )( size_t size, bool abortOnFail );
	void *( *CAlloc )( size_t num, size_t size, bool abortOnFail );
	void *( *ReAlloc )( void *ptr, size_t newSize, bool abortOnFail );
	void ( *Free )( void *ptr );

	/**
	 * FILE API
	 **/

	bool ( *LocalFileExists )( const char *path );
	bool ( *FileExists )( const char *path );
	bool ( *LocalPathExists )( const char *path );
	bool ( *PathExists )( const char *path );

	void ( *ScanDirectory )( const char *path, const char *extension, void ( *Function )( const char *, void * ), bool recursive, void *userData );

	bool ( *CreateDirectory )( const char *path );
	bool ( *CreatePath )( const char *path );

	PLFile *( *CreateFileFromMemory )( const char *path, void *buf, size_t bufSize, PLFileMemoryBufferType bufType );
	PLFile *( *OpenLocalFile )( const char *path, bool cache );
	PLFile *( *OpenFile )( const char *path, bool cache );
	void ( *CloseFile )( PLFile *file );

	bool ( *IsEndOfFile )( const PLFile *file );

	const char *( *GetFilePath )( const PLFile *file );
	const void *( *GetFileData )( const PLFile *file );
	size_t ( *GetFileSize )( const PLFile *file );
	PLFileOffset ( *GetFileOffset )( const PLFile *file );

	size_t ( *ReadFile )( PLFile *file, void *destination, size_t size, size_t count );

	int8_t ( *ReadInt8 )( PLFile *file, bool *status );
	int16_t ( *ReadInt16 )( PLFile *file, bool bigEndian, bool *status );
	int32_t ( *ReadInt32 )( PLFile *file, bool bigEndian, bool *status );
	int64_t ( *ReadInt64 )( PLFile *file, bool bigEndian, bool *status );

	char *( *ReadString )( PLFile *file, char *destination, size_t size );

	bool ( *FileSeek )( PLFile *file, PLFileOffset pos, PLFileSeek seek );
	void ( *RewindFile )( PLFile *file );

	const void *( *CacheFile )( PLFile *file );

	/**
 	 * PLUGIN API
 	 **/

	PLPackage *( *CreatePackageHandle )( const char *path, unsigned int tableSize, uint8_t *( *OpenFile )( PLFile *filePtr, PLPackageIndex *index ) );

	void ( *RegisterPackageLoader )( const char *extension, PLPackage *( *LoadFunction )( const char *path ) );
	void ( *RegisterImageLoader )( const char *extension, PLImage *( *LoadFunction )( PLFile *file ) );

	const char *( *GetPackagePath )( const PLPackage *package );
	unsigned int ( *GetPackageTableSize )( const PLPackage *package );
	int ( *GetPackageTableIndex )( const PLPackage *package, const char *indexName );

	const char *( *GetPackageFileName )( const PLPackage *package, unsigned int index );

	/**
	 * IMAGE API
	 **/

	PLImage *( *CreateImage )( uint8_t *buf, unsigned int width, unsigned int height, PLColourFormat colourFormat, PLImageFormat dataFormat );
	void ( *DestroyImage )( PLImage *image );
	bool ( *ConvertPixelFormat )( PLImage *image, PLImageFormat newFormat );
	void ( *InvertImageColour )( PLImage *image );
	void ( *ReplaceImageColour )( PLImage *image, PLColour target, PLColour destination );
	bool ( *FlipImageVertical )( PLImage *image );

	unsigned int ( *GetImageSize )( PLImageFormat format, unsigned int width, unsigned int height );

	/** v2.0 ************************************************/

	/**
	 * CONSOLE API
	 **/

	int ( *AddLogLevel )( const char *prefix, PLColour colour, bool status );
	void ( *SetLogLevelStatus )( int id, bool status );
	void ( *LogMessage )( int id, const char *msg, ... );

	const char *( *GetConsoleVariableValue )( const char *name );
	const char *( *GetConsoleVariableDefaultValue )( const char *name );
	void ( *SetConsoleVariable )( const char *name, const char *value );
	PLConsoleVariable *( *RegisterConsoleVariable )( const char *name, const char *def, PLVariableType type, void ( *CallbackFunction )( const PLConsoleVariable *var ), const char *description );
	void ( *RegisterConsoleCommand )( const char *name, void ( *CallbackFunction )( unsigned int argc, char **argv ), const char *description );
	void ( *ParseConsoleString )( const char *string );

	/**
	 * SCRIPT API
	 **/

	bool ( *IsEndOfLine )( const char *p );

	void ( *SkipWhitespace )( const char **p );
	void ( *SkipLine )( const char **p );

	const char *( *ParseEnclosedString )( const char **p, char *dest, size_t size );
	const char *( *ParseToken )( const char **p, char *dest, size_t size );
	int ( *ParseInteger )( const char **p, bool *status );
	float ( *ParseFloat )( const char **p, bool *status );

	/** v4.1 ************************************************/

	void ( *GenerateChecksumCRC32 )( const void *buf, uint32_t bufSize, uint32_t *crc );
} PLPluginExportTable;

/* be absolutely sure to change this whenever the API is updated! */
#define PL_PLUGIN_INTERFACE_VERSION_MAJOR 6
#define PL_PLUGIN_INTERFACE_VERSION_MINOR 1
#define PL_PLUGIN_INTERFACE_VERSION \
	( uint16_t[ 2 ] ) { PL_PLUGIN_INTERFACE_VERSION_MAJOR, PL_PLUGIN_INTERFACE_VERSION_MINOR }

#define PL_PLUGIN_QUERY_FUNCTION "PLQueryPlugin"
typedef const PLPluginDescription *( *PLPluginQueryFunction )( void );
#define PL_PLUGIN_INIT_FUNCTION "PLInitializePlugin"
typedef void ( *PLPluginInitializationFunction )( const PLPluginExportTable *exportTable );

/* 2022-01-12
 * - Expose CacheFile functionality
 *
 * 2022-01-10
 * - RegisterImageLoader now takes a File handle
 *
 * 2021-10-03
 * - Memory allocation functions now take an 'abortOnFail' parameter
 *
 * 2021-04-22
 * - Removed some functions from the default interface
 *
 * 2021-03-29;
 * - changed how version is queried and introduced minor
 * - plugins can now push log messages
 * - plugins can now create and execute commands via console api
 * - introduced graphics api interface */
