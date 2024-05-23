cmake_minimum_required(VERSION 3.12)

file(GLOB_RECURSE SatDump_testing_CPPS main.cpp)

add_executable(satdump-testing ${SatDump_testing_CPPS})
target_include_directories(satdump-testing PUBLIC src-testing src-core)

# Link against the core
target_link_libraries(satdump-testing PUBLIC satdump_core) # mfhdf)


find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk4)
# Setup CMake to use GTK+, tell the compiler where to look for headers
# and to the linker where to look for libraries
target_include_directories(satdump-testing PUBLIC ${GTK3_INCLUDE_DIRS})
link_directories(${GTK3_LIBRARY_DIRS})
target_link_libraries(satdump-testing PUBLIC ${GTK3_LIBRARIES})


find_package(Qt5 REQUIRED COMPONENTS Widgets)
target_include_directories(satdump-testing PUBLIC /usr/include/x86_64-linux-gnu/)
target_link_libraries(satdump-testing PUBLIC Qt5::Widgets)