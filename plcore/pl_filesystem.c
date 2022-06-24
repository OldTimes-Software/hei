/**
 * Hei Platform Library
 * Copyright (C) 2017-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is licensed under MIT. See LICENSE for more details.
 */

#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

#if !defined( _MSC_VER )
#	include <unistd.h>
#	include <dirent.h>
#endif

#include <plcore/pl_console.h>
#include <plcore/pl_package.h>

#include "filesystem_private.h"
#include "pl_private.h"

#if defined( _WIN32 )

#	include "3rdparty/portable_endian.h"

/*  this is required by secext.h */
#	define SECURITY_WIN32

#	include <security.h>
#	include <ShlObj.h>
#	include <direct.h>
#	include <io.h>

#	if defined( _MSC_VER )
#		if !defined( S_ISREG ) && defined( S_IFMT ) && defined( S_IFREG )
#			define S_ISREG( m ) ( ( ( m ) &S_IFMT ) == S_IFREG )
#		endif
#	endif
#else
#	if defined( __APPLE__ )
#		include "3rdparty/portable_endian.h"
#	endif
#	include <pwd.h>
#endif

/* this is gross... but provide a dumb interface so we can
 * use 64-bit file calls if they're available and we're on
 * a system that supports it. */

static PLFileOffset pl_ftell( FILE *file ) {
#ifdef PL_FILESYSTEM_64
#	if ( PL_SYSTEM_OS == PL_SYSTEM_OS_WINDOWS )
#		if defined( _MSC_VER )
	return _ftelli64( file );
#		else
	return ftello64( file );
#		endif
#	else
	return ftello( file );
#	endif
#else
	return ftell( file );
#endif
}

static int pl_fseek( FILE *file, PLFileOffset off, int wence ) {
#ifdef PL_FILESYSTEM_64
#	if ( PL_SYSTEM_OS == PL_SYSTEM_OS_WINDOWS )
#		if defined( _MSC_VER )
	return _fseeki64( file, off, wence );
#		else
	return fseeko64( file, off, wence );
#		endif
#	else
	return fseeko( file, off, wence );
#	endif
#else
	return fseek( file, off, wence );
#endif
}

/*	File System	*/

static void PlNormalizePath_( char *path, size_t length ) {
	for ( size_t i = 0; i < length; ++i ) {
		if ( path[ i ] == '\0' ) {
			if ( path[ i - 1 ] == '/' ) {
				path[ i - 1 ] = '\0';
			}
			break;
		} else if ( path[ i ] != '\\' ) {
			continue;
		}

		path[ i ] = '/';
	}
}

/** FS Mounting **/
/** Future
 * 		- Descriptor for locations; then can use <location>:// to mount from a specific location
 */

typedef struct PLFileSystemMount {
	PLFileSystemMountType type;
	PLPackage *pkg;                  /* PL_FS_MOUNT_PACKAGE */
	char path[ PL_SYSTEM_MAX_PATH ]; /* PL_FS_MOUNT_DIR */
	struct PLFileSystemMount *next, *prev;
} PLFileSystemMount;
static PLFileSystemMount *fs_mount_root = NULL;
static PLFileSystemMount *fs_mount_ceiling = NULL;

#define VFS_LOCAL_HINT "local://"
#define VFS_MAX_HINT   16
#define VFS_MAX_PATH   ( ( PL_SYSTEM_MAX_PATH + VFS_MAX_HINT ) + 1 )

PL_STATIC_ASSERT( sizeof( VFS_LOCAL_HINT ) < VFS_MAX_HINT, "Local hint is larger than maximum hint length, please adjust limit!" );

IMPLEMENT_COMMAND( pkgext, "Extract the contents of a package." ) {
	if ( argc == 1 ) {
		Print( "%s", pkgext_var.description );
		return;
	}

	const char *path = argv[ 1 ];
	if ( path == NULL ) {
		PrintWarning( "Invalid path specified!\n" );
		return;
	}

	PLPackage *pkg = PlLoadPackage( path );
	if ( pkg == NULL ) {
		PrintWarning( "Failed to load package \"%s\"!\nPL: %s\n", path, PlGetError() );
		return;
	}

	for ( unsigned int i = 0; i < pkg->table_size; ++i ) {
		PLFile *file = PlLoadPackageFileByIndex( pkg, i );
		if ( file == NULL ) {
			PrintWarning( "Failed to load file at index %d, \"%s\"!\nPL: %s\n", i, pkg->table[ i ].fileName, PlGetError() );
			continue;
		}

		char pkgPath[ PL_SYSTEM_MAX_PATH ];
		memset( pkgPath, 0, sizeof( pkgPath ) );
		strncpy( pkgPath, file->path, strlen( file->path ) - strlen( PlGetFileName( file->path ) ) );

		char outPath[ PL_SYSTEM_MAX_PATH + 10 ];
		snprintf( outPath, sizeof( outPath ), "extracted/%s", pkgPath );
		if ( !PlCreatePath( outPath ) ) {
			PrintWarning( "Failed to create path, \"%s\"!\nPL: %s\n", outPath, PlGetError() );
			break;
		}

		snprintf( outPath, sizeof( outPath ), "extracted/%s", file->path );

		FILE *fout = fopen( outPath, "wb" );
		if ( fout == NULL ) {
			PrintWarning( "Failed to write file to destination, \"%s\"!\n", outPath );
			continue;
		}
		fwrite( PlGetFileData( file ), sizeof( uint8_t ), PlGetFileSize( file ), fout );
		fclose( fout );

		Print( "Wrote \"%s\"\n", outPath );
	}
	Print( "End\n" );

	PlDestroyPackage( pkg );
}

