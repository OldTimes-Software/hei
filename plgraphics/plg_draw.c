/**
 * Hei Platform Library
 * Copyright (C) 2017-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is licensed under MIT. See LICENSE for more details.
 */

#include <plgraphics/plg_driver_interface.h>

#include "plg_private.h"

#define MAXIMUM_STORAGE 4096

static PLGMesh *meshes[ PLG_NUM_PRIMITIVES ];
static PLGMesh *GetInternalMesh( PLGMeshPrimitive primitive ) {
	if ( meshes[ primitive ] == NULL ) {
		return ( meshes[ primitive ] = PlgCreateMesh( primitive, PLG_DRAW_DYNAMIC, MAXIMUM_STORAGE, MAXIMUM_STORAGE ) );
	}

	PlgClearMesh( meshes[ primitive ] );
	return meshes[ primitive ];
}

void PlgInitializeInternalMeshes( void ) {
	memset( meshes, 0, sizeof( meshes ) );
}

void PlgClearInternalMeshes( void ) {
	for ( unsigned int i = 0; i < PLG_NUM_PRIMITIVES; ++i ) {
		if ( meshes[ i ] == NULL ) {
			continue;
		}

		PlgDestroyMesh( meshes[ i ] );
		meshes[ i ] = NULL;
	}
}

void PlgDrawEllipse( unsigned int segments, PLVector2 position, float w, float h, PLColour colour ) {
	PLGMesh *mesh = GetInternalMesh( PLG_MESH_TRIANGLE_FAN );

	for ( unsigned int i = 0, pos = 0; i < 360; i += ( 360 / segments ) ) {
		if ( pos >= segments ) {
			break;
		}

		PLVector3 coord = PLVector3(
		        ( position.x + w ) + cosf( PlDegreesToRadians( ( float ) i ) ) * w,
		        ( position.y + h ) + sinf( PlDegreesToRadians( ( float ) i ) ) * h,
		        0.0f );

		PlgAddMeshVertex( mesh, coord, pl_vecOrigin3, colour, pl_vecOrigin2 );
	}

	PlMatrixMode( PL_MODELVIEW_MATRIX );
	PlPushMatrix();

	PlLoadIdentityMatrix();

	PlgSetShaderUniformValue( PlgGetCurrentShaderProgram(), "pl_model", PlGetMatrix( PL_MODELVIEW_MATRIX ), false );

	PlgUploadMesh( mesh );
	PlgDrawMesh( mesh );

	PlPopMatrix();
}

static void SetupRectangleMesh( PLGMesh *mesh, float x, float y, float w, float h, PLColour colour ) {
	PlgAddMeshVertex( mesh, PLVector3( x, y, 0.0f ), pl_vecOrigin3, colour, PLVector2( 0.0f, 0.0f ) );
	PlgAddMeshVertex( mesh, PLVector3( x, y + h, 0.0f ), pl_vecOrigin3, colour, PLVector2( 0.0f, 1.0f ) );
	PlgAddMeshVertex( mesh, PLVector3( x + w, y, 0.0f ), pl_vecOrigin3, colour, PLVector2( 1.0f, 0.0f ) );
	PlgAddMeshVertex( mesh, PLVector3( x + w, y + h, 0.0f ), pl_vecOrigin3, colour, PLVector2( 1.0f, 1.0f ) );
}

void PlgDrawTexturedRectangle( const PLMatrix4 *transform, float x, float y, float w, float h, PLGTexture *texture ) {
	PlgSetTexture( texture, 0 );

	PlgDrawRectangle( transform, x, y, w, h, PLColour( 255, 255, 255, 255 ) );

	PlgSetTexture( NULL, 0 );
}

PLGMesh *PlgCreateMeshRectangle( float x, float y, float w, float h, PLColour colour ) {
	PLGMesh *mesh = PlgCreateMesh( PLG_MESH_TRIANGLE_STRIP, PLG_DRAW_DYNAMIC, 0, 4 );
	if ( mesh == NULL ) {
		return NULL;
	}

	SetupRectangleMesh( mesh, x, y, w, h, colour );

	return mesh;
}

void PlgDrawRectangle( const PLMatrix4 *transform, float x, float y, float w, float h, PLColour colour ) {
	PLGMesh *mesh = GetInternalMesh( PLG_MESH_TRIANGLE_STRIP );
	if ( mesh == NULL ) {
		return;
	}

	SetupRectangleMesh( mesh, x, y, w, h, colour );

	if ( transform == NULL ) {
		transform = PlGetMatrix( PL_MODELVIEW_MATRIX );
	}

	PlgSetShaderUniformValue( PlgGetCurrentShaderProgram(), "pl_model", transform, true );

	PlgUploadMesh( mesh );
	PlgDrawMesh( mesh );
}

