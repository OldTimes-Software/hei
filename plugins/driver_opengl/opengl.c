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

#include "plugin.h"

#include <plgraphics/plg.h>
#include <plgraphics/plg_camera.h>

#include <plcore/pl_parse.h>

#include <GL/glew.h>

#define DEBUG_GL

struct {
	bool generate_mipmap;
	bool depth_texture;
	bool shadow;
	bool vertex_buffer_object;
	bool texture_compression;
	bool texture_compression_s3tc;
	bool multitexture;
	bool texture_env_combine;
	bool texture_env_add;
	bool vertex_program;
	bool fragment_program;
} gl_capabilities;

static int gl_version_major = 0;
static int gl_version_minor = 0;

#define XGL_VERSION( maj, min ) ( ( ( maj ) == gl_version_major && ( min ) <= gl_version_minor ) || ( maj ) < gl_version_major )
#define XGL_LOG( ... )          gInterface->core->LogMessage( glLogLevel, __VA_ARGS__ )

unsigned int gl_num_extensions = 0;

static GLuint VAO[ 1 ];

#if !defined( NDEBUG )
#	define XGL_CALL( X )                     \
		{                                     \
			glGetError();                     \
			X;                                \
			unsigned int _err = glGetError(); \
			assert( _err == GL_NO_ERROR );    \
		}
#else
#	define XGL_CALL( X ) X
#endif

///////////////////////////////////////////
// Debug

static void GLInsertDebugMarker( const char *msg ) {
	if ( !XGL_VERSION( 4, 3 ) ) {
		return;
	}

	XGL_CALL( glDebugMessageInsert( GL_DEBUG_SOURCE_APPLICATION,
	                                GL_DEBUG_TYPE_MARKER,
	                                0,
	                                GL_DEBUG_SEVERITY_NOTIFICATION,
	                                -1,
	                                msg ) );
}

static void GLPushDebugGroupMarker( const char *msg ) {
	if ( !XGL_VERSION( 4, 3 ) ) {
		return;
	}

	XGL_CALL( glPushDebugGroup( GL_DEBUG_SOURCE_APPLICATION, 0, -1, msg ) );
}

static void GLPopDebugGroupMarker( void ) {
	if ( !XGL_VERSION( 4, 3 ) ) {
		return;
	}

	XGL_CALL( glPopDebugGroup() );
}

#if 0
static void ClearBoundTextures( void ) {
	for ( unsigned int i = 0; i < gfx_state.hw_maxtextureunits; ++i ) {
		glActiveTexture( GL_TEXTURE0 + i );
		glBindTexture( GL_TEXTURE_2D, 0 );
	}
	glActiveTexture( GL_TEXTURE0 );
}

static void ClearBoundBuffers( void ) {
	glBindFramebuffer( GL_FRAMEBUFFER, 0 );
	glBindRenderbuffer( GL_RENDERBUFFER, 0 );
}
#endif

static void GL_TranslateTextureFilterFormat( PLGTextureFilter filterMode, int *min, int *mag ) {
	switch ( filterMode ) {
		case PLG_TEXTURE_FILTER_LINEAR:
			*min = *mag = GL_LINEAR;
			break;
		default:
		case PLG_TEXTURE_FILTER_NEAREST:
			*min = *mag = GL_NEAREST;
			break;
		case PLG_TEXTURE_FILTER_MIPMAP_LINEAR:
			*min = GL_LINEAR_MIPMAP_LINEAR;
			*mag = GL_LINEAR;
			break;
		case PLG_TEXTURE_FILTER_MIPMAP_LINEAR_NEAREST:
			*min = GL_LINEAR_MIPMAP_NEAREST;
			*mag = GL_LINEAR;
			break;
		case PLG_TEXTURE_FILTER_MIPMAP_NEAREST:
			*min = GL_NEAREST_MIPMAP_NEAREST;
			*mag = GL_NEAREST;
			break;
		case PLG_TEXTURE_FILTER_MIPMAP_NEAREST_LINEAR:
			*min = GL_NEAREST_MIPMAP_LINEAR;
			*mag = GL_NEAREST;
			break;
	}
}

/////////////////////////////////////////////////////////////

static bool GLSupportsHWShaders( void ) {
	return ( XGL_VERSION( 2, 1 ) || ( gl_capabilities.fragment_program && gl_capabilities.vertex_program ) );
}

static void GLGetMaxTextureUnits( unsigned int *num_units ) {
	XGL_CALL( glGetIntegerv( GL_MAX_TEXTURE_IMAGE_UNITS, ( GLint * ) num_units ) );
}

static void GLGetMaxTextureSize( unsigned int *s ) {
	XGL_CALL( glGetIntegerv( GL_MAX_TEXTURE_SIZE, ( GLint * ) s ) );
}

/////////////////////////////////////////////////////////////

static void GLSetClearColour( PLColour rgba ) {
	XGL_CALL( glClearColor(
	        PlByteToFloat( rgba.r ),
	        PlByteToFloat( rgba.g ),
	        PlByteToFloat( rgba.b ),
	        PlByteToFloat( rgba.a ) ) );
}

static void GLClearBuffers( unsigned int buffers ) {
	// Rather ugly, but translate it over to GL.
	unsigned int glclear = 0;
	if ( buffers & PLG_BUFFER_COLOUR ) glclear |= GL_COLOR_BUFFER_BIT;
	if ( buffers & PLG_BUFFER_DEPTH ) glclear |= GL_DEPTH_BUFFER_BIT;
	if ( buffers & PLG_BUFFER_STENCIL ) glclear |= GL_STENCIL_BUFFER_BIT;
	XGL_CALL( glClear( glclear ) );
}

static void GLSetDepthBufferMode( unsigned int mode ) {
	switch ( mode ) {
		default:
			XGL_LOG( "Unknown depth buffer mode, %d\n", mode );
			break;

		case PLG_DEPTHBUFFER_DISABLE:
			XGL_CALL( glDisable( GL_DEPTH_TEST ) );
			break;

		case PLG_DEPTHBUFFER_ENABLE:
			XGL_CALL( glEnable( GL_DEPTH_TEST ) );
			break;
	}
}

static void GLSetDepthMask( bool enable ) {
	XGL_CALL( glDepthMask( enable ) );
}

/////////////////////////////////////////////////////////////

static unsigned int TranslateBlendFunc( PLGBlend blend ) {
	switch ( blend ) {
		default:
		case PLG_BLEND_ONE:
			return GL_ONE;
		case PLG_BLEND_ZERO:
			return GL_ZERO;
		case PLG_BLEND_SRC_COLOR:
			return GL_SRC_COLOR;
		case PLG_BLEND_ONE_MINUS_SRC_COLOR:
			return GL_ONE_MINUS_SRC_COLOR;
		case PLG_BLEND_SRC_ALPHA:
			return GL_SRC_ALPHA;
		case PLG_BLEND_ONE_MINUS_SRC_ALPHA:
			return GL_ONE_MINUS_SRC_ALPHA;
		case PLG_BLEND_DST_ALPHA:
			return GL_DST_ALPHA;
		case PLG_BLEND_ONE_MINUS_DST_ALPHA:
			return GL_ONE_MINUS_DST_ALPHA;
		case PLG_BLEND_DST_COLOR:
			return GL_DST_COLOR;
		case PLG_BLEND_ONE_MINUS_DST_COLOR:
			return GL_ONE_MINUS_DST_COLOR;
		case PLG_BLEND_SRC_ALPHA_SATURATE:
			return GL_SRC_ALPHA_SATURATE;
	}
}

static void GLSetBlendMode( PLGBlend a, PLGBlend b ) {
	if ( a == PLG_BLEND_NONE && b == PLG_BLEND_NONE ) {
		XGL_CALL( glDisable( GL_BLEND ) );
	} else {
		XGL_CALL( glEnable( GL_BLEND ) );
	}

	XGL_CALL( glBlendFunc( TranslateBlendFunc( a ), TranslateBlendFunc( b ) ) );
}

static void GLSetCullMode( PLGCullMode mode ) {
	if ( mode == PLG_CULL_NONE ) {
		XGL_CALL( glDisable( GL_CULL_FACE ) );
	} else {
		XGL_CALL( glEnable( GL_CULL_FACE ) );
		XGL_CALL( glCullFace( GL_BACK ) );
		switch ( mode ) {
			default:
			case PLG_CULL_NEGATIVE:
				XGL_CALL( glFrontFace( GL_CW ) );
				break;

			case PLG_CULL_POSITIVE:
				XGL_CALL( glFrontFace( GL_CCW ) );
				break;
		}
	}
}


/////////////////////////////////////////////////////////////
// Framebuffer

static unsigned int TranslateFrameBufferBinding( PLGFrameBufferObjectTarget targetBinding ) {
	switch ( targetBinding ) {
		case PLG_FRAMEBUFFER_DEFAULT:
			return GL_FRAMEBUFFER;
		case PLG_FRAMEBUFFER_DRAW:
			return GL_DRAW_FRAMEBUFFER;
		case PLG_FRAMEBUFFER_READ:
			return GL_READ_FRAMEBUFFER;
		default:
			return 0;
	}
}

