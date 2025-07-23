cmake_minimum_required(VERSION 3.12)

file(GLOB_RECURSE SatDump_testing_CPPS main.cpp)

add_executable(satdump-testing ${SatDump_testing_CPPS})
target_include_directories(satdump-testing PUBLIC src-testing src-core)

target_include_directories(satdump-testing PUBLIC /home/alan/Downloads/supernovas/include /home/alan/Downloads/calceph/src)
target_link_directories(satdump-testing PUBLIC /home/alan/Downloads/supernovas/lib/ /home/alan/Downloads/calceph/build/src)

# Link against the core
target_link_libraries(satdump-testing PUBLIC satdump_core supernovas calceph solsys-calceph sqlite3) # mfhdf)
