#[[
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
]]

project(plgraphics)
option(PLG_COMPILE_STATIC "Compile as static library" ON)

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

file(GLOB PLG_SOURCE_FILES
        plg.c
        plg_camera.c
        plg_draw.c
        plg_driver_interface.c
        plg_layer_software.c
        plg_mesh.c
        plg_shader.c
        plg_texture.c
        polygon.c
        )

if (PLG_COMPILE_STATIC)
    add_library(plgraphics STATIC ${PLG_SOURCE_FILES})
else ()
    add_library(plgraphics SHARED ${PLG_SOURCE_FILES})
endif ()

target_include_directories(plgraphics PUBLIC include/)
target_link_libraries(plgraphics plcore)