static void GLCreateFrameBuffer( PLGFrameBuffer *buffer ) {
	XGL_CALL( glGenFramebuffers( 1, &buffer->fbo ) );
	XGL_CALL( glBindFramebuffer( GL_FRAMEBUFFER, buffer->fbo ) );

	if ( buffer->flags & PLG_BUFFER_COLOUR ) {
		XGL_CALL( glGenRenderbuffers( 1, &buffer->renderBuffers[ PLG_RENDERBUFFER_COLOUR ] ) );
		XGL_CALL( glBindRenderbuffer( GL_RENDERBUFFER, buffer->renderBuffers[ PLG_RENDERBUFFER_COLOUR ] ) );
		XGL_CALL( glRenderbufferStorage( GL_RENDERBUFFER, GL_RGBA, ( int ) buffer->width, ( int ) buffer->height ) );
		XGL_CALL( glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, buffer->renderBuffers[ PLG_RENDERBUFFER_COLOUR ] ) );
	}

	if ( ( buffer->flags & PLG_BUFFER_DEPTH ) && ( buffer->flags & PLG_BUFFER_STENCIL ) ) {
		XGL_CALL( glGenRenderbuffers( 1, &buffer->renderBuffers[ PLG_RENDERBUFFER_DEPTH ] ) );
		XGL_CALL( glBindRenderbuffer( GL_RENDERBUFFER, buffer->renderBuffers[ PLG_RENDERBUFFER_DEPTH ] ) );
		XGL_CALL( glRenderbufferStorage( GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, ( int ) buffer->width, ( int ) buffer->height ) );
		XGL_CALL( glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, buffer->renderBuffers[ PLG_RENDERBUFFER_DEPTH ] ) );
	} else if ( buffer->flags & PLG_BUFFER_DEPTH ) {
		XGL_CALL( glGenRenderbuffers( 1, &buffer->renderBuffers[ PLG_RENDERBUFFER_DEPTH ] ) );
		XGL_CALL( glBindRenderbuffer( GL_RENDERBUFFER, buffer->renderBuffers[ PLG_RENDERBUFFER_DEPTH ] ) );
		XGL_CALL( glRenderbufferStorage( GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, ( int ) buffer->width, ( int ) buffer->height ) );
		XGL_CALL( glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, buffer->renderBuffers[ PLG_RENDERBUFFER_DEPTH ] ) );
	} else if ( buffer->flags & PLG_BUFFER_STENCIL ) {
		XGL_CALL( glGenRenderbuffers( 1, &buffer->renderBuffers[ PLG_RENDERBUFFER_STENCIL ] ) );
		XGL_CALL( glBindRenderbuffer( GL_RENDERBUFFER, buffer->renderBuffers[ PLG_RENDERBUFFER_STENCIL ] ) );
		XGL_CALL( glRenderbufferStorage( GL_RENDERBUFFER, GL_STENCIL_INDEX8, ( int ) buffer->width, ( int ) buffer->height ) );
		XGL_CALL( glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, buffer->renderBuffers[ PLG_RENDERBUFFER_STENCIL ] ) );
	}
}

static void GLDeleteFrameBuffer( PLGFrameBuffer *buffer ) {
	if ( buffer == NULL ) {
		return;
	}

	if ( buffer->fbo != 0 ) {
		XGL_CALL( glDeleteFramebuffers( 1, &buffer->fbo ) );
		buffer->fbo = 0;
	}

	for ( unsigned int i = 0; i < PLG_MAX_RENDERBUFFER_TYPES; ++i ) {
		if ( buffer->renderBuffers[ i ] == 0 ) {
			continue;
		}

		XGL_CALL( glDeleteRenderbuffers( 1, &buffer->renderBuffers[ i ] ) );
		buffer->renderBuffers[ i ] = 0;
	}
}

static void GLBindFrameBuffer( PLGFrameBuffer *buffer, PLGFrameBufferObjectTarget target_binding ) {
	XGL_CALL( glBindFramebuffer( TranslateFrameBufferBinding( target_binding ), ( buffer != NULL ) ? buffer->fbo : 0 ) );
}

static void GLBlitFrameBuffers( PLGFrameBuffer *src_buffer,
                                unsigned int src_w,
                                unsigned int src_h,
                                PLGFrameBuffer *dst_buffer,
                                unsigned int dst_w,
                                unsigned int dst_h,
                                bool linear ) {
	XGL_CALL( glBindFramebuffer( GL_READ_FRAMEBUFFER, ( src_buffer != NULL ) ? src_buffer->fbo : 0 ) );
	XGL_CALL( glBindFramebuffer( GL_DRAW_FRAMEBUFFER, ( dst_buffer != NULL ) ? dst_buffer->fbo : 0 ) );

	XGL_CALL( glBlitFramebuffer( 0, 0, src_w, src_h, 0, 0, dst_w, dst_h, GL_COLOR_BUFFER_BIT, linear ? GL_LINEAR : GL_NEAREST ) );
}

static void GLSetFrameBufferSize( PLGFrameBuffer *frameBuffer, unsigned int width, unsigned int height ) {
	/* just to be safe, flush the whole thing */
	GLDeleteFrameBuffer( frameBuffer );

	/* and given we don't flush any flags etc., we can
	 * pretty much just update what we want and create
	 * it again */
	frameBuffer->width = width;
	frameBuffer->height = height;
	GLCreateFrameBuffer( frameBuffer );
}

static void GLBindTexture( const PLGTexture *texture );

static PLGTexture *GLGetFrameBufferTextureAttachment( PLGFrameBuffer *buffer, unsigned int components, PLGTextureFilter filter ) {
	PLGTexture *texture = gInterface->CreateTexture();
	if ( texture == NULL ) {
		return NULL;
	}

	/* all of this is going to change later...
	 * this is just the bare minimum to get things going */

	texture->filter = filter;
	texture->w = buffer->width;
	texture->h = buffer->height;

	GLBindFrameBuffer( buffer, PLG_FRAMEBUFFER_DRAW );
	GLBindTexture( texture );

	int min, mag;
	GL_TranslateTextureFilterFormat( filter, &min, &mag );
	XGL_CALL( glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min ) );
	XGL_CALL( glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag ) );

	XGL_CALL( glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE ) );
	XGL_CALL( glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE ) );

	/* sigh... */
	if ( ( components & PLG_BUFFER_DEPTH ) || ( components & PLG_BUFFER_STENCIL ) ) {
		if ( ( buffer->flags & PLG_BUFFER_DEPTH ) && ( buffer->flags & PLG_BUFFER_STENCIL ) ) {
			/* so yeah, this sucks, but if both of these are active we assume it's packed */
			XGL_CALL( glTexImage2D( GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, buffer->width, buffer->height, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL ) );
			if ( components & PLG_BUFFER_DEPTH ) {
				XGL_CALL( glFramebufferTexture2D( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, texture->internal.id, 0 ) );
			}
			if ( components & PLG_BUFFER_STENCIL ) {
				XGL_CALL( glFramebufferTexture2D( GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, texture->internal.id, 0 ) );
			}
		} else {
			/* otherwise, assumed not packed */
			if ( components & PLG_BUFFER_DEPTH ) {
				XGL_CALL( glTexImage2D( GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, buffer->width, buffer->height, 0, GL_DEPTH_ATTACHMENT, GL_UNSIGNED_INT, NULL ) );
				XGL_CALL( glFramebufferTexture2D( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, texture->internal.id, 0 ) );
			} else if ( components & PLG_BUFFER_STENCIL ) {
				XGL_CALL( glTexImage2D( GL_TEXTURE_2D, 0, GL_STENCIL_INDEX8, buffer->width, buffer->height, 0, GL_STENCIL_ATTACHMENT, GL_UNSIGNED_BYTE, NULL ) );
				XGL_CALL( glFramebufferTexture2D( GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, texture->internal.id, 0 ) );
			}
		}
	} else if ( components & PLG_BUFFER_COLOUR ) {
		XGL_CALL( glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, buffer->width, buffer->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL ) );
		XGL_CALL( glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture->internal.id, 0 ) );
	}

	GLBindTexture( NULL );

	return texture;
}

/////////////////////////////////////////////////////////////
// Stencil Operations

/////////////////////////////////////////////////////////////
// Texture

static unsigned int TranslateImageFormat( PLImageFormat format ) {
	switch ( format ) {
		case PL_IMAGEFORMAT_RGB8:
			return GL_RGB8;
		case PL_IMAGEFORMAT_RGBA8:
			return GL_RGBA8;
		case PL_IMAGEFORMAT_RGB4:
			return GL_RGB4;
		case PL_IMAGEFORMAT_RGBA4:
			return GL_RGBA4;
		case PL_IMAGEFORMAT_RGB5:
			return GL_RGB5;
		case PL_IMAGEFORMAT_RGB5A1:
			return GL_RGB5_A1;

		case PL_IMAGEFORMAT_RGB_DXT1:
			return GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
		case PL_IMAGEFORMAT_RGBA_DXT1:
			return GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
		case PL_IMAGEFORMAT_RGBA_DXT3:
			return GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
		case PL_IMAGEFORMAT_RGBA_DXT5:
			return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;

		case PL_IMAGEFORMAT_RGB_FXT1:
			return GL_COMPRESSED_RGB_FXT1_3DFX;

		default:
			return 0;
	}
}