void PlgDrawFilledRectangle( const PLRectangle2D *rectangle ) {
	PLGMesh *mesh = GetInternalMesh( PLG_MESH_TRIANGLE_STRIP );

	SetupRectangleMesh( mesh, rectangle->xy.x, rectangle->xy.y, rectangle->wh.x, rectangle->wh.y, PLColour( 255, 255, 255, 255 ) );

	PlgSetMeshVertexColour( mesh, 0, rectangle->ul );
	PlgSetMeshVertexColour( mesh, 1, rectangle->ll );
	PlgSetMeshVertexColour( mesh, 2, rectangle->ur );
	PlgSetMeshVertexColour( mesh, 3, rectangle->lr );

	PlMatrixMode( PL_MODELVIEW_MATRIX );
	PlPushMatrix();

	PlLoadIdentityMatrix();

	PlgSetShaderUniformValue( PlgGetCurrentShaderProgram(), "pl_model", PlGetMatrix( PL_MODELVIEW_MATRIX ), false );

	PlgUploadMesh( mesh );
	PlgDrawMesh( mesh );

	PlPopMatrix();
}

void PlgDrawTexturedQuad( const PLVector3 *ul, const PLVector3 *ur, const PLVector3 *ll, const PLVector3 *lr, float hScale, float vScale, PLGTexture *texture ) {
	PLGMesh *mesh = GetInternalMesh( PLG_MESH_TRIANGLES );

	PLVector3 upperDist = PlSubtractVector3( *ul, *ur );
	float quadWidth = PlVector3Length( upperDist ) / hScale;
	PLVector3 lowerDist = PlSubtractVector3( *ll, *ul );
	float quadHeight = PlVector3Length( lowerDist ) / vScale;

	PlgAddMeshVertex( mesh, *ul, pl_vecOrigin3, PL_COLOUR_WHITE, PLVector2( 0.0f, quadHeight / texture->h ) );
	PlgAddMeshVertex( mesh, *ur, pl_vecOrigin3, PL_COLOUR_WHITE, PLVector2( quadWidth / texture->w, quadHeight / texture->h ) );
	PlgAddMeshVertex( mesh, *ll, pl_vecOrigin3, PL_COLOUR_WHITE, PLVector2( 0.0f, 0.0f ) );
	PlgAddMeshVertex( mesh, *lr, pl_vecOrigin3, PL_COLOUR_WHITE, PLVector2( quadWidth / texture->w, 0.0f ) );

	PlgAddMeshTriangle( mesh, 0, 1, 2 );
	PlgAddMeshTriangle( mesh, 2, 1, 3 );

	PlgGenerateMeshNormals( mesh, true );

	PlgSetTexture( texture, 0 );

	PlMatrixMode( PL_MODELVIEW_MATRIX );
	PlPushMatrix();

	PlLoadIdentityMatrix();

	PlgSetShaderUniformValue( PlgGetCurrentShaderProgram(), "pl_model", PlGetMatrix( PL_MODELVIEW_MATRIX ), false );

	PlgUploadMesh( mesh );
	PlgDrawMesh( mesh );

	PlPopMatrix();
}

void PlgDrawTriangle( int x, int y, unsigned int w, unsigned int h ) {
	PLGMesh *mesh = GetInternalMesh( PLG_MESH_TRIANGLE_FAN );

	PlgAddMeshVertex( mesh, PLVector3( x, y + h, 0.0f ), pl_vecOrigin3, PLColour( 255, 0, 0, 255 ), pl_vecOrigin2 );
	PlgAddMeshVertex( mesh, PLVector3( x + w / 2, x, 0.0f ), pl_vecOrigin3, PLColour( 0, 255, 0, 255 ), pl_vecOrigin2 );
	PlgAddMeshVertex( mesh, PLVector3( x + w, y + h, 0.0f ), pl_vecOrigin3, PLColour( 0, 0, 255, 255 ), pl_vecOrigin2 );

	//plSetMeshUniformColour(mesh, PLColour(255, 0, 0, 255));

	PlMatrixMode( PL_MODELVIEW_MATRIX );
	PlPushMatrix();

	PlLoadIdentityMatrix();

	PlgSetShaderUniformValue( PlgGetCurrentShaderProgram(), "pl_model", PlGetMatrix( PL_MODELVIEW_MATRIX ), false );

	PlgUploadMesh( mesh );
	PlgDrawMesh( mesh );

	PlPopMatrix();
}

