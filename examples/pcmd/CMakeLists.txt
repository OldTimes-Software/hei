#[[
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
]]

add_subdirectory(../../extras/blitz/ ../../bin/plugins/blitz/)
add_subdirectory(../../extras/core/ ../../bin/plugins/core/)

set(CMAKE_C_STANDARD 11)

add_executable(pcmd
        main.c

        ../../extras/asa/asa.c
        ../../extras/asa/asa_format_tre.c
        ../../extras/aitd/aitd_pak.c
        ../../extras/eradicator_rid.c

        ../../extras/package/package_ftactics_pak.c
        ../../extras/package/package_haven_dat.c

        ../../extras/ice3d_dat.c
        ../../extras/istorm_ibf.c
        ../../extras/mortyr_hal.c
        ../../extras/outwars_ff.c
        ../../extras/package/package_rof.c
        ../../extras/sentient_vsr.c

        ../../extras/pl_extra_okre.c
        ../../extras/pl_extra_kri_wad.c
)
add_dependencies(pcmd
        blitz_plugin
        core_plugin)

target_include_directories(pcmd PRIVATE ../../extras)
target_link_libraries(pcmd plcore plgraphics plmodel)