static unsigned int GetStorageFormatForImageFormat( PLImageFormat format ) {
	return GL_UNSIGNED_BYTE;
}

static unsigned int GetColourFormatForImageFormat( PLImageFormat format ) {
	switch ( format ) {
		case PL_IMAGEFORMAT_RGB4:
		case PL_IMAGEFORMAT_RGB5:
		case PL_IMAGEFORMAT_RGB565:
		case PL_IMAGEFORMAT_RGB8:
		case PL_IMAGEFORMAT_RGB_DXT1:
		case PL_IMAGEFORMAT_RGB_FXT1:
			return GL_RGB;
		default:
			break;
	}

	return GL_RGBA;
}

static void GLCreateTexture( PLGTexture *texture ) {
	XGL_CALL( glGenTextures( 1, &texture->internal.id ) );
}

static void GLDeleteTexture( PLGTexture *texture ) {
	XGL_CALL( glDeleteTextures( 1, &texture->internal.id ) );
}

static void GLBindTexture( const PLGTexture *texture ) {
	if ( texture == NULL ) {
		XGL_CALL( glBindTexture( GL_TEXTURE_2D, 0 ) );
		return;
	}
	XGL_CALL( glBindTexture( GL_TEXTURE_2D, texture->internal.id ) );
}

static bool IsCompressedImageFormat( PLImageFormat format ) {
	switch ( format ) {
		default:
			return false;
		case PL_IMAGEFORMAT_RGBA_DXT1:
		case PL_IMAGEFORMAT_RGBA_DXT3:
		case PL_IMAGEFORMAT_RGBA_DXT5:
		case PL_IMAGEFORMAT_RGB_DXT1:
		case PL_IMAGEFORMAT_RGB_FXT1:
			return true;
	}
}

static void GLSetTextureFilter( PLGTexture *texture, PLGTextureFilter filter );
static void GLUploadTexture( PLGTexture *texture, const PLImage *upload ) {
	GLBindTexture( texture );

	/* was originally GL_CLAMP; deprecated in GL3+, though some drivers
	 * still seem to accept it anyway except for newer Intel GPUs apparently */
	/* todo: make this configurable */
	XGL_CALL( glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT ) );
	XGL_CALL( glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT ) );

	GLSetTextureFilter( texture, texture->filter );

	unsigned int levels = upload->levels;
	if ( levels == 0 ) {
		levels = 1;
	}

	unsigned int image_format = TranslateImageFormat( upload->format );
	for ( unsigned int i = 0; i < levels; ++i ) {
		GLsizei w = texture->w / ( unsigned int ) pow( 2, i );
		GLsizei h = texture->h / ( unsigned int ) pow( 2, i );
		if ( IsCompressedImageFormat( upload->format ) ) {
			XGL_CALL( glCompressedTexImage2D(
			        GL_TEXTURE_2D,
			        i,
			        image_format,
			        w, h,
			        0,
			        ( GLsizei ) upload->size,
			        upload->data[ 0 ] ) );
		} else {
			XGL_CALL( glTexImage2D(
			        GL_TEXTURE_2D,
			        i,
			        image_format,
			        w, h,
			        0,
			        GetColourFormatForImageFormat( upload->format ),
			        GetStorageFormatForImageFormat( upload->format ),
			        upload->data[ 0 ] ) );
		}
	}

	if ( levels == 1 && !( texture->flags & PLG_TEXTURE_FLAG_NOMIPS ) ) {
		XGL_CALL( glGenerateMipmap( GL_TEXTURE_2D ) );
	}
}

static void GLSetTextureAnisotropy( PLGTexture *texture, uint32_t value ) {
	GLBindTexture( texture );
	XGL_CALL( glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, ( int ) value ) );
}

static void GLSetTextureFilter( PLGTexture *texture, PLGTextureFilter filter ) {
	GLBindTexture( texture );

	int min, mag;
	GL_TranslateTextureFilterFormat( filter, &min, &mag );
	if ( filter == PLG_TEXTURE_FILTER_LINEAR || filter == PLG_TEXTURE_FILTER_NEAREST ) {
		texture->flags |= PLG_TEXTURE_FLAG_NOMIPS;
	}

	XGL_CALL( glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag ) );
	XGL_CALL( glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min ) );

	texture->filter = filter;
}

static void GLActiveTexture( unsigned int target ) {
	XGL_CALL( glActiveTexture( GL_TEXTURE0 + target ) );
}

/* Swizzle texture channels */

static int TranslateColourChannel( int channel ) {
	switch ( channel ) {
		case PL_RED:
			return GL_RED;
		case PL_GREEN:
			return GL_GREEN;
		case PL_BLUE:
			return GL_BLUE;
		case PL_ALPHA:
			return GL_ALPHA;
		default:
			return channel;
	}
}

static void GLSwizzleTexture( PLGTexture *texture, uint8_t r, uint8_t g, uint8_t b, uint8_t a ) {
	GLBindTexture( texture );
	if ( XGL_VERSION( 3, 3 ) ) {
		int swizzle[] = {
		        TranslateColourChannel( r ),
		        TranslateColourChannel( g ),
		        TranslateColourChannel( b ),
		        TranslateColourChannel( a ) };
		XGL_CALL( glTexParameteriv( GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzle ) );
	} else {
		gInterface->core->ReportError( PL_RESULT_UNSUPPORTED, PL_FUNCTION, "missing software implementation" );
	}
}

/////////////////////////////////////////////////////////////
// Mesh

typedef struct MeshTranslatePrimitive {
	PLGMeshPrimitive mode;
	unsigned int target;
	const char *name;
} MeshTranslatePrimitive;

static MeshTranslatePrimitive primitives[] = {
        {PLG_MESH_LINES,              GL_LINES,          "LINES"            },
        { PLG_MESH_LINE_LOOP,         GL_LINE_LOOP,      "LINE_LOOP"        },
        { PLG_MESH_POINTS,            GL_POINTS,         "POINTS"           },
        { PLG_MESH_TRIANGLES,         GL_TRIANGLES,      "TRIANGLES"        },
        { PLG_MESH_TRIANGLE_FAN,      GL_TRIANGLE_FAN,   "TRIANGLE_FAN"     },
        { PLG_MESH_TRIANGLE_FAN_LINE, GL_LINES,          "TRIANGLE_FAN_LINE"},
        { PLG_MESH_TRIANGLE_STRIP,    GL_TRIANGLE_STRIP, "TRIANGLE_STRIP"   },
        { PLG_MESH_QUADS,             GL_TRIANGLES,      "QUADS"            }  // todo, translate
};

static unsigned int TranslatePrimitiveMode( PLGMeshPrimitive mode ) {
	for ( unsigned int i = 0; i < PL_ARRAY_ELEMENTS( primitives ); i++ ) {
		if ( mode == primitives[ i ].mode )
			return primitives[ i ].target;
	}

	// Hacky, but just return initial otherwise.
	return primitives[ 0 ].target;
}

static unsigned int TranslateDrawMode( PLGMeshDrawMode mode ) {
	switch ( mode ) {
		case PLG_DRAW_DYNAMIC:
			return GL_DYNAMIC_DRAW;
		case PLG_DRAW_STATIC:
			return GL_STATIC_DRAW;
		case PLG_DRAW_STREAM:
			return GL_STREAM_DRAW;
		default:
			return 0;
	}
}

enum {
	BUFFER_VERTEX_DATA = 0,
	BUFFER_ELEMENT_DATA,

	MAX_GPU_MESH_BUFFERS
};
PL_STATIC_ASSERT( MAX_GPU_MESH_BUFFERS < PLG_MAX_MESH_BUFFERS, "Invalid MAX_GL_BUFFERS size!" );

static void GLCreateMesh( PLGMesh *mesh ) {
	if ( !XGL_VERSION( 2, 0 ) ) {
		return;
	}

	// Create our internal buffers for GL
	XGL_CALL( glGenBuffers( MAX_GPU_MESH_BUFFERS, mesh->buffers ) );
}

static void GLUploadMesh( PLGMesh *mesh, PLGShaderProgram *program ) {
	if ( !mesh->isDirty )
		return;

	//Bind VBO
	XGL_CALL( glBindBuffer( GL_ARRAY_BUFFER, mesh->buffers[ BUFFER_VERTEX_DATA ] ) );

	//Write the current CPU vertex data into the VBO
	unsigned int drawMode = TranslateDrawMode( mesh->mode );
	XGL_CALL( glBufferData( GL_ARRAY_BUFFER, ( GLsizei ) ( sizeof( PLGVertex ) * mesh->num_verts ), &mesh->vertices[ 0 ], drawMode ) );

	//Point to the different substreams of the interleaved BVO
	//Args: Index, Size, Type, (Normalized), Stride, StartPtr

	if ( mesh->buffers[ BUFFER_ELEMENT_DATA ] != 0 ) {
		XGL_CALL( glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, mesh->buffers[ BUFFER_ELEMENT_DATA ] ) );
		XGL_CALL( glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof( unsigned int ) * mesh->num_indices, &mesh->indices[ 0 ], drawMode ) );
	}

	mesh->isDirty = false;
}

static void GLDeleteMesh( PLGMesh *mesh ) {
	if ( !XGL_VERSION( 2, 0 ) ) {
		return;
	}

	XGL_CALL( glDeleteBuffers( MAX_GPU_MESH_BUFFERS, mesh->buffers ) );
}

