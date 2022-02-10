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

#include "../platform/platform_private.h"
#include "window_private.h"

#include <Windows.h>

typedef struct PLWindow {
	PLSharedWindowState	sharedState;
	HWND				winHandle;
	HINSTANCE			appInstance;
	HDC                 deviceContext;
} PLWindow;

static LRESULT WindowCallbackProcedure( HWND windowHandle, unsigned int msg, WPARAM wParam, LPARAM lParam ) {
	return 0;
}

PLWindow *PlCreateWindow( int w, int h, const char *title ) {
	HINSTANCE instance = GetModuleHandle( NULL );
	if( instance == NULL ) {
		ReportError( PL_RESULT_FAIL, "failed to fetch valid module handle" );
		return NULL;
	}

	/* create the window class */
	WNDCLASS windowClass;
	memset( &windowClass, 0, sizeof( WNDCLASS ) );
	windowClass.lpfnWndProc		= ( WNDPROC ) WindowCallbackProcedure;
	windowClass.lpszClassName	= title;
	windowClass.hInstance		= instance;

	if( !RegisterClass( &windowClass ) ) {
		ReportError( PL_RESULT_FAIL, "failed to register window class" );
		return NULL;
	}

	/* and now create the window */

	HWND windowInstance = CreateWindowEx(
		0,
		title,
		title,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, w, h,

		NULL,
		NULL,
		instance,
		NULL
	);
	if( windowInstance == NULL ) {
		ReportError( PL_RESULT_FAIL, "failed to create window" );
		return NULL;
	}

	ShowWindow( windowInstance, SW_SHOW );

	PLWindow *window = PlCAllocA( 1, sizeof( PLWindow ) );
	window->winHandle	    = windowInstance;
	window->appInstance     = instance;
	/* window->deviceContext   = wglGetCurrentDC(); */

	return window;
}

void PlDestroyWindow( PLWindow *windowPtr ) {
	if( windowPtr == NULL ) {
		return;
	}

	if( windowPtr->winHandle != NULL ) {
		DestroyWindow( windowPtr->winHandle );

		UnregisterClass( windowPtr->sharedState.windowTitle, windowPtr->appInstance );
	}
}

void PlGetWindowPosition( PLWindow *windowPtr, int *x, int *y ) {
	*x = 0; *y = 0;

	RECT position;
	if( !GetWindowRect( windowPtr->winHandle, &position ) ) {
		ReportError( PL_RESULT_FAIL, "failed to get window position" );
		return;
	}

	*x = position.left;
	*y = position.top;
}

void PlSwapWindow( PLWindow *windowPtr ) {
	SwapBuffers( windowPtr->deviceContext );
}
