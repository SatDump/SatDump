cmake_minimum_required(VERSION 3.0.0)
project(proba_support)


file(GLOB_RECURSE proba_support_CPPS *.cpp)
add_library(proba_support SHARED ${proba_support_CPPS})
target_link_libraries(proba_support PUBLIC satdump_core)
target_include_directories(proba_support PUBLIC src)

install(TARGETS proba_support DESTINATION share/satdump/plugins)