/**
 * Hei Platform Library
 * Copyright (C) 2017-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is licensed under MIT. See LICENSE for more details.
 */

#include <plgraphics/plg.h>
#include <plgraphics/plg_camera.h>

#include "imgui_renderer.h"

static PLGMesh *renderMesh = nullptr;
static PLGTexture *fontTexture = nullptr;

bool PlImGui_Initialize() {
	ImGuiIO &io = ImGui::GetIO();
	io.BackendRendererName = "plgraphics";

	renderMesh = PlgCreateMesh( PLG_MESH_TRIANGLES, PLG_DRAW_DYNAMIC, 1024, 1024 );
	if ( renderMesh == nullptr ) {
		return false;
	}

	return true;
}

void PlImGui_Shutdown() {
	PlgDestroyMesh( renderMesh );
	renderMesh = nullptr;

	PlgDestroyTexture( fontTexture );
	fontTexture = nullptr;
}

void PlImGui_NewFrame() {
	if ( fontTexture != nullptr ) {
		return;
	}

	ImGuiIO &io = ImGui::GetIO();

	unsigned char *pixels;
	int w, h;
	io.Fonts->GetTexDataAsRGBA32( &pixels, &w, &h );

	PLImage *image = PlCreateImage( pixels, w, h, PL_COLOURFORMAT_RGBA, PL_IMAGEFORMAT_RGBA8 );
	if ( image == nullptr ) {
		return;
	}

	fontTexture = PlgCreateTexture();
	PlgUploadTextureImage( fontTexture, image );

	PlDestroyImage( image );

	io.Fonts->TexID = ( PLGTexture * ) ( intptr_t ) fontTexture;

	PlgSetTexture( nullptr, 0 );
}

void PlImGui_RenderDrawData( ImDrawData *drawData ) {
	return;

	const PLGViewport *viewport = PlgGetCurrentViewport();
	if ( viewport == nullptr ) {
		return;
	}

	for ( int i = 0; i < drawData->CmdListsCount; ++i ) {
		const ImDrawList *cmdList = drawData->CmdLists[ i ];
		PlgClearMesh( renderMesh );
		for ( int j = 0; j < cmdList->CmdBuffer.Size; ++j ) {
			const ImDrawCmd *cmd = &cmdList->CmdBuffer[ j ];

			PlgSetTexture( ( PLGTexture * ) cmd->TextureId, 0 );
		}
	}

	PlgSetTexture( nullptr, 0 );
}
