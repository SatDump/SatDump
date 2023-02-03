cmake_minimum_required(VERSION 3.0.0)
project(elektro_arktika_support)


file(GLOB_RECURSE elektro_arktika_support_CPPS *.cpp)
add_library(elektro_arktika_support SHARED ${elektro_arktika_support_CPPS})
target_link_libraries(elektro_arktika_support PUBLIC satdump_core)
target_include_directories(elektro_arktika_support PUBLIC .)

install(TARGETS elektro_arktika_support DESTINATION share/satdump/plugins)