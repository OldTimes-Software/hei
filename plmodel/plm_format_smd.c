/* SPDX-License-Identifier: MIT */
/* Copyright © 2017-2022 Mark E Sowden <hogsy@oldtimes-software.com> */

#include "plm_private.h"

#define SMD_VERSION   1
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
	if ( sscanf( buffer, "version %d\n", &version ) < 1 || version != SMD_VERSION ) {
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
PLMModel *PlmLoadSmdModel( const char *path ) {
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

static void WriteNodesBlock( FILE *file, const PLMModel *model ) {
	fprintf( file, "nodes\n" );
	if ( model->type != PLM_MODELTYPE_SKELETAL ) {
		/* write out a dummy bone! */
		fprintf( file, "0 \"root\" -1\n" );
	} else {
		/* todo, revisit this so we're correctly connecting child/parent */
		for ( unsigned int j = 0; j < model->internal.skeletal_data.numBones; ++j ) {
			int parentBone;
			if ( model->internal.skeletal_data.bones[ j ].parent == ( unsigned int ) -1 ) {
				parentBone = -1;
			} else {
				parentBone = model->internal.skeletal_data.bones[ j ].parent;
			}
			fprintf( file, "%u %s %d\n", j, model->internal.skeletal_data.bones[ j ].name, parentBone );
		}
	}
	fprintf( file, "end\n" );
}

static void WriteSkeletonBlock( FILE *file, const PLMModel *model ) {
	fprintf( file, "skeleton\ntime 0\n" );
	if ( model->type == PLM_MODELTYPE_SKELETAL ) {
		for ( unsigned int i = 0; i < model->internal.skeletal_data.numBones; ++i ) {
#if 1
			PLVector3 rotation = PlVector3( model->internal.skeletal_data.bones[ i ].orientation.x,
			                                model->internal.skeletal_data.bones[ i ].orientation.y,
			                                model->internal.skeletal_data.bones[ i ].orientation.z );
#else
			PLVector3 rotation = PlQuaternionToEuler( &model->internal.skeletal_data.bones[ i ].orientation );
#endif
			fprintf( file, "%u %f %f %f %f %f %f\n",
			         i,                                                   /* bone id */
			         model->internal.skeletal_data.bones[ i ].position.x, /* bone pos */
			         model->internal.skeletal_data.bones[ i ].position.y,
			         model->internal.skeletal_data.bones[ i ].position.z,
			         rotation.x, /* bone rot */
			         rotation.y,
			         rotation.z );
		}
	} else {
		/* write out dummy bone coords! */
		fprintf( file, "0 0 0 0 0 0 0\n" );
	}
	fprintf( file, "end\n" );
}

static void SMD_WriteVertex( FILE *file, const PLMModel *model, const PLMBoneWeight *boneWeight, const PLGVertex *vertex ) {
	if ( model->type == PLM_MODELTYPE_SKELETAL && boneWeight != NULL ) {
		fprintf( file, "%u %f %f %f %f %f %f %f %f %u ",
		         boneWeight->subWeights[ 0 ].boneIndex,
		         vertex->position.x,
		         vertex->position.y,
		         vertex->position.z,
		         vertex->normal.x,
		         vertex->normal.y,
		         vertex->normal.z,
		         vertex->st[ 0 ].x,
		         vertex->st[ 0 ].y,
		         boneWeight->numSubWeights );
		/* and now for weights */
		for ( unsigned int i = 0; i < boneWeight->numSubWeights; ++i ) {
			fprintf( file, "%u %f ", boneWeight->subWeights[ i ].boneIndex, boneWeight->subWeights[ i ].factor );
		}
		fprintf( file, "\n" );
	} else {
		fprintf( file, "0 %f %f %f %f %f %f %f %f\n",
		         vertex->position.x,
		         vertex->position.y,
		         vertex->position.z,
		         vertex->normal.x,
		         vertex->normal.y,
		         vertex->normal.z,
		         vertex->st[ 0 ].x,
		         vertex->st[ 0 ].y );
	}
}

static void WriteTrianglesBlock( FILE *file, const PLMModel *model ) {
	fprintf( file, "triangles\n" );
	for ( unsigned int j = 0; j < model->numMeshes; ++j ) {
		for ( unsigned int k = 0; k < model->meshes[ j ]->num_indices; ) {
			if ( model->materials == NULL || model->meshes[ j ]->materialIndex >= model->numMaterials ) {
				fprintf( file, "null\n" );
			} else {
				fprintf( file, "%s\n", model->materials[ model->meshes[ j ]->materialIndex ] );
			}

			if ( model->type == PLM_MODELTYPE_SKELETAL ) {
				SMD_WriteVertex( file,
				                 model,
				                 &model->internal.skeletal_data.weights[ model->meshes[ j ]->indices[ k ] ],
				                 &model->meshes[ j ]->vertices[ model->meshes[ j ]->indices[ k ] ] );
				k++;
				SMD_WriteVertex( file,
				                 model,
				                 &model->internal.skeletal_data.weights[ model->meshes[ j ]->indices[ k ] ],
				                 &model->meshes[ j ]->vertices[ model->meshes[ j ]->indices[ k ] ] );
				k++;
				SMD_WriteVertex( file,
				                 model,
				                 &model->internal.skeletal_data.weights[ model->meshes[ j ]->indices[ k ] ],
				                 &model->meshes[ j ]->vertices[ model->meshes[ j ]->indices[ k ] ] );
				k++;
			} else {
				SMD_WriteVertex( file, model, NULL, &model->meshes[ j ]->vertices[ model->meshes[ j ]->indices[ k++ ] ] );
				SMD_WriteVertex( file, model, NULL, &model->meshes[ j ]->vertices[ model->meshes[ j ]->indices[ k++ ] ] );
				SMD_WriteVertex( file, model, NULL, &model->meshes[ j ]->vertices[ model->meshes[ j ]->indices[ k++ ] ] );
			}
		}
	}
	fprintf( file, "end\n" );
}

/* writes given model out to Valve's SMD model format */
bool PlmWriteSmdModel( PLMModel *model, const char *path ) {
	FILE *fp_out = NULL;

	if ( PlGetFileExtension( path ) == NULL ) {
		PLPath fp;
		snprintf( fp, sizeof( fp ), "%s.smd", path );
		path = fp;
	}

	fp_out = fopen( path, "w" );
	if ( fp_out == NULL ) {
		PlReportBasicError( PL_RESULT_FILEWRITE );
		return false;
	}

	fprintf( fp_out, "version 1\n" );

	WriteNodesBlock( fp_out, model );
	WriteSkeletonBlock( fp_out, model );
	WriteTrianglesBlock( fp_out, model );

	//for ( unsigned int i = 0; i < model->internal.skeletal_data.numBoneWeights; ++i ) {
	//	printf( "%d %f %u\n", i, model->internal.skeletal_data.weights[ i ].weightFactor, model->internal.skeletal_data.weights[ i ].boneIndex );
	//}

	/* and leave a blank line at the end, to keep studiomdl happy */
	fprintf( fp_out, "\n" );

	fclose( fp_out );

	return true;
}
