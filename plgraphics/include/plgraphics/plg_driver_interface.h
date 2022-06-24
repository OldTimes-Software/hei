/**
 * Hei Platform Library
 * Copyright (C) 2017-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is licensed under MIT. See LICENSE for more details.
 */

#pragma once

#include <plcore/pl_plugin_interface.h>
#include <plgraphics/plg.h>

PL_EXTERN_C

/**
 * For a little further clarity, technically
 * speaking 'drivers' in this context are handled
 * exactly the same as plugins for the most part
 * but are setup through the plgraphics module
 * instead - and are exclusively assumed to support
 * either audio/video.
 */

#define PLG_INTERFACE_VERSION_MAJOR 2
#define PLG_INTERFACE_VERSION_MINOR 1
#define PLG_INTERFACE_VERSION \
	( uint16_t[ 2 ] ) { PLG_INTERFACE_VERSION_MAJOR, PLG_INTERFACE_VERSION_MINOR }

typedef struct PLGDriverDescription {
	uint16_t coreInterfaceVersion[ 2 ];
	uint16_t graphicsInterfaceVersion[ 2 ];

	const char *identifier;          /* 'opengl3' */
	unsigned int driverVersion[ 3 ]; /* major, minor and patch */
	const char *description;         /* 'OpenGL 3.3 Graphics Driver' */
} PLGDriverDescription;

typedef struct PLGDriverExportTable {
	const PLPluginExportTable *core;

	/* plg_texture */
	PLGTexture *( *CreateTexture )( void );
	void ( *DestroyTexture )( PLGTexture *texture );
	unsigned int ( *GetMaxTextureSize )( void );
	unsigned int ( *GetMaxTextureUnits )( void );
	unsigned int ( *GetMaxTextureAnistropy )( void );
	void ( *SetTextureAnisotropy )( PLGTexture *texture, unsigned int amount );
	void ( *SetTexture )( PLGTexture *texture, unsigned int tmu );
	void ( *SetTextureUnit )( unsigned int tmu );
	void ( *SetTextureEnvironmentMode )( PLGTextureEnvironmentMode mode );
	void ( *SetTextureFlags )( PLGTexture *texture, unsigned int flags );

	const char *( *GetShaderCacheLocation )( void );
} PLGDriverExportTable;

typedef struct PLGDriverImportTable {
	PLFunctionResult ( *Initialize )( void );
	void ( *Shutdown )( void );

	// Debug
	void ( *InsertDebugMarker )( const char *msg );
	void ( *PushDebugGroupMarker )( const char *msg );
	void ( *PopDebugGroupMarker )( void );

	/* hw information */

	bool ( *SupportsHWShaders )( void );

	void ( *GetMaxTextureUnits )( unsigned int *num_units );
	void ( *GetMaxTextureSize )( unsigned int *s );

	/******************************************/

	/* generic state management */
	void ( *EnableState )( PLGDrawState state );
	void ( *DisableState )( PLGDrawState state );

	void ( *SetBlendMode )( PLGBlend a, PLGBlend b );
	void ( *SetCullMode )( PLGCullMode mode );

	void ( *SetClearColour )( PLColour rgba );
	void ( *ClearBuffers )( unsigned int buffers );

	void ( *DrawPixel )( int x, int y, PLColour colour );

	void ( *SetDepthBufferMode )( unsigned int mode );
	void ( *SetDepthMask )( bool enable );

	// Mesh
	void ( *CreateMesh )( PLGMesh *mesh );
	void ( *UploadMesh )( PLGMesh *mesh, PLGShaderProgram *program ); /* todo: deprecate? */
	void ( *DrawMesh )( PLGMesh *mesh, PLGShaderProgram *program );
	void ( *DrawInstancedMesh )( PLGMesh *mesh, PLGShaderProgram *program, const PLMatrix4 *transforms, unsigned int instanceCount );
	void ( *DeleteMesh )( PLGMesh *mesh );

	// Framebuffer
	void ( *CreateFrameBuffer )( PLGFrameBuffer *buffer );
	void ( *DeleteFrameBuffer )( PLGFrameBuffer *buffer );
	void ( *BindFrameBuffer )( PLGFrameBuffer *buffer, PLGFrameBufferObjectTarget targetBinding );
	PLGTexture *( *GetFrameBufferTextureAttachment )( PLGFrameBuffer *buffer, unsigned int component, PLGTextureFilter filter );
	void ( *BlitFrameBuffers )( PLGFrameBuffer *srcBuffer,
	                            unsigned int srcW,
	                            unsigned int srcH,
	                            PLGFrameBuffer *dstBuffer,
	                            unsigned int dstW,
	                            unsigned int dstH,
	                            bool linear );

	// Texture
	void ( *CreateTexture )( PLGTexture *texture );
	void ( *DeleteTexture )( PLGTexture *texture );
	void ( *BindTexture )( const PLGTexture *texture );
	void ( *UploadTexture )( PLGTexture *texture, const PLImage *upload );
	void ( *SwizzleTexture )( PLGTexture *texture, uint8_t r, uint8_t g, uint8_t b, uint8_t a );
	void ( *SetTextureAnisotropy )( PLGTexture *texture, uint32_t value );
	void ( *SetTextureFilter )( PLGTexture *texture, PLGTextureFilter filter );
	void ( *ActiveTexture )( unsigned int target );

	// Camera
	/* todo: simplify/remove this interface */
	void ( *CreateCamera )( PLGCamera *camera );
	void ( *DestroyCamera )( PLGCamera *camera );
	void ( *SetupCamera )( PLGCamera *camera );

	///////////////////////////////////////////

	// Shaders
	/* todo: simplify this interface */
	void ( *CreateShaderProgram )( PLGShaderProgram *program );
	void ( *DestroyShaderProgram )( PLGShaderProgram *program );
	void ( *AttachShaderStage )( PLGShaderProgram *program, PLGShaderStage *stage );
	void ( *DetachShaderStage )( PLGShaderProgram *program, PLGShaderStage *stage );
	void ( *LinkShaderProgram )( PLGShaderProgram *program );
	void ( *SetShaderProgram )( PLGShaderProgram *program );
	void ( *CreateShaderStage )( PLGShaderStage *stage );
	void ( *DestroyShaderStage )( PLGShaderStage *stage );
	void ( *CompileShaderStage )( PLGShaderStage *stage, const char *buf, size_t length );
	void ( *SetShaderUniformValue )( PLGShaderProgram *program, int slot, const void *value, bool transpose );

	// Stencil operations
	void ( *StencilFunction )( PLGStencilTestFunction function, int reference, unsigned int mask );
} PLGDriverImportTable;

#if !defined( PL_COMPILE_PLUGIN )
#	define PLG_DRIVER_QUERY_FUNCTION "QueryGraphicsDriver"
typedef const PLGDriverDescription *( *PLGDriverQueryFunction )( void );
#	define PLG_DRIVER_INIT_FUNCTION "InitializeGraphicsDriver"
typedef const PLGDriverImportTable *( *PLGDriverInitializationFunction )( const PLGDriverExportTable *exportTable );

PL_EXTERN bool PlgRegisterDriver( const char *path );
PL_EXTERN void PlgScanForDrivers( const char *path );

PL_EXTERN const char **PlgGetAvailableDriverInterfaces( unsigned int *numModes );
PL_EXTERN PLFunctionResult PlgSetDriver( const char *mode );
#endif

PL_EXTERN_C_END
