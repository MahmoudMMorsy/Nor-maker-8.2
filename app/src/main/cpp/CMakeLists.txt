cmake_minimum_required(VERSION 3.22.1)
project("gm82_android")

add_library(
    gm82_android
    SHARED
    gm82_jni.c
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
