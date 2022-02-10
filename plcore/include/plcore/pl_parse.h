/**
 * Hei Platform Library
 * Copyright (C) 2017-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is licensed under MIT. See LICENSE for more details.
 */

#pragma once

#include <plcore/pl.h>

enum {
	PL_PARSE_NL_INVALID = -1,
	PL_PARSE_NL_EOF = 0,
	PL_PARSE_NL_LF = 1,
	PL_PARSE_NL_CRLF = 2,
};

PL_EXTERN_C

typedef struct PLVector3 PLVector3;

#if !defined( PL_COMPILE_PLUGIN )

extern bool PlIsEndOfLine( const char *p );
extern int PlGetLineEndType( const char *p );

extern void PlSkipWhitespace( const char **p );
extern void PlSkipLine( const char **p );
extern const char *PlParseEnclosedString( const char **p, char *dest, size_t size );
extern const char *PlParseToken( const char **p, char *dest, size_t size );
extern int PlParseInteger( const char **p, bool *status );
extern float PlParseFloat( const char **p, bool *status );
extern PLVector3 PlParseVector( const char **p, bool *status );

extern unsigned int PlDetermineLineLength( const char *p );
const char *PlParseLine( const char **p, char *dest, size_t size );

#endif /* !defined( PL_COMPILE_PLUGIN ) */

PL_EXTERN_C_END
