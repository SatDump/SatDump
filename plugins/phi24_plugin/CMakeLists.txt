project(phi24_plugin)
cmake_minimum_required(VERSION 3.10)

set(PLUGIN_SOURCES
    phi24_main.cpp
    ../../src-core/modules/ctt_topological_resonator.cpp
)

add_library(phi24_resonator SHARED ${PLUGIN_SOURCES})

# Link against SatDump core so it can use the Module classes
target_link_libraries(phi24_resonator satdump-core)
