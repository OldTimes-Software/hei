/*
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org>
*/

#include <plcore/pl.h>
#include <plcore/pl_console.h>
#include <plcore/pl_image.h>
#include <plcore/pl_package.h>

#include <plmodel/plm.h>

/**
 * Command line utility to interface with the platform lib.
 **/

#define Error( ... ) fprintf( stderr, __VA_ARGS__ )

static void ConvertImage( const char *path, const char *destination ) {
	PLImage *image = PlLoadImage( path );
	if ( image == NULL ) {
		printf( "Failed to load \"%s\"! (%s)\n", path, PlGetError() );
		return;
	}

	/* ensure it's a valid format before we write it out */
	PlConvertPixelFormat( image, PL_IMAGEFORMAT_RGBA8 );

	if ( PlWriteImage( image, destination ) ) {
		printf( "Wrote \"%s\"\n", destination );
	} else {
		printf( "Failed to write \"%s\"! (%s)\n", destination, PlGetError() );
	}

	PlDestroyImage( image );
}

static void ConvertImageCallback( const char *path, void *userData ) {
	char *outDir = ( char * ) userData;

	if ( !PlCreateDirectory( outDir ) ) {
		Error( "Error: %s\n", PlGetError() );
		return;
	}

	const char *fileName = PlGetFileName( path );
	if ( fileName == NULL ) {
		Error( "Error: %s\n", PlGetError() );
		return;
	}

	char outPath[ PL_SYSTEM_MAX_PATH ];
	snprintf( outPath, sizeof( outPath ), "%s%s.png", outDir, fileName );

	ConvertImage( path, outPath );
}

static void Cmd_IMGConvert( unsigned int argc, char **argv ) {
	if ( argc < 2 ) {
		return;
	}

	const char *outPath = "./out.png";
	if ( argc >= 3 ) {
		outPath = argv[ 2 ];
	}

	ConvertImage( argv[ 1 ], outPath );
}

static void Cmd_IMGBulkConvert( unsigned int argc, char **argv ) {
	if ( argc < 3 ) {
		return;
	}

	char outDir[ PL_SYSTEM_MAX_PATH ];
	if ( argc >= 4 ) {
		snprintf( outDir, sizeof( outDir ), "%s/", argv[ 3 ] );
	} else {
		snprintf( outDir, sizeof( outDir ), "out/" );
	}

	PlScanDirectory( argv[ 1 ], argv[ 2 ], ConvertImageCallback, false, outDir );

	printf( "Done!\n" );
}

static void ConvertModel( const char *path, const char *destination ) {
	PLMModel *model = PlmLoadModel( path );
	if ( model == NULL ) {
		printf( "Failed to load model: %s\n", PlGetError() );
		return;
	}

	PlmWriteModel( destination, model, PLM_MODEL_OUTPUT_SMD );

	PlmDestroyModel( model );
}

static void Cmd_ConvertModel( unsigned int argc, char **argv ) {
	if ( argc < 2 ) {
		return;
	}

	PLPath outPath;
	if ( argc >= 3 ) {
		snprintf( outPath, sizeof( outPath ), "%s", argv[ 2 ] );
	} else {
		const char *fn = PlGetFileName( argv[ 1 ] );
		size_t ns = strlen( fn );
		const char *fe = PlGetFileExtension( fn );
		size_t es = strlen( fe );

		char name[ ns ];
		snprintf( name, ns - es, "%s", fn );

		snprintf( outPath, sizeof( outPath ), "./%s.smd", name );
	}

	ConvertModel( argv[ 1 ], outPath );
}

static void ConvertModelCallback( const char *path, void *userData ) {
	char *outDir = ( char * ) userData;

	if ( !PlCreateDirectory( outDir ) ) {
		Error( "Error: %s\n", PlGetError() );
		return;
	}

	const char *fn = PlGetFileName( path );
	size_t ns = strlen( fn );
	const char *fe = PlGetFileExtension( fn );
	size_t es = strlen( fe );

	char name[ ns ];
	snprintf( name, ns - es, "%s", fn );

	char outPath[ PL_SYSTEM_MAX_PATH ];
	snprintf( outPath, sizeof( outPath ), "%s%s.smd", outDir, name );

	printf( "Converting %s to %s\n", path, outPath );

	ConvertModel( path, outPath );
}

static void Cmd_BulkConvertModel( unsigned int argc, char **argv ) {
	if ( argc < 3 ) {
		return;
	}

	char outDir[ PL_SYSTEM_MAX_PATH ];
	if ( argc >= 4 ) {
		snprintf( outDir, sizeof( outDir ), "%s/", argv[ 3 ] );
	} else {
		snprintf( outDir, sizeof( outDir ), "out/" );
	}

	PlScanDirectory( argv[ 1 ], argv[ 2 ], ConvertModelCallback, true, outDir );

	printf( "Done!\n" );
}

