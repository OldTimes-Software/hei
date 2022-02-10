/**
 * Hei Platform Library
 * Copyright (C) 2017-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is licensed under MIT. See LICENSE for more details.
 */

#include <plcore/pl_compression.h>

#include "3rdparty/miniz/miniz.h"

/*
 * Expose miniz functionality through the platform library,
 * so other libraries don't need to also include it.
 */

void *PlDeflateCompression( const void *data, unsigned long length, unsigned long *outLength ) {
	unsigned long compressedLength = mz_compressBound( length );
	void *compressedData = PlMAllocA( compressedLength );
	int status = mz_compress( compressedData, &compressedLength, data, length );
	if ( status != MZ_OK ) {
		PlReportErrorF( PL_RESULT_FAIL, "failed to compress data: %s", mz_error( status ) );
		PlFree( compressedData );
		return NULL;
	}

	*outLength = compressedLength;
	return compressedData;
}