void PlgDrawLines( const PLVector3 *points, unsigned int numPoints, PLColour colour ) {
	PLGMesh *mesh = GetInternalMesh( PLG_MESH_LINES );

	for ( unsigned int i = 0; i < numPoints; ++i ) {
		PlgAddMeshVertex( mesh, points[ i ], pl_vecOrigin3, colour, pl_vecOrigin2 );
	}

	PlgSetShaderUniformValue( PlgGetCurrentShaderProgram(), "pl_model", PlGetMatrix( PL_MODELVIEW_MATRIX ), true );

	PlgUploadMesh( mesh );
	PlgDrawMesh( mesh );
}

void PlgDrawLine( PLMatrix4 transform, PLVector3 startPos, PLColour startColour, PLVector3 endPos, PLColour endColour ) {
	PLGMesh *mesh = GetInternalMesh( PLG_MESH_LINES );

	PlgAddMeshVertex( mesh, startPos, pl_vecOrigin3, startColour, pl_vecOrigin2 );
	PlgAddMeshVertex( mesh, endPos, pl_vecOrigin3, endColour, pl_vecOrigin2 );

	PlgSetShaderUniformValue( PlgGetCurrentShaderProgram(), "pl_model", &transform, true );

	PlgUploadMesh( mesh );
	PlgDrawMesh( mesh );
}

void PlgDrawSimpleLine( PLMatrix4 transform, PLVector3 startPos, PLVector3 endPos, PLColour colour ) {
	PlgDrawLine( transform, startPos, colour, endPos, colour );
}

void PlgDrawGrid( int x, int y, int w, int h, unsigned int gridSize ) {
	PLGMesh *mesh = GetInternalMesh( PLG_MESH_LINES );

	int c = 0, r = 0;
	for ( ; r < h + 1; r += ( int ) gridSize ) {
		PlgAddMeshVertex( mesh, PLVector3( x, y + r, 0.0f ), pl_vecOrigin3, PLColour( 0, 0, 255, 255 ), pl_vecOrigin2 );
		PlgAddMeshVertex( mesh, PLVector3( x + w, r + y, 0 ), pl_vecOrigin3, PLColour( 0, 0, 255, 255 ), pl_vecOrigin2 );
		for ( ; c < w + 1; c += ( int ) gridSize ) {
			PlgAddMeshVertex( mesh, PLVector3( c + x, y, 0 ), pl_vecOrigin3, PLColour( 0, 0, 255, 255 ), pl_vecOrigin2 );
			PlgAddMeshVertex( mesh, PLVector3( c + x, y + h, 0 ), pl_vecOrigin3, PLColour( 0, 0, 255, 255 ), pl_vecOrigin2 );
		}
	}

	PlgSetShaderUniformValue( PlgGetCurrentShaderProgram(), "pl_model", PlGetMatrix( PL_MODELVIEW_MATRIX ), true );

	PlgUploadMesh( mesh );
	PlgDrawMesh( mesh );
}

/**
 * Draw lines at each vertex point representing the direction of the normal.
 */
void PlgDrawMeshNormals( const PLGMesh *mesh ) {
	PLGMesh *linesMesh = GetInternalMesh( PLG_MESH_LINES );

	for ( unsigned int i = 0; i < mesh->num_verts; ++i ) {
		PLVector3 linePos = mesh->vertices[ i ].position;
		PLVector3 lineEndPos = PlAddVector3( linePos, PlScaleVector3F( mesh->vertices[ i ].normal, 64.0f ) );
		PlgAddMeshVertex( linesMesh, linePos, pl_vecOrigin3, PLColour( 255, 0, 0, 255 ), pl_vecOrigin2 );
		PlgAddMeshVertex( linesMesh, lineEndPos, pl_vecOrigin3, PLColour( 0, 255, 0, 255 ), pl_vecOrigin2 );
	}

	PlgUploadMesh( linesMesh );
	PlgDrawMesh( linesMesh );
}

void PlgDrawPixel( int x, int y, PLColour colour ) {
	const PLGViewport *viewport = PlgGetCurrentViewport();
	if ( viewport == NULL ) {
		return;
	}

	/* make sure that the pixel is within the viewport */
	if ( x > viewport->w || x < 0 || y > viewport->h || y < 0 ) {
		return;
	}

	CallGfxFunction( DrawPixel, x, y, colour );
}

/**
 * Utility function for drawing a bounding volume.
 */
