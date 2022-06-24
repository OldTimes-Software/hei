/**
 * Hei Platform Library
 * Copyright (C) 2017-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is licensed under MIT. See LICENSE for more details.
 */

#include <plgraphics/plg_driver_interface.h>

#include "plg_private.h"


/****************************************
 * Immediate-mode style API
 * Possibly consider moving these into
 * some higher-level library?
 ****************************************/

static PLGMesh *currentDynamicMesh;
static unsigned int currentVertex;

static unsigned int currentTriangle;

#define MAXIMUM_STORAGE 4096

static PLGMesh *meshes[ PLG_NUM_PRIMITIVES ];
static PLGMesh *GetInternalMesh( PLGMeshPrimitive primitive ) {
	if ( meshes[ primitive ] == NULL ) {
		return ( meshes[ primitive ] = PlgCreateMesh( primitive, PLG_DRAW_DYNAMIC, MAXIMUM_STORAGE, MAXIMUM_STORAGE ) );
	}

	PlgClearMesh( meshes[ primitive ] );
	return meshes[ primitive ];
}

void PlgImmBegin( PLGMeshPrimitive primitive ) {
	currentDynamicMesh = GetInternalMesh( primitive );
	PlgClearMesh( currentDynamicMesh );
	currentVertex = 0;
}

unsigned int PlgImmPushVertex( float x, float y, float z ) {
	return ( currentVertex = PlgAddMeshVertex( currentDynamicMesh, PlVector3( x, y, z ), pl_vecOrigin3, PL_COLOUR_WHITE, pl_vecOrigin2 ) );
}

void PlgImmNormal( float x, float y, float z ) {
	PlgSetMeshVertexNormal( currentDynamicMesh, currentVertex, PlVector3( x, y, z ) );
}

void PlgImmColour( uint8_t r, uint8_t g, uint8_t b, uint8_t a ) {
	PlgSetMeshVertexColour( currentDynamicMesh, currentVertex, PlColourU8( r, g, b, a ) );
}

void PlgImmTextureCoord( float s, float t ) {
	PlgSetMeshVertexST( currentDynamicMesh, currentVertex, s, t );
}

unsigned int PlgImmPushTriangle( unsigned int x, unsigned int y, unsigned int z ) {
	return ( currentTriangle = PlgAddMeshTriangle( currentDynamicMesh, x, y, z ) );
}

void PlgImmDraw( void ) {
	PlgSetShaderUniformValue( PlgGetCurrentShaderProgram(), "pl_model", PlGetMatrix( PL_MODELVIEW_MATRIX ), true );

	PlgUploadMesh( currentDynamicMesh );
	PlgDrawMesh( currentDynamicMesh );
}

/****************************************
 ****************************************/

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

static void SetupRectangleMesh( float x, float y, float w, float h, PLColour colour ) {
	PlgImmPushVertex( x, y, 0.0f );
	PlgImmTextureCoord( 0.0f, 0.0f );
	PlgImmColour( colour.r, colour.g, colour.b, colour.a );

	PlgImmPushVertex( x, y + h, 0.0f );
	PlgImmTextureCoord( 0.0f, 1.0f );
	PlgImmColour( colour.r, colour.g, colour.b, colour.a );

	PlgImmPushVertex( x + w, y, 0.0f );
	PlgImmTextureCoord( 1.0f, 0.0f );
	PlgImmColour( colour.r, colour.g, colour.b, colour.a );

	PlgImmPushVertex( x + w, y + h, 0.0f );
	PlgImmTextureCoord( 1.0f, 1.0f );
	PlgImmColour( colour.r, colour.g, colour.b, colour.a );
}

void PlgDrawTexturedRectangle( float x, float y, float w, float h, PLGTexture *texture ) {
	PlgSetTexture( texture, 0 );

	PlgDrawRectangle( x, y, w, h, PLColour( 255, 255, 255, 255 ) );

	PlgSetTexture( NULL, 0 );
}

void PlgDrawRectangle( float x, float y, float w, float h, PLColour colour ) {
	PlgImmBegin( PLG_MESH_TRIANGLE_STRIP );

	SetupRectangleMesh( x, y, w, h, colour );

	PlgImmDraw();
}