static void GLDrawInstancedMesh( PLGMesh *mesh, PLGShaderProgram *program, const PLMatrix4 *transforms, unsigned int instanceCount ) {
	if ( program == NULL ) {
		XGL_LOG( "no shader assigned!\n" );
		return;
	}

	if ( mesh->buffers[ BUFFER_VERTEX_DATA ] == 0 ) {
		XGL_LOG( "invalid buffer provided, skipping draw!\n" );
		return;
	}

	if ( mesh->primitiveScale != 0.0f ) {
		if ( mesh->primitive == PLG_MESH_LINES ) {
			XGL_CALL( glLineWidth( mesh->primitiveScale ) );
		} else if ( mesh->primitive == PLG_MESH_POINTS ) {
			XGL_CALL( glPointSize( mesh->primitiveScale ) );
		}
	}

	//Ensure VAO/VBO/EBO are bound
	XGL_CALL( glBindVertexArray( VAO[ 0 ] ) );

	XGL_CALL( glBindBuffer( GL_ARRAY_BUFFER, mesh->buffers[ BUFFER_VERTEX_DATA ] ) );
	XGL_CALL( glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, mesh->buffers[ BUFFER_ELEMENT_DATA ] ) );

	//draw
	GLuint mode = TranslatePrimitiveMode( mesh->primitive );
	if ( mesh->num_indices > 0 ) {
		XGL_CALL( glDrawElementsInstanced( mode, mesh->num_indices, GL_UNSIGNED_INT, 0, instanceCount ) );
	} else {
		XGL_CALL( glDrawArraysInstanced( mode, 0, mesh->num_verts, instanceCount ) );
	}

	if ( mesh->primitiveScale != 0.0f ) {
		if ( mesh->primitive == PLG_MESH_LINES ) {
			XGL_CALL( glLineWidth( 1.0f ) );
		} else if ( mesh->primitive == PLG_MESH_POINTS ) {
			XGL_CALL( glPointSize( 1.0f ) );
		}
	}
}

static void GLDrawMesh( PLGMesh *mesh, PLGShaderProgram *program ) {
	if ( program == NULL ) {
		XGL_LOG( "No shader assigned!\n" );
		return;
	}

	if ( mesh->primitiveScale != 0.0f ) {
		if ( mesh->primitive == PLG_MESH_LINES ) {
			XGL_CALL( glLineWidth( mesh->primitiveScale ) );
		} else if ( mesh->primitive == PLG_MESH_POINTS ) {
			XGL_CALL( glPointSize( mesh->primitiveScale ) );
		}
	}

	/* anything less and we'll just fallback to immediate */
	if ( !XGL_VERSION( 2, 0 ) ) {
		/* todo... */
		for ( unsigned int i = 0; i < program->num_stages; ++i ) {
			PLGShaderStage *stage = program->stages[ i ];
			if ( stage->SWFallback == NULL ) {
				continue;
			}

			stage->SWFallback( program, stage->type );

			GLuint mode = TranslatePrimitiveMode( mesh->primitive );
			XGL_CALL( glBegin( mode ) );
			if ( mode == GL_TRIANGLES ) {
				for ( unsigned int j = 0; j < mesh->num_indices; ++j ) {
					PLGVertex *vertex = &mesh->vertices[ mesh->indices[ j ] ];
					XGL_CALL( glVertex3f( vertex->position.x, vertex->position.y, vertex->position.z ) );
					XGL_CALL( glNormal3f( vertex->normal.x, vertex->normal.y, vertex->normal.z ) );
					XGL_CALL( glColor4ub( vertex->colour.r, vertex->colour.g, vertex->colour.b, vertex->colour.a ) );
				}
			} else {
				for ( unsigned int j = 0; j < mesh->num_verts; ++j ) {
					PLGVertex *vertex = &mesh->vertices[ j ];
					XGL_CALL( glVertex3f( vertex->position.x, vertex->position.y, vertex->position.z ) );
					XGL_CALL( glNormal3f( vertex->normal.x, vertex->normal.y, vertex->normal.z ) );
					XGL_CALL( glColor4ub( vertex->colour.r, vertex->colour.g, vertex->colour.b, vertex->colour.a ) );
				}
			}
			XGL_CALL( glEnd() );
		}
		return;
	}

	if ( mesh->buffers[ BUFFER_VERTEX_DATA ] == 0 ) {
		XGL_LOG( "invalid vertex buffer provided, skipping draw!\n" );
		return;
	} else if ( mesh->num_indices > 0 && mesh->buffers[ BUFFER_ELEMENT_DATA ] == 0 ) {
		XGL_LOG( "invalid element buffer provided, skipping draw!\n" );
		return;
	}

	//Ensure VAO/VBO/EBO are bound
	if ( XGL_VERSION( 3, 0 ) ) {
		XGL_CALL( glBindVertexArray( VAO[ 0 ] ) );
		/* todo: fallback for legacy... */
	}

	XGL_CALL( glBindBuffer( GL_ARRAY_BUFFER, mesh->buffers[ BUFFER_VERTEX_DATA ] ) );

	if ( program->internal.v_position != -1 ) {
		XGL_CALL( glEnableVertexAttribArray( program->internal.v_position ) );
		XGL_CALL( glVertexAttribPointer( program->internal.v_position, 3, GL_FLOAT, GL_FALSE, sizeof( PLGVertex ), ( const GLvoid * ) PL_OFFSETOF( PLGVertex, position ) ) );
	}
	if ( program->internal.v_normal != -1 ) {
		XGL_CALL( glEnableVertexAttribArray( program->internal.v_normal ) );
		XGL_CALL( glVertexAttribPointer( program->internal.v_normal, 3, GL_FLOAT, GL_FALSE, sizeof( PLGVertex ), ( const GLvoid * ) PL_OFFSETOF( PLGVertex, normal ) ) );
	}
	if ( program->internal.v_uv != -1 ) {
		XGL_CALL( glEnableVertexAttribArray( program->internal.v_uv ) );
		XGL_CALL( glVertexAttribPointer( program->internal.v_uv, 2, GL_FLOAT, GL_FALSE, sizeof( PLGVertex ), ( const GLvoid * ) PL_OFFSETOF( PLGVertex, st ) ) );
	}
	if ( program->internal.v_colour != -1 ) {
		XGL_CALL( glEnableVertexAttribArray( program->internal.v_colour ) );
		XGL_CALL( glVertexAttribPointer( program->internal.v_colour, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof( PLGVertex ), ( const GLvoid * ) PL_OFFSETOF( PLGVertex, colour ) ) );
	}
	if ( program->internal.v_tangent != -1 ) {
		XGL_CALL( glEnableVertexAttribArray( program->internal.v_tangent ) );
		XGL_CALL( glVertexAttribPointer( program->internal.v_tangent, 3, GL_FLOAT, GL_FALSE, sizeof( PLGVertex ), ( const GLvoid * ) PL_OFFSETOF( PLGVertex, tangent ) ) );
	}
	if ( program->internal.v_bitangent != -1 ) {
		XGL_CALL( glEnableVertexAttribArray( program->internal.v_bitangent ) );
		XGL_CALL( glVertexAttribPointer( program->internal.v_bitangent, 3, GL_FLOAT, GL_FALSE, sizeof( PLGVertex ), ( const GLvoid * ) PL_OFFSETOF( PLGVertex, bitangent ) ) );
	}

	//draw
	GLuint mode = TranslatePrimitiveMode( mesh->primitive );
	if ( mesh->num_indices > 0 ) {
		XGL_CALL( glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, mesh->buffers[ BUFFER_ELEMENT_DATA ] ) );
		XGL_CALL( glDrawElements( mode, mesh->num_indices, GL_UNSIGNED_INT, 0 ) );
	} else {
		XGL_CALL( glDrawArrays( mode, 0, mesh->num_verts ) );
	}

	if ( mesh->primitiveScale != 0.0f ) {
		if ( mesh->primitive == PLG_MESH_LINES ) {
			XGL_CALL( glLineWidth( 1.0f ) );
		} else if ( mesh->primitive == PLG_MESH_POINTS ) {
			XGL_CALL( glPointSize( 1.0f ) );
		}
	}
}

/////////////////////////////////////////////////////////////
// Viewport

static void GLSetViewport( int x, int y, int width, int height ) {
	XGL_CALL( glViewport( x, y, width, height ) );
	XGL_CALL( glScissor( x, y, width, height ) );
}

/////////////////////////////////////////////////////////////
// Shader

#define SHADER_INVALID_TYPE ( ( uint32_t ) 0 - 1 )

static const char *uniformDescriptors[ PLG_MAX_UNIFORM_TYPES ] = {
        [PLG_INVALID_UNIFORM] = "invalid",
        [PLG_UNIFORM_FLOAT] = "float",
        [PLG_UNIFORM_INT] = "int",
        [PLG_UNIFORM_UINT] = "uint",
        [PLG_UNIFORM_BOOL] = "bool",
        [PLG_UNIFORM_DOUBLE] = "double",
        [PLG_UNIFORM_SAMPLER1D] = "sampler1D",
        [PLG_UNIFORM_SAMPLER2D] = "sampler2D",
        [PLG_UNIFORM_SAMPLER3D] = "sampler3D",
        [PLG_UNIFORM_SAMPLERCUBE] = "samplerCube",
        [PLG_UNIFORM_SAMPLER1DSHADOW] = "sampler1DShadow",
        [PLG_UNIFORM_VEC2] = "vec2",
        [PLG_UNIFORM_VEC3] = "vec3",
        [PLG_UNIFORM_VEC4] = "vec4",
        [PLG_UNIFORM_MAT3] = "mat3",
        [PLG_UNIFORM_MAT4] = "mat4",
};