void PlgDrawBoundingVolume( const PLCollisionAABB *bounds, PLColour colour ) {
	PLGMesh *mesh = GetInternalMesh( PLG_MESH_LINES );

	PlMatrixMode( PL_MODELVIEW_MATRIX );
	PlPushMatrix();

	PlLoadIdentityMatrix();
	PlTranslateMatrix( bounds->origin );

	PLVector3 boxPoints[ 8 ] = {
	        { bounds->maxs.x, bounds->maxs.y, bounds->maxs.z },
	        { bounds->maxs.x, bounds->mins.y, bounds->maxs.z },
	        { bounds->maxs.x, bounds->maxs.y, bounds->mins.z },
	        { bounds->mins.x, bounds->maxs.y, bounds->maxs.z },

	        { bounds->mins.x, bounds->mins.y, bounds->mins.z },
	        { bounds->mins.x, bounds->maxs.y, bounds->mins.z },
	        { bounds->mins.x, bounds->mins.y, bounds->maxs.z },
	        { bounds->maxs.x, bounds->mins.y, bounds->mins.z },
	};

	PlgAddMeshVertex( mesh, boxPoints[ 0 ], pl_vecOrigin3, colour, pl_vecOrigin2 );
	PlgAddMeshVertex( mesh, boxPoints[ 1 ], pl_vecOrigin3, colour, pl_vecOrigin2 );
	PlgAddMeshVertex( mesh, boxPoints[ 0 ], pl_vecOrigin3, colour, pl_vecOrigin2 );
	PlgAddMeshVertex( mesh, boxPoints[ 2 ], pl_vecOrigin3, colour, pl_vecOrigin2 );
	PlgAddMeshVertex( mesh, boxPoints[ 0 ], pl_vecOrigin3, colour, pl_vecOrigin2 );
	PlgAddMeshVertex( mesh, boxPoints[ 3 ], pl_vecOrigin3, colour, pl_vecOrigin2 );

	PlgAddMeshVertex( mesh, boxPoints[ 4 ], pl_vecOrigin3, colour, pl_vecOrigin2 );
	PlgAddMeshVertex( mesh, boxPoints[ 5 ], pl_vecOrigin3, colour, pl_vecOrigin2 );
	PlgAddMeshVertex( mesh, boxPoints[ 4 ], pl_vecOrigin3, colour, pl_vecOrigin2 );
	PlgAddMeshVertex( mesh, boxPoints[ 6 ], pl_vecOrigin3, colour, pl_vecOrigin2 );
	PlgAddMeshVertex( mesh, boxPoints[ 4 ], pl_vecOrigin3, colour, pl_vecOrigin2 );
	PlgAddMeshVertex( mesh, boxPoints[ 7 ], pl_vecOrigin3, colour, pl_vecOrigin2 );

	PlgAddMeshVertex( mesh, boxPoints[ 2 ], pl_vecOrigin3, colour, pl_vecOrigin2 );
	PlgAddMeshVertex( mesh, boxPoints[ 5 ], pl_vecOrigin3, colour, pl_vecOrigin2 );
	PlgAddMeshVertex( mesh, boxPoints[ 2 ], pl_vecOrigin3, colour, pl_vecOrigin2 );
	PlgAddMeshVertex( mesh, boxPoints[ 7 ], pl_vecOrigin3, colour, pl_vecOrigin2 );

	PlgAddMeshVertex( mesh, boxPoints[ 1 ], pl_vecOrigin3, colour, pl_vecOrigin2 );
	PlgAddMeshVertex( mesh, boxPoints[ 6 ], pl_vecOrigin3, colour, pl_vecOrigin2 );
	PlgAddMeshVertex( mesh, boxPoints[ 1 ], pl_vecOrigin3, colour, pl_vecOrigin2 );
	PlgAddMeshVertex( mesh, boxPoints[ 7 ], pl_vecOrigin3, colour, pl_vecOrigin2 );

	PlgAddMeshVertex( mesh, boxPoints[ 3 ], pl_vecOrigin3, colour, pl_vecOrigin2 );
	PlgAddMeshVertex( mesh, boxPoints[ 5 ], pl_vecOrigin3, colour, pl_vecOrigin2 );
	PlgAddMeshVertex( mesh, boxPoints[ 3 ], pl_vecOrigin3, colour, pl_vecOrigin2 );
	PlgAddMeshVertex( mesh, boxPoints[ 6 ], pl_vecOrigin3, colour, pl_vecOrigin2 );

	PlgSetShaderUniformValue( PlgGetCurrentShaderProgram(), "pl_model", PlGetMatrix( PL_MODELVIEW_MATRIX ), true );

	PlgUploadMesh( mesh );
	PlgDrawMesh( mesh );

	PlPopMatrix();
}
