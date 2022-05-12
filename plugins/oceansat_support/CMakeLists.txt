cmake_minimum_required (VERSION 3.0.0)
project(oceansat_support)

set(SATDUMP_ROOT "/usr")

file(GLOB_RECURSE oceansat_support_CPPS *.cpp)
add_library(oceansat_support SHARED ${oceansat_support_CPPS})
target_link_libraries(oceansat_support PUBLIC satdump_core)
target_include_directories(oceansat_support PUBLIC src ${SATDUMP_ROOT}/include/satdump)

install(TARGETS oceansat_support DESTINATION share/satdump/plugins)