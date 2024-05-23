cmake_minimum_required(VERSION 3.12)
project(gtk_filedialog_support)


file(GLOB_RECURSE gtk_filedialog_support_CPPS *.cpp)
add_library(gtk_filedialog_support SHARED ${gtk_filedialog_support_CPPS})
target_link_libraries(gtk_filedialog_support PUBLIC satdump_core)
target_include_directories(gtk_filedialog_support PUBLIC src)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk4)
target_include_directories(gtk_filedialog_support PUBLIC ${GTK3_INCLUDE_DIRS}) 
link_directories(${GTK3_LIBRARY_DIRS})
target_link_libraries(gtk_filedialog_support PUBLIC ${GTK3_LIBRARIES})

install(TARGETS gtk_filedialog_support DESTINATION lib/satdump/plugins)