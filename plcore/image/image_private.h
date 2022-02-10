/**
 * Hei Platform Library
 * Copyright (C) 2017-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is licensed under MIT. See LICENSE for more details.
 */

#pragma once

#include "pl_private.h"

#include <plcore/pl_image.h>

PLImage *PlParse3dfImage( PLFile *file );
PLImage *PlParseFtxImage( PLFile *file );
PLImage *PlParseTimImage( PLFile *file );
PLImage *PlParseSwlImage( PLFile *file );
PLImage *PlParseQoiImage( PLFile *file );
PLImage *PlParseDdsImage( PLFile *file );

bool PlWriteQoiImage( const PLImage *image, const char *path );