IMPLEMENT_COMMAND( pkglst, "List all the files in a particular package." ) {
	if ( argc == 1 ) {
		Print( "%s", pkglst_var.description );
		return;
	}

	const char *path = argv[ 1 ];
	if ( path == NULL ) {
		Print( "Invalid path specified!\n" );
		return;
	}

	PLPackage *pkg = PlLoadPackage( path );
	if ( pkg == NULL ) {
		Print( "Failed to load package \"%s\"!\nPL: %s\n", path, PlGetError() );
		return;
	}

	Print( "Listing contents of %s (%d)...\n", path, pkg->table_size );
	for ( unsigned int i = 0; i < pkg->table_size; ++i ) {
		Print( "\n"
		       " num:    %d\n"
		       " name:   %s\n"
		       " size:   %u\n"
		       " csize:  %u\n"
		       " ctype:  %d\n"
		       " offset: %u\n",
		       i,
		       pkg->table[ i ].fileName,
		       pkg->table[ i ].fileSize,
		       pkg->table[ i ].compressedSize,
		       pkg->table[ i ].compressionType,
		       pkg->table[ i ].offset );
	}
	Print( "End\n" );

	PlDestroyPackage( pkg );
}

IMPLEMENT_COMMAND( lstmnt, "Lists all of the mounted directories." ) {
	PlUnused( argv );
	PlUnused( argc );

	if ( fs_mount_root == NULL ) {
		Print( "No locations mounted\n" );
		return;
	}

	unsigned int numLocations = 0;
	PLFileSystemMount *location = fs_mount_root;
	while ( location != NULL ) {
		numLocations++;

		/* this sucks... */
		const char *path;
		if ( location->type == PL_FS_MOUNT_PACKAGE )
			path = location->pkg->path;
		else
			path = location->path;

		Print( " (%d) %-20s : %-20s\n", numLocations, path, location->type == PL_FS_MOUNT_DIR ? "DIRECTORY" : "PACKAGE" );
		location = location->next;
	}
	Print( "%d locations mounted\n", numLocations );
}

IMPLEMENT_COMMAND( unmnt, "Unmount the specified directory." ) {
	if ( argc == 1 ) {
		Print( "%s", unmnt_var.description );
		return;
	}

	if ( fs_mount_root == NULL ) {
		Print( "No locations mounted\n" );
		return;
	}

	PLFileSystemMount *location = fs_mount_root;
	while ( location != NULL ) {
		if ( strcasecmp( argv[ 1 ], location->path ) == 0 ) {
			PlClearMountedLocation( location );
			Print( "Done!\n" );
			return;
		}

		location = location->next;
	}

	Print( "Failed to find location: \"%s\"!\n", argv[ 1 ] );
}

IMPLEMENT_COMMAND( mnt, "Mount the specified directory." ) {
	if ( argc == 1 ) {
		Print( "%s", mnt_var.description );
		return;
	}

	const char *path = argv[ 1 ];
	if ( path == NULL ) {
		Print( "Invalid path specified!\n" );
		return;
	}

	PlMountLocation( path );
}

static void PlRegisterFSCommands_( void ) {
	PLConsoleCommand fsCommands[] = {
	        pkgext_var,
	        pkglst_var,
	        lstmnt_var,
	        unmnt_var,
	        mnt_var,
	};
	for ( unsigned int i = 0; i < PL_ARRAY_ELEMENTS( fsCommands ); ++i ) {
		PlRegisterConsoleCommand( fsCommands[ i ].cmd, fsCommands[ i ].Callback, fsCommands[ i ].description );
	}
}

void PlClearMountedLocation( PLFileSystemMount *location ) {
	if ( location->type == PL_FS_MOUNT_PACKAGE ) {
		PlDestroyPackage( location->pkg );
		location->pkg = NULL;
	}

	if ( location->prev != NULL ) {
		location->prev->next = location->next;
	}
	if ( location->next != NULL ) {
		location->next->prev = location->prev;
	}

	/* ensure root and ceiling are always pointing to a valid location */
	if ( location == fs_mount_root ) {
		fs_mount_root = location->next;
	}
	if ( location == fs_mount_ceiling ) {
		fs_mount_ceiling = location->prev;
	}

	PlFree( location );
}

/**
 * Clear all of the mounted locations
 */
void PlClearMountedLocations( void ) {
	while ( fs_mount_root != NULL ) { PlClearMountedLocation( fs_mount_root ); }
}

static void PlInsertMountLocation_( PLFileSystemMount *location ) {
	if ( fs_mount_root == NULL ) {
		fs_mount_root = location;
	}

	location->prev = fs_mount_ceiling;
	if ( fs_mount_ceiling != NULL ) {
		fs_mount_ceiling->next = location;
	}
	fs_mount_ceiling = location;
	location->next = NULL;
}

PLFileSystemMount *PlMountLocalLocation( const char *path ) {
	if ( PlLocalPathExists( path ) ) { /* attempt to mount it as a path */
		PLFileSystemMount *location = PlMAllocA( sizeof( PLFileSystemMount ) );
		PlInsertMountLocation_( location );
		location->type = PL_FS_MOUNT_DIR;
		snprintf( location->path, sizeof( location->path ), "%s", path );

		PlNormalizePath_( location->path, sizeof( location->path ) );

		Print( "Mounted directory %s successfully!\n", location->path );

		return location;
	}

#if 0
	// LoadPackage operates via the VFS, but we want to enforce a local
	// path here, so the only reasonable solution right now is to prefix
	// it with the local dir hint
	char localPath[ VFS_MAX_PATH ];
	if ( strncmp( VFS_LOCAL_HINT, path, sizeof( VFS_LOCAL_HINT ) ) != 0 ) {
		snprintf( localPath, sizeof( localPath ), VFS_LOCAL_HINT "%s", path );
	} else {
		snprintf( localPath, sizeof( localPath ), "%s", path );
	}
#endif

	PLPackage *pkg = PlLoadPackage( path );
	if ( pkg != NULL ) {
		PLFileSystemMount *location = PlMAllocA( sizeof( PLFileSystemMount ) );
		PlInsertMountLocation_( location );
		location->type = PL_FS_MOUNT_PACKAGE;
		location->pkg = pkg;

		Print( "Mounted package %s successfully!\n", path );

		return location;
	}

	PlReportErrorF( 0, "failed to mount location, %s", path );
	return NULL;
}

