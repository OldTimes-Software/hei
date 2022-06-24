/**
 * Hei Platform Library
 * Copyright (C) 2017-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is licensed under MIT. See LICENSE for more details.
 */

#pragma once

#include "pl_private.h"

#include <plcore/pl_filesystem.h>
#include <plcore/pl_package.h>

PL_EXTERN_C

PLPackage *PlLoadIWADPackage_( const char *path );
PLPackage *PlLoadWAD2Package_( const char *path );
PLPackage *PlLoadPAKPackage_( const char *path );
PLPackage *PlLoadVPKPackage_( const char *path );
PLPackage *PlLoadMadPackage( const char *path );
PLPackage *PlLoadTabPackage( const char *path );

PL_EXTERN_C_END
