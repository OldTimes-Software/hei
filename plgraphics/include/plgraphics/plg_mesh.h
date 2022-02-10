/**
 * Hei Platform Library
 * Copyright (C) 2017-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is licensed under MIT. See LICENSE for more details.
 */

#pragma once

#include <plcore/pl_math.h>

PL_EXTERN_C

typedef enum PLGMeshPrimitive {
	PLG_MESH_LINES,
	PLG_MESH_LINE_STIPPLE, /* todo */
	PLG_MESH_LINE_LOOP,
	PLG_MESH_LINE_STRIP,
	PLG_MESH_POINTS,
	PLG_MESH_TRIANGLES,
	PLG_MESH_TRIANGLE_STRIP,
	PLG_MESH_TRIANGLE_FAN,
	PLG_MESH_TRIANGLE_FAN_LINE,
	PLG_MESH_QUADS,      /* todo */
	PLG_MESH_QUAD_STRIP, /* todo */

	PLG_NUM_PRIMITIVES
} PLGMeshPrimitive;

typedef enum PLGMeshDrawMode {
	PLG_DRAW_STREAM,
	PLG_DRAW_STATIC,
	PLG_DRAW_DYNAMIC,

	PLG_NUM_DRAWMODES
} PLGMeshDrawMode;

typedef struct PLGVertex {
	PLVector3 position, normal;
	PLVector3 tangent, bitangent;
	PLVector2 st[ 16 ];
	PLColour colour;
	/* specific to skeletal animation */
	unsigned int bone_index;
	float bone_weight;
} PLGVertex;

static inline PLGVertex PlgInitializeVertex( void ) {
	PLGVertex m;
	memset( &m, 0, sizeof( PLGVertex ) );
	return m;
}

typedef struct PLGTexture PLGTexture;

#define PLG_MAX_MESH_BUFFERS 32

typedef struct PLGMesh {
	PLGVertex *vertices;
	unsigned int num_verts;
	unsigned int maxVertices;

	unsigned int *indices;
	unsigned int num_indices;
	unsigned int maxIndices;
	unsigned int num_triangles;

	/* todo: consider throwing out the below */
	struct PLGShaderProgram *shader_program;
	PLGTexture *texture;
	unsigned int materialIndex;

	PLGMeshPrimitive primitive;
	PLGMeshDrawMode mode;

	bool isDirty;

	unsigned int buffers[ PLG_MAX_MESH_BUFFERS ];
} PLGMesh;

typedef struct PLCollisionAABB PLCollisionAABB;

#if !defined( PL_COMPILE_PLUGIN )

PL_EXTERN PLGMesh *PlgCreateMesh( PLGMeshPrimitive primitive, PLGMeshDrawMode mode, unsigned int num_tris, unsigned int num_verts );
PL_EXTERN PLGMesh *PlgCreateMeshInit( PLGMeshPrimitive primitive, PLGMeshDrawMode mode, unsigned int numTriangles, unsigned int numVerts,
                                      const unsigned int *indicies, const PLGVertex *vertices );
PL_EXTERN PLGMesh *PlgCreateMeshRectangle( float x, float y, float w, float h, PLColour colour );
PL_EXTERN void PlgDestroyMesh( PLGMesh *mesh );

PL_EXTERN void PlgDrawEllipse( unsigned int segments, PLVector2 position, float w, float h, PLColour colour );
PL_EXTERN void PlgDrawRectangle( const PLMatrix4 *transform, float x, float y, float w, float h, PLColour colour );
PL_EXTERN void PlgDrawTexturedRectangle( const PLMatrix4 *transform, float x, float y, float w, float h, PLGTexture *texture );
PL_EXTERN void PlgDrawFilledRectangle( const PLRectangle2D *rectangle );
PL_EXTERN void PlgDrawTexturedQuad( const PLVector3 *ul, const PLVector3 *ur, const PLVector3 *ll, const PLVector3 *lr,
                                    float hScale, float vScale, PLGTexture *texture );