/**
 * Mount the given location. On failure returns -1.
 */
PLFileSystemMount *PlMountLocation( const char *path ) {
	char buf[ VFS_MAX_PATH ];
	const char *vpath = PlResolveVirtualPath_( path, buf, sizeof( buf ) );
	if ( vpath == NULL ) {
		PlReportErrorF( PL_RESULT_FILEPATH, "failed to resolve path, %s", path );
		return NULL;
	}

	return PlMountLocalLocation( vpath );
}

PLFileSystemMountType PlGetMountLocationType( const PLFileSystemMount *fileSystemMount ) {
	return fileSystemMount->type;
}

const char *PlGetMountLocationPath( const PLFileSystemMount *fileSystemMount ) {
	if ( fileSystemMount->type != PL_FS_MOUNT_DIR ) {
		return NULL;
	}

	return fileSystemMount->path;
}

/****/

PLFunctionResult PlInitFileSystem( void ) {
	PlRegisterFSCommands_();

	PlClearMountedLocations();
	return PL_RESULT_SUCCESS;
}

void PlShutdownFileSystem( void ) {
	PlClearMountedLocations();
}

// Checks whether a file has been modified or not.
bool PlIsFileModified( time_t oldtime, const char *path ) {
	if ( !oldtime ) {
		PlReportErrorF( PL_RESULT_FILEERR, "invalid time, skipping check" );
		return false;
	}

	struct stat attributes;
	if ( stat( path, &attributes ) == -1 ) {
		PlReportErrorF( PL_RESULT_FILEERR, "failed to stat %s: %s", path, strerror( errno ) );
		return false;
	}

	if ( attributes.st_mtime > oldtime ) {
		return true;
	}

	return false;
}

bool PlIsEndOfFile( const PLFile *ptr ) {
	return ( ( size_t ) PlGetFileOffset( ptr ) == PlGetFileSize( ptr ) );
}

/**
 * Returns the modified time of the given file.
 * @param path
 * @return Modification time in seconds. returns 0 upon fail.
 */
time_t PlGetLocalFileTimeStamp( const char *path ) {
	struct stat attributes;
	if ( stat( path, &attributes ) == -1 ) {
		PlReportErrorF( PL_RESULT_FILEERR, "failed to stat %s: %s", path, strerror( errno ) );
		return 0;
	}

	return attributes.st_mtime;
}

time_t PlGetFileTimeStamp( PLFile *ptr ) {
	/* timestamp defaults to -1 for files loaded locally */
	if ( ptr->timeStamp < 0 ) {
		PlGetLocalFileTimeStamp( ptr->path );
	}

	return ptr->timeStamp;
}

// Creates a folder at the given path.
bool PlCreateDirectory( const char *path ) {
	if ( PlLocalPathExists( path ) ) {
		return true;
	}

	if ( _pl_mkdir( path ) == 0 ) {
		return true;
	}

	PlReportErrorF( PL_RESULT_FILEERR, "%s", strerror( errno ) );

	return false;
}

bool PlCreatePath( const char *path ) {
	size_t length = strlen( path );
	if ( length >= PL_SYSTEM_MAX_PATH ) {
		PlReportErrorF( PL_RESULT_INVALID_PARM1, "path is greater that maximum supported path length, %d vs %d",
		                length, PL_SYSTEM_MAX_PATH );
		return false;
	}

	char dir_path[ PL_SYSTEM_MAX_PATH ];
	memset( dir_path, 0, sizeof( dir_path ) );
	for ( size_t i = 0; i < length; ++i ) {
		dir_path[ i ] = path[ i ];
		if ( i != 0 &&
		     ( path[ i ] == '\\' || path[ i ] == '/' ) &&
		     ( path[ i - 1 ] != '\\' && path[ i - 1 ] != '/' ) ) {
			if ( !PlCreateDirectory( dir_path ) ) {
				return false;
			}
		}
	}

	return PlCreateDirectory( dir_path );
}

// Returns the extension for the file.
const char *PlGetFileExtension( const char *in ) {
	const char *s = strrchr( in, '.' );
	if ( !s || s == in ) {
		return "";
	}

	return s + 1;
}

// Strips the extension from the filename.
void PlStripExtension( char *dest, size_t length, const char *in ) {
	if ( PL_INVALID_STRING( in ) ) {
		*dest = 0;
		return;
	}

	const char *s = strrchr( in, '.' );
	while ( in < s ) {
		if ( --length <= 1 ) {
			break;
		}
		*dest++ = *in++;
	}
	*dest = 0;
}

/**
 * Returns pointer to the last component in the given filename.
 *
 * @param path
 * @return
 */
const char *PlGetFileName( const char *path ) {
	const char *lslash;
	if ( ( lslash = strrchr( path, '/' ) ) == NULL ) {
		lslash = strrchr( path, '\\' );
	}

	if ( lslash != NULL ) {
		path = lslash + 1;
	}

	return path;
}

/** Returns the name of the systems current user.
 *
 * @param out
 */
char *PlGetUserName( char *out, size_t n ) {
#ifdef _WIN32
	char user_string[ PL_SYSTEM_MAX_USERNAME ];
	ULONG size = PL_SYSTEM_MAX_USERNAME;
	if ( GetUserNameEx( NameDisplay, user_string, &size ) == 0 ) {
		snprintf( user_string, sizeof( user_string ), "user" );
	}
#else// Linux
	char *user_string = getenv( "LOGNAME" );
	if ( user_string == NULL ) {
		user_string = "user";
	}
#endif

	strncpy( out, user_string, n );
	return out;
}

