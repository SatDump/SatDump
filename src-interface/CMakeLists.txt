cmake_minimum_required(VERSION 3.12)

file(GLOB_RECURSE INTERFACE_CPPS *.cpp)

add_library(satdump_interface SHARED ${INTERFACE_CPPS})
target_include_directories(satdump_interface PUBLIC . ../src-core)
#set_target_properties(satdump_interface PROPERTIES COMPILE_DEFINITIONS BUILDER_STATIC_DEFINE)

if(BUILD_ZIQ)
    target_compile_definitions(satdump_interface PUBLIC BUILD_ZIQ="1")
endif()

if(BUILD_ZIQ2)
    target_compile_definitions(satdump_interface PUBLIC BUILD_ZIQ2="1")
endif()

# Link against the core
target_link_libraries(satdump_interface PUBLIC satdump_core)

# Install
install(TARGETS satdump_core DESTINATION lib)