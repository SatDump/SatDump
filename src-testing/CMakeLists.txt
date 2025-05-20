cmake_minimum_required(VERSION 3.12)

file(GLOB_RECURSE SatDump_testing_CPPS main.cpp)

add_executable(satdump-testing ${SatDump_testing_CPPS})
target_include_directories(satdump-testing PUBLIC src-testing src-core /home/alan/Downloads/supernovas/include)

target_link_directories(satdump-testing PUBLIC satdump_core /home/alan/Downloads/supernovas/lib/)

# Link against the core
target_link_libraries(satdump-testing PUBLIC satdump_core supernovas calceph solsys-calceph solsys-cspice cspice) # mfhdf)