/** Returns directory for saving application data.
 *
 * @param app_name Name of your application.
 * @param out Buffer we'll be storing the path to.
 * @param n Length of the buffer.
 * @return Pointer to the output, will return NULL on error.
 */
char *PlGetApplicationDataDirectory( const char *app_name, char *out, size_t n ) {
	if ( PL_INVALID_STRING( app_name ) ) {
		PlReportErrorF( PL_RESULT_FILEPATH, "invalid app name" );
		return NULL;
	}

#ifndef _WIN32
	const char *home;
	if ( ( home = getenv( "HOME" ) ) == NULL ) {
		struct passwd *pw = getpwuid( getuid() );
		home = pw->pw_dir;
	}
	snprintf( out, n, "%s/.config/%s/", home, app_name );
#else
	char home[ MAX_PATH ];
	if ( SUCCEEDED( SHGetFolderPath( NULL, CSIDL_APPDATA, NULL, 0, home ) ) ) {
		snprintf( out, n, "%s/%s", home, app_name );
		return out;
	}
	snprintf( home, sizeof( home ), "." );
#endif

	return out;
}

/**
 * Returns true if the path ends in a forward/backward slash.
 */
bool PlPathEndsInSlash( const char *p ) {
	size_t l = strlen( p );
	return ( l > 0 && ( p[ l - 1 ] == '/' || p[ l - 1 ] == '\\' ) );
}

typedef struct FSScanInstance {
	char path[ PL_SYSTEM_MAX_PATH ];
	struct FSScanInstance *next;
} FSScanInstance;

static void ScanLocalDirectory( const PLFileSystemMount *mount, FSScanInstance **fileList, const char *path,
                                const char *extension, void ( *Function )( const char *, void * ), bool recursive, void *userData ) {
#if !defined( _MSC_VER )
	DIR *directory = opendir( path );
	if ( directory ) {
		struct dirent *entry;
		while ( ( entry = readdir( directory ) ) ) {
			if ( strcmp( entry->d_name, "." ) == 0 || strcmp( entry->d_name, ".." ) == 0 ) {
				continue;
			}

			char filestring[ PL_SYSTEM_MAX_PATH + 1 ];
			snprintf( filestring, sizeof( filestring ), PlPathEndsInSlash( path ) ? "%s%s" : "%s/%s", path, entry->d_name );

			struct stat st;
			if ( stat( filestring, &st ) == 0 ) {
				if ( S_ISREG( st.st_mode ) ) {
					if ( extension == NULL || pl_strcasecmp( PlGetFileExtension( entry->d_name ), extension ) == 0 ) {
						if ( mount == NULL ) {
							Function( filestring, userData );
							continue;
						}

						size_t pos = strlen( mount->path ) + 1;
						const char *filePath = &filestring[ pos ];

						// Ensure it's not already in the list
						FSScanInstance *cur = *fileList;
						while ( cur != NULL ) {
							if ( strcmp( filePath, cur->path ) == 0 ) {
								// File was already passed back
								break;
							}

							cur = cur->next;
						}

						// Jumped the list early, so abort here
						if ( cur != NULL ) {
							continue;
						}

						Function( filePath, userData );

						// Tack it onto the list
						cur = PlCAllocA( 1, sizeof( FSScanInstance ) );
						strncpy( cur->path, filePath, sizeof( cur->path ) );
						cur->next = *fileList;
						*fileList = cur;
					}
				} else if ( S_ISDIR( st.st_mode ) && recursive ) {
					ScanLocalDirectory( mount, fileList, filestring, extension, Function, recursive, userData );
				}
			}
		}

		closedir( directory );
	} else {
		PlReportErrorF( PL_RESULT_FILEPATH, "opendir failed!" );
	}
#else /* assumed win32 impl */
	if ( extension == NULL ) {
		extension = "*";
	}

	char selectorPath[ PL_SYSTEM_MAX_PATH ];
	snprintf( selectorPath, sizeof( selectorPath ), PlPathEndsInSlash( path ) ? "%s*.%s" : "%s/*.%s", path, extension );

	WIN32_FIND_DATA ffd;
	HANDLE find = FindFirstFile( selectorPath, &ffd );
	if ( find == INVALID_HANDLE_VALUE ) {
		return;
	}

	size_t ml;
	if ( mount != NULL )
		ml = strlen( mount->path );

	do {
		snprintf( selectorPath, sizeof( selectorPath ), PlPathEndsInSlash( path ) ? "%s%s" : "%s/%s", path, ffd.cFileName );

		if ( ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) {
			if ( recursive && !( strcmp( ffd.cFileName, "." ) == 0 || strcmp( ffd.cFileName, ".." ) == 0 ) ) {
				ScanLocalDirectory( mount, fileList, selectorPath, extension, Function, recursive, userData );
			}
			continue;
		}

		const char *rp = selectorPath;
		if ( mount != NULL ) {
			rp = &selectorPath[ ml ];
		}

		Function( rp, userData );
	} while ( FindNextFile( find, &ffd ) != FALSE );

	FindClose( find );
#endif
}

/**
 * Scans the given directory.
 *
 * @param path path to directory.
 * @param extension the extension to scan for (exclude '.').
 * @param Function callback function to deal with the file.
 * @param recursive if true, also scans the contents of each sub-directory.
 */
