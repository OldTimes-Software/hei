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

#include "model_private.h"
#include "filesystem_private.h"

/* Support for Shadow Man's old and new mesh formats */

/* new formats */

static PLModel *LoadEMsh(const char *path, FILE *fp) {
    ReportError(PL_RESULT_UNSUPPORTED, "EMsh is not supported");
    fclose(fp);
    return NULL;
}

/* old formats */

#define VERSION_WEI 3

typedef struct WEIHeader {
    char    identity[4];    /* last byte is the version */
} WEIHeader;

static PLModel *LoadMESH(const char *path, FILE *fp) {
    uint32_t u0;
    if(fread(&u0, sizeof(uint32_t), 1, fp) != 1) {
        ReportBasicError(PL_RESULT_FILEREAD);
        goto ABORT;
    }

    char list_identifier[4];
    if(fread(list_identifier, sizeof(char), 4, fp) != 4) {
        ReportBasicError(PL_RESULT_FILEREAD);
        goto ABORT;
    }

    if(strncmp(list_identifier, "OLST", 4) != 0) {
        ReportError(PL_RESULT_FILETYPE, "invalid identifier, expected OLST but found %s", list_identifier);
        goto ABORT;
    }

    char filename[64];
    strncpy(filename, plGetFileName(path), sizeof(filename));
    filename[strlen(filename) - 3] = '\0';

    ABORT:

    fclose(fp);
    return NULL;
}

/* */

PLModel *plLoadMSHModel(const char *path) {
    FILE *fp = fopen(path, "rb");
    if(fp == NULL) {
        ReportBasicError(PL_RESULT_FILEREAD);
        return NULL;
    }

    char identifier[4];
    if(fread(identifier, sizeof(char), 4, fp) != 4) {
        ReportBasicError(PL_RESULT_FILEREAD);
        goto ABORT;
    }

    if(strncmp(identifier, "EMsh", 4) == 0) {
        return LoadEMsh(path, fp);
    } else if(strncmp(identifier, "MESH", 4) == 0) {
        return LoadMESH(path, fp);
    }

    ReportError(PL_RESULT_FILETYPE, "unrecognised identifier");

    ABORT:

    fclose(fp);
    return NULL;
}