static bool isRunning = true;

static void Cmd_Exit( unsigned int argc, char **argv ) {
	/* we don't use these here, and ommitting var names is a C2x feature :( */
	( void ) ( argc );
	( void ) ( argv );

	isRunning = false;
}

PLPackage *IStorm_LST_LoadFile( const char *path );
PLPackage *Outcast_OPK_LoadFile( const char *path );
PLPackage *FTactics_PAK_LoadFile( const char *path );
PLPackage *Sentient_VSR_LoadFile( const char *path );
PLPackage *Mortyr_HAL_LoadFile( const char *path );
PLPackage *Eradicator_RID_LoadFile( const char *path );
PLPackage *Outwars_FF_LoadFile( const char *path );
PLPackage *Core_CLU_LoadPackage( const char *path );
PLPackage *Angel_DAT_LoadPackage( const char *path );
PLPackage *AITD_PAK_LoadPackage( const char *path );

PLImage *Core_HGT_ParseImage( PLFile *file );
PLImage *Angel_TEX_ParseImage( PLFile *file );

#define MAX_COMMAND_LENGTH 256
static char cmdLine[ MAX_COMMAND_LENGTH ];
int main( int argc, char **argv ) {
	/* no buffering stdout! */
	setvbuf( stdout, NULL, _IONBF, 0 );

	PlInitialize( argc, argv );
	PlInitializeSubSystems( PL_SUBSYSTEM_IO );

	PlRegisterStandardImageLoaders( PL_IMAGE_FILEFORMAT_ALL );
	PlRegisterStandardPackageLoaders();

	PlmRegisterStandardModelLoaders( PLM_MODEL_FILEFORMAT_ALL );

	PlRegisterPackageLoader( "lst", IStorm_LST_LoadFile );
	PlRegisterPackageLoader( "opk", Outcast_OPK_LoadFile );
	PlRegisterPackageLoader( "pak", AITD_PAK_LoadPackage );
	PlRegisterPackageLoader( "pak", FTactics_PAK_LoadFile );
	PlRegisterPackageLoader( "vsr", Sentient_VSR_LoadFile );
	PlRegisterPackageLoader( "hal", Mortyr_HAL_LoadFile );
	PlRegisterPackageLoader( "rid", Eradicator_RID_LoadFile );
	PlRegisterPackageLoader( "rim", Eradicator_RID_LoadFile );
	PlRegisterPackageLoader( "ff", Outwars_FF_LoadFile );
	PlRegisterPackageLoader( "clu", Core_CLU_LoadPackage );
	PlRegisterPackageLoader( "dat", Angel_DAT_LoadPackage );

	PlRegisterImageLoader( "hgt", Core_HGT_ParseImage );
	PlRegisterImageLoader( "tex", Angel_TEX_ParseImage );

	PlRegisterPlugins( "./" );

	/* register all our custom console commands */
	PlRegisterConsoleCommand( "exit", Cmd_Exit, "Exit the application." );
	PlRegisterConsoleCommand( "quit", Cmd_Exit, "Exit the application." );
	PlRegisterConsoleCommand( "mdlconv", Cmd_ConvertModel, "Convert the specified image.\n"
	                                                       "Usage: mdlconv ./model.mdl [./out.mdl]" );
	PlRegisterConsoleCommand( "mdlconvdir", Cmd_BulkConvertModel, "Bulk convert images." );
	PlRegisterConsoleCommand( "imgconv", Cmd_IMGConvert,
	                          "Convert the given image.\n"
	                          "Usage: img_convert ./image.bmp [./out.png]" );
	PlRegisterConsoleCommand( "imgconvdir", Cmd_IMGBulkConvert,
	                          "Bulk convert images in the given directory.\n"
	                          "Usage: img_bulkconvert ./path bmp [./outpath]" );

	PlInitializePlugins();

	/* allow us to just push a command via the command line if we want */
	const char *arg = PlGetCommandLineArgumentValue( "-cmd" );
	if ( arg != NULL ) {
		PlParseConsoleString( arg );
		PlShutdown();
		return EXIT_SUCCESS;
	}

	while ( isRunning ) {
		printf( "> " );

		int i;
		char *p = cmdLine;
		while ( ( i = getchar() ) != '\n' ) {
			*p++ = ( char ) i;

			unsigned int numChars = p - cmdLine;
			if ( numChars >= MAX_COMMAND_LENGTH - 1 ) {
				Error( "Hit character limit!\n" );
				break;
			}
		}

		PlParseConsoleString( cmdLine );

		memset( cmdLine, 0, sizeof( cmdLine ) );
	}

	PlShutdown();

	return EXIT_SUCCESS;
}
