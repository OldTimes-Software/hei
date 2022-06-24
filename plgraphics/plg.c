/**
 * Hei Platform Library
 * Copyright (C) 2017-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is licensed under MIT. See LICENSE for more details.
 */

#include <plcore/pl_console.h>
#include <plcore/pl_image.h>
#include <plgraphics/plg_driver_interface.h>

#include "plg_private.h"

/*	Graphics	*/

GfxState gfx_state;

/*	TODO:
- Add somewhere we can store tracking
data for each of these functions
- Display that data as an overlay?
*/
#define GRAPHICS_TRACK()                    \
	{                                       \
		static unsigned int num_calls = 0;  \
		if ( gfx_state.mode_debug ) {       \
			GfxLog( " %s\n", PL_FUNCTION ); \
			num_calls++;                    \
		}                                   \
	}

/*===========================
	INITIALIZATION
===========================*/

int LOG_LEVEL_GRAPHICS = 0;

void PlgInitializeInternalMeshes( void ); /* plg_draw.c */

PLFunctionResult PlgInitializeGraphics( void ) {
	memset( &gfx_state, 0, sizeof( GfxState ) );

	LOG_LEVEL_GRAPHICS = PlAddLogLevel( "plgraphics", ( PLColour ){ 0, 255, 255, 255 },
#if !defined( NDEBUG )
	                                    true
#else
	                                    false
#endif
	);

	PlgInitializeInternalMeshes();

	return PL_RESULT_SUCCESS;
}

void PlgShutdownTextures( void );    // platform_graphics_texture
void PlgClearInternalMeshes( void ); /* plg_draw.c */

void PlgShutdownGraphics( void ) {
	GRAPHICS_TRACK();

	PlgClearInternalMeshes();
	PlgShutdownTextures();

	CallGfxFunction( Shutdown );
}

/*===========================
	DEBUGGING
===========================*/

void PlgInsertDebugMarker( const char *msg ) {
	CallGfxFunction( InsertDebugMarker, msg );
}

void PlgPushDebugGroupMarker( const char *msg ) {
	CallGfxFunction( PushDebugGroupMarker, msg );
}

void PlgPopDebugGroupMarker( void ) {
	CallGfxFunction( PopDebugGroupMarker );
}

/*===========================
	HARDWARE INFORMATION
===========================*/

bool PlgSupportsHWShaders( void ) {
	GRAPHICS_TRACK();
	CallReturningGfxFunction( SupportsHWShaders, false );
}

/*===========================
	FRAMEBUFFERS
===========================*/

PLGFrameBuffer *PlgCreateFrameBuffer( unsigned int w, unsigned int h, unsigned int flags ) {
	if ( flags == 0 ) {
		return NULL;
	}

	PLGFrameBuffer *buffer = ( PLGFrameBuffer * ) PlMAllocA( sizeof( PLGFrameBuffer ) );
	buffer->width = w;
	buffer->height = h;
	buffer->flags = flags;

	CallGfxFunction( CreateFrameBuffer, buffer );

	return buffer;
}

void PlgDestroyFrameBuffer( PLGFrameBuffer *buffer ) {
	if ( !buffer ) {
		return;
	}

	CallGfxFunction( DeleteFrameBuffer, buffer );

	PlFree( buffer );
}

PLGTexture *PlgGetFrameBufferTextureAttachment( PLGFrameBuffer *buffer, unsigned int component, PLGTextureFilter filter ) {
	CallReturningGfxFunction( GetFrameBufferTextureAttachment, NULL, buffer, component, filter );
}

void PlgGetFrameBufferResolution( const PLGFrameBuffer *buffer, unsigned int *width, unsigned int *height ) {
	*width = buffer->width;
	*height = buffer->height;
}

void PlgBindFrameBuffer( PLGFrameBuffer *buffer, PLGFrameBufferObjectTarget target_binding ) {
	if ( ( buffer == gfx_state.frameBufferTarget ) && ( target_binding == gfx_state.frameBufferTargetMode ) ) {
		return;
	}

	//NOTE: NULL is valid for *buffer, to bind the SDL window default backbuffer
	CallGfxFunction( BindFrameBuffer, buffer, target_binding );

	gfx_state.frameBufferTarget = buffer;
	gfx_state.frameBufferTargetMode = target_binding;
}

PLGFrameBuffer *PlgGetCurrentFrameBufferTarget( PLGFrameBufferObjectTarget *currentMode ) {
	if ( currentMode != NULL ) {
		*currentMode = gfx_state.frameBufferTargetMode;
	}

	return gfx_state.frameBufferTarget;
}

