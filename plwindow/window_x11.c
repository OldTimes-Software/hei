/**
 * Hei Platform Library
 * Copyright (C) 2017-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is licensed under MIT. See LICENSE for more details.
 */

#include "../platform/graphics/graphics_private.h"
#include "../platform/platform_private.h"

#include "window_private.h"

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

typedef struct PLWindow {
	PLSharedWindowState	sharedState;
	Window				winHandle;
} PLWindow;

PLWindow *PlCreateWindow( int w, int h, const char *title ) {
	/* before we do anything, check whether or not the graphics
	 * subsystem has been initialised */
#if 0
	if (	/* for now only support OpenGL modes here */
			gfx_layer.mode != PL_GFX_MODE_OPENGL &&
			gfx_layer.mode != PL_GFX_MODE_OPENGL_CORE &&
			gfx_layer.mode != PL_GFX_MODE_OPENGL_1_0 &&
			gfx_layer.mode != PL_GFX_MODE_OPENGL_ES ) {
		ReportError( PL_RESULT_FAIL, "invalid graphics mode, please set graphics mode" );
		return NULL;
	}
#endif

	Display *displayPtr = XOpenDisplay( NULL );
	if ( displayPtr == NULL ) {
		ReportError( PL_RESULT_FAIL, "failed to open display" );
		return NULL;
	}

	/* fetch the "root window", aka the desktop */
	Window rootHandle = XDefaultRootWindow( displayPtr );
}
