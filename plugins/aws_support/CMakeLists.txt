cmake_minimum_required(VERSION 3.12)
project(aws_support)

file(GLOB_RECURSE aws_support_CPPS *.cpp)
add_library(aws_support SHARED ${aws_support_CPPS})
target_link_libraries(aws_support PUBLIC satdump_core)
target_include_directories(aws_support PUBLIC src)

install(TARGETS aws_support DESTINATION lib/satdump/plugins)
