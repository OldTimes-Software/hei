/**
 * Hei Platform Library
 * Copyright (C) 2017-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is licensed under MIT. See LICENSE for more details.
 */

#pragma once

#define PL_COMPILE_PLUGIN 1
#include <plcore/pl_plugin_interface.h>

PL_EXTERN_C

extern const PLPluginExportTable *gInterface;

extern int vkLogLevel;

PL_EXTERN_C_END
