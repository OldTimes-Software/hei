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

#pragma once

#include <plcore/pl.h>
#include <plcore/pl_math.h>

/**
 * References:
 *	http://www.3dkingdoms.com/weekly/weekly.php?a=21
 */

PL_EXTERN_C

typedef struct PLCollisionRay {
	PLVector3 origin, direction;
} PLCollisionRay;
typedef struct PLCollisionAABB {
	PLVector3 origin, mins, maxs, absOrigin;
} PLCollisionAABB;
typedef struct PLCollisionSphere {
	PLVector3 origin;
	float radius;
} PLCollisionSphere;
typedef struct PLCollisionPlane {
	PLVector3 origin, normal;
} PLCollisionPlane;

typedef struct PLCollision {
	PLVector3 contactNormal;
	PLVector3 contactPoint;
	float penetration;
} PLCollision;

#ifndef __cplusplus
#define PlSetupCollisionRay( ORIGIN, DIRECTION ) \
	( PLCollisionRay ) { ( ORIGIN ), ( DIRECTION ) }
#define PlSetupCollisionAABB( ORIGIN, MINS, MAXS ) \
	( PLCollisionAABB ) { ( ORIGIN ), ( MINS ), ( MAXS ) }
#define PlSetupCollisionSphere( ORIGIN, RADIUS ) \
	( PLCollisionSphere ) { ( ORIGIN ), ( RADIUS ) }
#define PlSetupCollisionPlane( ORIGIN, NORMAL ) \
	( PLCollisionPlane ) { ( ORIGIN ), ( NORMAL ) }
#endif

float PlComputeSphereFromCoords( const PLVector3 *vertices, unsigned int numVertices );

PLCollisionAABB PlGenerateAabbFromCoords( const PLVector3 *vertices, unsigned int numVertices, bool absolute );
PLVector3 PlGetAabbAbsOrigin( const PLCollisionAABB *bounds, PLVector3 origin );

bool PlIsAabbIntersecting( const PLCollisionAABB *aBounds, const PLCollisionAABB *bBounds );
bool PlIsPointIntersectingAabb( const PLCollisionAABB *bounds, PLVector3 point );

/**
 * Get which side of a line a point is on.
 */
inline static float PlTestPointLinePosition( const PLVector2 *position, const PLVector2 *lineStart, const PLVector2 *lineEnd ) {
	float d = ( lineEnd->x - lineStart->x ) * ( position->y - lineStart->y ) - ( lineEnd->y - lineStart->y ) * ( position->x - lineStart->x );
	return d;
}

inline static bool PlIsPointIntersectingLine( const PLVector2 *position, const PLVector2 *lineStart, const PLVector2 *lineEnd, const PLVector2 *lineNormal, float *intersection ) {
	*intersection = 0.0f;

#if 0
	PLVector2 xCoord = ( lineEnd->x > lineStart->x ) ? PLVector2( lineEnd->x, lineStart->x ) : PLVector2( lineStart->x, lineEnd->x );
	PLVector2 yCoord = ( lineEnd->y > lineStart->y ) ? PLVector2( lineEnd->y, lineStart->y ) : PLVector2( lineStart->y, lineEnd->y );
	if( !( position->x > xCoord.x && position->x < xCoord.y ) && !( position->y > yCoord.x && position->y < yCoord.y ) ) {
		return false;
	}
#endif

	//if( ( position->x > xCoord.y || position->x < xCoord.x ) && ( position->y > yCoord.y || position->y < yCoord.x ) ) {
	//	return false;
	//}

	/* figure out which side of the line we're on */
	float lineSide = PlTestPointLinePosition( position, lineStart, lineEnd );
	if ( lineSide <= -1000.0f ) {
		return false;
	}

	*intersection = lineSide;

	return ( *intersection <= 1000.0f );
}

inline static bool PlTestLineIntersection( float dst1, float dst2, const PLVector3 *lineStart, const PLVector3 *lineEnd, PLVector3 *hit ) {
	if ( ( dst1 * dst2 ) >= 0.0f || dst1 == dst2 ) {
		return false;
	}

	for ( unsigned int i = 0; i < 3; ++i ) {
		PlVector3Index( hit, i ) = PlVector3Index( lineStart, i ) + ( PlVector3Index( lineEnd, i ) - PlVector3Index( lineStart, i ) ) * ( -dst1 / ( dst2 - dst1 ) );
	}

	return true;
}

/**
 * Checks whether or not AABB is intersecting with the given line.
 * Currently only works in 2D space (X & Z).
 */
inline static bool PlIsAabbIntersectingLine( const PLCollisionAABB *bounds, const PLVector2 *lineStart, const PLVector2 *lineEnd, const PLVector2 *lineNormal ) {
#if 0
	PLVector2 origin = PLVector2( bounds->origin.x, bounds->origin.z );

	PLVector2 a = plAddVector2( PLVector2( bounds->mins.x, bounds->mins.z ), origin );
	PLVector2 b = plAddVector2( PLVector2( bounds->maxs.x, bounds->mins.z ), origin );
	//PLVector2 c = plAddVector2( PLVector2( bounds->mins.x, bounds->maxs.z ), origin );
	//PLVector2 d = plAddVector2( PLVector2( bounds->maxs.x, bounds->maxs.z ), origin );

	float aR, bR, cR, dR;
	plIsPointIntersectingLine( &a, lineStart, lineEnd, lineNormal, &aR );
	plIsPointIntersectingLine( &b, lineStart, lineEnd, lineNormal, &bR );
	//plIsPointIntersectingLine( &c, lineStart, lineEnd, lineNormal, &cR );
	//plIsPointIntersectingLine( &d, lineStart, lineEnd, lineNormal, &dR );

#if 0
	if( x == 0.0f || y == 0.0f ) {
		return PLVector2( x, y );
	}
#endif
#endif

	return false;
}

