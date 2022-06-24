/**
 * Hei Platform Library
 * Copyright (C) 2017-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is licensed under MIT. See LICENSE for more details.
 */

#include <plcore/pl_math.h>

int *PlSeedRandom( int seed ) {
	int *s = ( int * ) PlMAllocA( sizeof( int ) );
	*s = seed;
	return s;
}