static PLGShaderUniformType GLConvertGLUniformType( unsigned int type ) {
	switch ( type ) {
		case GL_FLOAT:
			return PLG_UNIFORM_FLOAT;
		case GL_FLOAT_VEC2:
			return PLG_UNIFORM_VEC2;
		case GL_FLOAT_VEC3:
			return PLG_UNIFORM_VEC3;
		case GL_FLOAT_VEC4:
			return PLG_UNIFORM_VEC4;
		case GL_FLOAT_MAT3:
			return PLG_UNIFORM_MAT3;
		case GL_FLOAT_MAT4:
			return PLG_UNIFORM_MAT4;

		case GL_DOUBLE:
			return PLG_UNIFORM_DOUBLE;

		case GL_INT:
			return PLG_UNIFORM_INT;
		case GL_UNSIGNED_INT:
			return PLG_UNIFORM_UINT;

		case GL_BOOL:
			return PLG_UNIFORM_BOOL;

		case GL_SAMPLER_1D:
			return PLG_UNIFORM_SAMPLER1D;
		case GL_SAMPLER_1D_SHADOW:
			return PLG_UNIFORM_SAMPLER1DSHADOW;
		case GL_SAMPLER_2D:
			return PLG_UNIFORM_SAMPLER2D;
		case GL_SAMPLER_2D_SHADOW:
			return PLG_UNIFORM_SAMPLER2DSHADOW;

		default: {
			XGL_LOG( "Unhandled GLSL data type, \"%u\"!\n", type );
			return PLG_INVALID_UNIFORM;
		}
	}
}

static GLenum TranslateShaderStageType( PLGShaderStageType type ) {
	switch ( type ) {
		case PLG_SHADER_TYPE_VERTEX:
			return GL_VERTEX_SHADER;
		case PLG_SHADER_TYPE_COMPUTE:
			return GL_COMPUTE_SHADER;
		case PLG_SHADER_TYPE_FRAGMENT:
			return GL_FRAGMENT_SHADER;
		case PLG_SHADER_TYPE_GEOMETRY:
			return GL_GEOMETRY_SHADER;
		default:
			return SHADER_INVALID_TYPE;
	}
}

static const char *GetGLShaderStageDescriptor( GLenum type ) {
	switch ( type ) {
		case GL_VERTEX_SHADER:
			return "GL_VERTEX_SHADER";
		case GL_COMPUTE_SHADER:
			return "GL_COMPUTE_SHADER";
		case GL_FRAGMENT_SHADER:
			return "GL_FRAGMENT_SHADER";
		case GL_GEOMETRY_SHADER:
			return "GL_GEOMETRY_SHADER";
		default:
			return "unknown";
	}
}

static GLenum TranslateShaderUniformType( PLGShaderUniformType type ) {
	switch ( type ) {
		case PLG_UNIFORM_BOOL:
			return GL_BOOL;
		case PLG_UNIFORM_DOUBLE:
			return GL_DOUBLE;
		case PLG_UNIFORM_FLOAT:
			return GL_FLOAT;
		case PLG_UNIFORM_INT:
			return GL_INT;
		case PLG_UNIFORM_UINT:
			return GL_UNSIGNED_INT;

		case PLG_UNIFORM_SAMPLER1D:
			return GL_SAMPLER_1D;
		case PLG_UNIFORM_SAMPLER1DSHADOW:
			return GL_SAMPLER_1D_SHADOW;
		case PLG_UNIFORM_SAMPLER2D:
			return GL_SAMPLER_2D;
		case PLG_UNIFORM_SAMPLER2DSHADOW:
			return GL_SAMPLER_2D_SHADOW;
		case PLG_UNIFORM_SAMPLER3D:
			return GL_SAMPLER_3D;
		case PLG_UNIFORM_SAMPLERCUBE:
			return GL_SAMPLER_CUBE;

		case PLG_UNIFORM_VEC2:
			return GL_FLOAT_VEC2;
		case PLG_UNIFORM_VEC3:
			return GL_FLOAT_VEC3;
		case PLG_UNIFORM_VEC4:
			return GL_FLOAT_VEC4;

		case PLG_UNIFORM_MAT3:
			return GL_FLOAT_MAT3;

		default:
			return SHADER_INVALID_TYPE;
	}
}

static unsigned int TranslateGLShaderUniformType( GLenum type ) {
	switch ( type ) {
		case GL_BOOL:
			return PLG_UNIFORM_BOOL;
		case GL_DOUBLE:
			return PLG_UNIFORM_DOUBLE;
		case GL_FLOAT:
			return PLG_UNIFORM_FLOAT;
		case GL_INT:
			return PLG_UNIFORM_INT;
		case GL_UNSIGNED_INT:
			return PLG_UNIFORM_UINT;

		case GL_SAMPLER_1D:
			return PLG_UNIFORM_SAMPLER1D;
		case GL_SAMPLER_1D_SHADOW:
			return PLG_UNIFORM_SAMPLER1DSHADOW;
		case GL_SAMPLER_2D:
			return PLG_UNIFORM_SAMPLER2D;
		case GL_SAMPLER_2D_SHADOW:
			return PLG_UNIFORM_SAMPLER2DSHADOW;
		case GL_SAMPLER_3D:
			return PLG_UNIFORM_SAMPLER3D;
		case GL_SAMPLER_CUBE:
			return PLG_UNIFORM_SAMPLERCUBE;

		case GL_FLOAT_VEC2:
			return PLG_UNIFORM_VEC2;
		case GL_FLOAT_VEC3:
			return PLG_UNIFORM_VEC3;
		case GL_FLOAT_VEC4:
			return PLG_UNIFORM_VEC4;

		case GL_FLOAT_MAT3:
			return PLG_UNIFORM_MAT3;

		default:
			return SHADER_INVALID_TYPE;
	}
}

/**
 * Inserts the given string into an existing string buffer.
 * Automatically reallocs buffer if it doesn't fit.
 * todo: consider cleaning this up and making part of API?
 */
static char *InsertString( const char *string, char **buf, size_t *bufSize, size_t *maxBufSize ) {
	/* check if it's going to fit first */
	size_t strLength = strlen( string );
	size_t originalSize = *bufSize;
	*bufSize += strLength;
	if ( *bufSize >= *maxBufSize ) {
		*maxBufSize = *bufSize + strLength;
		*buf = gInterface->core->ReAlloc( *buf, *maxBufSize + 1, true );
	}

	/* now copy it into our buffer */
	strncpy( *buf + originalSize, string, strLength );

	return *buf + originalSize + strLength;
}

/**
 * A basic pre-processor for GLSL - will condense the shader as much as possible
 * and handle any pre-processor commands.
 * todo: this is dumb... rewrite it
 */
