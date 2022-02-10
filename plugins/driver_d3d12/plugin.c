/**
 * Hei Platform Library
 * Copyright (C) 2017-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is licensed under MIT. See LICENSE for more details.
 */

#include "plugin.h"

static PLGDriverDescription pluginDesc = {
        .identifier = "d3d12",
        .description = "Direct3D 12 Graphics Driver.",
        .driverVersion = { 0, 0, 1 },
        .coreInterfaceVersion = { PL_PLUGIN_INTERFACE_VERSION_MAJOR, PL_PLUGIN_INTERFACE_VERSION_MINOR },
        .graphicsInterfaceVersion = { PLG_INTERFACE_VERSION_MAJOR, PLG_INTERFACE_VERSION_MINOR },
};

const PLGDriverExportTable *gInterface = NULL;

PL_EXPORT const PLGDriverDescription *QueryGraphicsDriver( void ) {
	return &pluginDesc;
}

int glLogLevel;

PL_EXPORT const PLGDriverImportTable *InitializeGraphicsDriver( const PLGDriverExportTable *functionTable ) {
	gInterface = functionTable;

	glLogLevel = gInterface->core->AddLogLevel( "plugin/d3d12", PLColourRGB( 255, 255, 255 ), true );

	static PLGDriverImportTable graphicsInterface = {
	        .Initialize = NULL,
	        .Shutdown = NULL,

	        .InsertDebugMarker = NULL,
	        .PushDebugGroupMarker = NULL,
	        .PopDebugGroupMarker = NULL,

	        .SupportsHWShaders = NULL,
	        .GetMaxTextureUnits = NULL,
	        .GetMaxTextureSize = NULL,

	        .EnableState = NULL,
	        .DisableState = NULL,

	        .SetBlendMode = NULL,
	        .SetCullMode = NULL,

	        .SetClearColour = NULL,
	        .ClearBuffers = NULL,

	        .DrawPixel = NULL,

	        .SetDepthBufferMode = NULL,
	        .SetDepthMask = NULL,

	        .CreateMesh = NULL,
	        .UploadMesh = NULL,
	        .DrawMesh = NULL,
	        .DrawInstancedMesh = NULL,
	        .DeleteMesh = NULL,

	        .CreateFrameBuffer = NULL,
	        .DeleteFrameBuffer = NULL,
	        .BindFrameBuffer = NULL,
	        .GetFrameBufferTextureAttachment = NULL,
	        .BlitFrameBuffers = NULL,

	        .CreateTexture = NULL,
	        .DeleteTexture = NULL,
	        .BindTexture = NULL,
	        .UploadTexture = NULL,
	        .SwizzleTexture = NULL,
	        .SetTextureAnisotropy = NULL,
	        .ActiveTexture = NULL,

	        .CreateCamera = NULL,
	        .DestroyCamera = NULL,
	        .SetupCamera = NULL,

	        .CreateShaderProgram = NULL,
	        .DestroyShaderProgram = NULL,
	        .AttachShaderStage = NULL,
	        .DetachShaderStage = NULL,
	        .LinkShaderProgram = NULL,
	        .SetShaderProgram = NULL,
	        .CreateShaderStage = NULL,
	        .DestroyShaderStage = NULL,
	        .CompileShaderStage = NULL,
	        .SetShaderUniformValue = NULL,
	};
	return &graphicsInterface;
}
