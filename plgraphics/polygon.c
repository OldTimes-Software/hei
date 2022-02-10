/**
 * Hei Platform Library
 * Copyright (C) 2017-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is licensed under MIT. See LICENSE for more details.
 */

#include <plgraphics/plg_polygon.h>

typedef struct PLGPolygon {
	PLGTexture *texture;
	PLVector2 textureOffset;
	PLVector2 textureScale;
	float textureRotation;

	PLVector3 normal;

	PLGVertex vertices[ PLG_POLYGON_MAX_SIDES ];
	unsigned int numVertices;
} PLGPolygon;

PLGPolygon *PlgCreatePolygon( PLGTexture *texture, PLVector2 textureOffset, PLVector2 textureScale, float textureRotation ) {
	PLGPolygon *polygon = PlCAllocA( 1, sizeof( PLGPolygon ) );
	polygon->texture = texture;
	polygon->textureOffset = textureOffset;
	polygon->textureScale = textureScale;
	polygon->textureRotation = textureRotation;
	return polygon;
}

void PlgDestroyPolygon( PLGPolygon *polygon ) {
	if( polygon == NULL ) {
		return;
	}

	PlFree( polygon );
}

/**
 * Generate vertex normals for the given polygon.
 */
void PlgGeneratePolygonNormals( PLGPolygon *polygon ) {
	unsigned int numTriangles;
	unsigned int *indices = PlgConvertPolygonToTriangles( polygon, &numTriangles );

	PlgGenerateVertexNormals( polygon->vertices, polygon->numVertices, indices, numTriangles, true );
	PlgGenerateTangentBasis( polygon->vertices, polygon->numVertices, indices, numTriangles );

	PlFree( indices );
}

/**
 * Generate face normal for the given polygon.
 */
void PlgGeneratePolygonFaceNormal( PLGPolygon *polygon ) {
	unsigned int numTriangles;
	unsigned int *indices = PlgConvertPolygonToTriangles( polygon, &numTriangles );

	PLVector3 *normals = PlMAllocA( sizeof( PLVector3 ) * polygon->numVertices );
	for ( unsigned int i = 0, idx = 0; i < numTriangles; ++i, idx += 3 ) {
		unsigned int a = indices[ idx ];
		unsigned int b = indices[ idx + 1 ];
		unsigned int c = indices[ idx + 2 ];

		PLVector3 normal = PlgGenerateVertexNormal(
		        polygon->vertices[ a ].position,
		        polygon->vertices[ b ].position,
		        polygon->vertices[ c ].position );

		normals[ a ] = PlAddVector3( normals[ a ], normal );
		normals[ b ] = PlAddVector3( normals[ b ], normal );
		normals[ c ] = PlAddVector3( normals[ c ], normal );
	}

	polygon->normal = normals[ 0 ];

	PlFree( normals );
	PlFree( indices );
}

void PlgAddPolygonVertex( PLGPolygon *polygon, const PLGVertex *vertex ) {
	if( polygon->numVertices >= PLG_POLYGON_MAX_SIDES ) {
		PlReportErrorF( PL_RESULT_INVALID_PARM2, "reached maximum number of polygon sides (%d)", PLG_POLYGON_MAX_SIDES );
		return;
	}

	polygon->vertices[ polygon->numVertices ] = *vertex;
	polygon->numVertices++;

	PlgGeneratePolygonFaceNormal( polygon );

#if 0
	if( polygon->texture != NULL ) {
		plGenerateTextureCoordinates( polygon->vertices, polygon->numVertices, polygon->textureOffset, polygon->textureScale );
	}
#endif
}

void PlgRemovePolygonVertex( PLGPolygon *polygon, unsigned int vertIndex ) {
	if( vertIndex >= polygon->numVertices ) {
		PlReportErrorF( PL_RESULT_INVALID_PARM2, "invalid vertex index (%d)", polygon->numVertices );
		return;
	}

	/* reshuffle the list */
	memmove( polygon->vertices + vertIndex, polygon->vertices + vertIndex + 1, ( polygon->numVertices - vertIndex ) - 1);
	--( polygon->numVertices );

	PlgGeneratePolygonFaceNormal( polygon );

#if 0
	if( polygon->texture != NULL ) {
		plGenerateTextureCoordinates( polygon->vertices, polygon->numVertices, polygon->textureOffset, polygon->textureScale );
	}
#endif
}

unsigned int PlgGetNumOfPolygonVertices( const PLGPolygon *polygon ) {
	return polygon->numVertices;
}

PLGVertex *PlgGetPolygonVertex( PLGPolygon *polygon, unsigned int vertIndex ) {
	if( vertIndex >= polygon->numVertices ) {
		PlReportErrorF( PL_RESULT_INVALID_PARM2, "invalid vertex index, %d", vertIndex );
		return NULL;
	}

	return &polygon->vertices[ vertIndex ];
}

PLGVertex *PlgGetPolygonVertices( PLGPolygon *polygon, unsigned int *numVertices ) {
	*numVertices = polygon->numVertices;
	return polygon->vertices;
}

PLGTexture *PlgGetPolygonTexture( PLGPolygon *polygon ) {
	return polygon->texture;
}

PLVector3 PlgGetPolygonFaceNormal( const PLGPolygon *polygon ) {
	return polygon->normal;
}

/**
 * Return the number of triangles in this polygon.
 */
unsigned int PlgGetNumOfPolygonTriangles( const PLGPolygon *polygon ) {
	if ( polygon->numVertices < 3 ) {
		return 0;
	}

	return polygon->numVertices - 2;
}

unsigned int *PlgConvertPolygonToTriangles( const PLGPolygon *polygon, unsigned int *numTriangles ) {
	*numTriangles = PlgGetNumOfPolygonTriangles( polygon );
	if ( *numTriangles == 0 ) {
		PlReportErrorF( PL_RESULT_INVALID_PARM1, "invalid polygon" );
		return NULL;
	}

	unsigned int *indices = PlMAllocA( sizeof( unsigned int ) * ( *numTriangles * 3 ) );
	unsigned int *index = indices;
	for ( unsigned int i = 1; i + 1 < polygon->numVertices; ++i ) {
		index[ 0 ] = 0;
		index[ 1 ] = i;
		index[ 2 ] = i + 1;
		index += 3;
	}

	return indices;
}

PLGMesh *PlgConvertPolygonToMesh( const PLGPolygon *polygon ) {
	unsigned int numTriangles;
	unsigned int *indices = PlgConvertPolygonToTriangles( polygon, &numTriangles );
	if ( indices == NULL ) {
		return NULL;
	}

	PLGMesh *mesh = PlgCreateMeshInit( PLG_MESH_TRIANGLES, PLG_DRAW_STATIC, numTriangles, polygon->numVertices, indices, polygon->vertices );

	PlFree( indices );

	return mesh;
}
