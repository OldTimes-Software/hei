/**
 * Hei Platform Library
 * Copyright (C) 2017-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is licensed under MIT. See LICENSE for more details.
 */

#include <stdio.h>

int pl_dectobin( int dec ) {
	if ( dec == 0 ) {
		return 0;
	}

	return ( dec % 2 + 10 * pl_dectobin( dec / 2 ) );
}

// lazy ass implementation ...
char *pl_itoa( int val, char *buf, size_t len, int base ) {
	switch ( base ) {
		default:
			return buf;
		case 10:
			snprintf( buf, len, "%d", val );
			break;
		case 16:
			snprintf( buf, len, "%x", val );
			break;
		case 8:
			snprintf( buf, len, "%o", val );
			break;
		case 2:
			val = pl_dectobin( val );
			snprintf( buf, len, "%d", val );
			break;
	}

	return buf;
}
