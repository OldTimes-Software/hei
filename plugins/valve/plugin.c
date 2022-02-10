/* Copyright (C) 2020 Mark E Sowden <markelswo@gmail.com> */

#include "plugin.h"

static PLPluginDescription pluginDesc = {
        .description = "Valve file format support.",
        .pluginVersion = { 0, 0, 1 },
        .interfaceVersion = { PL_PLUGIN_INTERFACE_VERSION_MAJOR, PL_PLUGIN_INTERFACE_VERSION_MINOR }
};

const PLPluginExportTable *gInterface = NULL;

PL_EXPORT const PLPluginDescription *PLQueryPlugin( void ) {
	return &pluginDesc;
}

PLImage *VTF_ParseImage( PLFile *file );

PL_EXPORT void PLInitializePlugin( const PLPluginExportTable *functionTable ) {
	gInterface = functionTable;

	gInterface->RegisterImageLoader( "vtf", VTF_ParseImage );
}
