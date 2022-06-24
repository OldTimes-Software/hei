/**
 * Hei Platform Library
 * Copyright (C) 2017-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is licensed under MIT. See LICENSE for more details.
 */

#pragma once

/******************************************************************/
/* Quaternions */

typedef struct PLQuaternion {
	float x, y, z, w;
} PLQuaternion;

#define PlQuaternion( x, y, z, w ) \
	( PLQuaternion ) { ( float ) ( x ), ( float ) ( y ), ( float ) ( z ), ( float ) ( w ) }

PLQuaternion PlMultiplyQuaternion( const PLQuaternion *q, const PLQuaternion *q2 );
PLQuaternion PlMultiplyQuaternion3FV( const PLQuaternion *q, const PLVector3 *v );

PLQuaternion PlScaleQuaternion( const PLQuaternion *q, float a );

PLQuaternion PlAddQuaternion( const PLQuaternion *q, const PLQuaternion *q2 );
PLQuaternion PlAddQuaternionF( const PLQuaternion *q, float a );

PLQuaternion PlInverseQuaternion( const PLQuaternion *q );

float PlQuaternionDotProduct( const PLQuaternion *q, const PLQuaternion *q2 );
float PlQuaternionLength( const PLQuaternion *q );
PLQuaternion PlNormalizeQuaternion( const PLQuaternion *q );

const char *PlPrintQuaternion( const PLQuaternion *q );

void PlComputeQuaternionW( PLQuaternion *q );

PLVector3 PlQuaternionToEuler( const PLQuaternion *q );
PLQuaternion PlEulerToQuaternion( const PLVector3 *v );

PLQuaternion PlRotateQuaternionPoint( const PLQuaternion *q, const PLVector3 *point );
