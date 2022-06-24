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

#include <plcore/pl_console.h>

#include <plgraphics/plg.h>
#include <plgraphics/plg_texture.h>
#include <plgraphics/plg_camera.h>

PL_EXTERN_C

#if !defined( NDEBUG )
extern int LOG_LEVEL_GRAPHICS;
#	define GfxLog( FORMAT, ... ) PlLogWFunction( LOG_LEVEL_GRAPHICS, FORMAT, ##__VA_ARGS__ )
#else
#	define GfxLog( ... )
#endif

typedef struct GfxState {
	const PLGDriverImportTable *interface;

	PLGCullMode current_cullmode;

	PLColour current_clearcolour;
	PLColour current_colour;// Current global colour.

	bool current_capabilities[ PLG_GFX_MAX_STATES ];// Enabled capabilities.
	unsigned int current_textureunit;

	// Textures

	PLGTextureMappingUnit *tmu;

	// Shader states

	PLGShaderProgram *current_program;
	PLPath shaderCacheLocation; /* where shaders should be cached to, if supported */

	// Hardware / Driver information

	const char *hw_vendor;
	const char *hw_renderer;
	const char *hw_version;
	const char *hw_extensions;

	unsigned int hw_maxtexturesize;
	unsigned int hw_maxtextureunits;
	unsigned int hw_maxtextureanistropy;

	// Lighting

	unsigned int num_lights;

	// Cameras

	PLGCamera **cameras;

	unsigned int num_cameras;
	unsigned int max_cameras;

	PLMatrix4 projection_matrix;
	PLMatrix4 view_matrix;

	////////////////////////////////////////

	PLGViewport *current_viewport;

	PLGFrameBuffer *frameBufferTarget;
	PLGFrameBufferObjectTarget frameBufferTargetMode;

	bool mode_debug;
} GfxState;

#define CallGfxFunction( FUNCTION, ... )                                   \
	if ( gfx_state.interface != NULL ) {                                   \
		if ( gfx_state.interface->FUNCTION != NULL )                       \
			gfx_state.interface->FUNCTION( __VA_ARGS__ );                  \
		else                                                               \
			GfxLog( "Unbound layer function %s was called\n", #FUNCTION ); \
	}
#define CallReturningGfxFunction( FUNCTION, RETURN, ... )                  \
	if ( gfx_state.interface != NULL ) {                                   \
		if ( gfx_state.interface->FUNCTION != NULL )                       \
			return gfx_state.interface->FUNCTION( __VA_ARGS__ );           \
		else {                                                             \
			GfxLog( "Unbound layer function %s was called\n", #FUNCTION ); \
			return ( RETURN );                                             \
		}                                                                  \
	} else                                                                 \
		return ( RETURN );

///////////////////////////////////////////////////////

#if 1
#	define UseBufferScaling( a )                                   \
		( ( a )->viewport.r_w != 0 && ( a )->viewport.r_h != 0 ) && \
		        ( ( a )->viewport.r_w != ( a )->viewport.w && ( a )->viewport.r_h != ( a )->viewport.h )
#else /* for debugging purposes */
#	define UseBufferScaling( a ) 0
#endif

///////////////////////////////////////////////////////

PL_EXTERN GfxState gfx_state;

PL_EXTERN_C_END