void PlScanDirectory( const char *path, const char *extension, void ( *Function )( const char *, void * ), bool recursive, void *userData ) {
	if ( strncmp( VFS_LOCAL_HINT, path, sizeof( VFS_LOCAL_HINT ) ) == 0 ) {
		ScanLocalDirectory( NULL, NULL, path + sizeof( VFS_LOCAL_HINT ), extension, Function, recursive, userData );
		return;
	}

	// If no mounted locations, assume local scan
	if ( fs_mount_root == NULL ) {
		ScanLocalDirectory( NULL, NULL, path, extension, Function, recursive, userData );
		return;
	}

	PLPath normPath;
	snprintf( normPath, sizeof( normPath ), "%s", path );
	PlNormalizePath_( normPath, sizeof( normPath ) );

	FSScanInstance *fileList = NULL;
	PLFileSystemMount *location = fs_mount_root;
	while ( location != NULL ) {
		if ( location->type == PL_FS_MOUNT_PACKAGE ) {
			// todo: Only works for directories for now
		} else if ( location->type == PL_FS_MOUNT_DIR ) {
			char mounted_path[ PL_SYSTEM_MAX_PATH + 1 ];
			snprintf( mounted_path, sizeof( mounted_path ), "%s/%s", location->path, normPath );
			ScanLocalDirectory( location, &fileList, mounted_path, extension, Function, recursive, userData );
		}

		location = location->next;
	}

	// Clean up the list
	FSScanInstance *current = fileList;
	while ( current != NULL ) {
		FSScanInstance *prev = current;
		current = current->next;
		PlFree( prev );
	}
}

const char *PlGetWorkingDirectory( void ) {
	static char out[ PL_SYSTEM_MAX_PATH ] = { '\0' };
	if ( getcwd( out, PL_SYSTEM_MAX_PATH ) == NULL ) {
		/* The MSDN documentation for getcwd() is gone, but it proooobably uses
		 * errno and friends.
		 */
		PlReportErrorF( PL_RESULT_SYSERR, "%s", strerror( errno ) );
		return NULL;
	}
	return out;
}

void PlSetWorkingDirectory( const char *path ) {
	if ( chdir( path ) != 0 ) {
		PlReportErrorF( PL_RESULT_SYSERR, "%s", strerror( errno ) );
	}
}

const char *PlGetExecutablePath( char *out, size_t outSize ) {
	PL_ZERO( out, outSize );

#if PL_SYSTEM_OS == PL_SYSTEM_OS_LINUX
	if ( readlink( "/proc/self/exe", out, outSize ) == -1 ) {
		return NULL;
	}
#elif PL_SYSTEM_OS == PL_SYSTEM_OS_FREEBSD
	if ( readlink( "/proc/curproc/file", out, outSize ) == -1 ) {
		return NULL;
	}
#elif PL_SYSTEM_OS == PL_SYSTEM_OS_WINDOWS
	GetModuleFileName( NULL, out, outSize );
#else
	PlReportBasicError( PL_RESULT_UNSUPPORTED );
#endif

	PlNormalizePath_( out, outSize );

	return out;
}

/**
 * Returns the directory the current executable is located within.
 */
const char *PlGetExecutableDirectory( char *out, size_t outSize ) {
	if ( PlGetExecutablePath( out, outSize ) == NULL ) {
		return NULL;
	}

	char *c = strrchr( out, '/' );
	if ( c == NULL ) {
		PlReportErrorF( PL_RESULT_FILEPATH, "couldn't find path seperator" );
		return NULL;
	}

	unsigned int i = c - out + 1;
	if ( i < outSize ) {
		out[ i ] = '\0';
	}

	return out;
}

/////////////////////////////////////////////////////////////////////////////////////
// FILE I/O

/**
 * Transform the given path to the direct path
 * relative to anything mounted under the VFS.
 */
static PLFileSystemMount *PlGetMountLocationForPath_( const char *path ) {
	if ( PlLocalPathExists( path ) || PlLocalFileExists( path ) ) {
		return NULL;
	}

#if 0
	if ( strncmp( VFS_LOCAL_HINT, path, sizeof( VFS_LOCAL_HINT ) ) == 0 ) {
		return NULL;
	}
#endif

	size_t sl = strlen( path );
	if ( fs_mount_root != NULL ) {
		PLFileSystemMount *location = fs_mount_root;
		while ( location != NULL ) {
			switch ( location->type ) {
				default: {
					/* todo: don't allow path to search outside of mounted path */
					char buf[ VFS_MAX_PATH ];
					snprintf( buf, sizeof( buf ), "%s/%s", location->path, path );
					if ( PlLocalPathExists( buf ) || PlLocalFileExists( buf ) ) {
						return location;
					}
					break;
				}
				case PL_FS_MOUNT_PACKAGE: {
					for ( unsigned int i = 0; i < location->pkg->table_size; ++i ) {
						/* packages don't necessarily have the concept of a directory
						 * the way we might expect (take Unreal packages for example),
						 * so down the line we might want to allow this to be handled
						 * by the specific package API (i.e. call to GetPackageDirectory?). */
						if ( strncmp( path, location->pkg->table[ i ].fileName, sl ) == 0 ) {
							return location;
						}
					}
					break;
				}
			}

			location = location->next;
		}
	}

	return NULL;
}

static const char *PlVirtualToLocalPath_( PLFileSystemMount *mount, const char *path, char *dest, size_t size ) {
	if ( mount == NULL || mount->type == PL_FS_MOUNT_PACKAGE ) {
		snprintf( dest, size, "%s", path );
	} else {
		const char *fmt;
		if ( *path == '\\' || *path == '/' )
			fmt = "%s%s";
		else
			fmt = "%s/%s";

		snprintf( dest, size, fmt, mount->path, path );
	}

	return dest;
}

/**
 * Transform the given path to the direct path
 * relative to anything mounted under the VFS.
 */
