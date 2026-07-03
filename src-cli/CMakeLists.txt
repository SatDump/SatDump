cmake_minimum_required(VERSION 3.12)

file(GLOB_RECURSE GobDump_cli_CPPS *.cpp)

add_executable(gobdump ${GobDump_cli_CPPS})
target_include_directories(gobdump PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/src-core)

# Link against the core
target_link_libraries(gobdump PUBLIC gobdump_core)

install(TARGETS gobdump DESTINATION ${CMAKE_INSTALL_BINDIR})