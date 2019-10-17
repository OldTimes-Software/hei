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

#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#if !defined(_MSC_VER)
#include <unistd.h>
#include <dirent.h>
#endif

#include <PL/platform_console.h>

#include "filesystem_private.h"
#include "platform_private.h"

#ifdef _WIN32
/*  this is required by secext.h */
#   define SECURITY_WIN32

#   include <afxres.h>
#   include <security.h>
#   include <direct.h>
#   include <shlobj.h>
#else
#   include <pwd.h>
#endif

/*	File System	*/

PLresult plInitFileSystem(void) {
    return PL_RESULT_SUCCESS;
}

void plShutdownFileSystem(void) {}

// Checks whether a file has been modified or not.
bool plIsFileModified(time_t oldtime, const char *path) {
    if (!oldtime) {
        ReportError(PL_RESULT_FILEERR, "invalid time, skipping check");
        return false;
    }

    struct stat attributes;
    if (stat(path, &attributes) == -1) {
        ReportError(PL_RESULT_FILEERR, "failed to stat %s: %s", path, strerror(errno));
        return false;
    }

    if (attributes.st_mtime > oldtime) {
        return true;
    }

    return false;
}

/**
 * returns the modified time of the given file.
 *
 * @param path
 * @return modification time in seconds. returns 0 upon fail.
 */
time_t plGetFileModifiedTime(const char *path) {
    struct stat attributes;
    if (stat(path, &attributes) == -1) {
        ReportError(PL_RESULT_FILEERR, "failed to stat %s: %s", path, strerror(errno));
        return 0;
    }
    return attributes.st_mtime;
}

// Creates a folder at the given path.
bool plCreateDirectory(const char *path) {
    if(plPathExists(path)) {
        return true;
    }

    if(_pl_mkdir(path) == 0) {
        return true;
    }

    ReportError(PL_RESULT_FILEERR, "%s", strerror(errno));

    return false;
}

bool plCreatePath(const char *path) {
    size_t length = strlen(path);
    char dir_path[length + 1];
    memset(dir_path, 0, sizeof(dir_path));
    for(size_t i = 0; i < length; ++i) {
        dir_path[i] = path[i];
        if(i != 0 &&
            (path[i] == '\\' || path[i] == '/') &&
            (path[i - 1] != '\\' && path[i - 1] != '/')) {
            if(!plCreateDirectory(dir_path)) {
                return false;
            }
        }
    }

    return plCreateDirectory(dir_path);
}

// Returns the extension for the file.
const char *plGetFileExtension(const char *in) {
    const char *s = strrchr(in, '.');
    if(!s || s == in) {
        return "";
    }

    return s + 1;
}

// Strips the extension from the filename.
void plStripExtension(char *dest, size_t length, const char *in) {
    if (plIsEmptyString(in)) {
        *dest = 0;
        return;
    }

    const char *s = strrchr(in, '.');
    while (in < s) {
        if(--length <= 1) {
            break;
        }
        *dest++ = *in++;
    } *dest = 0;
}

/**
 * returns pointer to the last component in the given filename.
 *
 * @param path
 * @return
 */
const char *plGetFileName(const char *path) {
    const char *lslash;
    if((lslash = strrchr(path, '/')) == NULL) {
        lslash = strrchr(path, '\\');
    }

    if (lslash != NULL) {
        path = lslash + 1;
    }

    return path;
}

/** Returns the name of the systems current user.
 *
 * @param out
 */
char *plGetUserName(char *out, size_t n) {
#ifdef _WIN32
    char user_string[PL_SYSTEM_MAX_USERNAME];
    ULONG size = PL_SYSTEM_MAX_USERNAME;
    if (GetUserNameEx(NameDisplay, user_string, &size) == 0) {
        snprintf(user_string, sizeof(user_string), "user");
    }
#else   // Linux
    char *user_string = getenv("LOGNAME");
    if (user_string == NULL) {
        user_string = "user";
    }
#endif

    strncpy(out, user_string, n);
    return out;
}

/** Returns directory for saving application data.
 *
 * @param app_name Name of your application.
 * @param out Buffer we'll be storing the path to.
 * @param n Length of the buffer.
 * @return Pointer to the output, will return NULL on error.
 */
char *plGetApplicationDataDirectory(const char *app_name, char *out, size_t n) {
    if(plIsEmptyString(app_name)) {
        ReportError(PL_RESULT_FILEPATH, "invalid app name");
        return NULL;
    }

#ifndef _WIN32
    const char *home;
    if((home = getenv("HOME")) == NULL) {
        struct passwd *pw = getpwuid(getuid());
        home = pw->pw_dir;
    }
    snprintf(out, n, "%s/.%s", home, app_name);
#else
    char home[MAX_PATH];
    if(SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, home))) {
        snprintf(out, n, "%s/.%s", home, app_name);
        return out;
    }
    snprintf(home, sizeof(home), ".");
#endif

    return out;
}

