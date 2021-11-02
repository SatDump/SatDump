cmake_minimum_required (VERSION 3.0.0)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tools)

file(GLOB_RECURSE SatDump_recorder_CPPS  *.cpp)

add_executable(satdump-recorder ${SatDump_recorder_CPPS})
target_include_directories(satdump-recorder PUBLIC src-cli src-core)

# Link against the core
target_link_libraries(satdump-recorder PUBLIC satdump_core)

# Link against SDR Libraries
target_include_directories(satdump-recorder PUBLIC ../libsdr)
target_link_libraries(satdump-recorder PUBLIC sdr)

install(TARGETS satdump-recorder DESTINATION bin)