/**
 * Hei Platform Library
 * Copyright (C) 2017-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is licensed under MIT. See LICENSE for more details.
 */

#pragma once

#include <plcore/pl.h>

typedef enum PLCompressionType {
	PL_COMPRESSION_NONE,
	PL_COMPRESSION_ZLIB,
	PL_COMPRESSION_IMPLODE,

	PL_MAX_COMPRESSION_FORMATS
} PLCompressionType;

PL_EXTERN_C

void *PlDeflateCompression( const void *data, unsigned long length, unsigned long *outLength );

PL_EXTERN_C_END
