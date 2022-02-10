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

/*
	UNREAL 3D Animated Model Format

	The following is based on information from the following page...
	http://paulbourke.net/dataformats/unreal/
*/

typedef struct U3DAnimationHeader {
	uint16_t frames;// Number of frames.
	uint16_t size;  // Size of each frame.
} U3DAnimationHeader;

typedef struct U3DDataHeader {
	uint16_t numpolys;// Number of polygons.
	uint16_t numverts;// Number of vertices.
	uint16_t rotation;// Mesh rotation?
	uint16_t frame;   // Initial frame.

	uint32_t norm_x;
	uint32_t norm_y;
	uint32_t norm_z;

	uint32_t fixscale;
	uint32_t unused[ 3 ];
} U3DDataHeader;

#define U3D_FLAG_UNLIT 16
#define U3D_FLAG_FLAT 32
#define U3D_FLAG_ENVIRONMENT 64
#define U3D_FLAG_NEAREST 128

enum U3DType {
	U3D_TYPE_NORMAL,
	U3D_TYPE_NORMALTWOSIDED,
	U3D_TYPE_TRANSLUCENT,
	U3D_TYPE_MASKED,
	U3D_TYPE_MODULATE,
	U3D_TYPE_ATTACHMENT
};

typedef struct U3DVertex {
	// This is a bit funky...
	int32_t x : 11;
	int32_t y : 11;
	int32_t z : 10;
} U3DVertex;

typedef struct U3DTriangle {
	uint16_t vertex[ 3 ];// Vertex indices

	uint8_t type;        // Triangle type
	uint8_t colour;      // Triangle colour
	uint8_t ST[ 3 ][ 2 ];// Texture coords
	uint8_t texturenum;  // Texture offset
	uint8_t flags;       // Triangle flags
} U3DTriangle;

static int CompareTriangles( const void *a, const void *b ) {
	if ( ( ( U3DTriangle * ) a )->texturenum > ( ( U3DTriangle * ) b )->texturenum ) {
		return -1;
	} else if ( ( ( U3DTriangle * ) a )->texturenum < ( ( U3DTriangle * ) b )->texturenum ) {
		return 1;
	}

	return 0;
}

static PLMModel *ReadU3DModelData( PLFile *data_ptr, PLFile *anim_ptr ) {
	U3DAnimationHeader anim_hdr;
	if ( PlReadFile( anim_ptr, &anim_hdr, sizeof( U3DAnimationHeader ), 1 ) != 1 ) {
		return NULL;
	}

	/* validate animation header */

	if ( anim_hdr.size == 0 ) {
		PlReportErrorF( PL_RESULT_FILEREAD, "incorrect animation hdr size for \"%s\"", PlGetFilePath( anim_ptr ) );
		return NULL;
	} else if ( anim_hdr.frames == 0 ) {
		PlReportErrorF( PL_RESULT_FILEREAD, "invalid number of frames for \"%s\"", PlGetFilePath( anim_ptr ) );
		return NULL;
	}

	U3DDataHeader data_hdr;
	if ( PlReadFile( data_ptr, &data_hdr, sizeof( U3DDataHeader ), 1 ) != 1 ) {
		return NULL;
	}

	/* validate data header */

	if ( data_hdr.numverts == 0 ) {
		PlReportErrorF( PL_RESULT_FILEREAD, "no vertices in model, \"%s\"", PlGetFilePath( data_ptr ) );
		return NULL;
	} else if ( data_hdr.numpolys == 0 ) {
		PlReportErrorF( PL_RESULT_FILEREAD, "no polygons in model, \"%s\"", PlGetFilePath( data_ptr ) );
		return NULL;
	} else if ( data_hdr.frame > anim_hdr.frames ) {
		PlReportErrorF( PL_RESULT_FILEREAD, "invalid frame specified in model, \"%s\"", PlGetFilePath( data_ptr ) );
		return NULL;
	}

	/* skip unused header data */
	PlFileSeek( data_ptr, 12, PL_SEEK_CUR );

	/* read all the triangle data from the data file */
	U3DTriangle *triangles = PlCAllocA( data_hdr.numpolys, sizeof( U3DTriangle ) );
	PlReadFile( data_ptr, triangles, sizeof( U3DTriangle ), data_hdr.numpolys );
	PlCloseFile( data_ptr );

	/* sort triangles by texture id */
	qsort( triangles, data_hdr.numpolys, sizeof( U3DTriangle ), CompareTriangles );

	/* read in all of the animation data from the anim file */
	U3DVertex *vertices = PlCAllocA( ( size_t ) data_hdr.numverts * anim_hdr.frames, sizeof( U3DVertex ) );
	PlReadFile( anim_ptr, vertices, sizeof( U3DVertex ), ( size_t ) data_hdr.numverts * anim_hdr.frames );
	PlCloseFile( anim_ptr );

	PLMModel *model_ptr = PlCAllocA( 1, sizeof( PLMModel ) );
	model_ptr->type = PLM_MODELTYPE_VERTEX;
	model_ptr->internal.vertex_data.animations = PlCAllocA( anim_hdr.frames, sizeof( PLMVertexAnimationFrame ) );

	for ( unsigned int i = 0; i < anim_hdr.frames; ++i ) {
		PLMVertexAnimationFrame *frame = &model_ptr->internal.vertex_data.animations[ i ];
	}

	PlFree( triangles );
	PlFree( vertices );

	PlmGenerateModelBounds( model_ptr );

	return NULL;
}

