cmake_minimum_required(VERSION 3.12)
project(remote_sdr_support)

message("Building with Remote Source support")

# Plugin
file(GLOB_RECURSE remote_sdr_support_CPPS plugin/*.cpp ../udp_discovery.cpp)
add_library(remote_sdr_support SHARED ${remote_sdr_support_CPPS})
target_link_libraries(remote_sdr_support PUBLIC satdump_core)
target_include_directories(remote_sdr_support PUBLIC plugin .)

if(MSVC)
    target_link_libraries(remote_sdr_support PUBLIC ws2_32)
endif()

install(TARGETS remote_sdr_support DESTINATION ${CMAKE_INSTALL_LIBDIR}/satdump/plugins)

# Server Exec
file(GLOB_RECURSE remote_sdr_server_files server/*.cpp ../udp_discovery.cpp)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
add_executable(satdump_sdr_server ${remote_sdr_server_files})
target_include_directories(satdump_sdr_server PUBLIC server src-core .)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins)

# Link against the core
target_link_libraries(satdump_sdr_server PUBLIC satdump_core)

install(TARGETS satdump_sdr_server DESTINATION ${CMAKE_INSTALL_BINDIR})