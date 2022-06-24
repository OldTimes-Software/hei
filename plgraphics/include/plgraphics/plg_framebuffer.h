/**
 * Hei Platform Library
 * Copyright (C) 2017-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is licensed under MIT. See LICENSE for more details.
 */

#pragma once

PL_EXTERN_C

typedef enum PLGFrameBufferObjectTarget {
	PLG_FRAMEBUFFER_DEFAULT,
	PLG_FRAMEBUFFER_DRAW,
	PLG_FRAMEBUFFER_READ
} PLGFrameBufferObjectTarget;

typedef enum PLGFrameBufferRenderFlags {
	PL_BITFLAG( PLG_BUFFER_COLOUR, 0 ),
	PL_BITFLAG( PLG_BUFFER_DEPTH, 1 ),
	PL_BITFLAG( PLG_BUFFER_STENCIL, 2 ),
} PLGFrameBufferRenderFlags;

enum {
	PLG_RENDERBUFFER_COLOUR,
	PLG_RENDERBUFFER_DEPTH,
	PLG_RENDERBUFFER_STENCIL,

	PLG_MAX_RENDERBUFFER_TYPES
};

/* !!!
 * be careful changing this as it could break
 * the plugin interface
 * !!! */
typedef struct PLGFrameBuffer {
	unsigned int fbo;
	unsigned int renderBuffers[ PLG_MAX_RENDERBUFFER_TYPES ];
	unsigned int width;
	unsigned int height;
	PLGFrameBufferRenderFlags flags;
} PLGFrameBuffer;

enum {
	PLG_DEPTHBUFFER_DISABLE,
	PLG_DEPTHBUFFER_ENABLE,
};

PL_EXTERN PLGFrameBuffer *PlgCreateFrameBuffer( unsigned int w, unsigned int h, unsigned int flags );
PL_EXTERN void PlgDestroyFrameBuffer( PLGFrameBuffer *buffer );
PL_EXTERN void PlgBindFrameBuffer( PLGFrameBuffer *buffer, PLGFrameBufferObjectTarget target_binding );
PL_EXTERN PLGFrameBuffer *PlgGetCurrentFrameBufferTarget( PLGFrameBufferObjectTarget *currentMode );
PL_EXTERN void PlgBlitFrameBuffers( PLGFrameBuffer *src_buffer, unsigned int src_w, unsigned int src_h, PLGFrameBuffer *dst_buffer, unsigned int dst_w, unsigned int dst_h, bool linear );

PL_EXTERN PLGTexture *PlgGetFrameBufferTextureAttachment( PLGFrameBuffer *buffer, unsigned int component, PLGTextureFilter filter );
PL_EXTERN void PlgGetFrameBufferResolution( const PLGFrameBuffer *buffer, unsigned int *width, unsigned int *height );

PL_EXTERN void PlgSetClearColour( PLColour rgba );
PL_EXTERN void PlgClearBuffers( unsigned int buffers );

PL_EXTERN_C_END
