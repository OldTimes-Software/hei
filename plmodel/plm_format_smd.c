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

#include "plm_private.h"

#define SMD_VERSION 1
#define SMD_MAX_NODES 4096

typedef struct SMDNode {
	int id;
	char boneName[ 64 ];
	int parentId;
} SMDNode;

/*
typedef struct SMDSkeleton {

} SMDSkeleton;

typedef struct SMDTriangle {

} SMDTriangle;
*/

static SMDNode *SMD_ReadNodes( PLFile *file, unsigned int *numNodes ) {
	*numNodes = 0;

	char buffer[ 256 ];
	if ( PlReadString( file, buffer, sizeof( buffer ) ) == NULL ) {
		return NULL;
	}

	/* ensure that it starts with 'nodes' */
	if ( strcmp( buffer, "nodes\n" ) != 0 ) {
		return NULL;
	}

	/* now read through and figure out how many nodes there are */
	size_t pos = PlGetFileOffset( file );
	while ( PlReadString( file, buffer, sizeof( buffer ) ) != NULL ) {
		if ( strcmp( buffer, "end\n" ) == 0 ) {
			break;
		}

		numNodes++;
	}

	if ( *numNodes == 0 ) {
		return NULL;
	}

	/* now set us back to the start of the list */
	PlFileSeek( file, pos, PL_SEEK_SET );

	/* allocate our nodes list */
	SMDNode *nodes = PlMAllocA( *numNodes * sizeof( SMDNode ) );
	SMDNode *curNode = nodes;
	while ( PlReadString( file, buffer, sizeof( buffer ) ) != NULL ) {
		if ( strcmp( buffer, "end\n" ) == 0 ) {
			break;
		}

		if ( sscanf( buffer, "%d %s %d\n", &curNode->id, curNode->boneName, &curNode->parentId ) != 3 ) {
		}

		curNode++;
	}

	return nodes;
}

static PLMModel *SMD_ReadFile( PLFile *file ) {
	char buffer[ 256 ];
	if ( PlReadString( file, buffer, sizeof( buffer ) ) == NULL ) {
		return NULL;
	}

	/* expect the version string first */
	unsigned int version = 0;
	if( sscanf( buffer, "version %d\n", &version ) < 1 || version != SMD_VERSION ) {
		PlReportBasicError( PL_RESULT_FILEVERSION );
		return NULL;
	}

	/* read in the bones (aka, nodes) */
	unsigned int numNodes;
	SMDNode *nodes = SMD_ReadNodes( file, &numNodes );
	if ( nodes == NULL ) {
		PlReportErrorF( PL_RESULT_FILEREAD, "failed to read in nodes" );
		return NULL;
	}

	return NULL;
}

/**
 * Loads an SMD model. These are stored as plain ASCII.
 */
PLMModel *plLoadSmdModel( const char *path ) {
	PLFile *file = PlOpenFile( path, false );
	if ( file == NULL ) {
		return NULL;
	}

#if 0
	const char *buf = ( char * ) plGetFileData( file );
	size_t length = plGetFileSize( file );

	PLModel *model = SMD_ParseBuffer( buf, length );
#else
	PLMModel *model = SMD_ReadFile( file );
#endif

	PlCloseFile( file );

	return model;
}

static void SMD_WriteVertex( FILE *fp, const PLGVertex *vertex ) {
	/*               P X  Y  Z  NX NY NZ U  V */
	fprintf( fp, "0 %f %f %f %f %f %f %f %f\n",

	         vertex->position.x,
	         vertex->position.y,
	         vertex->position.z,

	         vertex->normal.x,
	         vertex->normal.y,
	         vertex->normal.z,

	         vertex->st[ 0 ].x,
	         vertex->st[ 0 ].y );
}

/* writes given model out to Valve's SMD model format */
bool plWriteSmdModel( PLMModel *model, const char *path ) {
	FILE *fp_out = NULL;

	char full_path[ PL_SYSTEM_MAX_PATH ];
	snprintf( full_path, sizeof( full_path ), "%s.smd", path );

	fp_out = fopen( full_path, "w" );
	if ( fp_out == NULL ) {
		PlReportBasicError( PL_RESULT_FILEWRITE );
		return false;
	}

	/* header */
	fprintf( fp_out, "version 1\n\n" );

	/* write out the nodes block */
	fprintf( fp_out, "nodes\n" );
	if ( model->type != PLM_MODELTYPE_SKELETAL ) {
		/* write out a dummy bone! */
		fprintf( fp_out, "0 \"root\" -1\n" );
	} else {
		/* todo, revisit this so we're correctly connecting child/parent */
		for ( unsigned int j = 0; j < model->internal.skeletal_data.num_bones; ++j ) {
			fprintf( fp_out, "%u %s %d\n", j, model->internal.skeletal_data.bones[ j ].name, ( int ) j - 1 );
		}
	}
	fprintf( fp_out, "end\n\n" );

	/* skeleton block */
	fprintf( fp_out, "skeleton\ntime 0\n" );
	if ( model->type != PLM_MODELTYPE_SKELETAL ) {
		/* write out dummy bone coords! */
		fprintf( fp_out, "0 0 0 0 0 0 0\n" );
	} else {
		/* todo, print out default coords for each bone */
	}
	fprintf( fp_out, "end\n\n" );

	/* triangles block */
	fprintf( fp_out, "triangles\n" );
	for ( unsigned int j = 0; j < model->numMeshes; ++j ) {
		for ( unsigned int k = 0; k < model->meshes[ j ]->num_indices; ) {
			if ( model->meshes[ j ]->texture == NULL ) {
				fprintf( fp_out, "null\n" );
			} else {
				fprintf( fp_out, "%s\n", model->meshes[ j ]->texture->name );
			}
			SMD_WriteVertex( fp_out, &model->meshes[ j ]->vertices[ model->meshes[ j ]->indices[ k++ ] ] );
			SMD_WriteVertex( fp_out, &model->meshes[ j ]->vertices[ model->meshes[ j ]->indices[ k++ ] ] );
			SMD_WriteVertex( fp_out, &model->meshes[ j ]->vertices[ model->meshes[ j ]->indices[ k++ ] ] );
		}
	}

	/* and leave a blank line at the end, to keep studiomdl happy */
	fprintf( fp_out, "end\n\n\n" );

	fclose( fp_out );

	return true;
}
