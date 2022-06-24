/**
 * Hei Platform Library
 * Copyright (C) 2017-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is licensed under MIT. See LICENSE for more details.
 */

#pragma once

#include <plcore/pl.h>
#include <plcore/pl_math.h>

#include <plgraphics/plg_mesh.h>
#include <plgraphics/plg_texture.h>
#include <plgraphics/plg_framebuffer.h>

typedef struct PLGDriverImportTable PLGDriverImportTable;

typedef enum PLGCullMode {
	PLG_CULL_NONE, /* disables backface culling */
	PLG_CULL_POSITIVE,
	PLG_CULL_NEGATIVE
} PLGCullMode;

// Blending Modes
typedef enum PLGBlend {
	PLG_BLEND_NONE,// disables blending

	PLG_BLEND_ZERO,
	PLG_BLEND_ONE,
	PLG_BLEND_SRC_COLOR,
	PLG_BLEND_ONE_MINUS_SRC_COLOR,
	PLG_BLEND_SRC_ALPHA,
	PLG_BLEND_ONE_MINUS_SRC_ALPHA,
	PLG_BLEND_DST_ALPHA,
	PLG_BLEND_ONE_MINUS_DST_ALPHA,
	PLG_BLEND_DST_COLOR,
	PLG_BLEND_ONE_MINUS_DST_COLOR,
	PLG_BLEND_SRC_ALPHA_SATURATE,

	PLG_MAX_BLEND_MODES
} PLGBlend;
#define PLG_BLEND_DISABLE  PLG_BLEND_NONE, PLG_BLEND_NONE
#define PLG_BLEND_ADDITIVE PLG_BLEND_SRC_ALPHA, PLG_BLEND_ONE
#define PLG_BLEND_DEFAULT  PLG_BLEND_SRC_ALPHA, PLG_BLEND_ONE_MINUS_SRC_ALPHA

//-----------------
// Capabilities

/* Replace these with generic functions, rather than relying on just flags?
 * plEnableFog(bool active)
 * plEnableAlphaTest(bool active)
 * plEnableBlend(bool active)
 */

typedef enum PLGDrawState {
	PLG_GFX_STATE_FOG,            // Fog.
	PLG_GFX_STATE_ALPHATEST,      // Alpha-testing.
	PLG_GFX_STATE_BLEND,          // Blending.
	PLG_GFX_STATE_DEPTHTEST,      // Depth-testing.
	PLG_GFX_STATE_STENCILTEST,    // Stencil-testing.
	PLG_GFX_STATE_MULTISAMPLE,    // Multisampling.
	PLG_GFX_STATE_SCISSORTEST,    // Scissor test for buffer clear.
	PLG_GFX_STATE_ALPHATOCOVERAGE,// Alpha to Coverage
	PLG_GFX_STATE_DEPTH_CLAMP,
	PLG_GFX_STATE_WIREFRAME,

	PLG_GFX_MAX_STATES
} PLGDrawState;

//-----------------
// Framebuffers
// todo: move all of this into pl_graphics_framebuffer.h

typedef enum PLGStencilTestFunction {
	PLG_STENCIL_TEST_ALWAYS,
	PLG_STENCIL_TEST_NEVER,
	PLG_STENCIL_TEST_LESS,
	PLG_STENCIL_TEST_LEQUAL,

	PLG_MAX_STENCIL_TEST_OPERATIONS
} PLGStencilTestFunction;

typedef enum PLGStencilFace {
	PLG_STENCIL_FACE_FRONT,
	PLG_STENCIL_FACE_BACK,
} PLGStencilFace;

//-----------------
// Lighting

typedef enum PLGLightType {
	PLG_LIGHT_TYPE_SPOT,// Spotlight
	PLG_LIGHT_TYPE_OMNI // Omni-directional
} PLGLightType;

typedef struct PLGLight {
	PLVector3 position;
	PLVector3 angles;

	PLColour colour;

	PLGLightType type;
} PLGLight;

//-----------------
// Shaders
// todo: move all of this into pl_graphics_shader.h

typedef int PLGShaderAttribute;

typedef enum PLGShaderStageType {
	PLG_SHADER_TYPE_VERTEX,
	PLG_SHADER_TYPE_FRAGMENT,
	PLG_SHADER_TYPE_GEOMETRY,
	PLG_SHADER_TYPE_COMPUTE,

	PLG_MAX_SHADER_TYPES
} PLGShaderStageType;

