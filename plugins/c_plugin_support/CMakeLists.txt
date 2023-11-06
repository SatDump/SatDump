cmake_minimum_required(VERSION 3.0.0)
project(c_plugin_support)


file(GLOB_RECURSE c_plugin_support_CPPS src/*.cpp)
add_library(c_plugin_support SHARED ${c_plugin_support_CPPS})
target_link_libraries(c_plugin_support PUBLIC satdump_core)
target_include_directories(c_plugin_support PUBLIC include)

file(GLOB_RECURSE c_example_process_plugin_CPPS example_plugin_src/*.cpp)
add_library(c_example_process_plugin SHARED ${c_example_process_plugin_CPPS})
target_include_directories(c_example_process_plugin PUBLIC include)

# make satdump plugin loader happy
set_target_properties(c_example_process_plugin PROPERTIES PREFIX "")
set_target_properties(c_example_process_plugin PROPERTIES SUFFIX ".plugin")

file(GLOB_RECURSE fengyun2_svissr_fix_CPPS fengyun2_svissr_fix/*.cpp)
add_library(fengyun2_svissr_fix SHARED ${fengyun2_svissr_fix_CPPS})
target_include_directories(fengyun2_svissr_fix PUBLIC include)

# make satdump plugin loader happy
set_target_properties(fengyun2_svissr_fix PROPERTIES PREFIX "")
set_target_properties(fengyun2_svissr_fix PROPERTIES SUFFIX ".plugin")

install(TARGETS c_plugin_support DESTINATION lib/satdump/plugins)
install(TARGETS c_example_process_plugin DESTINATION share/satdump/resources/c_plugin)
install(TARGETS fengyun2_svissr_fix DESTINATION share/satdump/resources/c_plugin)
