/**
 * Hei Platform Library
 * Copyright (C) 2017-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is licensed under MIT. See LICENSE for more details.
 */

#pragma once

#include <imgui.h>

bool PlImGui_Initialize();
void PlImGui_Shutdown();
void PlImGui_NewFrame();
void PlImGui_RenderDrawData( ImDrawData *drawData );
