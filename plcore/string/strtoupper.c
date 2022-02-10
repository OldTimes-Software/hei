/**
 * Hei Platform Library
 * Copyright (C) 2017-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is licensed under MIT. See LICENSE for more details.
 */

#if !defined( _MSC_VER )
#include <strings.h>
#endif
#include <ctype.h>

char *pl_strtoupper( char *s ) {
	for ( size_t i = 0; s[ i ] != '\0'; ++i ) {
		s[ i ] = ( char ) toupper( s[ i ] );
	}
	return s;
}

char *pl_strntoupper( char *s, size_t n ) {
	for ( size_t i = 0; i < n; ++i ) {
		if ( s[ i ] == '\0' ) {
			break;
		}
		s[ i ] = ( char ) toupper( s[ i ] );
	}
	return s;
}
