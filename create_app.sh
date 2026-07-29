#!/bin/bash

for core in libxnes smsplus-gx temper walnut_cgb; do
    echo "Processing $core..."
    mkdir -p $core/main
    
    cat << CMAKE > $core/CMakeLists.txt
cmake_minimum_required(VERSION 3.5)
set(COMPONENTS "main" "retro-go")
set(RG_ENABLE_NETWORKING 0)
include(../base.cmake)
project($core)
CMAKE

    cat << CMAKE2 > $core/main/CMakeLists.txt
file(GLOB_RECURSE SRCS "../src/*.c" "../source/*.c" "../*.c")
list(FILTER SRCS EXCLUDE REGEX ".*/(examples|test|ports/cli)/.*")
list(FILTER SRCS EXCLUDE REGEX ".*cli\\.c$")
# Exclude main.c if it defines standard main and causes issues, but for temper main.c has initialize_pce. Let's redefine main using a macro if it conflicts, or let's try building first.
set(COMPONENT_SRCS \${SRCS} "app_main.c")
set(COMPONENT_ADD_INCLUDEDIRS ".." "../src" "../source" "../source/cpu_cores/z80" "../source/sound" "../source/sound/maxim_sn76489" "../source/sound_output")
register_component()
rg_setup_compile_options()
CMAKE2

    cat << CAPPMAIN > $core/main/app_main.c
#include <stdio.h>
void app_main(void) {
    printf("$core app_main started!\\\\n");
    while(1) {}
}
CAPPMAIN
done
./create_app.sh
