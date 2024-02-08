cmake_minimum_required(VERSION 3.12)
project(sdrpp_server_support)


find_package(ZSTD)

if(ZSTD_FOUND OR MSVC OR ANDROID)
    message("Building with SDR++ Server support")
    file(GLOB_RECURSE sdrpp_server_support_CPPS *.cpp)
    add_library(sdrpp_server_support SHARED ${sdrpp_server_support_CPPS})
    target_link_libraries(sdrpp_server_support PUBLIC satdump_core)
    target_include_directories(sdrpp_server_support PUBLIC src)

    if(MSVC)
        target_link_libraries(sdrpp_server_support PUBLIC ws2_32)
    endif()

    install(TARGETS sdrpp_server_support DESTINATION lib/satdump/plugins)
else()
    message("SDR++ Server requires ZSTD to build!")
endif()