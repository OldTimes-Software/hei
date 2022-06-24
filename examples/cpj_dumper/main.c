/* SPDX-License-Identifier: MIT */
/* Copyright © 2017-2022 Mark E Sowden <hogsy@oldtimes-software.com> */

/* this is a tool whipped up to quickly dump
 * the timestamps out of DNF2001 CPJ files */

#include <plcore/pl.h>
#include <plcore/pl_filesystem.h>

#define CPJ_FILE_START 12

typedef struct CPJChunkInfo {
	PLFileOffset offset;
	uint32_t length;
	uint32_t version;
	uint32_t timestamp;
	uint32_t nameOffset;
} CPJChunkInfo;

static bool SeekChunk( PLFile *file, unsigned int magic, CPJChunkInfo *out ) {
	PlFileSeek( file, CPJ_FILE_START, PL_SEEK_SET );

	while ( !PlIsEndOfFile( file ) ) {
		out->offset = PlGetFileOffset( file );
		unsigned int cm = PlReadInt32( file, false, NULL );
		if ( cm != magic ) {
			unsigned int length = PlReadInt32( file, false, NULL );
			if ( length == 0 || length >= PlGetFileSize( file ) ) {
				return false;
			}

			PLFileOffset offset = PlGetFileOffset( file ) + length;
			if ( ( offset % 2 ) != 0 ) {
				offset += 2 - ( offset % 2 );
			}

			PlFileSeek( file, offset, PL_SEEK_SET );
			continue;
		}

		out->length = PlReadInt32( file, false, NULL );
		out->version = PlReadInt32( file, false, NULL );
		out->timestamp = PlReadInt32( file, false, NULL );
		out->nameOffset = PlReadInt32( file, false, NULL );
		return true;
	}

	return false;
}

static void DumpHeaderTimestamp( const char *path, void *user ) {
	FILE *out = ( FILE * ) user;
	fprintf( out, "%s,", path );

	PLFile *file = PlOpenFile( path, false );
	if ( file == NULL ) {
		fprintf( out, "failed to open: %s\n", PlGetError() );
		return;
	}

	static unsigned int cpjMagic = PL_MAGIC_TO_NUM( 'R', 'I', 'F', 'F' );
	unsigned int magic = PlReadInt32( file, false, NULL );
	if ( magic != cpjMagic ) {
		PlCloseFile( file );
		fprintf( out, "unknown filetype\n" );
		return;
	}

	PlFileSeek( file, 24, PL_SEEK_SET );

	time_t timestamp = PlReadInt32( file, false, NULL );
	struct tm *time = gmtime( &timestamp );

	char author[ 64 ] = " ";
	char description[ 256 ] = " ";

	/* now seek to the commands and fetch additional data */
	CPJChunkInfo info;
	if ( SeekChunk( file, PL_MAGIC_TO_NUM( 'M', 'A', 'C', 'B' ), &info ) ) {
		struct {
			uint32_t numSections;
			uint32_t sectionsOffset;
			uint32_t numCommands;
			uint32_t commandsOffset;
		} macHeader;
		assert( PlReadFile( file, &macHeader, sizeof( macHeader ), 1 ) == 1 );

		/* offsets seem to be relative to here */
		PLFileOffset s = PlGetFileOffset( file );

		/* now seek over and read in all the command offsets */
		int32_t commands[ macHeader.numCommands ];
		PlFileSeek( file, macHeader.commandsOffset, PL_SEEK_CUR );
		for ( unsigned int i = 0; i < macHeader.numCommands; ++i ) {
			commands[ i ] = PlReadInt32( file, false, NULL );
		}

		for ( unsigned int i = 0; i < macHeader.numCommands; ++i ) {
			PlFileSeek( file, s, PL_SEEK_SET );
			PlFileSeek( file, commands[ i ], PL_SEEK_CUR );

			char command[ 256 ];
			PlReadFile( file, command, sizeof( char ), sizeof( command ) );
			if ( strncmp( command, "SetAuthor ", 10 ) == 0 ) {
				PL_ZERO_( author );
				char *p = &command[ 11 ];
				for ( unsigned int j = 0; j < sizeof( command ); ++j ) {
					if ( *p == '\0' || *p == '\"' ) {
						break;
					}

					author[ j ] = *p++;
				}
			} else if ( strncmp( command, "SetDescription ", 15 ) == 0 ) {
				PL_ZERO_( description );
				char *p = &command[ 16 ];
				for ( unsigned int j = 0; j < sizeof( command ); ++j ) {
					if ( *p == '\0' || *p == '\"' ) {
						break;
					}

					description[ j ] = *p++;
				}
			}
		}
	}

	fprintf( out, "%s,%s,", author, description );

	PlCloseFile( file );

	char buf[ 32 ];
	strftime( buf, sizeof( buf ), "%c", time );
	fprintf( out, "%s\n", buf );
}

int main( int argc, char **argv ) {
	PlInitialize( argc, argv );

	FILE *out = fopen( "dump.csv", "w" );
	if ( out == NULL ) {
		printf( "Failed to open destination for writing!\n" );
		return EXIT_FAILURE;
	}

	fprintf( out, "Path,Author,Description,Timestamp\n" );

	PlScanDirectory( "Meshes/", "cpj", DumpHeaderTimestamp, true, out );

	fclose( out );

	return EXIT_SUCCESS;
}