static char *GLPreProcessGLSLShader( PLGShaderStage *stage, char *buf, size_t *length, bool head, const char *directory ) {
	/* setup the destination buffer */
	size_t actualLength = 0;
	size_t maxLength = *length;
	char *dstBuffer = gInterface->core->MAlloc( maxLength + 1, true );
	char *dstPos = dstBuffer;

	/* built-ins */
#define insert( str ) dstPos = InsertString( ( str ), &dstBuffer, &actualLength, &maxLength )
	if ( head ) {
		insert( "#version 150 core\n" );//OpenGL 3.2 == GLSL 150
		insert( "uniform mat4 pl_model;\n" );
		insert( "uniform mat4 pl_view;\n" );
		insert( "uniform mat4 pl_proj;\n" );
		if ( stage->type == PLG_SHADER_TYPE_VERTEX ) {
			insert( "in vec3 pl_vposition;\n" );
			insert( "in vec3 pl_vnormal;\n" );
			insert( "in vec2 pl_vuv;\n" );
			insert( "in vec4 pl_vcolour;\n" );
			insert( "in vec3 pl_vtangent, pl_vbitangent;\n" );
			insert( "#define PLG_COMPILE_VERTEX 1\n" );
		} else if ( stage->type == PLG_SHADER_TYPE_FRAGMENT ) {
			insert( "out vec4 pl_frag;\n" );
			insert( "#define PLG_COMPILE_FRAGMENT 1\n" );
		}
		for ( unsigned int i = 0; i < stage->numDefinitions; ++i ) {
			char line[ 32 ];
			snprintf( line, sizeof( line ), "#define %s 1\n", stage->definitions[ i ] );
			insert( line );
		}
	}

	const char *srcPos = buf;
	const char *srcEnd = buf + *length;
	while ( srcPos < srcEnd ) {
		if ( *srcPos == '\0' ) {
			break;
		}

		if ( *srcPos == '#' ) {
			const char *p = srcPos;
			srcPos++;

			char token[ 32 ];
			gInterface->core->ParseToken( &srcPos, token, sizeof( token ) );
			if ( strcmp( token, "include" ) == 0 ) {
				gInterface->core->SkipWhitespace( &srcPos );

				/* pull the path - needs to be enclosed otherwise this'll fail */
				PLPath path;
				gInterface->core->ParseEnclosedString( &srcPos, path, sizeof( path ) );
				if ( directory != NULL ) {
					PLPath tmp;
					gInterface->core->SetupPath( tmp, true, "%s/%s", directory, path );
					strcpy( path, tmp );
				}

				PLFile *file = gInterface->core->OpenFile( path, true );
				if ( file != NULL ) {
					/* allocate a temporary buffer */
					size_t incLength = gInterface->core->GetFileSize( file );
					char *incBuf = gInterface->core->MAlloc( incLength + 1, true );
					memcpy( incBuf, gInterface->core->GetFileData( file ), incLength );

					/* close the current file, to avoid recursively opening files
					 * and hitting any limits */
					gInterface->core->CloseFile( file );

					PLPath tmp;
					gInterface->core->SetupPath( tmp, true, "%s", path );
					char *sep = strrchr( tmp, '/' );
					if ( sep != NULL ) {
						*sep = '\0';
					}

					/* now throw it into the pre-processor */
					incBuf = GLPreProcessGLSLShader( stage, incBuf, &incLength, false, tmp );

					/* and finally, push it into our destination */
					dstPos = InsertString( incBuf, &dstBuffer, &actualLength, &maxLength );
					gInterface->core->Free( incBuf );
				} else {
					XGL_LOG( "Failed to load include \"%s\": %s\n", gInterface->core->GetError() );
				}

				gInterface->core->SkipLine( &srcPos );
				continue;
			}

			/* we didn't need to do anything, so restore our position */
			srcPos = p;
		}

		if ( ++actualLength > maxLength ) {
			maxLength += 256;
			char *oldDstBuffer = dstBuffer;
			dstBuffer = gInterface->core->ReAlloc( dstBuffer, maxLength + 1, true );
			dstPos = dstBuffer + ( dstPos - oldDstBuffer );
		}

		*dstPos++ = *srcPos++;
	}

	/* free the original buffer that was passed in */
	gInterface->core->Free( buf );

	/* resize and update buf to match */
	*length = actualLength;
	dstBuffer[ *length ] = '\0';

	return dstBuffer;
}

static void GLCreateShaderProgram( PLGShaderProgram *program ) {
	if ( !XGL_VERSION( 2, 0 ) ) {
		XGL_LOG( "HW shaders unsupported on platform, relying on SW fallback\n" );
		return;
	}

	program->internal.id = glCreateProgram();
	if ( program->internal.id == 0 ) {
		XGL_LOG( "Failed to generate shader program!\n" );
		return;
	}
}

static void GLDestroyShaderProgram( PLGShaderProgram *program ) {
	if ( program->internal.id == 0 ) {
		return;
	}

	if ( XGL_VERSION( 2, 0 ) ) {
		glDeleteProgram( program->internal.id );
	}

	program->internal.id = 0;
}

static void GLCreateShaderStage( PLGShaderStage *stage ) {
	if ( !XGL_VERSION( 2, 0 ) ) {
		XGL_LOG( "HW shaders unsupported on platform, relying on SW fallback\n" );
		return;
	}

	GLenum type = TranslateShaderStageType( stage->type );
	if ( type == SHADER_INVALID_TYPE ) {
		gInterface->core->ReportError( PL_RESULT_INVALID_SHADER_TYPE, PL_FUNCTION, "%u", type );
		return;
	}

	if ( type == GL_GEOMETRY_SHADER && !XGL_VERSION( 3, 0 ) ) {
		gInterface->core->ReportError( PL_RESULT_UNSUPPORTED_SHADER_TYPE, PL_FUNCTION, "%s", GetGLShaderStageDescriptor( type ) );
		return;
	}

	if ( type == GL_COMPUTE_SHADER && !XGL_VERSION( 4, 3 ) ) {
		gInterface->core->ReportError( PL_RESULT_UNSUPPORTED_SHADER_TYPE, PL_FUNCTION, "%s", GetGLShaderStageDescriptor( type ) );
		return;
	}

	stage->internal.id = glCreateShader( type );
	if ( stage->internal.id == 0 ) {
		gInterface->core->ReportError( PL_RESULT_INVALID_SHADER_TYPE, PL_FUNCTION, "%u", type );
		return;
	}
}

static void GLDestroyShaderStage( PLGShaderStage *stage ) {
	if ( !XGL_VERSION( 2, 0 ) ) {
		return;
	}

	if ( stage->program != NULL ) {
		XGL_CALL( glDetachShader( stage->program->internal.id, stage->internal.id ) );
		stage->program = NULL;
	}

	XGL_CALL( glDeleteShader( stage->internal.id ) );
	stage->internal.id = 0;
}

static void GLAttachShaderStage( PLGShaderProgram *program, PLGShaderStage *stage ) {
	if ( !XGL_VERSION( 2, 0 ) ) {
		return;
	}

	XGL_CALL( glAttachShader( program->internal.id, stage->internal.id ) );
}

static void GLCompileShaderStage( PLGShaderStage *stage, const char *buf, size_t length, const char *directory ) {
	if ( !XGL_VERSION( 2, 0 ) ) {
		return;
	}

	/* shove this here for now... */
	char *temp = gInterface->core->MAlloc( length + 1, true );
	memcpy( temp, buf, length );

	temp = GLPreProcessGLSLShader( stage, temp, &length, true, directory );

	XGL_CALL( glShaderSource( stage->internal.id, 1, ( const GLchar ** ) &temp, ( GLint * ) &length ) );
	XGL_CALL( glCompileShader( stage->internal.id ) );

	int status;
	XGL_CALL( glGetShaderiv( stage->internal.id, GL_COMPILE_STATUS, &status ) );
	if ( status == 0 ) {
		int s_length;
		XGL_CALL( glGetShaderiv( stage->internal.id, GL_INFO_LOG_LENGTH, &s_length ) );
		if ( s_length > 1 ) {
			char *log = gInterface->core->CAlloc( ( size_t ) s_length, sizeof( char ), true );
			XGL_CALL( glGetShaderInfoLog( stage->internal.id, s_length, NULL, log ) );
			XGL_LOG( " COMPILE ERROR:\n%s\n", log );
			gInterface->core->ReportError( PL_RESULT_SHADER_COMPILE, "%s", log );
			gInterface->core->Free( log );
		}
	}

	gInterface->core->Free( temp );
}

static void GLSetShaderUniformValue( PLGShaderProgram *program, int slot, const void *value, bool transpose ) {
	switch ( program->uniforms[ slot ].type ) {
		case PLG_UNIFORM_FLOAT:
			XGL_CALL( glUniform1f( program->uniforms[ slot ].slot, *( float * ) value ) );
			break;
		case PLG_UNIFORM_SAMPLER2D:
		case PLG_UNIFORM_INT:
			XGL_CALL( glUniform1i( program->uniforms[ slot ].slot, *( int * ) value ) );
			break;
		case PLG_UNIFORM_UINT:
			XGL_CALL( glUniform1ui( program->uniforms[ slot ].slot, *( unsigned int * ) value ) );
			break;
		case PLG_UNIFORM_BOOL:
			XGL_CALL( glUniform1i( program->uniforms[ slot ].slot, *( bool * ) value ) );
			break;
		case PLG_UNIFORM_DOUBLE:
			XGL_CALL( glUniform1d( program->uniforms[ slot ].slot, *( double * ) value ) );
			break;
		case PLG_UNIFORM_VEC2:
			XGL_CALL( glUniform2fv( program->uniforms[ slot ].slot, 1, value ) );
			break;
		case PLG_UNIFORM_VEC3:
			XGL_CALL( glUniform3fv( program->uniforms[ slot ].slot, 1, value ) );
			break;
		case PLG_UNIFORM_VEC4:
			XGL_CALL( glUniform4fv( program->uniforms[ slot ].slot, 1, value ) );
			break;
		case PLG_UNIFORM_MAT3: {
			PLMatrix3 mat3 = *( PLMatrix3 * ) value;
			XGL_CALL( glUniformMatrix3fv( program->uniforms[ slot ].slot, 1, transpose ? GL_TRUE : GL_FALSE, mat3.m ) );
			break;
		}
		case PLG_UNIFORM_MAT4: {
			PLMatrix4 mat4 = *( PLMatrix4 * ) value;
			XGL_CALL( glUniformMatrix4fv( program->uniforms[ slot ].slot, 1, transpose ? GL_TRUE : GL_FALSE, mat4.m ) );
			break;
		}
		default:
			break;
	}
}

static void GLSetShaderUniformMatrix4( PLGShaderProgram *program, int slot, PLMatrix4 value, bool transpose ) {
	PL_UNUSEDVAR( program );

	if ( !XGL_VERSION( 2, 0 ) ) {
		return;
	}

	XGL_CALL( glUniformMatrix4fv( slot, 1, transpose ? GL_TRUE : GL_FALSE, value.m ) );
}