inline static bool PlIsLineInBox( const PLCollisionAABB *bounds, const PLVector3 *lineStart, const PLVector3 *lineEnd ) {
	/* get line midpoint and extent */
	//	PLVector3 lineMiddle = plScaleVector3f( plAddVector3( *lineStart, *lineEnd ), 0.5f );
	//	PLVector3 l = plSubtractVector3( *lineStart, lineMiddle );
	//	PLVector3 lineExtent = PLVector3( fabsf( l.x ), fabsf( l.y ), fabsf( l.z ) );


	/* no separating axis, the line intersects */
	return true;
}

bool PlIsSphereIntersecting( const PLCollisionSphere *aSphere, const PLCollisionSphere *bSphere );
PLCollision PlIsSphereIntersectingPlane( const PLCollisionSphere *sphere, const PLCollisionPlane *plane );

/* https://github.com/erich666/GraphicsGems/blob/master/gemsii/intersect/intsph.c */
inline static bool PlIsRayIntersectingSphere( const PLCollisionSphere *sphere, const PLCollisionRay *ray, float *enterDistance, float *leaveDistance ) {
	PLVector3 d = PlSubtractVector3( ray->origin, sphere->origin );
	float u = PlVector3DotProduct( d, d ) - sphere->radius * sphere->radius;
	float bsq = PlVector3DotProduct( d, ray->direction );
	float disc = bsq * bsq - u;

	if ( disc >= 0.0f ) {
		float root = sqrtf( disc );
		*enterDistance = -bsq - root;
		*leaveDistance = -bsq + root;
		return true;
	}

	return false;
}

/* https://github.com/erich666/GraphicsGems/blob/master/gems/RayBox.c */
inline static bool PlIsRayIntersectingAabb( const PLCollisionAABB *bounds, const PLCollisionRay *ray, PLVector3 *hitPoint ) {
	PLVector3 max = PlAddVector3( bounds->maxs, bounds->origin );
	PLVector3 min = PlAddVector3( bounds->mins, bounds->origin );

	/* Find candidate planes */

	static const unsigned char right = 0;
	static const unsigned char left = 1;
	static const unsigned char middle = 2;

	PLVector3 candidatePlane;
	unsigned char hitQuadrant[ 3 ];
	bool isInside = true;

	for ( unsigned int i = 0; i < 3; ++i ) {
		if ( PlVector3Index( ray->origin, i ) < PlVector3Index( min, i ) ) {
			hitQuadrant[ i ] = left;
			PlVector3Index( candidatePlane, i ) = PlVector3Index( min, i );
			isInside = false;
		} else if ( PlVector3Index( ray->origin, i ) > PlVector3Index( max, i ) ) {
			hitQuadrant[ i ] = right;
			PlVector3Index( candidatePlane, i ) = PlVector3Index( max, i );
			isInside = false;
		} else {
			hitQuadrant[ i ] = middle;
		}
	}

	/* Ray origin inside bounding box */
	if ( isInside ) {
		*hitPoint = ray->origin;
		return true;
	}

	/* Calculate T distances to candidate planes */
	PLVector3 maxT;
	for ( unsigned int i = 0; i < 3; ++i ) {
		if ( hitQuadrant[ i ] != middle && PlVector3Index( ray->direction, i ) != 0.0f ) {
			PlVector3Index( maxT, i ) = ( PlVector3Index( candidatePlane, i ) - PlVector3Index( ray->origin, i ) ) / PlVector3Index( ray->direction, i );
		} else {
			PlVector3Index( maxT, i ) = -1.0f;
		}
	}

	/* Get largest of the maxT's for final choice of intersection */
	unsigned int whichPlane = 0;
	for ( unsigned int i = 1; i < 3; ++i ) {
		if ( PlVector3Index( maxT, whichPlane ) < PlVector3Index( maxT, i ) ) {
			whichPlane = i;
		}
	}

	/* Check final candidate actually inside box */

	if ( PlVector3Index( maxT, whichPlane ) < 0.0f ) {
		return false;
	}

	for ( unsigned int i = 0; i < 3; ++i ) {
		if ( whichPlane != i ) {
			PlVector3Index( hitPoint, i ) = PlVector3Index( ray->origin, i ) + PlVector3Index( maxT, whichPlane ) * PlVector3Index( ray->direction, i );
			if ( PlVector3Index( hitPoint, i ) < PlVector3Index( min, i ) || PlVector3Index( hitPoint, i ) > PlVector3Index( max, i ) ) {
				return false;
			}
		} else {
			PlVector3Index( hitPoint, i ) = PlVector3Index( candidatePlane, i );
		}
	}

	return true;
}

PL_EXTERN_C_END

/************************************************************/
