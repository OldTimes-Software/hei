/**
 * Hei Platform Library
 * Copyright (C) 2017-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is licensed under MIT. See LICENSE for more details.
 */

#include <plgraphics/plg_driver_interface.h>

#include "plg_private.h"

#define CAMERA_DEFAULT_WIDTH 640
#define CAMERA_DEFAULT_HEIGHT 480
#define CAMERA_DEFAULT_BOUNDS 5
#define CAMERA_DEFAULT_FOV 75.f
#define CAMERA_DEFAULT_NEAR 0.1f
#define CAMERA_DEFAULT_FAR 1000.f

PLGCamera *PlgCreateCamera( void ) {
	PLGCamera *camera = ( PLGCamera * ) PlCAlloc( 1, sizeof( PLGCamera ), false );
	if ( camera == NULL ) {
		return NULL;
	}

	camera->fov = CAMERA_DEFAULT_FOV;
	camera->near = CAMERA_DEFAULT_NEAR;
	camera->far = CAMERA_DEFAULT_FAR;
	camera->mode = PLG_CAMERA_MODE_PERSPECTIVE;

	/*  XY * * * * W
     *  *
     *  *
     *  *
     *  *
     *  H
     */
	camera->viewport.w = CAMERA_DEFAULT_WIDTH;
	camera->viewport.h = CAMERA_DEFAULT_HEIGHT;

	camera->forward = PLVector3( 0, 0, 1 );
	camera->up = PLVector3( 0, 1, 0 );

	CallGfxFunction( CreateCamera, camera );

	camera->bounds.mins = PLVector3(
	        -CAMERA_DEFAULT_BOUNDS, -CAMERA_DEFAULT_BOUNDS, -CAMERA_DEFAULT_BOUNDS );
	camera->bounds.maxs = PLVector3(
	        CAMERA_DEFAULT_BOUNDS, CAMERA_DEFAULT_BOUNDS, CAMERA_DEFAULT_BOUNDS );

	return camera;
}

void PlgDestroyCamera( PLGCamera *camera ) {
	if ( camera == NULL ) {
		return;
	}

	CallGfxFunction( DestroyCamera, camera );

	PlFree( camera->viewport.buffer );
	PlFree( camera );
}

/**
 * Set the camera's field of view, carry out some basic validation.
 * (does not allow fov in excess of 1 or 179)
 */
void PlgSetCameraFieldOfView( PLGCamera *camera, float fieldOfView ) {
	if ( fieldOfView < 1.0f ) {
		fieldOfView = 1.0f;
	} else if ( fieldOfView > 179.0f ) {
		fieldOfView = 179.0f;
	}

	camera->fov = fieldOfView;
}

/**
 * Return the given camera's field of view.
 */
float PlgGetCameraFieldOfView( const PLGCamera *camera ) {
	return camera->fov;
}