const char *PlResolveVirtualPath_( const char *path, char *dest, size_t size ) {
	if ( strncmp( VFS_LOCAL_HINT, path, sizeof( VFS_LOCAL_HINT ) ) == 0 ) {
		path += sizeof( VFS_LOCAL_HINT );
		return path;
	}

	PLFileSystemMount *mount = PlGetMountLocationForPath_( path );
	if ( mount == NULL || mount->type == PL_FS_MOUNT_PACKAGE ) {
		return path;
	}

	const char *fmt;
	if ( *path == '\\' || *path == '/' )
		fmt = "%s%s";
	else
		fmt = "%s/%s";

	snprintf( dest, size, fmt, mount->path, path );
	return dest;
}

bool PlLocalFileExists( const char *path ) {
	struct stat buffer;
	return ( bool ) ( stat( path, &buffer ) == 0 );
}

/**
 * Checks whether or not the given file is accessible or exists.
 * @param path
 * @return False if the file wasn't accessible.
 */
bool PlFileExists( const char *path ) {
	PLFile *file = PlOpenFile( path, false );
	if ( file == NULL ) {
		return false;
	}

	PlCloseFile( file );
	return true;
}

bool PlLocalPathExists( const char *path ) {
#if defined( _MSC_VER )
	errno_t err = _access_s( path, 0 );
	if ( err != 0 )
		return false;

	struct _stat s;
	_stat( path, &s );
	return ( s.st_mode & S_IFDIR );
#else
	DIR *dir = opendir( path );
	if ( dir ) {
		closedir( dir );
		return true;
	}

	return false;
#endif
}

bool PlPathExists( const char *path ) {
	PLFileSystemMount *mount = PlGetMountLocationForPath_( path );
	if ( mount == NULL )
		return PlLocalPathExists( path );

	return true;
}

/**
 * Deletes the specified file. Not VFS compatible.
 * @param path Path to the file you want to delete.
 * @return True on success and false on fail.
 */
bool PlDeleteFile( const char *path ) {
	if ( !PlLocalFileExists( path ) ) {
		return true;
	}

	int result = remove( path );
	if ( result == 0 ) {
		return true;
	}

	PlReportErrorF( PL_RESULT_FILEREAD, strerror( errno ) );
	return false;
}

/**
 * Write the data to the specified location. Not VFS compatible.
 * @param path Path to the destination.
 * @param buf Data buffer for whatever you're writing.
 * @param length Length of the data buffer.
 * @return True on success and false on fail.
 */
bool PlWriteFile( const char *path, const uint8_t *buf, size_t length ) {
	FILE *fp = fopen( path, "wb" );
	if ( fp == NULL ) {
		PlReportErrorF( PL_RESULT_FILEREAD, "failed to open %s", path );
		return false;
	}

	bool result = true;
	if ( fwrite( buf, sizeof( char ), length, fp ) != length ) {
		PlReportErrorF( PL_RESULT_FILEWRITE, "failed to write entirety of file" );
		result = false;
	}

	_pl_fclose( fp );

	return result;
}

bool PlCopyFile( const char *path, const char *dest ) {
	// read in the original
	PLFile *original = PlOpenFile( path, true );
	if ( original == NULL ) {
		PlReportErrorF( PL_RESULT_FILEREAD, "failed to open %s", path );
		return false;
	}

	// write out the copy
	FILE *copy = fopen( dest, "wb" );
	if ( copy == NULL ) {
		PlReportErrorF( PL_RESULT_FILEWRITE, "failed to open %s for write", dest );
		goto BAIL;
	}

	if ( fwrite( original->data, 1, original->size, copy ) != original->size ) {
		PlReportErrorF( PL_RESULT_FILEWRITE, "failed to write out %d bytes for %s", original->size, path );
		goto BAIL;
	}

	_pl_fclose( copy );

	PlCloseFile( original );
	return true;

BAIL:

	if ( copy != NULL ) {
		_pl_fclose( copy );
	}

	PlCloseFile( original );
	return false;
}

size_t PlGetLocalFileSize( const char *path ) {
	struct stat buf;
	if ( stat( path, &buf ) != 0 ) {
		PlReportErrorF( PL_RESULT_FILEERR, "failed to stat %s: %s", path, strerror( errno ) );
		return 0;
	}

	return ( size_t ) buf.st_size;
}

///////////////////////////////////////////

/**
 * Creates a virtual file handle from the given buffer.
 * If 'isOwner' is true, the file handle takes ownership of the buffer and frees it.
 */
PLFile *PlCreateFileFromMemory( const char *path, void *buf, size_t bufSize, PLFileMemoryBufferType bufType ) {
	PLFile *file = PlMAllocA( sizeof( PLFile ) );

	if ( bufType == PL_FILE_MEMORYBUFFERTYPE_COPY ) {
		file->data = PlMAllocA( bufSize );
		memcpy( file->data, buf, bufSize );
	} else {
		if ( bufType == PL_FILE_MEMORYBUFFERTYPE_UNMANAGED ) {
			file->isUnmanaged = true;
		}
		file->data = buf;
	}
	file->pos = file->data;
	file->size = bufSize;

	snprintf( file->path, sizeof( file->path ), "%s", path );

	return file;
}

PLFile *PlOpenLocalFile( const char *path, bool cache ) {
	FILE *fp = fopen( path, "rb" );
	if ( fp == NULL ) {
		PlReportErrorF( PL_RESULT_FILEREAD, strerror( errno ) );
		return NULL;
	}

	PLFile *ptr = PlCAllocA( 1, sizeof( PLFile ) );
	snprintf( ptr->path, sizeof( ptr->path ), "%s", path );
	ptr->size = PlGetLocalFileSize( path );

	if ( cache ) {
		ptr->data = PlMAllocA( ptr->size * sizeof( uint8_t ) );
		ptr->pos = ptr->data;
		if ( fread( ptr->data, sizeof( uint8_t ), ptr->size, fp ) != ptr->size ) {
			FSLog( "Failed to read complete file (%s)!\n", path );
		}
		_pl_fclose( fp );
	} else {
		ptr->fptr = fp;
	}

	/* timestamp for local files is a special case */
	ptr->timeStamp = -1;

	return ptr;
}

