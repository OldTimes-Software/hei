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

project(plcore)

option(PL_COMPILE_STATIC "Compile as static library" ON)
option(PL_FILESYSTEM_64 "Use 64-bit interface for file IO" ON)

add_definitions("-D_DEBUG")

file(
        GLOB PLATFORM_SOURCE_FILES
        pl.c
        pl_compression.c
        pl_console.c
        pl_filesystem.c
        pl_memory.c
        pl_parser.c
        pl_library.c
        pl_linkedlist.c
        polygon.c
        pl_math_colour.c
        pl_math_matrix.c
        pl_math_vector.c
        pl_physics.c
        pl_timer.c

        string/crc32.c
        string/itoa.c
        string/pl_string.c
        string/strcasecmp.c
        string/strcasestr.c
        string/strcnt.c
        string/strisalnum.c
        string/strisalpha.c
        string/strisdigit.c
        string/strtolower.c
        string/strtoupper.c
        string/vscprintf.c

        3rdparty/blast/blast.c
        3rdparty/miniz/miniz.c
)

file(GLOB PLATFORM_PACKAGE_FILES
        package/*.c
        )
file(GLOB PLATFORM_IMAGE_FILES
        image/*.c
        )

if (PL_COMPILE_STATIC)
    add_library(plcore STATIC
            ${PLATFORM_SOURCE_FILES}
            ${PLATFORM_IMAGE_FILES}
            ${PLATFORM_PACKAGE_FILES})
else ()
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
    add_library(plcore SHARED
            ${PLATFORM_SOURCE_FILES}
            ${PLATFORM_IMAGE_FILES}
            ${PLATFORM_PACKAGE_FILES})
endif ()

target_compile_options(plcore PRIVATE -DPL_INTERNAL)
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    target_compile_options(plcore PUBLIC -fPIC)
elseif ("${CMAKE_C_COMPILER_ID}" STREQUAL "MSVC")
    target_compile_options(plcore PUBLIC -D_CRT_SECURE_NO_WARNINGS /W3)
endif ()

if (PL_FILESYSTEM_64)
    target_compile_definitions(plcore
            PUBLIC
            PL_FILESYSTEM_64=1
            PRIVATE
            __USE_LARGEFILE64=1
            _FILE_OFFSET_BITS=64
            )
endif ()

# Platform specific libraries should be provided here
if (UNIX)
    target_link_libraries(plcore dl m)
elseif (WIN32)
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        target_compile_options(plcore PRIVATE -static -static-libstdc++ -static-libgcc)
    endif ()
    target_link_libraries(plcore Secur32 ws2_32 Psapi)
endif ()

set_property(TARGET plcore PROPERTY C_STANDARD_REQUIRED 11)

target_include_directories(plcore
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include/
        ${CMAKE_SYSTEM_INCLUDE_PATH}
        PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        )
