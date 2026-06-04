cmake_minimum_required(VERSION 3.12)
project(radiosonde_support)

file(GLOB_RECURSE radiosonde_support_CPPS *.cpp)
add_library(radiosonde_support SHARED ${radiosonde_support_CPPS})
target_link_libraries(radiosonde_support PUBLIC gobdump_core)
target_include_directories(radiosonde_support PUBLIC src)

install(TARGETS radiosonde_support
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/gobdump/plugins)
