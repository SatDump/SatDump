cmake_minimum_required(VERSION 3.0.0)
project(inmarsat_support)


file(GLOB_RECURSE inmarsat_support_CPPS *.cpp)
add_library(inmarsat_support SHARED ${inmarsat_support_CPPS})
target_link_libraries(inmarsat_support PUBLIC satdump_core)
target_include_directories(inmarsat_support PUBLIC src)

if(MSVC)
    target_link_libraries(inmarsat_support PUBLIC ws2_32)
endif()

install(TARGETS inmarsat_support DESTINATION share/satdump/plugins)