void PlgBlitFrameBuffers( PLGFrameBuffer *src_buffer, unsigned int src_w, unsigned int src_h, PLGFrameBuffer *dst_buffer, unsigned int dst_w, unsigned int dst_h, bool linear ) {
	//NOTE: NULL is valid for *srcBuffer/*dstBuffer, to bind the SDL window default backbuffer
	//      SRC and DST can be the same buffer, in order to quickly copy a subregion of the buffer to a new location
	CallGfxFunction( BlitFrameBuffers, src_buffer, src_w, src_h, dst_buffer, dst_w, dst_h, linear );
}

void PlgSetClearColour( PLColour rgba ) {
	if ( PlCompareColour( rgba, gfx_state.current_clearcolour ) ) {
		return;
	}

	CallGfxFunction( SetClearColour, rgba );

	gfx_state.current_clearcolour = rgba;
}

void PlgClearBuffers( unsigned int buffers ) {
	CallGfxFunction( ClearBuffers, buffers );
}

/*===========================
	CAPABILITIES
===========================*/

bool PlgIsGraphicsStateEnabled( PLGDrawState state ) {
	return gfx_state.current_capabilities[ state ];
}

void PlgEnableGraphicsState( PLGDrawState state ) {
	if ( PlgIsGraphicsStateEnabled( state ) ) {
		return;
	}

	CallGfxFunction( EnableState, state );

	gfx_state.current_capabilities[ state ] = true;
}

void PlgDisableGraphicsState( PLGDrawState state ) {
	if ( !PlgIsGraphicsStateEnabled( state ) ) {
		return;
	}

	CallGfxFunction( DisableState, state );

	gfx_state.current_capabilities[ state ] = false;
}

/*===========================
	DRAW
===========================*/

void PlgSetBlendMode( PLGBlend a, PLGBlend b ) {
	CallGfxFunction( SetBlendMode, a, b );
}

void PlgSetCullMode( PLGCullMode mode ) {
	if ( mode == gfx_state.current_cullmode ) {
		return;
	}

	CallGfxFunction( SetCullMode, mode );

	gfx_state.current_cullmode = mode;
}

void PlgSetDepthBufferMode( unsigned int mode ) {
	CallGfxFunction( SetDepthBufferMode, mode );
}

void PlgSetDepthMask( bool enable ) {
	CallGfxFunction( SetDepthMask, enable );
}

/*===========================
	TEXTURES
===========================*/

#if 0
PLresult plUploadTextureData(PLTexture *texture, const PLTextureInfo *upload) {
    GRAPHICS_TRACK();

    _plSetActiveTexture(texture);

#	if defined( PL_MODE_OPENGL ) || defined( VL_MODE_OPENGL_CORE )
    unsigned int storage = _plTranslateTextureStorageFormat(upload->storage_type);
    unsigned int format = _plTranslateTextureFormat(upload->format);

    unsigned int levels = upload->levels;
    if(!levels) {
        levels = 1;
    }

    if (upload->initial) {
        glTexStorage2D(GL_TEXTURE_2D, levels, format, upload->width, upload->height);
    }

    // Check the format, to see if we're getting a compressed
    // format type.
    if (_plIsCompressedTextureFormat(upload->format)) {
        glCompressedTexSubImage2D
                (
                        GL_TEXTURE_2D,
                        0,
                        upload->x, upload->y,
                        upload->width, upload->height,
                        format,
                        upload->size,
                        upload->data
                );
    } else {
        glTexSubImage2D
                (
                        GL_TEXTURE_2D,
                        0,
                        upload->x, upload->y,
                        upload->width, upload->height,
                        _plTranslateColourFormat(upload->pixel_format),
                        storage,
                        upload->data
                );
    }

    if (plIsGraphicsStateEnabled(PL_CAPABILITY_GENERATEMIPMAP) && (levels > 1)) {
        glGenerateMipmap(GL_TEXTURE_2D);
    }
#	elif defined( VL_MODE_GLIDE )
#	elif defined( VL_MODE_DIRECT3D )
#	endif

    return PL_RESULT_SUCCESS;
}
#endif

/*===========================
	STENCIL OPERATIONS
===========================*/

void PlgStencilFunction( PLGStencilTestFunction function, int reference, unsigned int mask ) {
	CallGfxFunction( StencilFunction, function, reference, mask );
}
