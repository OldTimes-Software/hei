/*
MIT License

Copyright (c) 2017-2021 Mark E Sowden <hogsy@oldtimes-software.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once

#include <plcore/pl_physics.h>
#include <plcore/pl_image.h>

enum {
	PLG_CAMERA_MODE_PERSPECTIVE,
	PLG_CAMERA_MODE_ORTHOGRAPHIC,
	PLG_CAMERA_MODE_ISOMETRIC
};

typedef struct PLGViewport {
	bool auto_scale;//If true, viewport will update to match
	                // height/width of currently bound FBO when
	                // SetupCamera() is called
	int x, y;
	int w, h;
	int oldW, oldH;
	uint8_t *buffer;
	unsigned int bufferSize;
} PLGViewport;

enum {
	PLG_FRUSTUM_PLANE_RIGHT,
	PLG_FRUSTUM_PLANE_LEFT,
	PLG_FRUSTUM_PLANE_BOTTOM,
	PLG_FRUSTUM_PLANE_TOP,
	PLG_FRUSTUM_PLANE_FAR,
	PLG_FRUSTUM_PLANE_NEAR,

	PLG_MAX_FRUSTUM_PLANES
};

typedef PLVector4 PLGViewFrustum[ PLG_MAX_FRUSTUM_PLANES ];

typedef struct PLGCamera {
	float fov;
	float near, far;
	unsigned int mode;

	PLVector3 angles, position;
	PLVector3 forward, up;

	PLGViewFrustum frustum;

	PLCollisionAABB bounds;

	// Viewport
	PLGViewport viewport;

	struct {
		PLMatrix4 proj;
		PLMatrix4 view;
	} internal;
} PLGCamera;

PL_EXTERN_C

PLGCamera *PlgCreateCamera( void );
void PlgDestroyCamera( PLGCamera *camera );

void PlgSetCameraFieldOfView( PLGCamera *camera, float fieldOfView );
float PlgGetCameraFieldOfView( const PLGCamera *camera );

void PlgSetupCamera( PLGCamera *camera );

bool PlgIsBoxInsideView( const PLGCamera *camera, const PLCollisionAABB *bounds );
bool PlgIsSphereInsideView( const PLGCamera *camera, const PLCollisionSphere *sphere );

const PLGViewport *PlgGetCurrentViewport( void );

/***** TEMPORARY CRAP START *****/

void PlgSetViewMatrix( const PLMatrix4 *viewMatrix );
PLMatrix4 PlgGetViewMatrix( void );
void PlgSetProjectionMatrix( const PLMatrix4 *projMatrix );
PLMatrix4 PlgGetProjectionMatrix( void );

/***** TEMPORARY CRAP END 	*****/

PL_EXTERN_C_END