/**
 * Opens the specified file via the VFS.
 * @param path Path to the file you want to open.
 * @param cache Whether or not to cache the entire file into memory.
 * @return Returns handle to the file instance.
 */
PLFile *PlOpenFile( const char *path, bool cache ) {
	const char *p = PlGetPathForAlias( path );
	if ( p != NULL ) {
		path = p;
	}

	PLFileSystemMount *mount = PlGetMountLocationForPath_( path );
	if ( mount != NULL && mount->type == PL_FS_MOUNT_PACKAGE ) {
		return PlLoadPackageFile( mount->pkg, path );
	}

	char buf[ PL_SYSTEM_MAX_PATH ];
	PlVirtualToLocalPath_( mount, path, buf, sizeof( buf ) );

	return PlOpenLocalFile( buf, cache );
}

void PlCloseFile( PLFile *ptr ) {
	if ( ptr == NULL ) {
		return;
	}

	if ( ptr->fptr != NULL ) {
		_pl_fclose( ptr->fptr );
	}

	if ( !ptr->isUnmanaged ) {
		PlFree( ptr->data );
	}
	PlFree( ptr );
}

/**
 * Returns the path of the current open file.
 * @param ptr Pointer to file handle.
 * @return Full path to the current file.
 */
const char *PlGetFilePath( const PLFile *ptr ) {
	return ptr->path;
}

const void *PlGetFileData( const PLFile *ptr ) {
	return ptr->data;
}

/**
 * Returns file size in bytes.
 * @param ptr Pointer to file handle.
 * @return Number of bytes within file.
 */
size_t PlGetFileSize( const PLFile *ptr ) {
	if ( ptr->fptr != NULL ) {
		return PlGetLocalFileSize( ptr->path );
	}

	return ptr->size;
}

/**
 * Returns the current position within the file handle (ftell).
 * @param ptr Pointer to the file handle.
 * @return Number of bytes into the file.
 */
PLFileOffset PlGetFileOffset( const PLFile *ptr ) {
	if ( ptr->fptr != NULL ) {
		return pl_ftell( ptr->fptr );
	}

	return ptr->pos - ptr->data;
}

size_t PlReadFile( PLFile *ptr, void *dest, size_t size, size_t count ) {
	/* bail early if size is 0 to avoid division by 0 */
	if ( size == 0 ) {
		PlReportBasicError( PL_RESULT_FILESIZE );
		return 0;
	}

	if ( ptr->fptr != NULL ) {
		return fread( dest, size, count, ptr->fptr );
	}

	/* ensure that the read is valid */
	size_t length = size * count;
	PLFileOffset posn = PlGetFileOffset( ptr );
	if ( posn + length >= ptr->size ) {
		/* out of bounds, truncate it */
		length = ptr->size - posn;
	}

	memcpy( dest, ptr->pos, length );
	ptr->pos += length;
	return length / size;
}

int8_t PlReadInt8( PLFile *ptr, bool *status ) {
	if ( ( size_t ) PlGetFileOffset( ptr ) >= ptr->size ) {
		if ( status != NULL ) {
			*status = false;
		}
		return 0;
	}

	if ( status != NULL ) {
		*status = true;
	}

	if ( ptr->fptr != NULL ) {
		return ( int8_t ) ( fgetc( ptr->fptr ) );
	}

	return *( ( int8_t * ) ptr->pos++ );
}

static int64_t ReadSizedInteger( PLFile *ptr, size_t size, bool big_endian, bool *status ) {
	int64_t n;
	if ( PlReadFile( ptr, &n, size, 1 ) != 1 ) {
		if ( status != NULL ) {
			*status = false;
		}
		return 0;
	}

	if ( status != NULL ) {
		*status = true;
	}

	if ( big_endian ) {
		if ( size == sizeof( int16_t ) ) {
			return be16toh( ( int16_t ) n );
		} else if ( size == sizeof( int32_t ) ) {
			return be32toh( ( int32_t ) n );
		} else if ( size == sizeof( int64_t ) ) {
			return be64toh( n );
		} else {
			if ( status != NULL ) {
				*status = false;
			}
			return 0;
		}
	}

	return n;
}

int16_t PlReadInt16( PLFile *ptr, bool big_endian, bool *status ) {
	return ( int16_t ) ReadSizedInteger( ptr, sizeof( int16_t ), big_endian, status );
}

int32_t PlReadInt32( PLFile *ptr, bool big_endian, bool *status ) {
	return ( int32_t ) ReadSizedInteger( ptr, sizeof( int32_t ), big_endian, status );
}

int64_t PlReadInt64( PLFile *ptr, bool big_endian, bool *status ) {
	return ReadSizedInteger( ptr, sizeof( int64_t ), big_endian, status );
}

float PlReadFloat32( PLFile *ptr, bool big_endian, bool *status ) {
	float f;
	if ( PlReadFile( ptr, &f, sizeof( float ), 1 ) != 1 ) {
		if ( status != NULL ) *status = false;
		return 0.0f;
	}

	/* not entirely sure how well this'd work, if at all */
	if ( big_endian ) {
		f = ( float ) be32toh( ( uint32_t ) f );
	}

	if ( status != NULL ) *status = true;

	return f;
}

double PlReadFloat64( PLFile *ptr, bool big_endian, bool *status ) {
	double d;
	if ( PlReadFile( ptr, &d, sizeof( double ), 1 ) != 1 ) {
		if ( status != NULL ) *status = false;
		return 0.0;
	}

	/* not entirely sure how well this'd work, if at all */
	if ( big_endian ) {
		d = ( double ) be64toh( ( uint64_t ) d );
	}

	if ( status != NULL ) *status = true;

	return d;
}

