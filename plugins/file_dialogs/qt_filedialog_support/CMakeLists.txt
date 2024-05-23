cmake_minimum_required(VERSION 3.12)
project(qt_filedialog_support)


file(GLOB_RECURSE qt_filedialog_support_CPPS *.cpp)
add_library(qt_filedialog_support SHARED ${qt_filedialog_support_CPPS})
target_link_libraries(qt_filedialog_support PUBLIC satdump_core)
target_include_directories(qt_filedialog_support PUBLIC src)

find_package(Qt5 REQUIRED COMPONENTS Widgets)
target_include_directories(qt_filedialog_support PUBLIC /usr/include/x86_64-linux-gnu/)
target_link_libraries(qt_filedialog_support PUBLIC Qt5::Widgets)

install(TARGETS qt_filedialog_support DESTINATION lib/satdump/plugins)