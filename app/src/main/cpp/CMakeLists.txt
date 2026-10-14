cmake_minimum_required(VERSION 3.22.1)
project("gm82_android")

add_library(
    gm82_android
    SHARED
    gm82_jni.c
    gml_vm.c
    gml_frontend.c
    gm82_builtins.c
    gm82_gmk_reader.c
    gm82_assets.c
)

find_library(
    log-lib
    log
)

find_library(
    jnigraphics-lib
    jnigraphics
)

target_link_libraries(
    gm82_android
    ${log-lib}
    ${jnigraphics-lib}
    m
)
