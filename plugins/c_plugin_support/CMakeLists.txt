cmake_minimum_required(VERSION 3.0.0)
project(c_plugin_support)


file(GLOB_RECURSE c_plugin_support_CPPS src/*.cpp)
add_library(c_plugin_support SHARED ${c_plugin_support_CPPS})
target_link_libraries(c_plugin_support PUBLIC satdump_core)
target_include_directories(c_plugin_support PUBLIC include)

file(GLOB_RECURSE fengyun2_pn_sync_CPPS fengyun2_pn_sync/*.cpp)
add_library(fengyun2_pn_sync SHARED ${fengyun2_pn_sync_CPPS})
target_include_directories(fengyun2_pn_sync PUBLIC include)

# make satdump plugin loader happy
set_target_properties(fengyun2_pn_sync PROPERTIES PREFIX "")
set_target_properties(fengyun2_pn_sync PROPERTIES SUFFIX ".plugin")

file(GLOB_RECURSE fengyun2_svissr_fix_CPPS fengyun2_svissr_fix/*.cpp)
add_library(fengyun2_svissr_fix SHARED ${fengyun2_svissr_fix_CPPS})
target_include_directories(fengyun2_svissr_fix PUBLIC include)

# make satdump plugin loader happy
set_target_properties(fengyun2_svissr_fix PROPERTIES PREFIX "")
set_target_properties(fengyun2_svissr_fix PROPERTIES SUFFIX ".plugin")

install(TARGETS c_plugin_support DESTINATION lib/satdump/plugins)
install(TARGETS fengyun2_pn_sync DESTINATION share/satdump/resources/c_plugin)
install(TARGETS fengyun2_svissr_fix DESTINATION share/satdump/resources/c_plugin)
