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

/************************************************************/
/* MTL Format */

/* todo */

/************************************************************/
/* Obj Static Model Format */

typedef struct ObjVectorLst {
	PLVector3 v;
	struct ObjVectorLst *next;
} ObjVectorLst;

typedef struct ObjFaceLst {
	PLGVertex *vertices;
	unsigned int num_vertices;
	char mtl_name[ 64 ];
	struct ObjFaceLst *next;
} ObjFaceLst;

typedef struct ObjHandle {
	ObjVectorLst *vertex_normals;
	ObjVectorLst *vertex_positions;
	ObjVectorLst *vertex_tex_coords;
	ObjFaceLst *faces;
	char mtllib_path[ PL_SYSTEM_MAX_PATH ];
} ObjHandle;

static void FreeObjHandle( ObjHandle *obj ) {
	PlFree( obj );
}

static ObjVectorLst *GetVectorIndex( ObjVectorLst *start, unsigned int idx ) {
	ObjVectorLst *cur = start;
	for ( unsigned int i = 0; i < idx - 1; ++i ) {
		if ( cur->next == NULL ) {
			ModelLog( "Invalid vector index (%d)!\n", i );
			return NULL;
		}

		cur = cur->next;
	}

	return cur;
}

PLMModel *PlmLoadObjModel( const char *path ) {
	PLFile *fp = PlOpenFile( path, false );
	if ( fp == NULL ) {
		return NULL;
	}

	ObjHandle *obj = PlMAllocA( sizeof( ObjHandle ) );

	ObjVectorLst **cur_v = &( obj->vertex_positions );
	ObjVectorLst **cur_vn = &( obj->vertex_normals );
	ObjVectorLst **cur_vt = &( obj->vertex_tex_coords );
	*cur_v = *cur_vn = *cur_vt = NULL;

	ObjFaceLst **cur_face = &( obj->faces );
	*cur_face = NULL;

	char tk[ 256 ];
	while ( PlReadString( fp, tk, sizeof( tk ) ) != NULL ) {
		if ( tk[ 0 ] == '\0' || tk[ 0 ] == '#' || tk[ 0 ] == 'o' || tk[ 0 ] == 'g' || tk[ 0 ] == 's' ) {
			continue;
		} else if ( tk[ 0 ] == 'v' && tk[ 1 ] == ' ' ) { /* vertex position */
			ObjVectorLst *this_v = PlMAllocA( sizeof( ObjVectorLst ) );
			unsigned int n = sscanf( &tk[ 2 ], "%f %f %f", &this_v->v.x, &this_v->v.y, &this_v->v.z );
			if ( n < 3 ) {
				ModelLog( "Invalid vertex position, less than 3 coords!\n\"%s\"\n", tk );
			} else if ( n > 3 ) {
				ModelLog( "Ignoring fourth vertex position parameter, unsupported!\n\"%s\"\n", tk );
			}

			*cur_v = this_v;
			this_v->next = NULL;
			cur_v = &( this_v->next );
			continue;
		} else if ( tk[ 0 ] == 'v' && tk[ 1 ] == 't' ) { /* vertex texture */
			ObjVectorLst *this_vt = PlMAllocA( sizeof( ObjVectorLst ) );
			unsigned int n = sscanf( &tk[ 2 ], "%f %f", &this_vt->v.x, &this_vt->v.y );
			if ( n < 2 ) {
				ModelLog( "Invalid vertex uv, less than 2 coords!\n\"%s\"\n", tk );
			} else if ( n > 2 ) {
				ModelLog( "Ignoring third uv parameter, unsupported!\n\"%s\"\n", tk );
			}

			*cur_vt = this_vt;
			this_vt->next = NULL;
			cur_vt = &( this_vt->next );
			continue;
		} else if ( tk[ 0 ] == 'v' && tk[ 1 ] == 'n' ) { /* vertex normal */
			ObjVectorLst *this_vn = PlMAllocA( sizeof( ObjVectorLst ) );
			if ( sscanf( &tk[ 2 ], "%f %f %f", &this_vn->v.x, &this_vn->v.y, &this_vn->v.z ) < 3 ) {
				ModelLog( "Invalid vertex normal, less than 3 coords!\n\"%s\"\n", tk );
			}

			*cur_vn = this_vn;
			this_vn->next = NULL;
			cur_vn = &( this_vn->next );
			continue;
		} else if ( tk[ 0 ] == 'f' && tk[ 1 ] == ' ' ) { /* face */
			int idx[ 64 ];
			unsigned int num_elements = 1;

			/* faces are kind of a special case, so we'll need
             * to parse them manually here */
			char *pos = tk;
			while ( *pos != '\0' && *pos != '\n' && *pos != '\r' ) {
				if ( *pos == ' ' ) {
					continue;
				}

				pos++;
			}

			ObjFaceLst *this_face = PlMAllocA( sizeof( ObjFaceLst ) );

			*cur_face = this_face;
			this_face->next = NULL;
			cur_face = &( this_face->next );
			continue;
		}

		ModelLog( "Unknown/unsupported parameter '%s', ignoring!\n", tk[ 0 ] );
	}

	PlCloseFile( fp );

	/* right we're finally done, time to see what we hauled... */

	return NULL;
}

