cmake_minimum_required(VERSION 3.12)

file(GLOB_RECURSE SatDump2_cli_CPPS *.cpp)

add_executable(satdump2 ${SatDump2_cli_CPPS})
target_include_directories(satdump2 PUBLIC src-cli src-core)

# Link against the core
target_link_libraries(satdump2 PUBLIC satdump_core)

install(TARGETS satdump2 DESTINATION ${CMAKE_INSTALL_BINDIR})