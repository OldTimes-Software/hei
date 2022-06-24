/**
 * Hei Platform Library
 * Copyright (C) 2017-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is licensed under MIT. See LICENSE for more details.
 */

#include <plcore/pl_math.h>

PLQuaternion PlMultiplyQuaternion( const PLQuaternion *q, const PLQuaternion *q2 ) {
	return PlQuaternion(
	        ( q->x * q2->w ) + ( q->w * q2->x ) + ( q->y * q2->z ) - ( q->z * q2->y ),
	        ( q->y * q2->w ) + ( q->w * q2->y ) + ( q->z * q2->x ) - ( q->x * q2->z ),
	        ( q->z * q2->w ) + ( q->w * q2->z ) + ( q->x * q2->y ) - ( q->y * q2->x ),
	        ( q->w * q2->w ) - ( q->x * q2->x ) - ( q->y * q2->y ) - ( q->z * q2->z ) );
}

PLQuaternion PlMultiplyQuaternion3FV( const PLQuaternion *q, const PLVector3 *v ) {
	return PlQuaternion(
	        ( q->w * v->x ) + ( q->y * v->z ) - ( q->z * v->y ),
	        ( q->w * v->y ) + ( q->z * v->x ) - ( q->x * v->z ),
	        ( q->w * v->z ) + ( q->x * v->y ) - ( q->y * v->x ),
	        -( q->x * v->x ) - ( q->y * v->y ) - ( q->z * v->z ) );
}

PLQuaternion PlScaleQuaternion( const PLQuaternion *q, float a ) {
	return PlQuaternion( q->x * a, q->y * a, q->z * a, q->w * a );
}

PLQuaternion PlAddQuaternion( const PLQuaternion *q, const PLQuaternion *q2 ) {
	return PlQuaternion( q->x + q2->x, q->y + q2->y, q->z + q2->z, q->w + q2->w );
}

PLQuaternion PlAddQuaternionF( const PLQuaternion *q, float a ) {
	return PlQuaternion( q->x + a, q->y + a, q->z + a, q->z + a );
}

PLQuaternion PlInverseQuaternion( const PLQuaternion *q ) {
	return PlQuaternion( -q->x, -q->y, -q->z, q->w );
}

float PlQuaternionDotProduct( const PLQuaternion *q, const PLQuaternion *q2 ) {
	/* todo: revisit... */
	return acosf( q->x * q2->x + q->y * q2->y + q->z * q2->z + q->w * q2->w );
}

float PlQuaternionLength( const PLQuaternion *q ) {
	return sqrtf( q->x * q->x + q->y * q->y + q->z * q->z + q->w * q->w );
}

PLQuaternion PlNormalizeQuaternion( const PLQuaternion *q ) {
	float l = PlQuaternionLength( q );
	if ( l > 0 ) {
		float i = 1 / l;
		return PlScaleQuaternion( q, i );
	}

	return *q;
}

const char *PlPrintQuaternion( const PLQuaternion *q ) {
	static char s[ 32 ] = { '\0' };
	snprintf( s, 32, "%i %i %i %i", ( int ) q->x, ( int ) q->y, ( int ) q->z, ( int ) q->w );
	return s;
}

void PlComputeQuaternionW( PLQuaternion *q ) {
	float t = 1.f - ( q->x * q->x ) - ( q->y * q->y ) - ( q->z * q->z );
	if ( t < 0 ) {
		q->w = 0;
	} else {
		q->w = -sqrtf( t );
	}
}

PLVector3 PlQuaternionToEuler( const PLQuaternion *q ) {
	PLVector3 v;

	/* pitch */
	float sinp = 2.0f * ( q->w * q->y - q->z * q->x );
	if ( fabsf( sinp ) >= 1.0f )
		v.x = copysignf( PL_PI / 2.0f, sinp );
	else
		v.x = asinf( sinp );

	/* yaw */
	float sinyCosp = 2.0f * ( q->w * q->z + q->x * q->y );
	float cosyCosp = 1.0f - 2.0f * ( q->y * q->y + q->z * q->z );
	v.y = atan2f( sinyCosp, cosyCosp );

	/* roll */
	float sinrCosp = 2.0f * ( q->w * q->x + q->y * q->z );
	float cosrCosp = 1.0f - 2.0f * ( q->x * q->x + q->y * q->y );
	v.z = atan2f( sinrCosp, cosrCosp );

	return v;
}

PLQuaternion PlEulerToQuaternion( const PLVector3 *v ) {
	/* pitch */
	float cp = cosf( v->x * 0.5f );
	float sp = sinf( v->x * 0.5f );
	/* yaw */
	float cy = cosf( v->y * 0.5f );
	float sy = sinf( v->y * 0.5f );
	/* roll */
	float cr = cosf( v->z * 0.5f );
	float sr = sinf( v->z * 0.5f );

	PLQuaternion q;
	q.x = sr * cp * cy - cr * sp * sy;
	q.y = cr * sp * cy + sr * cp * sy;
	q.z = cr * cp * sy - sr * sp * cy;
	q.w = cr * cp * cy + sr * sp * sy;

	return q;
}

/* pulled from here: http://tfc.duke.free.fr/coding/md5-specs-en.html */
PLQuaternion PlRotateQuaternionPoint( const PLQuaternion *q, const PLVector3 *point ) {
	PLQuaternion b = PlInverseQuaternion( q );
	b = PlNormalizeQuaternion( &b );

	PLQuaternion a = PlMultiplyQuaternion3FV( q, point );
	return PlMultiplyQuaternion( &a, &b );
}
