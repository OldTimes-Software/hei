/**
 * Hei Platform Library
 * Copyright (C) 2017-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is licensed under MIT. See LICENSE for more details.
 */

#pragma once

#include <plcore/pl.h>
#include <plcore/pl_filesystem.h>
#include <plcore/pl_compression.h>

typedef struct PLPackageIndex {
	size_t offset;
	char fileName[ PL_SYSTEM_MAX_PATH ];
	size_t fileSize;
	size_t compressedSize;
	PLCompressionType compressionType;
} PLPackageIndex;

typedef struct PLPackage {
	char path[ PL_SYSTEM_MAX_PATH ];
	unsigned int table_size;
	PLPackageIndex *table;
	struct {
		uint8_t *( *LoadFile )( PLFile *package, PLPackageIndex *index );
	} internal;
} PLPackage;

PL_EXTERN_C

#if !defined( PL_COMPILE_PLUGIN )

PL_EXTERN PLPackage *PlCreatePackageHandle( const char *path, unsigned int tableSize, uint8_t *( *OpenFile )( PLFile *filePtr, PLPackageIndex *index ) );

PL_EXTERN PLPackage *PlLoadPackage( const char *path );
PL_EXTERN PLFile *PlLoadPackageFile( PLPackage *package, const char *path );
PL_EXTERN PLFile *PlLoadPackageFileByIndex( PLPackage *package, unsigned int index );
PL_EXTERN void PlDestroyPackage( PLPackage *package );

PL_EXTERN void PlRegisterPackageLoader( const char *ext, PLPackage *( *LoadFunction )( const char *path ) );
PL_EXTERN void PlRegisterStandardPackageLoaders( void );
PL_EXTERN void PlClearPackageLoaders( void );
PL_EXTERN const char **PlGetSupportedPackageFormats( unsigned int *numElements );

PL_EXTERN const char *PlGetPackagePath( const PLPackage *package );
PL_EXTERN unsigned int PlGetPackageTableSize( const PLPackage *package );
PL_EXTERN int PlGetPackageTableIndex( const PLPackage *package, const char *indexName );

const char *PlGetPackageFileName( const PLPackage *package, unsigned int index );

#endif

PL_EXTERN_C_END