bool plWriteObjModel( PLMModel *model, const char *path ) {
	if ( model == NULL ) {
		PlReportBasicError( PL_RESULT_INVALID_PARM1 );
		return false;
	}

	FILE *fp = fopen( path, "w" );
	if ( fp == NULL ) {
		PlReportBasicError( PL_RESULT_FILEWRITE );
		return false;
	}

	fprintf( fp, "# generated by hei platform lib (https://github.com/TalonBraveInfo/platform)\n" );
	if ( model->type == PLM_MODELTYPE_SKELETAL ) {
		ModelLog( "Model is of type skeletal; Obj only supports static models so skeleton will be discarded...\n" );
	}

	/* for now, use the same name as the model for the material */
	const char *filename = PlGetFileName( path );
	size_t len = strlen( filename );
	char *mtl_name = PlMAllocA( len );
	snprintf( mtl_name, len - 4, "%s", PlGetFileName( path ) );
	fprintf( fp, "mtllib ./%s.mtl\n", mtl_name );

	/* todo: kill duplicated data */
	for ( unsigned int i = 0; i < model->numMeshes; ++i ) {
		PLGMesh *mesh = model->meshes[ i ];
		if ( mesh->primitive == PLG_MESH_TRIANGLES ) {
			fprintf( fp, "o mesh.%0d\n", i );
			/* print out vertices */
			for ( unsigned int vi = 0; vi < mesh->num_verts; ++vi ) {
				fprintf( fp, "v %s\n", PlPrintVector3( &mesh->vertices[ vi ].position, pl_float_var ) );
			}
			/* print out texture coords */
			for ( unsigned int vi = 0; vi < mesh->num_verts; ++vi ) {
				fprintf( fp, "vt %s\n", PlPrintVector2( &mesh->vertices[ vi ].st[ 0 ], pl_float_var ) );
			}
			/* print out vertex normals */
			for ( unsigned int vi = 0; vi < mesh->num_verts; ++vi ) {
				fprintf( fp, "vn %s\n", PlPrintVector3( &mesh->vertices[ vi ].normal, pl_float_var ) );
			}
			fprintf( fp, "# %d vertices\n", mesh->num_verts );

			if ( mesh->texture != NULL && mesh->texture->name[ 0 ] != '\0' ) {
				fprintf( fp, "usemtl %s\n", mesh->texture->name );
			}

			for ( unsigned int fi = 0; fi < mesh->num_triangles; ++fi ) {
				fprintf( fp, "f %d/%d/%d\n",
				         mesh->indices[ fi ],
				         mesh->indices[ fi ],
				         mesh->indices[ fi ] );
			}
		}
	}

	PlFree( mtl_name );

	fclose( fp );

	// todo...
	return true;
}
