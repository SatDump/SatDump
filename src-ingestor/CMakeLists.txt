cmake_minimum_required (VERSION 3.0.0)

file(GLOB_RECURSE SatDumpIngestor_cli_CPPS  *.cpp)

add_executable(satdump-ingestor ${SatDumpIngestor_cli_CPPS})
target_include_directories(satdump-ingestor PUBLIC src-cli src-core)

# Link against the core
target_link_libraries(satdump-ingestor PUBLIC satdump_core)

# Link against SDR Libraries
target_include_directories(satdump-ingestor PUBLIC ../libsdr)
target_link_libraries(satdump-ingestor PUBLIC sdr)

install(TARGETS satdump-ingestor DESTINATION bin)