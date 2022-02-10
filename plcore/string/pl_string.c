/**
 * Hei Platform Library
 * Copyright (C) 2017-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is licensed under MIT. See LICENSE for more details.
 */

#include <plcore/pl.h>

/**
 * Splits a string up by the given seperator, every length characters.
 * Returns a newly allocated buffer on success.
 */
char *pl_strchunksplit( const char *string, unsigned int insLength, const char *seperator ) {
	size_t sl = strlen( string );
	size_t pl = strlen( seperator );
	char *dest = ( char * ) PlMAllocA( ( sl + ( pl * ( sl / insLength ) ) ) + 1 );
	char *p = dest;
	for ( size_t i = 0, j = 1; i < sl; ++i, ++j ) {
		*p++ = string[ i ];
		if ( j == insLength ) {
			j = 0;
			strcpy( p, seperator );
			p += pl;
		}
	}
	return dest;
}

/**
 * Inserts the given string into an existing string buffer.
 * Automatically reallocs buffer if it doesn't fit.
 */
char *pl_strinsert( const char *string, char **buf, size_t *bufSize, size_t *maxBufSize ) {
	/* check if it's going to fit first */
	size_t strLength = strlen( string );
	size_t originalSize = *bufSize;
	*bufSize += strLength;
	if ( *bufSize >= *maxBufSize ) {
		*maxBufSize = *bufSize + strLength;
		*buf = PlReAllocA( *buf, *maxBufSize );
	}

	/* now copy it into our buffer */
	strncpy( *buf + originalSize, string, strLength );

	return *buf + originalSize + strLength;
}