void PlgDrawFilledRectangle( const PLRectangle2D *rectangle ) {
	PlgImmBegin( PLG_MESH_TRIANGLE_STRIP );

	SetupRectangleMesh( rectangle->xy.x, rectangle->xy.y, rectangle->wh.x, rectangle->wh.y, PLColour( 255, 255, 255, 255 ) );

	int x = rectangle->xy.x;
	int y = rectangle->xy.y;
	int w = rectangle->wh.x;
	int h = rectangle->wh.y;

	PlgImmPushVertex( x, y, 0.0f );
	PlgImmColour( rectangle->ul.r, rectangle->ul.g, rectangle->ul.b, rectangle->ul.a );

	PlgImmPushVertex( x, y + h, 0.0f );
	PlgImmColour( rectangle->ll.r, rectangle->ll.g, rectangle->ll.b, rectangle->ll.a );

	PlgImmPushVertex( x + w, y, 0.0f );
	PlgImmColour( rectangle->ur.r, rectangle->ur.g, rectangle->ur.b, rectangle->ur.a );

	PlgImmPushVertex( x + w, y + h, 0.0f );
	PlgImmColour( rectangle->lr.r, rectangle->lr.g, rectangle->lr.b, rectangle->lr.a );

	PlgImmDraw();
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
	PlgImmBegin( PLG_MESH_LINES );

	for ( unsigned int i = 0; i < numPoints; ++i ) {
		PlgImmPushVertex( points[ i ].x, points[ i ].y, points[ i ].z );
		PlgImmColour( colour.r, colour.g, colour.b, colour.a );
	}

	PlgImmDraw();
}

void PlgDrawLine( PLMatrix4 transform, PLVector3 startPos, PLColour startColour, PLVector3 endPos, PLColour endColour ) {
	PlgImmBegin( PLG_MESH_LINES );

	PlgImmPushVertex( startPos.x, startPos.y, startPos.z );
	PlgImmColour( startColour.r, startColour.g, startColour.b, startColour.a );

	PlgImmPushVertex( endPos.x, endPos.y, endPos.z );
	PlgImmColour( endColour.r, endColour.g, endColour.b, endColour.a );

	PlgImmDraw();
}

void PlgDrawSimpleLine( PLMatrix4 transform, PLVector3 startPos, PLVector3 endPos, PLColour colour ) {
	PlgDrawLine( transform, startPos, colour, endPos, colour );
}

void PlgDrawGrid( int x, int y, int w, int h, unsigned int gridSize ) {
	PlgImmBegin( PLG_MESH_LINES );

	int c = 0, r = 0;
	for ( ; r < h + 1; r += ( int ) gridSize ) {
		PlgImmPushVertex( x, y + r, 0.0f );
		PlgImmColour( 0, 0, 255, 255 );
		PlgImmPushVertex( x + w, r + y, 0.0f );
		PlgImmColour( 0, 0, 255, 255 );
		for ( ; c < w + 1; c += ( int ) gridSize ) {
			PlgImmPushVertex( c + x, y, 0.0f );
			PlgImmColour( 0, 0, 255, 255 );
			PlgImmPushVertex( c + x, y + h, 0.0f );
			PlgImmColour( 0, 0, 255, 255 );
		}
	}

	PlgImmDraw();
}

void PlgDrawVertexNormals( const PLGVertex *vertices, unsigned int numVertices )
{
	PlgImmBegin( PLG_MESH_LINES );

	for ( unsigned int i = 0; i < numVertices; ++i ) {
		PLVector3 linePos, lineEndPos;

		linePos = vertices[ i ].position;
		PlgImmPushVertex( linePos.x, linePos.y, linePos.z );
		PlgImmColour( 255, 0, 255, 255 );

		lineEndPos = PlAddVector3( linePos, PlScaleVector3F( vertices[ i ].normal, 4.0f ) );
		PlgImmPushVertex( lineEndPos.x, lineEndPos.y, lineEndPos.z );
		PlgImmColour( 255, 0, 255, 255 );

		linePos = vertices[ i ].position;
		PlgImmPushVertex( linePos.x, linePos.y, linePos.z );
		PlgImmColour( 0, 255, 255, 255 );

		lineEndPos = PlAddVector3( linePos, PlScaleVector3F( vertices[ i ].tangent, 4.0f ) );
		PlgImmPushVertex( lineEndPos.x, lineEndPos.y, lineEndPos.z );
		PlgImmColour( 0, 255, 255, 255 );
	}

	PlgImmDraw();
}

/**
 * Draw lines at each vertex point representing the direction of the normal.
 */
void PlgDrawMeshNormals( const PLGMesh *mesh ) {
	PlgDrawVertexNormals( mesh->vertices, mesh->num_verts );
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
