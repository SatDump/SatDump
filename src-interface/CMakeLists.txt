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
if(MSVC OR BUILD_MSVC)
    install(TARGETS satdump_interface DESTINATION bin)
else()
    install(TARGETS satdump_interface DESTINATION lib)
endif()