PL_EXTERN void PlgDrawTriangle( int x, int y, unsigned int w, unsigned int h );
PL_EXTERN void PlgDrawLines( const PLVector3 *points, unsigned int numPoints, PLColour colour );
PL_EXTERN void PlgDrawLine( PLMatrix4 transform, PLVector3 startPos, PLColour startColour, PLVector3 endPos, PLColour endColour );
PL_EXTERN void PlgDrawSimpleLine( PLMatrix4 transform, PLVector3 startPos, PLVector3 endPos, PLColour colour );
PL_EXTERN void PlgDrawGrid( int x, int y, int w, int h, unsigned int gridSize );
PL_EXTERN void PlgDrawMeshNormals( const PLGMesh *mesh );
PL_EXTERN void PlgDrawBoundingVolume( const PLCollisionAABB *bounds, PLColour colour );

PL_EXTERN void PlgClearMesh( PLGMesh *mesh );
PL_EXTERN void PlgClearMeshVertices( PLGMesh *mesh );
PL_EXTERN void PlgClearMeshTriangles( PLGMesh *mesh );

PL_EXTERN void PlgScaleMesh( PLGMesh *mesh, PLVector3 scale );
PL_EXTERN void PlgSetMeshTrianglePosition( PLGMesh *mesh, unsigned int *index, unsigned int x, unsigned int y, unsigned int z );
PL_EXTERN void PlgSetMeshVertexPosition( PLGMesh *mesh, unsigned int index, PLVector3 vector );
PL_EXTERN void PlgSetMeshVertexNormal( PLGMesh *mesh, unsigned int index, PLVector3 vector );
PL_EXTERN void PlgSetMeshVertexST( PLGMesh *mesh, unsigned int index, float s, float t );
PL_EXTERN void PlgSetMeshVertexSTv( PLGMesh *mesh, uint8_t unit, unsigned int index, unsigned int size, const float *st );
PL_EXTERN void PlgSetMeshVertexColour( PLGMesh *mesh, unsigned int index, PLColour colour );
PL_EXTERN void PlgSetMeshUniformColour( PLGMesh *mesh, PLColour colour );
PL_EXTERN void PlgSetMeshShaderProgram( PLGMesh *mesh, struct PLGShaderProgram *program );

PL_EXTERN unsigned int PlgAddMeshVertex( PLGMesh *mesh, PLVector3 position, PLVector3 normal, PLColour colour, PLVector2 st );
PL_EXTERN unsigned int PlgAddMeshTriangle( PLGMesh *mesh, unsigned int x, unsigned int y, unsigned int z );

PL_EXTERN void PlgUploadMesh( PLGMesh *mesh );
PL_EXTERN void PlgDrawMesh( PLGMesh *mesh );

PLCollisionAABB PlgGenerateAabbFromVertices( const PLGVertex *vertices, unsigned int numVertices, bool absolute );
PLCollisionAABB PlgGenerateAabbFromMesh( const PLGMesh *mesh, bool absolute );

PL_EXTERN void PlgGenerateMeshNormals( PLGMesh *mesh, bool perFace );
PL_EXTERN void PlgGenerateMeshTangentBasis( PLGMesh *mesh );

PL_EXTERN void PlgGenerateTangentBasis( PLGVertex *vertices, unsigned int numVertices, const unsigned int *indices, unsigned int numTriangles );
PL_EXTERN void PlgGenerateTextureCoordinates( PLGVertex *vertices, unsigned int numVertices, PLVector2 textureOffset, PLVector2 textureScale );
PL_EXTERN void PlgGenerateVertexNormals( PLGVertex *vertices, unsigned int numVertices, unsigned int *indices, unsigned int numTriangles, bool perFace );

PL_EXTERN PLVector3 PlgGenerateVertexNormal( PLVector3 a, PLVector3 b, PLVector3 c );

#endif

PL_EXTERN_C_END
