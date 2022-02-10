/**
 * Hei Platform Library
 * Copyright (C) 2017-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is licensed under MIT. See LICENSE for more details.
 */

/*===========================
	DRIVER INTERFACE
===========================*/

#include <plcore/pl_linkedlist.h>
#include <plgraphics/plg_driver_interface.h>

#include "plg_private.h"

#define DRIVER_EXTENSION "_driver"

#define MAX_GRAPHICS_MODES 16

typedef struct PLGDriver {
	char identifier[ 32 ];
	char description[ 64 ];

	PLLibrary *libPtr; /* library handle */
	const PLGDriverImportTable *interface;
	PLGDriverInitializationFunction initFunction; /* initialization function */
} PLGDriver;

static PLGDriver drivers[ MAX_GRAPHICS_MODES ];
static unsigned int numDrivers = 0;

static PLGDriverExportTable exportTable = {
        .CreateTexture = PlgCreateTexture,
        .DestroyTexture = PlgDestroyTexture,
        .GetMaxTextureAnistropy = PlgGetMaxTextureAnistropy,
        .GetMaxTextureSize = PlgGetMaxTextureSize,
        .GetMaxTextureUnits = PlgGetMaxTextureUnits,
        .SetTexture = PlgSetTexture,
        .SetTextureAnisotropy = PlgSetTextureAnisotropy,
        .SetTextureEnvironmentMode = PlgSetTextureEnvironmentMode,
        .SetTextureFlags = PlgSetTextureFlags,

        .GetShaderCacheLocation = PlgGetShaderCacheLocation,
};

bool PlgRegisterDriver( const char *path ) {
	PlClearError();

	GfxLog( "Registering driver: \"%s\"\n", path );

	PLLibrary *library = PlLoadLibrary( path, false );
	if ( library == NULL ) {
		GfxLog( "Failed to load library: %s\n", PlGetError() );
		return false;
	}

	const PLGDriverDescription *description;
	PLGDriverInitializationFunction InitializeDriver;
	PLGDriverQueryFunction RegisterDriver = ( PLGDriverQueryFunction ) PlGetLibraryProcedure( library, PLG_DRIVER_QUERY_FUNCTION );
	if ( RegisterDriver != NULL ) {
        /* now fetch the driver description */
        description = RegisterDriver();
        if ( description != NULL ) {
#if !defined( NDEBUG )
			GfxLog( "\"%s\" :\n"
			        " identifier = \"%s\"\n"
			        " description = \"%s\"\n"
			        " driver version = %d.%d.%d\n",
			        path,
			        description->identifier,
			        description->description,
			        description->driverVersion[ 0 ],
			        description->driverVersion[ 1 ],
			        description->driverVersion[ 2 ] );
#endif
            if ( description->coreInterfaceVersion[ 0 ] != PL_PLUGIN_INTERFACE_VERSION_MAJOR ) {
                PlReportErrorF( PL_RESULT_UNSUPPORTED, "unsupported core interface version" );
            } else if ( description->graphicsInterfaceVersion[ 0 ] != PLG_INTERFACE_VERSION_MAJOR ) {
                PlReportErrorF( PL_RESULT_UNSUPPORTED, "unsupported graphics interface version" );
            }
        } else {
            PlReportErrorF( PL_RESULT_FAIL, "failed to fetch driver description" );
        }

		InitializeDriver = ( PLGDriverInitializationFunction ) PlGetLibraryProcedure( library, PLG_DRIVER_INIT_FUNCTION );
	}

	if ( RegisterDriver == NULL || InitializeDriver == NULL || PlGetFunctionResult() != PL_RESULT_SUCCESS ) {
        GfxLog( "Failed to load library!\nPL: %s\n", PlGetError() );
		PlUnloadLibrary( library );
		return false;
	}

	GfxLog( "Success, adding \"%s\" to drivers list\n", path );

	PLGDriver *driver = &drivers[ numDrivers++ ];
	snprintf( driver->description, sizeof( driver->description ), "%s", description->description );
	snprintf( driver->identifier, sizeof( driver->identifier ), "%s", description->identifier );
	driver->interface = NULL;
	driver->initFunction = InitializeDriver;
	driver->libPtr = library;

	return true;
}

/**
 * Private callback when scanning for plugins.
 */
static void RegisterScannedDriver( const char *path, void *unused ) {
	PlUnused( unused );

	/* validate it's actually a plugin */
	size_t length = strlen( path );
	const char *c = strrchr( path, '_' );
	if ( c == NULL ) {
		return;
	}

	/* remaining length */
	length -= c - path;
	if ( pl_strncasecmp( c, DRIVER_EXTENSION PL_SYSTEM_LIBRARY_EXTENSION, length ) != 0 ) {
		return;
	}

	PlgRegisterDriver( path );
}

void PlgScanForDrivers( const char *path ) {
	GfxLog( "Scanning for drivers in \"%s\"\n", path );

	PlScanDirectory( path, ( PL_SYSTEM_LIBRARY_EXTENSION ) + 1, RegisterScannedDriver, false, NULL );

	GfxLog( "Done, %d graphics drivers loaded.\n", numDrivers );
}

/**
 * Query what available graphics modes there are so that the
 * host can either choose their preference or attempt each.
 */
const char **PlgGetAvailableDriverInterfaces( unsigned int *numModes ) {
	static unsigned int cachedModes = 0;
	static const char *descriptors[ MAX_GRAPHICS_MODES ];
	if ( cachedModes != numDrivers ) {
		for ( unsigned int i = 0; i < numDrivers; ++i ) {
			descriptors[ i ] = drivers[ i ].identifier;
		}
	}
	*numModes = numDrivers;
	cachedModes = *numModes;

	return descriptors;
}

PLFunctionResult PlgSetDriver( const char *mode ) {
	GfxLog( "Attempting mode \"%s\"...\n", mode );

	exportTable.core = PlGetExportTable();

	const PLGDriverImportTable *interface = NULL;
	for ( unsigned int i = 0; i < numDrivers; ++i ) {
		if ( pl_strcasecmp( mode, drivers[ i ].identifier ) != 0 ) {
			continue;
		}

		interface = drivers[ i ].initFunction( &exportTable );
		break;
	}

	if ( interface == NULL ) {
		PlReportErrorF( PL_RESULT_GRAPHICSINIT, "invalid graphics interface \"%s\" selected", mode );
		return PL_RESULT_GRAPHICSINIT;
	}

	if ( gfx_state.interface != NULL && interface == gfx_state.interface ) {
		PlReportErrorF( PL_RESULT_GRAPHICSINIT, "chosen interface \"%s\" is already active", mode );
		return PL_RESULT_GRAPHICSINIT;
	}

	gfx_state.interface = interface;

	CallGfxFunction( Initialize );

	GfxLog( "Mode \"%s\" initialized!\n", mode );

	PlFree( gfx_state.tmu );

	unsigned int numUnits = PlgGetMaxTextureUnits();
    gfx_state.tmu = ( PLGTextureMappingUnit * ) PlCAllocA( numUnits, sizeof( PLGTextureMappingUnit ) );
    for ( unsigned int i = 0; i < numUnits; i++ ) {
        gfx_state.tmu[ i ].current_envmode = PLG_TEXTUREMODE_REPLACE;
    }

	return PL_RESULT_SUCCESS;
}