/**
 * Load U3D model from local path.
 * @param path Path to the U3D Data file.
 */
PLMModel *PlmLoadU3DModel( const char *path ) {
	char anim_path[ PL_SYSTEM_MAX_PATH ];
	snprintf( anim_path, sizeof( anim_path ), "%s", path );
	char *p_ext = strstr( anim_path, "Data" );
	if ( p_ext != NULL ) {
		strcpy( p_ext, "Anim" );
	} else {
		p_ext = strstr( anim_path, "D." );
		if ( p_ext != NULL ) {
			p_ext[ 0 ] = 'A';
		}
	}

	if ( !PlFileExists( anim_path ) ) {
		PlReportErrorF( PL_RESULT_FILEPATH, "failed to find anim companion for \"%s\" at \"%s\"", path, anim_path );
		return NULL;
	}

	PLFile *anim_ptr = PlOpenFile( anim_path, false );
	PLFile *data_ptr = PlOpenFile( path, false );

	PLMModel *model_ptr = NULL;
	if ( anim_ptr != NULL && data_ptr != NULL ) {
		model_ptr = ReadU3DModelData( data_ptr, anim_ptr );
	}

	PlCloseFile( data_ptr );
	PlCloseFile( anim_ptr );

	return model_ptr;
}

bool plWriteU3DModel( PLMModel *ptr, const char *path ) {
	return false;
}

/* Example UC file, this is what we _should_ be loading from.

#exec MESH IMPORT MESH=wafr1 ANIVFILE=MODELS\wafr1_a.3D DATAFILE=MODELS\wafr1_d.3D X=0 Y=0 Z=0
#exec MESH ORIGIN MESH=wafr1 X=0 Y=0 Z=0 YAW=64 ROLL=64
#exec MESH SEQUENCE MESH=wafr1 SEQ=All  STARTFRAME=0  NUMFRAMES=1
#exec MESH SEQUENCE MESH=wafr1 SEQ=Still  STARTFRAME=0   NUMFRAMES=1
#exec MESHMAP SCALE MESHMAP=wafr1 X=0.1 Y=0.1 Z=0.2
#exec MESHMAP SETTEXTURE MESHMAP=wafr1 NUM=0 TEXTURE=DefaultTexture

#exec MESH IMPORT MESH=wafr2 ANIVFILE=MODELS\wafr2_a.3D DATAFILE=MODELS\wafr2_d.3D X=0 Y=0 Z=0
#exec MESH ORIGIN MESH=wafr2 X=0 Y=0 Z=0 YAW=64 ROLL=64
#exec MESH SEQUENCE MESH=wafr2 SEQ=All  STARTFRAME=0  NUMFRAMES=1
#exec MESH SEQUENCE MESH=wafr2 SEQ=Still  STARTFRAME=0   NUMFRAMES=1
#exec MESHMAP SCALE MESHMAP=wafr2 X=0.1 Y=0.1 Z=0.2
#exec MESHMAP SETTEXTURE MESHMAP=wafr2 NUM=0 TEXTURE=DefaultTexture

#exec MESH IMPORT MESH=wafr4 ANIVFILE=MODELS\wafr4_a.3D DATAFILE=MODELS\wafr4_d.3D X=0 Y=0 Z=0
#exec MESH ORIGIN MESH=wafr4 X=0 Y=0 Z=0 YAW=64 ROLL=64
#exec MESH SEQUENCE MESH=wafr4 SEQ=All  STARTFRAME=0  NUMFRAMES=1
#exec MESH SEQUENCE MESH=wafr4 SEQ=Still  STARTFRAME=0   NUMFRAMES=1
#exec MESHMAP SCALE MESHMAP=wafr4 X=0.1 Y=0.1 Z=0.2
#exec MESHMAP SETTEXTURE MESHMAP=wafr4 NUM=0 TEXTURE=DefaultTexture

*/
