/**
 * Hei Platform Library
 * Copyright (C) 2017-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is licensed under MIT. See LICENSE for more details.
 */

#include <plcore/pl_parse.h>
#include <plcore/pl_math.h>

/**
 * On success, returns the number of characters
 * to skip to reach the next line. If EOF, returns
 * 0 but on error returns -1.
 */
int PlGetLineEndType( const char *p ) {
	if ( *p == '\0' ) return PL_PARSE_NL_EOF;
	if ( *p == '\n' ) return PL_PARSE_NL_LF;
	if ( *p == '\r' && *( p + 1 ) == '\n' ) return PL_PARSE_NL_CRLF;

	return PL_PARSE_NL_INVALID;
}

bool PlIsEndOfLine( const char *p ) {
	return ( PlGetLineEndType( p ) != PL_PARSE_NL_INVALID );
}

bool PlIsWhitespace( const char *p ) {
	/* don't treat line end as whitespace */
	if ( PlIsEndOfLine( p ) ) {
		return false;
	}

	return isspace( *p );
}

void PlSkipWhitespace( const char **p ) {
	if ( !PlIsWhitespace( ( *p ) ) ) {
		return;
	}

	do {
		( *p )++;
	} while ( PlIsWhitespace( ( *p ) ) );
}

#define NOT_TERMINATING_CHAR( P ) ( ( P ) != '\0' && ( P ) != '\n' && ( P ) != '\r' )

void PlSkipLine( const char **p ) {
	while ( NOT_TERMINATING_CHAR( *( *p ) ) ) ( *p )++;
	if ( *( *p ) == '\r' ) ( *p )++;
	if ( *( *p ) == '\n' ) ( *p )++;
}

const char *PlParseEnclosedString( const char **p, char *dest, size_t size ) {
	PlSkipWhitespace( p );

	bool isEnclosed = false;
	if ( *( *p ) == '\"' ) {
		( *p )++;
		isEnclosed = true;
	}
	size_t i = 0;
	while ( NOT_TERMINATING_CHAR( *( *p ) ) ) {
		if ( !isEnclosed && ( *( *p ) == ' ' || *( *p ) == '\t' || *( *p ) == '\n' || *( *p ) == '\r' ) ) {
			break;
		} else if ( isEnclosed && *( *p ) == '\"' ) {
			( *p )++;
			break;
		}

		if ( ( i + 1 ) < size ) {
			dest[ i++ ] = *( *p );
		}
		( *p )++;
	}

	dest[ i ] = '\0';
	return dest;
}

/**
 * Returns the potential length of a token in a buffer.
 */
unsigned int PlDetermineTokenLength( const char *p ) {
	PlSkipWhitespace( &p );

	unsigned int length = 0;
	while ( *p != '\0' && *p != ' ' ) {
		if ( PlIsEndOfLine( p ) ) {
			break;
		}

		length++;
		p++;
	}

	return length;
}

const char *PlParseToken( const char **p, char *dest, size_t size ) {
	PlSkipWhitespace( p );

	unsigned int length = PlDetermineTokenLength( ( *p ) );
	size_t i;
	for ( i = 0; i < length; ++i ) {
		if ( ( i + 1 ) >= size ) {
			break;
		}

		dest[ i ] = ( *p )[ i ];
	}

	( *p ) += length;

	dest[ i ] = '\0';
	return dest;
}

int PlParseInteger( const char **p, bool *status ) {
	if ( status != NULL ) *status = false;

	char num[ 64 ] = { '\0' };
	PlParseToken( p, num, sizeof( num ) );

	int v = 0;
	if ( *num != '\0' ) {
		v = strtol( num, NULL, 10 );
		/* todo: validate strtol succeeded! */
		if ( status != NULL ) *status = true;
	}

	return v;
}

float PlParseFloat( const char **p, bool *status ) {
	if ( status != NULL ) *status = false;

	char num[ 64 ];
	PlParseToken( p, num, sizeof( num ) );

	float v = 0.0f;
	if ( *num != '\0' ) {
		v = strtof( num, NULL );
		/* todo: validate strtof succeeded! */
		if ( status != NULL ) *status = true;
	}

	return v;
}

PLVector3 PlParseVector( const char **p, bool *status ) {
	PlSkipWhitespace( p );
	if ( *( *p ) == '(' ) { ( *p )++; }
	float x = PlParseFloat( p, status );
	float y = PlParseFloat( p, status );
	float z = PlParseFloat( p, status );
	if ( *( *p ) == ')' ) { ( *p )++; }
	return PLVector3( x, y, z );
}

/**
 * Returns the potential length of a line in a
 * buffer. This is either up to the point of a
 * new line or alternatively, a null-terminator,
 * whichever is encountered first.
 */
unsigned int PlDetermineLineLength( const char *p ) {
	unsigned int length = 0;
	while ( NOT_TERMINATING_CHAR( *p ) ) {
		length++;
		p++;
	}

	return length;
}

const char *PlParseLine( const char **p, char *dest, size_t size ) {
	unsigned int length = PlDetermineLineLength( ( *p ) );
	size_t i;
	for ( i = 0; i < length; ++i ) {
		if ( ( i + 1 ) >= size ) {
			break;
		}

		dest[ i ] = ( *p )[ i ];
	}

	( *p ) += length + PlGetLineEndType( ( *p ) + length );

	dest[ i ] = '\0';
	return dest;
}