typedef enum PLGShaderUniformType {
	PLG_INVALID_UNIFORM,

	PLG_UNIFORM_FLOAT,
	PLG_UNIFORM_INT,
	PLG_UNIFORM_UINT,
	PLG_UNIFORM_BOOL,
	PLG_UNIFORM_DOUBLE,

	/* textures */
	PLG_UNIFORM_SAMPLER1D,
	PLG_UNIFORM_SAMPLER2D,
	PLG_UNIFORM_SAMPLER3D,
	PLG_UNIFORM_SAMPLERCUBE,
	PLG_UNIFORM_SAMPLER1DSHADOW,
	PLG_UNIFORM_SAMPLER2DSHADOW,

	/* vectors */
	PLG_UNIFORM_VEC2,
	PLG_UNIFORM_VEC3,
	PLG_UNIFORM_VEC4,

	/* matrices */
	PLG_UNIFORM_MAT3,
	PLG_UNIFORM_MAT4,

	PLG_MAX_UNIFORM_TYPES
} PLGShaderUniformType;

typedef struct PLGShaderProgram PLGShaderProgram;
typedef struct PLGShaderStage PLGShaderStage;

#define PLG_MAX_DEFINITION_LENGTH 16
#define PLG_MAX_DEFINITIONS       16

#define PLG_MAX_SHADER_PROGRAM_ID 128

typedef struct PLGShaderStage {
	PLGShaderStageType type;

	/* software implementation of the shader stage */
	void ( *SWFallback )( PLGShaderProgram *program, PLGShaderStageType type );

	PLGShaderProgram *program;

	struct {
		unsigned int id;
	} internal;

	unsigned int numDefinitions;
	char definitions[ PLG_MAX_DEFINITIONS ][ PLG_MAX_DEFINITION_LENGTH ];

	PLPath path; /* original location it was loaded from */
} PLGShaderStage;

typedef struct PLGShaderProgram {
	struct {
		char *name;
		int slot;
		PLGShaderUniformType type;

		union {
			int defaultInt;
			unsigned int defaultUInt;
			bool defaultBool;
			double defaultDouble;
			float defaultFloat;

			PLVector2 defaultVec2;
			PLVector3 defaultVec3;
			PLVector4 defaultVec4;

			PLMatrix3 defaultMat3;
			PLMatrix4 defaultMat4;
		};
	} * uniforms;
	unsigned int num_uniforms;

	struct {
		char name[ 32 ];

		unsigned int slot;
	} * attributes;
	unsigned int num_attributes;

	PLGShaderStage *stages[ PLG_MAX_SHADER_TYPES ];
	unsigned int num_stages;

	bool is_linked;

	struct {
		unsigned int id;

		PLGShaderAttribute v_position;
		PLGShaderAttribute v_normal;
		PLGShaderAttribute v_uv;
		PLGShaderAttribute v_colour;
		PLGShaderAttribute v_tangent, v_bitangent;
	} internal;

	char id[ PLG_MAX_SHADER_PROGRAM_ID ];
} PLGShaderProgram;

typedef struct PLGPolygon PLGPolygon;
typedef struct PLGVertex PLGVertex;
typedef struct PLGMesh PLGMesh;
typedef struct PLGCamera PLGCamera;

PL_EXTERN_C

#if !defined( PL_COMPILE_PLUGIN )

PLFunctionResult PlgInitializeGraphics( void );
void PlgShutdownGraphics( void );

PL_EXTERN PLGShaderStage *PlgCreateShaderStage( PLGShaderStageType type );
PL_EXTERN PLGShaderStage *PlgParseShaderStage( PLGShaderStageType type, const char *buf, size_t length );
PL_EXTERN PLGShaderStage *PlgLoadShaderStage( const char *path, PLGShaderStageType type );
PL_EXTERN void PlgCompileShaderStage( PLGShaderStage *stage, const char *buf, size_t length );

PL_EXTERN void PlgSetShaderStageDefinitions( PLGShaderStage *stage, const char definitions[][ PLG_MAX_DEFINITION_LENGTH ], unsigned int numDefinitions );

PL_EXTERN void PlgAttachShaderStage( PLGShaderProgram *program, PLGShaderStage *stage );
PL_EXTERN bool PlgRegisterShaderStageFromMemory( PLGShaderProgram *program, const char *buffer, size_t length,
                                                 PLGShaderStageType type );
PL_EXTERN bool PlgRegisterShaderStageFromDisk( PLGShaderProgram *program, const char *path, PLGShaderStageType type );
PL_EXTERN bool PlgLinkShaderProgram( PLGShaderProgram *program );

