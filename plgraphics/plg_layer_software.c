/**
 * Hei Platform Library
 * Copyright (C) 2017-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is licensed under MIT. See LICENSE for more details.
 */

#include <plgraphics/plg_driver_interface.h>

#include "plg_private.h"

/* todo:
 * - buffer upscaling and downscaling
 */

#define SWGetDisplayBufferSize( a ) PlGetImageSize( PL_IMAGEFORMAT_RGBA8, ( a )->w, ( a )->h )
#define SWGetCurrentDisplayBuffer() gfx_state.current_viewport->buffer

static void SWSetClearColour( PLColour colour ) {}

static void SWClearBuffers( unsigned int buffers ) {
	if ( gfx_state.current_viewport->buffer == NULL ) {
		return;
	}

	unsigned int size = SWGetDisplayBufferSize( gfx_state.current_viewport );
	if ( buffers & PLG_BUFFER_COLOUR ) {
		for ( unsigned int i = 0; i < size; i += 4 ) {
			SWGetCurrentDisplayBuffer()[ i ] = gfx_state.current_clearcolour.r;
			SWGetCurrentDisplayBuffer()[ i + 1 ] = gfx_state.current_clearcolour.g;
			SWGetCurrentDisplayBuffer()[ i + 2 ] = gfx_state.current_clearcolour.b;
			SWGetCurrentDisplayBuffer()[ i + 3 ] = gfx_state.current_clearcolour.a;
		}
	}
}

/**********************************************************/
/** camera **/

static uint8_t *SWCreateDisplayBuffer( PLGViewport *viewport ) {
	PlFree( viewport->buffer );
	viewport->buffer = ( uint8_t * ) PlMAllocA( SWGetDisplayBufferSize( viewport ) );
	viewport->oldW = viewport->w;
	viewport->oldH = viewport->h;
	return viewport->buffer;
}

static void SWCreateCamera( PLGCamera *camera ) {
	plAssert( camera != NULL );
	if ( camera == NULL ) {
		return;
	}

	PLGViewport *viewport = &camera->viewport;
	if ( viewport->buffer != NULL ) {
		/* only update the display buffer if the target size has changed */
		if ( viewport->oldH != viewport->h && viewport->oldW != viewport->w ) {
			SWCreateDisplayBuffer( viewport );
		}
		return;
	}

	SWCreateDisplayBuffer( viewport );
}

static void SWDestroyCamera( PLGCamera *camera ) {
	plAssert( camera );
} /* camera API takes care of deletion, so nothing to do here */

static void SWSetupCamera( PLGCamera *camera ) {
	plAssert( camera );

	/* only update the display buffer if the target size has changed */
	if ( camera->viewport.oldH != camera->viewport.h &&
	     camera->viewport.oldW != camera->viewport.w ) {
		SWCreateDisplayBuffer( &camera->viewport );
	}
}

/**********************************************************/

static void SWDrawPixel( int x, int y, PLColour colour ) {
	PLGViewport *viewport = gfx_state.current_viewport;
	if ( viewport->buffer == NULL ) {
		return;
	}

	unsigned int pos = y * viewport->w + x;
	if ( pos >= SWGetDisplayBufferSize( viewport ) ) {
		return;
	}

	PLColour *buffer = ( PLColour * ) viewport->buffer;
	buffer[ pos ] = colour;
}

static void SWDrawLine( const PLGVertex *start, const PLGVertex *end ) {
	PLGViewport *viewport = gfx_state.current_viewport;
	if ( viewport->buffer == NULL ) {
		return;
	}

	//PLColour *buffer = ( PLColour * ) viewport->buffer;
}

static void SWDrawMesh( PLGMesh *mesh ) {
	PLVector3 transform = PlGetMatrix4Translation( PlGetMatrix( PL_MODELVIEW_MATRIX ) );
	switch( mesh->primitive ) {
		case PLG_MESH_LINES: {
			for ( unsigned int i = 0; i < mesh->num_verts; i += 2 ) {
				PLGVertex a = mesh->vertices[ i ];
				a.position = PlScaleVector3( a.position, transform );
				PLGVertex b = mesh->vertices[ i ];
				b.position = PlScaleVector3( b.position, transform );
				SWDrawLine( &a, &b );
			}
			break;
		}
		default: /* for now... */
		case PLG_MESH_POINTS: {
			for ( unsigned int i = 0; i < mesh->num_verts; ++i ) {
				PLGVertex a = mesh->vertices[ i ];
				a.position = PlScaleVector3( a.position, transform );
				SWDrawPixel( ( int ) a.position.x, ( int ) a.position.y, a.colour );
			}
			break;
		}
	}
}

static bool SWSupportsHWShaders( void ) {
	return false;
}
