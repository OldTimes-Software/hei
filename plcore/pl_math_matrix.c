/**
 * Hei Platform Library
 * Copyright (C) 2017-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is licensed under MIT. See LICENSE for more details.
 */

#include "pl_private.h"

#include <plcore/pl_math.h>

PLVector2 PlConvertWorldToScreen( const PLVector3 *position, const PLMatrix4 *viewProjMatrix ) {
	PLVector4 posw = PlVector4( position->x, position->y, position->z, 1.0f );
	PLVector4 ppos = PlTransformVector4( &posw, viewProjMatrix );
	return PlVector2( ppos.x / ppos.w, ppos.y / ppos.w );
}

/****************************************
 ****************************************/
 
PLMatrix4 PlRotateMatrix4( float angle, const PLVector3 *axis ) {
	float s = sinf( angle );
	float c = cosf( angle );
	float t = 1.0f - c;

	PLVector3 tv = PlVector3( t * axis->x, t * axis->y, t * axis->z );
	PLVector3 sv = PlVector3( s * axis->x, s * axis->y, s * axis->z );

	PLMatrix4 m;

	m.pl_m4pos( 0, 0 ) = tv.x * axis->x + c;
	m.pl_m4pos( 1, 0 ) = tv.x * axis->y + sv.z;
	m.pl_m4pos( 2, 0 ) = tv.x * axis->z - sv.y;

	m.pl_m4pos( 0, 1 ) = tv.x * axis->y - sv.z;
	m.pl_m4pos( 1, 1 ) = tv.y * axis->y + c;
	m.pl_m4pos( 2, 1 ) = tv.y * axis->z + sv.x;

	m.pl_m4pos( 0, 2 ) = tv.x * axis->z + sv.y;
	m.pl_m4pos( 1, 2 ) = tv.y * axis->z - sv.x;
	m.pl_m4pos( 2, 2 ) = tv.z * axis->z + c;

	m.pl_m4pos( 3, 0 ) = 0;
	m.pl_m4pos( 3, 1 ) = 0;
	m.pl_m4pos( 3, 2 ) = 0;
	m.pl_m4pos( 0, 3 ) = 0;
	m.pl_m4pos( 1, 3 ) = 0;
	m.pl_m4pos( 2, 3 ) = 0;
	m.pl_m4pos( 3, 3 ) = 1.0f;

	return m;
}

PLVector3 PlGetMatrix4Translation( const PLMatrix4 *m ) {
	return PlVector3( m->pl_m4pos( 0, 3 ), m->pl_m4pos( 1, 3 ), m->pl_m4pos( 2, 3 ) );
}

PLVector3 PlGetMatrix4Angle( const PLMatrix4 *m ) {
	PLVector3 out = PlVector3( 0, 0, 0 );
	out.y = PlRadiansToDegrees( asinf( m->m[ 8 ] ) );
	if ( m->m[ 10 ] < 0 ) {
		if ( out.y >= 0 ) {
			out.y = 180.f - out.y;
		} else {
			out.y = -180.f - out.y;
		}
	}
	if ( m->m[ 0 ] > -PL_EPSILON && m->m[ 0 ] < PL_EPSILON ) {
		out.x = PlRadiansToDegrees( atan2f( m->m[ 1 ], m->m[ 5 ] ) );
	} else {
		out.z = PlRadiansToDegrees( atan2f( -m->m[ 4 ], m->m[ 0 ] ) );
		out.x = PlRadiansToDegrees( atan2f( -m->m[ 9 ], m->m[ 10 ] ) );
	}
	return out;
}

/****************************************
 * Matrix Stack, sorta mirrors OpenGL behaviour
 ****************************************/

#define MAX_STACK_SIZE 64
static PLMatrix4 stacks[ PL_NUM_MATRIX_MODES ][ MAX_STACK_SIZE ];
static PLMatrixMode curMatrixMode = PL_MODELVIEW_MATRIX;
static unsigned int curStackSlot[ PL_NUM_MATRIX_MODES ];

/**
 * Private function that initializes all of the matrix stacks.
 */
void PlInitializeMatrixStacks_( void ) {
	/* make sure all of the slots default to 0 */
	memset( curStackSlot, 0, sizeof( unsigned int ) * PL_NUM_MATRIX_MODES );

	/* every stack starts off with identity matrix for first slot */
	for ( unsigned int i = 0; i < PL_NUM_MATRIX_MODES; ++i ) {
		stacks[ i ][ 0 ] = PlMatrix4Identity();
	}
}

/**
 * Returns a pointer to the current matrix stack.
 */
PLMatrix4 *PlGetMatrix( PLMatrixMode mode ) {
	return &stacks[ mode ][ curStackSlot[ mode ] ];
}

void PlMatrixMode( PLMatrixMode mode ) {
	curMatrixMode = mode;
}

PLMatrixMode PlGetMatrixMode( void ) {
	return curMatrixMode;
}

void PlLoadMatrix( const PLMatrix4 *matrix ) {
	PLMatrix4 *curStack = PlGetMatrix( curMatrixMode );
	*curStack = *matrix;
}

void PlLoadIdentityMatrix( void ) {
	PLMatrix4 identity = PlMatrix4Identity();
	PlLoadMatrix( &identity );
}

void PlMultiMatrix( const PLMatrix4 *matrix ) {
	PLMatrix4 *curStack = PlGetMatrix( curMatrixMode );
	*curStack = PlMultiplyMatrix4( *curStack, *matrix );
}

void PlRotateMatrix( float angle, float x, float y, float z ) {
	PLMatrix4 rotation = PlRotateMatrix4( angle, &PlVector3( x, y, z ) );
	PlMultiMatrix( &rotation );
}

void PlTranslateMatrix( PLVector3 vector ) {
	PLMatrix4 translate = PlTranslateMatrix4( vector );
	PlMultiMatrix( &translate );
}

void PlScaleMatrix( PLVector3 scale ) {
	PLMatrix4 *curStack = PlGetMatrix( curMatrixMode );
	*curStack = PlScaleMatrix4( *curStack, scale );
}

void PlInverseMatrix( void ) {
	PLMatrix4 *curStack = PlGetMatrix( curMatrixMode );
	*curStack = PlInverseMatrix4( *curStack );
}

void PlPushMatrix( void ) {
	if ( curStackSlot[ curMatrixMode ] >= MAX_STACK_SIZE ) {
		PlReportBasicError( PL_RESULT_MEMORY_EOA );
		return;
	}

	PLMatrix4 *curStack = PlGetMatrix( curMatrixMode );
	curStackSlot[ curMatrixMode ]++;
	PLMatrix4 *newStack = PlGetMatrix( curMatrixMode );
	*newStack = *curStack;
}

void PlPopMatrix( void ) {
	if ( curStackSlot[ curMatrixMode ] == 0 ) {
		PlReportBasicError( PL_RESULT_MEMORY_UNDERFLOW );
		return;
	}

	curStackSlot[ curMatrixMode ]--;
}
