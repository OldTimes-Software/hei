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

#include <plcore/pl.h>
#include <plcore/pl_console.h>

#include <plmodel/plm.h>

extern int LOG_LEVEL_MODEL;
#define ModelLog( ... ) PlLogMessage( LOG_LEVEL_MODEL, __VA_ARGS__ )

PL_EXTERN_C

PLMModel *PlmLoadHdvModel( const char *path );
PLMModel *PlmLoadU3DModel( const char *path );
PLMModel *PlmLoadObjModel( const char *path );
PLMModel *PlmLoadRequiemModel( const char *path );
PLMModel *PlmLoadSmdModel( const char *path );

bool PlmWriteSmdModel( PLMModel *model, const char *path );
bool PlmWriteObjModel( PLMModel *model, const char *path );

PL_EXTERN_C_END