static void RegisterShaderProgramData( PLGShaderProgram *program ) {
	if ( program->uniforms != NULL ) {
		XGL_LOG( "Uniforms have already been initialised!\n" );
		return;
	}

	XGL_CALL( program->internal.v_position = glGetAttribLocation( program->internal.id, "pl_vposition" ) );
	XGL_CALL( program->internal.v_normal = glGetAttribLocation( program->internal.id, "pl_vnormal" ) );
	XGL_CALL( program->internal.v_uv = glGetAttribLocation( program->internal.id, "pl_vuv" ) );
	XGL_CALL( program->internal.v_colour = glGetAttribLocation( program->internal.id, "pl_vcolour" ) );
	XGL_CALL( program->internal.v_tangent = glGetAttribLocation( program->internal.id, "pl_vtangent" ) );
	XGL_CALL( program->internal.v_bitangent = glGetAttribLocation( program->internal.id, "pl_vbitangent" ) );

	int num_uniforms = 0;
	XGL_CALL( glGetProgramiv( program->internal.id, GL_ACTIVE_UNIFORMS, &num_uniforms ) );
	if ( num_uniforms <= 0 ) {
		/* true, because technically this isn't a fault - there just aren't any */
		XGL_LOG( "No uniforms found in shader program...\n" );
		return;
	}
	program->num_uniforms = ( unsigned int ) num_uniforms;

#if !defined( NDEBUG )
	XGL_LOG( "Found %u uniforms in shader\n", program->num_uniforms );
#endif

	program->uniforms = gInterface->core->CAlloc( ( size_t ) program->num_uniforms, sizeof( *program->uniforms ), true );
	unsigned int registered = 0;
	for ( unsigned int i = 0; i < program->num_uniforms; ++i ) {
		int maxUniformNameLength;
		XGL_CALL( glGetActiveUniformsiv( program->internal.id, 1, &i, GL_UNIFORM_NAME_LENGTH, &maxUniformNameLength ) );

		GLchar *uniformName = gInterface->core->MAlloc( maxUniformNameLength, true );
		GLsizei nameLength;

		GLenum glType;
		GLint uniformSize;

		XGL_CALL( glGetActiveUniform( program->internal.id, i, maxUniformNameLength, &nameLength, &uniformSize, &glType, uniformName ) );
		if ( nameLength == 0 ) {
			gInterface->core->Free( uniformName );

			XGL_LOG( "No information available for uniform %d!\n", i );
			continue;
		}

		XGL_CALL( program->uniforms[ i ].slot = glGetUniformLocation( program->internal.id, uniformName ) );

		program->uniforms[ i ].type = GLConvertGLUniformType( glType );
		program->uniforms[ i ].name = uniformName;

		/* fetch it's current value, assume it's the default */
		switch ( program->uniforms[ i ].type ) {
			case PLG_UNIFORM_FLOAT:
				XGL_CALL( glGetUniformfv( program->internal.id, program->uniforms[ i ].slot, &program->uniforms[ i ].defaultFloat ) );
				break;
			case PLG_UNIFORM_SAMPLER2D:
			case PLG_UNIFORM_INT:
				XGL_CALL( glGetUniformiv( program->internal.id, program->uniforms[ i ].slot, &program->uniforms[ i ].defaultInt ) );
				break;
			case PLG_UNIFORM_UINT:
				XGL_CALL( glGetUniformuiv( program->internal.id, program->uniforms[ i ].slot, &program->uniforms[ i ].defaultUInt ) );
				break;
			case PLG_UNIFORM_BOOL:
				XGL_CALL( glGetUniformiv( program->internal.id, program->uniforms[ i ].slot, ( GLint * ) &program->uniforms[ i ].defaultBool ) );
				break;
			case PLG_UNIFORM_DOUBLE:
				XGL_CALL( glGetUniformdv( program->internal.id, program->uniforms[ i ].slot, &program->uniforms[ i ].defaultDouble ) );
				break;
			case PLG_UNIFORM_VEC2:
				XGL_CALL( glGetUniformfv( program->internal.id, program->uniforms[ i ].slot, ( GLfloat * ) &program->uniforms[ i ].defaultVec2 ) );
				break;
			case PLG_UNIFORM_VEC3:
				XGL_CALL( glGetUniformfv( program->internal.id, program->uniforms[ i ].slot, ( GLfloat * ) &program->uniforms[ i ].defaultVec3 ) );
				break;
			case PLG_UNIFORM_VEC4:
				XGL_CALL( glGetUniformfv( program->internal.id, program->uniforms[ i ].slot, ( GLfloat * ) &program->uniforms[ i ].defaultVec4 ) );
				break;
			case PLG_UNIFORM_MAT3:
				XGL_CALL( glGetUniformfv( program->internal.id, program->uniforms[ i ].slot, ( GLfloat * ) &program->uniforms[ i ].defaultMat3 ) );
				break;
			case PLG_UNIFORM_MAT4:
				XGL_CALL( glGetUniformfv( program->internal.id, program->uniforms[ i ].slot, ( GLfloat * ) &program->uniforms[ i ].defaultMat4 ) );
				break;
			default:
				break;
		}

		//GLLog( " %4d (%20s) %s\n", i, program->uniforms[ i ].name, uniformDescriptors[ program->uniforms[ i ].type ] );

		registered++;
	}

	if ( registered == 0 ) {
		XGL_LOG( "Failed to validate any shader program uniforms!\n" );
	}
}

static void GLSetShaderProgram( PLGShaderProgram *program ) {
	if ( !XGL_VERSION( 2, 0 ) ) {
		return;
	}

	unsigned int id = 0;
	if ( program != NULL ) {
		id = program->internal.id;
	}

	XGL_CALL( glUseProgram( id ) );
}

#define SHADER_CACHE_MAGIC PL_MAGIC_TO_NUM( 'G', 'L', 'S', 'B' )

typedef struct ShaderCacheHeader {
	uint32_t magic;
	uint64_t checksum;
	uint32_t format;
	uint32_t length;
} ShaderCacheHeader;

static void CacheShaderProgram( PLGShaderProgram *program ) {
	const char *cacheLocation = gInterface->GetShaderCacheLocation();
	if ( *cacheLocation == '\0' ) {
		return;
	}

	if ( !XGL_VERSION( 4, 1 ) && !GLEW_ARB_get_program_binary ) {
		XGL_LOG( "Shader cache unsupported, skipping.\n" );
		return;
	}

	if ( *program->id == '\0' ) {
		XGL_LOG( "No valid ID provided for program, skipping caching.\n" );
		return;
	}

	PLPath path;
	snprintf( path, sizeof( path ), "%s%s.glb", cacheLocation, program->id );
	if ( gInterface->core->LocalFileExists( path ) ) {
		XGL_LOG( "Program has already been cached, skipping.\n" );
		return;
	}

	int length;
	XGL_CALL( glGetProgramiv( program->internal.id, GL_PROGRAM_BINARY_LENGTH, &length ) );

	uint32_t format;
	void *buf = gInterface->core->MAlloc( length, true );
	XGL_CALL( glGetProgramBinary( program->internal.id, length, NULL, &format, buf ) );

	unsigned long checksum = gInterface->core->GenerateChecksumCRC32( buf, length, 0 );

	FILE *file = fopen( path, "wb" );
	if ( file == NULL ) {
		XGL_LOG( "Failed to open write location: %s\n", path );
		return;
	}

	ShaderCacheHeader header;
	header.magic = SHADER_CACHE_MAGIC;
	header.checksum = checksum;
	header.format = format;
	header.length = length;

	fwrite( &header, sizeof( ShaderCacheHeader ), 1, file );
	fwrite( buf, sizeof( char ), length, file );

	fclose( file );
}

static void GLLinkShaderProgram( PLGShaderProgram *program ) {
	if ( !XGL_VERSION( 2, 0 ) ) {
		gInterface->core->ReportError( PL_RESULT_SHADER_COMPILE, PL_FUNCTION, "unsupported" );
		return;
	}

	XGL_CALL( glLinkProgram( program->internal.id ) );

	int status;
	XGL_CALL( glGetProgramiv( program->internal.id, GL_LINK_STATUS, &status ) );
	if ( status == 0 ) {
		int s_length;
		XGL_CALL( glGetProgramiv( program->internal.id, GL_INFO_LOG_LENGTH, &s_length ) );
		if ( s_length > 1 ) {
			char *log = gInterface->core->CAlloc( ( size_t ) s_length, sizeof( char ), true );
			XGL_CALL( glGetProgramInfoLog( program->internal.id, s_length, NULL, log ) );
			XGL_LOG( " LINK ERROR:\n%s\n", log );
			gInterface->core->Free( log );
			gInterface->core->ReportError( PL_RESULT_SHADER_COMPILE, PL_FUNCTION, log );
		} else {
			XGL_LOG( " UNKNOWN LINK ERROR!\n" );
			gInterface->core->ReportError( PL_RESULT_SHADER_COMPILE, PL_FUNCTION, "unknown error" );
		}

		return;
	}

	program->is_linked = true;

	CacheShaderProgram( program );
	RegisterShaderProgramData( program );
}

/////////////////////////////////////////////////////////////
// Generic State Management

