/*
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org>
*/

#include <PL/platform_filesystem.h>
#include <PL/platform_package.h>

typedef struct XDFString {
	unsigned int    length;
	char            string[ PL_SYSTEM_MAX_PATH ];
} XDFString;

typedef struct XDFHeader {
	uint32_t    tableLength;
	char        *tableBuffer;
	uint32_t    numTableIndices;
} XDFHeader;

/**
 * Reads in the XDF header.
 */
static bool XDF_LoadHeader( PLFile *filePtr, XDFHeader *xdfOut ) {
	XDFHeader header;

	bool status;
	header.tableLength = plReadInt32( filePtr, false, &status );
	if ( !status ) {
		return false;
	}

	header.tableBuffer = PlMAllocA( sizeof( char ) * header.tableLength );

	if ( plReadFile( filePtr, header.tableBuffer, sizeof( char ), header.tableLength ) != header.tableLength ) {
		PlFree( header.tableBuffer );
		return false;
	}

	return true;
}

/**
 * Navigates the header file table and splits it up into an array.
 */
static XDFString *XDF_CreateFileTable( const XDFHeader *header ) {
	XDFString *fileList = PlMAllocA( header->numTableIndices * sizeof( XDFString ) );
	XDFString *curIndex = fileList;
	for ( unsigned int i = 0, j = 0; i < header->tableLength; ++i ) {
		curIndex->string[ j++ ] = header->tableBuffer[ i ];
		if ( header->tableBuffer[ i ] == '\0' ) {
			curIndex++;
		}
	}

	return fileList;
}

PLPackage *plLoadXDFPackage( const char* path ) {
	_plResetError();

	PLFile *filePtr = plOpenFile( path, false );
	if ( filePtr == NULL) {
		return NULL;
	}

	XDFHeader header;
	bool status = XDF_LoadHeader( filePtr, &header );

	/* close the file, we shouldn't need anything else */
	plCloseFile( filePtr );

	if ( !status ) {
		return NULL;
	}

	/* allocate our list container */
	XDFString *fileTable = XDF_CreateFileTable( &header );

	/* now free up the table buffer, as we don't need it anymore */
	PlFree( header.tableBuffer );

	if ( fileTable == NULL ) {

	}

	return NULL;
}
