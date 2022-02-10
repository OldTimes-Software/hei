/*
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org>
*/

#include "../graphics/graphics_private.h"

typedef struct PLPrimitive {
	PLPolygon *polygons;
	unsigned int numPolygons;
	unsigned int maxPolygons;
	
	PLVector3 position;
	PLVector3 angles;
} PLPrimitive;

PLPrimitive *plCreatePrimitive( void ) {
	PLPrimitive *primitive = PlCAllocA( 1, sizeof( PLPrimitive ) );

	

	/* todo: create default cube... */
	return primitive;
}

PLPolygon *plGetPrimitivePolygon( PLPrimitive *primitive, unsigned int polyIndex ) {
	if( polyIndex >= primitive->numPolygons ) {
		ReportError( PL_RESULT_INVALID_PARM2, "Invalid polygon index, %d (%d)!\n", polyIndex, primitive->numPolygons );
		return NULL;
	}

	PLPolygon *polygon = &primitive->polygons[ polyIndex ];
	if( polygon == NULL ) {
		ReportError( PL_RESULT_INVALID_PARM2, "Polygon was null at %d!\n", polyIndex );
		return NULL;
	}

	return polygon;
}

unsigned int plGetNumberOfPrimitivePolygons( const PLPrimitive *primitive ) {
	return primitive->numPolygons;
}

PLPolygon *plGetPrimitivePolygons( PLPrimitive *primitive, unsigned int *numPolygons ) {
	*numPolygons = primitive->numPolygons;
	return primitive->polygons;
}

/**
 * Returns the number of adjacent polygons.
 */
unsigned int plGetNumberOfAdjacentPrimitivePolygons( PLPrimitive *primitive, unsigned int polyIndex ) {
	PLPolygon *polygon = plGetPrimitivePolygon( primitive, polyIndex );
	if( polygon == NULL ) {
		return 0;
	}

	unsigned int numPolys = 0;
	for( unsigned int i = 0; i < polygon->numVertices; ++i ) {
		for( unsigned int j = 0; j < primitive->numPolygons; ++j ) {
			/* ensure we're not checked out ourself */
			if( &primitive->polygons[ j ] == polygon ) {
				continue;
			}

			for( unsigned int k = 0; k < primitive->polygons[ j ].numVertices; ++k ) {
				if( &primitive->polygons[ j ].vertices[ k ] == &polygon->vertices[ i ] ) {
					numPolys++;
					break;
				}
			}
		}
	}

	return numPolys;
}

/**
 * Returns a list of adjacent polygons by index.
 */
void plGetAdjacentPrimitivePolygons( PLPrimitive *primitive, unsigned int polyIndex, PLPolygon *dest[], unsigned int destSize ) {
	PLPolygon *polygon = plGetPrimitivePolygon( primitive, polyIndex );
	if( polygon == NULL ) {
		return;
	}

	unsigned int numPolys = 0;
	for( unsigned int i = 0; i < polygon->numVertices; ++i ) {
		// Check against each polygon this primitive has
		for( unsigned int j = 0; j < primitive->numPolygons; ++j ) {
			// Ensure we're not checked ourself
			if( &primitive->polygons[ j ] == polygon ) {
				continue;
			}

			for( unsigned int k = 0; k < primitive->polygons[ j ].numVertices; ++k ) {
				if( &primitive->polygons[ j ].vertices[ k ] == &polygon->vertices[ i ] ) {
					dest[ numPolys++ ] = &primitive->polygons[ j ];
					if( numPolys >= destSize ) {
						ReportError( PL_RESULT_INVALID_PARM4, "Destination size is too small for adjacent polygons!\n" );
						return;
					}

					break;
				}
			}
		}
	}
}

/**
 * Checks all faces are connected and discards any that aren't
 */
void plDestroyDisconnectedPrimitivePolygons( PLPrimitive *primitive ) {
	unsigned int numActualPolys = 0;

	for( unsigned int i = 0; i < primitive->numPolygons; ++i ) {
		unsigned int numPolys = plGetNumberOfAdjacentPrimitivePolygons( primitive, i );
		if( numPolys == 0 ) {
			/* disconnected poly, destroy it */
			PLPolygon *polygon = plGetPrimitivePolygon( primitive, i );
			plDestroyPolygon( polygon );
		}

		numActualPolys++;

#if 0
		unsigned int numPolys = plGetNumOfAdjacentPrimitivePolygons( prim, i );
		if( numPolys == 0 ) {
			continue;
		}

		PLPolygon **polygons = PlCAllocA( numPolys, sizeof( *PLPolygon ) );
		plGetAdjacentPrimitivePolygons( prim, i, polygons, numPolys );

		/* blah... */

		PlFree( polygons );
#endif
	}

	if( numActualPolys == primitive->numPolygons ) {
		/* nothing changed */
		return;
	}

	PLPolygon *polygons = PlCAllocA( numActualPolys, sizeof( PLPolygon ) );
	for( unsigned int i = 0; i < numActualPolys; ++i ) {
		if( &primitive->polygons[ i ] == NULL ) {
			continue;
		}

		polygons[ i ] = primitive->polygons[ i ];
	}
}
