/**
 * Hei Platform Library
 * Copyright (C) 2017-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is licensed under MIT. See LICENSE for more details.
 */

#include <plcore/pl_math.h>

/****************************************
 * Type Conversion
 ****************************************/

PLColour PlColourF32ToU8( const PLColourF32 *in ) {
	return ( PLColour ){
	        PlFloatToByte( in->r ),
	        PlFloatToByte( in->g ),
	        PlFloatToByte( in->b ),
	        PlFloatToByte( in->a ) };
}

PLColourF32 PlColourU8ToF32( const PLColour *in ) {
	return ( PLColourF32 ){
	        PlByteToFloat( in->r ),
	        PlByteToFloat( in->g ),
	        PlByteToFloat( in->b ),
	        PlByteToFloat( in->a ) };
}

/****************************************
 * Addition
 ****************************************/

PLColour PlAddColour( const PLColour *c, const PLColour *c2 ) {
	return ( PLColour ){
	        c->r + c2->r,
	        c->g + c2->g,
	        c->b + c2->b,
	        c->a + c2->a };
}

PLColourF32 PlAddColourF32( const PLColourF32 *c, const PLColourF32 *c2 ) {
	return ( PLColourF32 ){
		c->r + c2->r,
		c->g + c2->g,
		c->b + c2->b,
		c->a + c2->a };
}
