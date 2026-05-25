cmake_minimum_required(VERSION 3.12)

file(GLOB_RECURSE INTERFACE_CPPS *.cpp)

add_library(gobdump_interface SHARED ${INTERFACE_CPPS})
target_include_directories(gobdump_interface PUBLIC . ../src-core)
#set_target_properties(gobdump_interface PROPERTIES COMPILE_DEFINITIONS BUILDER_STATIC_DEFINE)

if(BUILD_ZIQ)
    target_compile_definitions(gobdump_interface PUBLIC BUILD_ZIQ="1")
endif()

if(BUILD_ZIQ2)
    target_compile_definitions(gobdump_interface PUBLIC BUILD_ZIQ2="1")
endif()

# Link against the core
target_link_libraries(gobdump_interface PUBLIC gobdump_core)

# Install
if(MSVC OR BUILD_MSVC)
    install(TARGETS gobdump_interface DESTINATION ${CMAKE_INSTALL_BINDIR})
else()
    install(TARGETS gobdump_interface DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()