/** Scans the given directory.
 * on each file that's found, it calls the given function to handle the file.
 *
 * @param path path to directory.
 * @param extension the extension to scan for (exclude '.').
 * @param Function callback function to deal with the file.
 * @param recursive if true, also scans the contents of each sub-directory.
 */
void plScanDirectory(const char *path, const char *extension, void (*Function)(const char *), bool recursive) {
    DIR *directory = opendir(path);
    if (directory) {
        struct dirent *entry;
        while ((entry = readdir(directory))) {
            if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }

            char filestring[PL_SYSTEM_MAX_PATH + 1];
            snprintf(filestring, sizeof(filestring), "%s/%s", path, entry->d_name);

            struct stat st;
            if(stat(filestring, &st) == 0) {
                if(S_ISREG(st.st_mode)) {
                    if(extension == NULL || pl_strcasecmp(plGetFileExtension(entry->d_name), extension) == 0) {
                        Function(filestring);
                    }
                } else if(S_ISDIR(st.st_mode) && recursive) {
                    plScanDirectory(filestring, extension, Function, recursive);
                }
            }
        }

        closedir(directory);
    } else {
        ReportError(PL_RESULT_FILEPATH, "opendir failed!");
    }
}

const char *plGetWorkingDirectory(void) {
    static char out[PL_SYSTEM_MAX_PATH] = { '\0' };
    if (getcwd(out, PL_SYSTEM_MAX_PATH) == NULL) {
        /* The MSDN documentation for getcwd() is gone, but it proooobably uses
         * errno and friends.
         */
        ReportError(PL_RESULT_SYSERR, "%s", strerror(errno));
        return NULL;
    }
    return out;
}

void plSetWorkingDirectory(const char *path) {
    if(chdir(path) != 0) {
        ReportError(PL_RESULT_SYSERR, "%s", strerror(errno));
    }
}

/////////////////////////////////////////////////////////////////////////////////////
// FILE I/O

/**
 * checks whether or not the given file is accessible
 * or exists.
 *
 * @param path
 * @return false if the file wasn't accessible.
 */
bool plFileExists(const char *path) {
    struct stat buffer;
    return (bool) (stat(path, &buffer) == 0);
}

bool plPathExists(const char *path) {
#if defined(_MSC_VER)
	DWORD fa = GetFileAttributes(path);
	if (fa & FILE_ATTRIBUTE_DIRECTORY) {
		return true;
	}
#else
    DIR *dir = opendir(path);
    if(dir) {
        closedir(dir);
        return true;
    }
#endif
    return false;
}

bool plDeleteFile(const char *path) {
    if(!plFileExists(path)) {
        return true;
    }

    int result = remove(path);
    if(result == 0) {
        return true;
    }

    ReportError(PL_RESULT_FILEREAD, strerror(errno));
    return false;
}

bool plWriteFile(const char *path, const uint8_t *buf, size_t length) {
    FILE *fp = fopen(path, "wb");
    if(fp == NULL) {
        ReportError(PL_RESULT_FILEREAD, "failed to open %s", path);
        return false;
    }

    bool result = true;
    if(fwrite(buf, sizeof(char), length, fp) != length) {
        ReportError(PL_RESULT_FILEWRITE, "failed to write entirety of file");
        result = false;
    }

    pl_fclose(fp);

    return result;
}

bool plCopyFile(const char *path, const char *dest) {
    size_t file_size = plGetFileSize(path);
    if(file_size == 0) {
        return false;
    }

    uint8_t *data = pl_malloc(file_size);
    if(data == NULL) {
        return false;
    }

    FILE *original = NULL;
    FILE *copy = NULL;

    // read in the original
    if((original = fopen(path, "rb")) == NULL) {
        ReportError(PL_RESULT_FILEREAD, "failed to open %s", path);
        goto BAIL;
    }
    if(fread(data, 1, file_size, original) != file_size) {
        ReportError(PL_RESULT_FILEREAD, "failed to read in %d bytes for %s", file_size, path);
        goto BAIL;
    }
    pl_fclose(original);

    // write out the copy
    if((copy = fopen(dest, "wb")) == NULL) {
        ReportError(PL_RESULT_FILEWRITE, "failed to open %s for write", dest);
        goto BAIL;
    }
    if(fwrite(data, 1, file_size, copy) != file_size) {
        ReportError(PL_RESULT_FILEWRITE, "failed to write out %d bytes for %s", file_size, path);
        goto BAIL;
    }
    pl_fclose(copy);

    pl_free(data);

    return true;

    BAIL:

    pl_free(data);

    if(original != NULL) {
      pl_fclose(original);
    }

    if(copy != NULL) {
      pl_fclose(copy);
    }

    return false;
}

size_t plGetFileSize(const char *path) {
    struct stat buf;
    if(stat(path, &buf) != 0) {
        ReportError(PL_RESULT_FILEERR, "failed to stat %s: %s", path, strerror(errno));
        return 0;
    }

    return (size_t)buf.st_size;
}

