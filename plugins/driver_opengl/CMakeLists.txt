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

project(pl_gl_driver)

add_library(
        pl_gl_driver SHARED
        opengl.c
        plugin.c
        plugin.h)

function(build_glew)
    set(BUILD_UTILS OFF CACHE BOOL "utilities")
    add_subdirectory(3rdparty/glew-2.2.0/build/cmake/)
endfunction()

if (WIN32 OR ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    build_glew()
    target_link_libraries(pl_gl_driver glew_s)
    target_include_directories(pl_gl_driver PRIVATE
            3rdparty/glew-2.2.0/include/)
else ()
    target_link_libraries(pl_gl_driver GLEW GL)
endif ()

target_include_directories(pl_gl_driver PRIVATE
        ../../plcore/include/
        ../../plgraphics/include/
        )
