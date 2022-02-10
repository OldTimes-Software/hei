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

enum {
	TEST_RETURN_SUCCESS,
	TEST_RETURN_FAILURE,
	TEST_RETURN_FATAL,
};

#define FUNC_TEST( NAME )         \
	uint8_t test_##NAME( void ) { \
		printf( " Starting " #NAME "\n" );
#define FUNC_TEST_END()         \
	return TEST_RETURN_SUCCESS; \
	}

/*============================================================
 * CONSOLE
 ===========================================================*/

static bool test_cmd_called = false;
void CB_test_cmd( unsigned int argc, char **argv ) {
	test_cmd_called = true;
}

FUNC_TEST( RegisterConsoleCommand )
PlRegisterConsoleCommand( "test_cmd", CB_test_cmd, "testing" );
PLConsoleCommand *cmd = PlGetConsoleCommand( "test_cmd" );
if ( cmd == NULL ) {
	printf( "test_cmd was not registered!\n" );
	return TEST_RETURN_FAILURE;
}
PlParseConsoleString( "test_cmd" );
if ( !test_cmd_called ) {
	printf( "Failed to call \"test_cmd\" command!\n" );
	return TEST_RETURN_FAILURE;
}
test_cmd_called = false;
FUNC_TEST_END()

FUNC_TEST( GetConsoleCommands )
PLConsoleCommand **cmds;
size_t numCmds;
PlGetConsoleCommands( &cmds, &numCmds );
if ( numCmds < 1 ) {
	printf( "Did not receive any commands!\n" );
	return TEST_RETURN_FAILURE;
}
if ( cmds == NULL ) {
	printf( "Returned null cmds list!\n" );
	return TEST_RETURN_FAILURE;
}
bool foundResult = false;
for ( unsigned int i = 0; i < numCmds; ++i ) {
	const PLConsoleCommand *cmd = cmds[ i ];
	if ( strcmp( "test_cmd", cmd->cmd ) != 0 ) {
		continue;
	}
	foundResult = true;
}
if ( !foundResult ) {
	printf( "Failed to find test_cmd!\n" );
	return TEST_RETURN_FAILURE;
}
FUNC_TEST_END()

FUNC_TEST( GetConsoleCommand )
const PLConsoleCommand *cmd = PlGetConsoleCommand( "test_cmd" );
if ( cmd == NULL ) {
	printf( "Failed to get test_cmd!\n" );
	return TEST_RETURN_FAILURE;
}
FUNC_TEST_END()

FUNC_TEST( StringChunk )
static const char *testString = "thisthisthisthisthisthis";
char *c = pl_strchunksplit( testString, 4, " " );
if ( c == NULL ) {
	printf( "Returned null!\n" );
	return EXIT_FAILURE;
}
bool status = ( strcmp( c, "this this this this this this " ) == 0 );
PlFree( c );
if ( !status ) {
	printf( "Failed to split string!\n" );
	return EXIT_FAILURE;
}
FUNC_TEST_END()

int main( int argc, char **argv ) {
	printf( "Starting tests...\n" );

	PlInitialize( argc, argv );

#define CALL_FUNC_TEST( NAME )                                       \
	{                                                                \
		int ret = test_##NAME();                                     \
		if ( ret != TEST_RETURN_SUCCESS ) {                          \
			printf( "Failed on " #NAME "!\n" );                      \
			if ( ret == TEST_RETURN_FATAL ) { return EXIT_FAILURE; } \
		}                                                            \
	}

	CALL_FUNC_TEST( RegisterConsoleCommand )
	CALL_FUNC_TEST( GetConsoleCommands )
	CALL_FUNC_TEST( GetConsoleCommand )

	/* string */
	CALL_FUNC_TEST( StringChunk )

	printf( "\nAll tests passed successfully!\n" );

	return EXIT_SUCCESS;
}