PL_EXTERN int PlgGetShaderUniformSlot( PLGShaderProgram *program, const char *name );
PL_EXTERN PLGShaderUniformType PlgGetShaderUniformType( const PLGShaderProgram *program, int slot );

PL_EXTERN void PlgSetShaderUniformDefaultValueByIndex( PLGShaderProgram *program, int slot, const void *defaultValue );
PL_EXTERN void PlgSetShaderUniformDefaultValue( PLGShaderProgram *program, const char *name, const void *defaultValue );
PL_EXTERN void PlgSetShaderUniformToDefaultByIndex( PLGShaderProgram *program, int slot );
PL_EXTERN void PlgSetShaderUniformToDefault( PLGShaderProgram *program, const char *name );
PL_EXTERN void PlgSetShaderUniformsToDefault( PLGShaderProgram *program );

PL_EXTERN void PlgSetShaderUniformValueByIndex( PLGShaderProgram *program, int slot, const void *value, bool transpose );
PL_EXTERN void PlgSetShaderUniformValue( PLGShaderProgram *program, const char *name, const void *value, bool transpose );

#	if !defined( PL_EXCLUDE_DEPRECATED_API )
PL_EXTERN PL_DEPRECATED( void PlgSetShaderUniformFloat( PLGShaderProgram *program, int slot, float value ) );
PL_EXTERN PL_DEPRECATED( void PlgSetShaderUniformInt( PLGShaderProgram *program, int slot, int value ) );
PL_EXTERN PL_DEPRECATED( void PlgSetShaderUniformVector4( PLGShaderProgram *program, int slot, PLVector4 value ) );
PL_EXTERN PL_DEPRECATED( void PlgSetShaderUniformVector3( PLGShaderProgram *program, int slot, PLVector3 value ) );
PL_EXTERN PL_DEPRECATED( void PlgSetShaderUniformMatrix4( PLGShaderProgram *program, int slot, PLMatrix4 value, bool transpose ) );
#	endif

PL_EXTERN PLGShaderProgram *PlgCreateShaderProgram( void );
PL_EXTERN PLGShaderProgram *PlgLoadCachedShaderProgram( const char *path );
PL_EXTERN void PlgDestroyShaderProgram( PLGShaderProgram *program, bool free_stages );

PL_EXTERN void PlgSetShaderProgramId( PLGShaderProgram *program, const char *id );
PL_EXTERN void PlgClearShaderProgramId( PLGShaderProgram *program );
PL_EXTERN const char *PlgGetShaderProgramId( PLGShaderProgram *program );

PL_EXTERN void PlgSetShaderCacheLocation( const char *path );
PL_EXTERN void PlgClearShaderCacheLocation( const char *path );
PL_EXTERN const char *PlgGetShaderCacheLocation( void );

PL_EXTERN PLGShaderProgram *PlgGetCurrentShaderProgram( void );

PL_EXTERN void PlgSetShaderProgram( PLGShaderProgram *program );
PL_EXTERN bool PlgIsShaderProgramEnabled( PLGShaderProgram *program );

// Debugging
PL_EXTERN void PlgInsertDebugMarker( const char *msg );
PL_EXTERN void PlgPushDebugGroupMarker( const char *msg );
PL_EXTERN void PlgPopDebugGroupMarker( void );

// Hardware Information
PL_EXTERN const char *PlgGetHWExtensions( void );
PL_EXTERN const char *PlgGetHWRenderer( void );
PL_EXTERN const char *PlgGetHWVendor( void );
PL_EXTERN const char *PlgGetHWVersion( void );

PL_EXTERN bool PlgSupportsHWShaders( void );

PL_EXTERN void PlgSetCullMode( PLGCullMode mode );
PL_EXTERN void PlgSetBlendMode( PLGBlend a, PLGBlend b );

PL_EXTERN void PlgSetDepthBufferMode( unsigned int mode );
PL_EXTERN void PlgSetDepthMask( bool enable );

/* stencil operations */

PL_EXTERN void PlgStencilFunction( PLGStencilTestFunction function, int reference, unsigned int mask );

PL_EXTERN bool PlgIsGraphicsStateEnabled( PLGDrawState state );

PL_EXTERN void PlgEnableGraphicsState( PLGDrawState state );
PL_EXTERN void PlgDisableGraphicsState( PLGDrawState state );

#endif

PL_EXTERN_C_END
