/**
 * Hei Platform Library
 * Copyright (C) 2017-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is licensed under MIT. See LICENSE for more details.
 */

#include <ctype.h>

int pl_strisdigit( const char *s ) {
	for ( unsigned int i = 0; s[ i ] != '\0'; ++i ) {
		if ( !( ( i == 0 && s[ i ] == '-' && isdigit( s[ i + 1 ] ) ) || isdigit( s[ i ] ) ) ) {
			return i;
		}
	}
	return -1;
}

int pl_strnisdigit( const char *s, unsigned int n ) {
	for ( unsigned int i = 0; i < n; ++i ) {
		if ( s[ i ] == '\0' ) {
			break;
		}
		if ( !( ( i == 0 && s[ i ] == '-' && isdigit( s[ i + 1 ] ) ) || isdigit( s[ i ] ) ) ) {
			return i;
		}
	}
	return -1;
}
