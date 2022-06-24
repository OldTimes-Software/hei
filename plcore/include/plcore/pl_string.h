/**
 * Hei Platform Library
 * Copyright (C) 2017-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is licensed under MIT. See LICENSE for more details.
 */

#pragma once

PL_EXTERN_C

#define PL_FMT_float  "%f"
#define PL_FMT_double "%lf"
#define PL_FMT_int16  "%hd"
#define PL_FMT_uint16 "%hu"
#define PL_FMT_int32  "%d"
#define PL_FMT_uint32 "%u"
#if defined( __x86_64__ )
#define PL_FMT_int64  "%ld"
#define PL_FMT_uint64 "%lu"
#else
#define PL_FMT_int64  "%lld"
#define PL_FMT_uint64 "%llu"
#endif
#define PL_FMT_hex     "%x"
#define PL_FMT_string  "%s"
#define PL_FMT_address "%p"

char *pl_itoa( int val, char *buf, size_t len, int base );

char *pl_strtolower( char *s );
char *pl_strntolower( char *s, size_t n );
char *pl_strtoupper( char *s );
char *pl_strntoupper( char *s, size_t n );

char *pl_strcasestr( const char *s, const char *find );

int pl_strcasecmp( const char *s1, const char *s2 );
int pl_strncasecmp( const char *s1, const char *s2, size_t n );

int pl_strisalpha( const char *s );
int pl_strnisalpha( const char *s, unsigned int n );
int pl_strisalnum( const char *s );
int pl_strnisalnum( const char *s, unsigned int n );
int pl_strisdigit( const char *s );
int pl_strnisdigit( const char *s, unsigned int n );

int pl_vscprintf( const char *format, va_list pArgs );

unsigned int pl_strcnt( const char *s, char c );
unsigned int pl_strncnt( const char *s, char c, unsigned int n );

char *pl_strchunksplit( const char *string, unsigned int length, const char *seperator );
char *pl_strinsert( const char *string, char **buf, size_t *bufSize, size_t *maxBufSize );

/**
 * http://www.cse.yorku.ca/~oz/hash.html#sdbm
 */
static inline uint32_t pl_strhash_sdbm( const char *str ) {
	uint32_t hash = 0;
	int c;
	while ( ( c = *str++ ) ) {
		hash = c + ( hash << 6 ) + ( hash << 16 ) - hash;
	}

	return hash;
}

PL_EXTERN_C_END
