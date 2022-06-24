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

#include <plcore/pl.h>
#include <plcore/pl_math.h>
#include <plcore/pl_physics.h>

#include <plgraphics/plg.h>
#include <plgraphics/plg_mesh.h>

typedef struct PLAnimationFrame {
	PLVector3 transform;
} PLAnimationFrame;

typedef struct PLAnimation {
	char name[ 64 ];
	PLAnimationFrame *frames;
	uint32_t num_frames;
	float framerate;
} PLAnimation;

////////////////////////////////////////////////////////////////////////////

typedef enum PLMModelType {
	PLM_MODELTYPE_STATIC,   /* static non-animated */
	PLM_MODELTYPE_VERTEX,   /* per-vertex animated */
	PLM_MODELTYPE_SKELETAL, /* skeletal/bones animated */

	PLM_NUM_MODELTYPES
} PLMModelType;

typedef struct PLMVertexAnimationFrame {
	/* todo: store submeshes into PLMesh struct */
	PLGMesh **meshes;
	uint32_t num_meshes;
} PLMVertexAnimationFrame;

typedef struct PLMVertexAnimModelData {
	uint32_t current_animation; /* current animation index */
	uint32_t current_frame;     /* current animation frame */
	PLMVertexAnimationFrame *animations;
} PLMVertexAnimModelData;

/* * * * * * * * * * * * * * * * * */
/* Skeletal Model Data */

#define PLM_MAX_BONE_WEIGHTS 4

typedef struct PLMBoneWeight {
	struct {
		unsigned int boneIndex;
		float factor;
	} subWeights[ PLM_MAX_BONE_WEIGHTS ];
	unsigned int numSubWeights;
} PLMBoneWeight;

typedef struct PLMModelBone {
	char name[ 64 ];
	unsigned int parent;
	PLVector3 position;
	PLQuaternion orientation;
} PLMBone;

typedef struct PLMSkeletalModelData {
	unsigned int rootIndex; /* root bone */

	PLMBone *bones;        /* list of bones */
	unsigned int numBones; /* number of bones in the array */

	unsigned int numBoneWeights; /* should be the same as the number of vertices */
	PLMBoneWeight *weights;
} PLMSkeletalModelData;

typedef struct PLMModel {
	char name[ 64 ];
	char path[ PL_SYSTEM_MAX_PATH ];
	PLMModelType type;
	uint16_t flags;

	/* used for visibility culling */
	float radius;
	PLCollisionAABB bounds;

	/* transformations */
	PLMatrix4 modelMatrix;

	PLGMesh **meshes;
	unsigned int numMeshes;

	PLPath *materials;
	unsigned int numMaterials;

	void *userData; /* for storing material references etc. */

	struct {
		/* model type data */
		union {
			PLMSkeletalModelData skeletal_data; /* skeletal animation data */
			//PLStaticModelData       static_data;    /* static model data */
			PLMVertexAnimModelData vertex_data; /* per-vertex animation data */
		};
	} internal;
} PLMModel;

PL_EXTERN_C

PLMModel *PlmCreateStaticModel( PLGMesh **meshes, unsigned int numMeshes );
PLMModel *PlmCreateBasicStaticModel( PLGMesh *mesh );
PLMModel *PlmCreateSkeletalModel( PLGMesh **meshes, unsigned int numMeshes, PLMBone *bones, unsigned int numBones, PLMBoneWeight *weights, unsigned int numWeights );
PLMModel *PlmCreateBasicSkeletalModel( PLGMesh *mesh, PLMBone *bones, unsigned int numBones, PLMBoneWeight *weights, unsigned int numWeights );

PLMModel *PlmLoadModel( const char *path );

PLMModel *PlmLoadU3DModel( const char *path );
PLMModel *PlmLoadHdvModel( const char *path );
PLMModel *PlmLoadRequiemModel( const char *path );
PLMModel *PlmLoadObjModel( const char *path );
PLMModel *PlmLoadCpjModel( const char *path );

void PlmDestroyModel( PLMModel *model );

void PlmDrawModel( PLMModel *model );
void PlmDrawModelBounds( const PLMModel *model );
void PlmDrawModelSkeleton( PLMModel *model );

void PlmGenerateModelNormals( PLMModel *model, bool perFace );
void PlmGenerateModelBounds( PLMModel *model );

enum {
	PLM_MODEL_FILEFORMAT_ALL = 0,

	PL_BITFLAG( PLM_MODEL_FILEFORMAT_CYCLONE, 0 ),
	PL_BITFLAG( PLM_MODEL_FILEFORMAT_HDV, 1 ),
	PL_BITFLAG( PLM_MODEL_FILEFORMAT_U3D, 2 ),
	PL_BITFLAG( PLM_MODEL_FILEFORMAT_OBJ, 3 ),
	PL_BITFLAG( PLM_MODEL_FILEFORMAT_CPJ, 4 ),
};

void PlmRegisterModelLoader( const char *ext, PLMModel *( *LoadFunction )( const char *path ) );
void PlmRegisterStandardModelLoaders( unsigned int flags );
void PlmClearModelLoaders( void );

typedef enum PLModelOutputType {
	PLM_MODEL_OUTPUT_DEFAULT,
	PLM_MODEL_OUTPUT_SMD,

	PLM_MAX_MODEL_OUTPUT_FORMATS
} PLMModelOutputType;
bool PlmWriteModel( const char *path, PLMModel *model, PLMModelOutputType type );

PL_EXTERN_C_END