static void MakeFrustumPlanes( const PLMatrix4 *matrix, PLGViewFrustum outFrustum ) {
	// Right
	outFrustum[ PLG_FRUSTUM_PLANE_RIGHT ].x = matrix->m[ 3 ] - matrix->m[ 0 ];
	outFrustum[ PLG_FRUSTUM_PLANE_RIGHT ].y = matrix->m[ 7 ] - matrix->m[ 4 ];
	outFrustum[ PLG_FRUSTUM_PLANE_RIGHT ].z = matrix->m[ 11 ] - matrix->m[ 8 ];
	outFrustum[ PLG_FRUSTUM_PLANE_RIGHT ].w = matrix->m[ 15 ] - matrix->m[ 12 ];
	outFrustum[ PLG_FRUSTUM_PLANE_RIGHT ] = PlNormalizePlane( outFrustum[ PLG_FRUSTUM_PLANE_RIGHT ] );
	// Left
	outFrustum[ PLG_FRUSTUM_PLANE_LEFT ].x = matrix->m[ 3 ] + matrix->m[ 0 ];
	outFrustum[ PLG_FRUSTUM_PLANE_LEFT ].y = matrix->m[ 7 ] + matrix->m[ 4 ];
	outFrustum[ PLG_FRUSTUM_PLANE_LEFT ].z = matrix->m[ 11 ] + matrix->m[ 8 ];
	outFrustum[ PLG_FRUSTUM_PLANE_LEFT ].w = matrix->m[ 15 ] + matrix->m[ 12 ];
	outFrustum[ PLG_FRUSTUM_PLANE_LEFT ] = PlNormalizePlane( outFrustum[ PLG_FRUSTUM_PLANE_LEFT ] );
	// Bottom
	outFrustum[ PLG_FRUSTUM_PLANE_BOTTOM ].x = matrix->m[ 3 ] - matrix->m[ 1 ];
	outFrustum[ PLG_FRUSTUM_PLANE_BOTTOM ].y = matrix->m[ 7 ] - matrix->m[ 5 ];
	outFrustum[ PLG_FRUSTUM_PLANE_BOTTOM ].z = matrix->m[ 11 ] - matrix->m[ 9 ];
	outFrustum[ PLG_FRUSTUM_PLANE_BOTTOM ].w = matrix->m[ 15 ] - matrix->m[ 13 ];
	outFrustum[ PLG_FRUSTUM_PLANE_BOTTOM ] = PlNormalizePlane( outFrustum[ PLG_FRUSTUM_PLANE_BOTTOM ] );
	// Top
	outFrustum[ PLG_FRUSTUM_PLANE_TOP ].x = matrix->m[ 3 ] + matrix->m[ 1 ];
	outFrustum[ PLG_FRUSTUM_PLANE_TOP ].y = matrix->m[ 7 ] + matrix->m[ 5 ];
	outFrustum[ PLG_FRUSTUM_PLANE_TOP ].z = matrix->m[ 11 ] + matrix->m[ 9 ];
	outFrustum[ PLG_FRUSTUM_PLANE_TOP ].w = matrix->m[ 15 ] + matrix->m[ 13 ];
	outFrustum[ PLG_FRUSTUM_PLANE_TOP ] = PlNormalizePlane( outFrustum[ PLG_FRUSTUM_PLANE_TOP ] );
	// Far
	outFrustum[ PLG_FRUSTUM_PLANE_FAR ].x = matrix->m[ 3 ] - matrix->m[ 2 ];
	outFrustum[ PLG_FRUSTUM_PLANE_FAR ].y = matrix->m[ 7 ] - matrix->m[ 6 ];
	outFrustum[ PLG_FRUSTUM_PLANE_FAR ].z = matrix->m[ 11 ] - matrix->m[ 10 ];
	outFrustum[ PLG_FRUSTUM_PLANE_FAR ].w = matrix->m[ 15 ] - matrix->m[ 14 ];
	outFrustum[ PLG_FRUSTUM_PLANE_FAR ] = PlNormalizePlane( outFrustum[ PLG_FRUSTUM_PLANE_FAR ] );
	// Near
	outFrustum[ PLG_FRUSTUM_PLANE_NEAR ].x = matrix->m[ 3 ] + matrix->m[ 2 ];
	outFrustum[ PLG_FRUSTUM_PLANE_NEAR ].y = matrix->m[ 7 ] + matrix->m[ 6 ];
	outFrustum[ PLG_FRUSTUM_PLANE_NEAR ].z = matrix->m[ 11 ] + matrix->m[ 10 ];
	outFrustum[ PLG_FRUSTUM_PLANE_NEAR ].w = matrix->m[ 15 ] + matrix->m[ 14 ];
	outFrustum[ PLG_FRUSTUM_PLANE_NEAR ] = PlNormalizePlane( outFrustum[ PLG_FRUSTUM_PLANE_NEAR ] );
}

static void SetupCameraFrustum( PLGCamera *camera ) {
	PLMatrix4 mvp = PlMatrix4Identity();
	mvp = PlMultiplyMatrix4( mvp, camera->internal.proj );
	mvp = PlMultiplyMatrix4( mvp, camera->internal.view );
	MakeFrustumPlanes( &mvp, camera->frustum );
}