char *PlReadString( PLFile *ptr, char *str, size_t size ) {
	if ( size == 0 ) {
		PlReportBasicError( PL_RESULT_INVALID_PARM3 );
		return NULL;
	}

	if ( ptr->fptr != NULL ) {
		return fgets( str, ( int ) size, ptr->fptr );
	}

	if ( ptr->pos >= ptr->data + ptr->size ) {
		PlReportBasicError( PL_RESULT_FILEREAD );
		return NULL;
	}

	char *nl = memchr( ptr->pos, '\n', ptr->size - ( ptr->pos - ptr->data ) );
	if ( nl == NULL ) {
		nl = ( char * ) ( ptr->data + ptr->size - 1 );
	}

	if ( ( nl - ( char * ) ptr->pos ) + 1 >= ( signed long ) size ) {
		nl = ( char * ) ( ptr->pos + size );
	}

	memcpy( str, ptr->pos, ( nl - ( char * ) ptr->pos ) + 1 );
	str[ ( nl - ( char * ) ptr->pos ) + 1 ] = '\0';

	ptr->pos = ( uint8_t * ) ( nl + 1 );

	return str;
}

bool PlFileSeek( PLFile *ptr, PLFileOffset pos, PLFileSeek seek ) {
	if ( ptr->fptr != NULL ) {
		int err = pl_fseek( ptr->fptr, pos, seek );
		if ( err != 0 ) {
			PlReportErrorF( PL_RESULT_FILEREAD, "failed to seek file (%s)", GetLastError_strerror( GetLastError() ) );
			return false;
		}

		return true;
	}

	PLFileOffset posn = PlGetFileOffset( ptr );
	switch ( seek ) {
		case PL_SEEK_CUR:
			if ( ( size_t ) ( posn + pos ) > ptr->size || pos < -( ( signed long ) posn ) ) {
				PlReportBasicError( PL_RESULT_INVALID_PARM2 );
				return false;
			}
			ptr->pos += pos;
			break;

		case PL_SEEK_SET:
			if ( pos > ( signed long ) ptr->size || pos < 0 ) {
				PlReportBasicError( PL_RESULT_INVALID_PARM2 );
				return false;
			}
			ptr->pos = &ptr->data[ pos ];
			break;

		case PL_SEEK_END:
			if ( pos <= -( ( signed long ) ptr->size ) ) {
				PlReportBasicError( PL_RESULT_INVALID_PARM2 );
				return false;
			}
			ptr->pos = &ptr->data[ ptr->size - pos ];
			break;

		default:
			PlReportBasicError( PL_RESULT_INVALID_PARM3 );
			return false;
	}

	return true;
}

void PlRewindFile( PLFile *ptr ) {
	if ( ptr->fptr != NULL ) {
		rewind( ptr->fptr );
		return;
	}

	ptr->pos = ptr->data;
}

/**
 * If the file is being streamed from disk, cache
 * it into memory. This will also attempt to retain
 * the original offset into the file.
 * Returns pointer to position relative to read
 * location, or null on fail.
 */
const void *PlCacheFile( PLFile *file ) {
	/* make sure it's not already cached */
	if ( file->fptr == NULL ) {
		return NULL;
	}

	PLFileOffset p = PlGetFileOffset( file );
	size_t s = PlGetFileSize( file );

	/* jump back to the start */
	PlRewindFile( file );

	/* allocate the new buffer and attempt to read in the whole thing */
	file->data = PlMAllocA( s );
	if ( PlReadFile( file, file->data, sizeof( char ), s ) != s ) {
		/* seek back and restore where we were */
		PlFileSeek( file, ( long ) p, PL_SEEK_SET );
		PlFree( file->data );
		return NULL;
	}

	/* close the original file handle we had */
	_pl_fclose( file->fptr );

	/* match pos with where we originally were, so it's like nothing changed */
	file->pos = file->data + p;

	return file->pos;
}

/****************************************
 * File Aliases
 * This system allows you to register an alias that can be caught to load a
 * different file in it's place. Useful when dealing with older titles that
 * didn't utilise full paths and just single file names instead (i.e. mapping
 * between a WAD and local file).
 ****************************************/

#define MAX_ALIASES 4096

typedef struct FileAlias {
	PLPath alias;
	PLPath target;
	uint32_t hash;
} FileAlias;
static FileAlias fileAliases[ MAX_ALIASES ];
static unsigned int numFileAliases = 0;

void PlClearFileAliases( void ) {
	numFileAliases = 0;
}

/**
 * Adds a new alias to the list.
 */
void PlAddFileAlias( const char *alias, const char *target ) {
	if ( numFileAliases >= MAX_ALIASES ) {
		PlReportBasicError( PL_RESULT_MEMORY_EOA );
		return;
	}

	const char *p = PlGetPathForAlias( alias );
	if ( p != NULL ) {
		PlReportErrorF( PL_RESULT_INVALID_PARM1, "duplicate alias" );
		return;
	}

	snprintf( fileAliases[ numFileAliases ].alias, sizeof( PLPath ), "%s", alias );
	fileAliases[ numFileAliases ].hash = pl_strhash_sdbm( fileAliases[ numFileAliases ].alias );
	snprintf( fileAliases[ numFileAliases ].target, sizeof( PLPath ), "%s", target );
	numFileAliases++;
}

const char *PlGetPathForAlias( const char *alias ) {
	if ( numFileAliases == 0 ) {
		return NULL;
	}

	uint32_t hash = pl_strhash_sdbm( alias );
	for ( unsigned int i = 0; i < numFileAliases; ++i ) {
		if ( hash != fileAliases[ i ].hash ) {
			continue;
		}

		return fileAliases[ i ].target;
	}

	return NULL;
}