unsigned int TranslateGraphicsState( PLGDrawState state ) {
	switch ( state ) {
		default:
			break;
		case PLG_GFX_STATE_FOG:
			if ( XGL_VERSION( 3, 0 ) ) {
				return 0;
			}
			return GL_FOG;
		case PLG_GFX_STATE_ALPHATEST:
			if ( XGL_VERSION( 3, 0 ) ) {
				return 0;
			}
			return GL_ALPHA_TEST;
		case PLG_GFX_STATE_BLEND:
			return GL_BLEND;
		case PLG_GFX_STATE_DEPTHTEST:
			return GL_DEPTH_TEST;
		case PLG_GFX_STATE_STENCILTEST:
			return GL_STENCIL_TEST;
		case PLG_GFX_STATE_MULTISAMPLE:
			return GL_MULTISAMPLE;
		case PLG_GFX_STATE_SCISSORTEST:
			return GL_SCISSOR_TEST;
		case PLG_GFX_STATE_ALPHATOCOVERAGE:
			return GL_SAMPLE_ALPHA_TO_COVERAGE;
		case PLG_GFX_STATE_DEPTH_CLAMP:
			return GL_DEPTH_CLAMP;
	}

	return 0;
}

void GLEnableState( PLGDrawState state ) {
	unsigned int gl_state = TranslateGraphicsState( state );
	if ( !gl_state ) {
		if ( state == PLG_GFX_STATE_WIREFRAME ) {
			XGL_CALL( glPolygonMode( GL_FRONT_AND_BACK, GL_LINE ) );
		}

		/* probably unsupported */
		return;
	}

	XGL_CALL( glEnable( gl_state ) );
}

void GLDisableState( PLGDrawState state ) {
	unsigned int gl_state = TranslateGraphicsState( state );
	if ( !gl_state ) {
		if ( state == PLG_GFX_STATE_WIREFRAME ) {
			XGL_CALL( glPolygonMode( GL_FRONT_AND_BACK, GL_FILL ) );
		}

		/* probably unsupported */
		return;
	}

	XGL_CALL( glDisable( gl_state ) );
}

/////////////////////////////////////////////////////////////

static char gl_extensions[ 4096 ][ 4096 ] = { { '\0' } };

#if defined( DEBUG_GL )
static void MessageCallback(
        GLenum source,
        GLenum type,
        GLuint id,
        GLenum severity,
        GLsizei length,
        const GLchar *message,
        void *param ) {
	PL_UNUSEDVAR( source );
	PL_UNUSEDVAR( id );
	PL_UNUSEDVAR( length );
	PL_UNUSEDVAR( param );

	const char *s_severity;
	switch ( severity ) {
		case GL_DEBUG_SEVERITY_HIGH:
			s_severity = "HIGH";
			break;
		case GL_DEBUG_SEVERITY_MEDIUM:
			s_severity = "MEDIUM";
			break;
		case GL_DEBUG_SEVERITY_LOW:
			s_severity = "LOW";
			break;

		default:
			return;
	}

	const char *s_type;
	switch ( type ) {
		case GL_DEBUG_TYPE_ERROR:
			s_type = "ERROR";
			break;
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
			s_type = "DEPRECATED";
			break;
		case GL_DEBUG_TYPE_MARKER:
			s_type = "MARKER";
			break;
		case GL_DEBUG_TYPE_PERFORMANCE:
			s_type = "PERFORMANCE";
			break;
		case GL_DEBUG_TYPE_PORTABILITY:
			s_type = "PORTABILITY";
			break;
		default:
			s_type = "OTHER";
			break;
	}

	if ( message != NULL && message[ 0 ] != '\0' ) {
		XGL_LOG( "(%s) %s - %s\n", s_type, s_severity, message );
	}
}
#endif

PLFunctionResult GLInitialize( void ) {
	glewExperimental = true;
	GLenum err = glewInit();
	if ( err != GLEW_OK ) {
		gInterface->core->ReportError( PL_RESULT_GRAPHICSINIT, "failed to initialize glew, %s", ( char * ) glewGetErrorString( err ) );
		return PL_RESULT_GRAPHICSINIT;
	}

	memset( &gl_capabilities, 0, sizeof( gl_capabilities ) );

	const char *version = ( const char * ) glGetString( GL_VERSION );
	gl_version_major = ( version[ 0 ] - '0' );
	gl_version_minor = ( version[ 2 ] - '0' );

	if ( XGL_VERSION( 3, 0 ) ) {
		int minor, major;
		glGetIntegerv( GL_MAJOR_VERSION, &major );
		glGetIntegerv( GL_MINOR_VERSION, &minor );
		if ( major > 0 ) {
			gl_version_major = major;
			gl_version_minor = minor;
		} else {
			XGL_LOG( "failed to get OpenGL version, expect some functionality not to work!\n" );
		}
	}

	XGL_LOG( " OpenGL %d.%d\n", gl_version_major, gl_version_minor );
	XGL_LOG( "  renderer:   %s\n", ( const char * ) glGetString( GL_RENDERER ) );
	XGL_LOG( "  vendor:     %s\n", ( const char * ) glGetString( GL_VENDOR ) );
	XGL_LOG( "  version:    %s\n", version );
	//GLLog( "  extensions:\n" );

	XGL_CALL( glGetIntegerv( GL_NUM_EXTENSIONS, ( GLint * ) ( &gl_num_extensions ) ) );
	for ( unsigned int i = 0; i < gl_num_extensions; ++i ) {
		const uint8_t *extension = glGetStringi( GL_EXTENSIONS, i );
		snprintf( gl_extensions[ i ], sizeof( gl_extensions[ i ] ), "%s", extension );
		//GLLog( "    %s\n", extension );
	}

#if defined( DEBUG_GL )
	if ( XGL_VERSION( 4, 3 ) ) {
		XGL_CALL( glEnable( GL_DEBUG_OUTPUT ) );
		XGL_CALL( glEnable( GL_DEBUG_OUTPUT_SYNCHRONOUS ) );

		XGL_CALL( glDebugMessageControl( GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE ) );
		XGL_CALL( glDebugMessageCallback( ( GLDEBUGPROC ) MessageCallback, NULL ) );
	}
#endif

	// Init vertex attributes
	XGL_CALL( glGenVertexArrays( 1, VAO ) );
	XGL_CALL( glBindVertexArray( VAO[ 0 ] ) );

#if 0
	/* in OpenGL, multisample is automatically enabled per spec */
	gfx_state.current_capabilities[ PL_GFX_STATE_MULTISAMPLE ] = true;
#endif

	return PL_RESULT_SUCCESS;
}

void GLShutdown( void ) {
#if defined( DEBUG_GL )
	if ( XGL_VERSION( 4, 3 ) ) {
		XGL_CALL( glDisable( GL_DEBUG_OUTPUT ) );
		XGL_CALL( glDisable( GL_DEBUG_OUTPUT_SYNCHRONOUS ) );
	}
#endif
}

PLGDriverImportTable graphicsInterface = {
        .Initialize = GLInitialize,
        .Shutdown = GLShutdown,

        .InsertDebugMarker = GLInsertDebugMarker,
        .PushDebugGroupMarker = GLPushDebugGroupMarker,
        .PopDebugGroupMarker = GLPopDebugGroupMarker,

        .SupportsHWShaders = GLSupportsHWShaders,
        .GetMaxTextureUnits = GLGetMaxTextureUnits,
        .GetMaxTextureSize = GLGetMaxTextureSize,

        .EnableState = GLEnableState,
        .DisableState = GLDisableState,

        .SetBlendMode = GLSetBlendMode,
        .SetCullMode = GLSetCullMode,

        .SetClearColour = GLSetClearColour,
        .ClearBuffers = GLClearBuffers,

        //.DrawPixel = ,

        .SetDepthBufferMode = GLSetDepthBufferMode,
        .SetDepthMask = GLSetDepthMask,

        .CreateMesh = GLCreateMesh,
        .UploadMesh = GLUploadMesh,
        .DrawMesh = GLDrawMesh,
        .DrawInstancedMesh = GLDrawInstancedMesh,
        .DeleteMesh = GLDeleteMesh,

        .CreateFrameBuffer = GLCreateFrameBuffer,
        .DeleteFrameBuffer = GLDeleteFrameBuffer,
        .BindFrameBuffer = GLBindFrameBuffer,
        .GetFrameBufferTextureAttachment = GLGetFrameBufferTextureAttachment,
        .BlitFrameBuffers = GLBlitFrameBuffers,
        .SetFrameBufferSize = GLSetFrameBufferSize,

        .CreateTexture = GLCreateTexture,
        .DeleteTexture = GLDeleteTexture,
        .BindTexture = GLBindTexture,
        .UploadTexture = GLUploadTexture,
        .SwizzleTexture = GLSwizzleTexture,
        .SetTextureAnisotropy = GLSetTextureAnisotropy,
        .ActiveTexture = GLActiveTexture,
        .SetTextureFilter = GLSetTextureFilter,

        .SetViewport = GLSetViewport,

        .CreateShaderProgram = GLCreateShaderProgram,
        .DestroyShaderProgram = GLDestroyShaderProgram,
        .AttachShaderStage = GLAttachShaderStage,
        //.DetachShaderStage = ,
        .LinkShaderProgram = GLLinkShaderProgram,
        .SetShaderProgram = GLSetShaderProgram,
        .CreateShaderStage = GLCreateShaderStage,
        .DestroyShaderStage = GLDestroyShaderStage,
        .CompileShaderStage = GLCompileShaderStage,
        .SetShaderUniformValue = GLSetShaderUniformValue,

        //.StencilFunction = ,
};