///////////////////////////////////////////

PLFile* plOpenFile(const char *path, bool cache) {
    if(plIsEmptyString(path)) {
        ReportBasicError(PL_RESULT_FILEPATH);
        return NULL;
    }

    FILE *fp = fopen(path, "rb");
    if(fp == NULL) {
        ReportError(PL_RESULT_FILEREAD, strerror(errno));
        return NULL;
    }

    PLFile* ptr = pl_calloc(1, sizeof(PLFile));
    snprintf(ptr->path, sizeof(ptr->path), "%s", path);
    ptr->size = plGetFileSize(path);

    if(cache) {
        ptr->data = pl_malloc(ptr->size * sizeof(uint8_t));
        if(fread(ptr->data, sizeof(uint8_t), ptr->size, fp) != ptr->size) {
            FSLog("Failed to read complete file (%s)!\n", path);
        }
        pl_fclose(fp);
    } else {
        ptr->fptr = fp;
    }

    return (PLFile*)ptr;
}

void plCloseFile(PLFile* ptr) {
    if(ptr == NULL) {
        return;
    }

    if(ptr->fptr != NULL) {
        pl_fclose(ptr->fptr);
    }

    pl_free(ptr->data);
    pl_free(ptr);
}

const char* plGetFilePath(const PLFile* ptr) {
    return ptr->path;
}

size_t plGetFileOffset(const PLFile* ptr) {
    if(ptr->fptr != NULL) {
        return ftell(ptr->fptr);
    }

    return ptr->pos - ptr->data;
}

size_t plReadFile(PLFile* ptr, void* dest, size_t size, size_t count) {
    /* bail early if size is 0 to avoid division by 0 */
    if(size == 0) {
        return 0;
    }

    if(ptr->fptr != NULL) {
        return fread(dest, size, count, ptr->fptr);
    }

    /* ensure that the read is valid */
    size_t length = size * count;
    size_t posn = plGetFileOffset(ptr);
    if(posn + length >= ptr->size) {
        /* out of bounds, truncate it */
        length = ptr->size - posn;
    }

    memcpy(dest, ptr->pos, length);
    ptr->pos += length;
    return length / size;
}

char plReadInt8(PLFile* ptr, bool* status) {
  if(plGetFileOffset(ptr) >= ptr->size) {
    *status = false;
    return 0;
  }

  *status = true;

  if(ptr->fptr != NULL) {
    return (char)(fgetc(ptr->fptr));
  }

  return (char)*(ptr->pos);
}

static int64_t ReadSizedInteger(PLFile* ptr, size_t size, bool big_endian, bool* status) {
  int64_t n;
  if(plReadFile(ptr, &n, size, 1) != 1) {
    *status = false;
    return 0;
  }

  *status = true;

  if(big_endian) {
    if(size == sizeof(int16_t)) {
      return be16toh(n);
    } else if(size == sizeof(int32_t)) {
      return be32toh(n);
    } else if(size == sizeof(int64_t)) {
      return be64toh(n);
    } else {
      *status = false;
      return 0;
    }
  }

  return n;
}

int16_t plReadInt16(PLFile* ptr, bool big_endian, bool* status) {
  return ReadSizedInteger(ptr, sizeof(int16_t), big_endian, status);
}

int32_t plReadInt32(PLFile* ptr, bool big_endian, bool* status) {
  return ReadSizedInteger(ptr, sizeof(int32_t), big_endian, status);
}

int64_t plReadInt64(PLFile* ptr, bool big_endian, bool* status) {
  return ReadSizedInteger(ptr, sizeof(int64_t), big_endian, status);
}

bool plFileSeek(PLFile* ptr, long int pos, PLFileSeek seek) {
    if(ptr->fptr != NULL) {
        return !fseek(ptr->fptr, pos, seek);
    }

    size_t posn = plGetFileOffset(ptr);
    switch(seek) {
        case PL_SEEK_CUR:
            if(posn + pos > ptr->size || pos < -posn) {
                ReportBasicError(PL_RESULT_INVALID_PARM2);
                return false;
            }
            ptr->pos += pos;
            break;

        case PL_SEEK_SET:
            if(pos > ptr->size || pos < 0) {
                ReportBasicError(PL_RESULT_INVALID_PARM2);
                return false;
            }
            ptr->pos = &ptr->data[pos];
            break;

        case PL_SEEK_END:
            if(pos <= -ptr->size) {
                ReportBasicError(PL_RESULT_INVALID_PARM2);
                return false;
            }
            ptr->pos = &ptr->data[ptr->size - pos];
            break;

        default:
          ReportBasicError(PL_RESULT_INVALID_PARM3);
          return false;
    }

    return true;
}

void plFileRewind(PLFile* ptr) {
    if(ptr->fptr != NULL) {
        rewind(ptr->fptr);
        return;
    }

    ptr->pos = ptr->data;
}