static void SetupCameraPerspective( PLGCamera *camera ) {
	float w = ( float ) camera->viewport.w;
	float h = ( float ) camera->viewport.h;

	camera->internal.proj = PlPerspective( camera->fov, w / h, camera->near, camera->far );

	float x = cosf( PlDegreesToRadians( camera->angles.y ) ) * cosf( PlDegreesToRadians( camera->angles.x ) );
	float y = sinf( PlDegreesToRadians( camera->angles.x ) );
	float z = sinf( PlDegreesToRadians( camera->angles.y ) ) * cosf( PlDegreesToRadians( camera->angles.x ) );

	camera->forward = PlNormalizeVector3( PLVector3( x, y, z ) );
	camera->internal.view = PlLookAt( camera->position, PlAddVector3( camera->position, camera->forward ), camera->up );
}

void PlgSetupCamera( PLGCamera *camera ) {
	plAssert( camera );

	switch ( camera->mode ) {
		case PLG_CAMERA_MODE_PERSPECTIVE: {
			SetupCameraPerspective( camera );
			break;
		}

		case PLG_CAMERA_MODE_ORTHOGRAPHIC:
			camera->internal.proj = PlOrtho( 0, ( float ) camera->viewport.w, ( float ) camera->viewport.h, 0, camera->near, camera->far );
			camera->internal.view = PlMatrix4Identity();
			break;

		case PLG_CAMERA_MODE_ISOMETRIC:
			camera->internal.proj = PlOrtho( -camera->fov, camera->fov, -camera->fov, 5, -5, 40 );
			camera->internal.view = PlMatrix4Identity();
			break;

		default:
			break;
	}

	/* setup the camera frustum */
	SetupCameraFrustum( camera );

	// keep the gfx_state up-to-date on the situation
	gfx_state.current_viewport = &camera->viewport;

	// Copy camera matrices
	gfx_state.view_matrix = camera->internal.view;
	gfx_state.projection_matrix = camera->internal.proj;

	CallGfxFunction( SetupCamera, camera );
}

/**
 * Checks that the given bounding box is within the view space.
 */
bool PlgIsBoxInsideView( const PLGCamera *camera, const PLCollisionAABB *bounds ) {
	PLVector3 mins = PlAddVector3( bounds->mins, bounds->origin );
	PLVector3 maxs = PlAddVector3( bounds->maxs, bounds->origin );
	for ( unsigned int i = 0; i < 5; ++i ) {
		if ( PlGetPlaneDotProduct( &camera->frustum[ i ], &PLVector3( mins.x, mins.y, mins.z ) ) >= 0.0f ) {
			continue;
		}
		if ( PlGetPlaneDotProduct( &camera->frustum[ i ], &PLVector3( maxs.x, mins.y, mins.z ) ) >= 0.0f ) {
			continue;
		}
		if ( PlGetPlaneDotProduct( &camera->frustum[ i ], &PLVector3( mins.x, maxs.y, mins.z ) ) >= 0.0f ) {
			continue;
		}
		if ( PlGetPlaneDotProduct( &camera->frustum[ i ], &PLVector3( maxs.x, maxs.y, mins.z ) ) >= 0.0f ) {
			continue;
		}
		if ( PlGetPlaneDotProduct( &camera->frustum[ i ], &PLVector3( mins.x, mins.y, maxs.z ) ) >= 0.0f ) {
			continue;
		}
		if ( PlGetPlaneDotProduct( &camera->frustum[ i ], &PLVector3( mins.x, maxs.y, maxs.z ) ) >= 0.0f ) {
			continue;
		}
		if ( PlGetPlaneDotProduct( &camera->frustum[ i ], &PLVector3( maxs.x, maxs.y, maxs.z ) ) >= 0.0f ) {
			continue;
		}

		return false;
	}

	return true;
}

/**
 * Checks that the given sphere is within the view space.
 */
bool PlgIsSphereInsideView( const PLGCamera *camera, const PLCollisionSphere *sphere ) {
	for ( unsigned int i = 0; i < 5; ++i ) {
		if ( PlGetPlaneDotProduct( &camera->frustum[ i ], &sphere->origin ) < -sphere->radius ) {
			return false;
		}
	}

	return true;
}

const PLGViewport *PlgGetCurrentViewport( void ) {
	return gfx_state.current_viewport;
}

void PlgLookAtTargetVector( PLGCamera *camera, PLVector3 target ) {
	camera->internal.view = PlLookAt( camera->position, target, PLVector3( 0, 1, 0 ) );
}
