cmake_minimum_required(VERSION 3.12)
project(official_products_support)

find_package(PkgConfig)
pkg_check_modules(HDF5 hdf5)
if(${HDF5_FOUND})
  file(GLOB_RECURSE official_products_support_CPPS *.cpp fcidecomp/*.c
       fcidecomp/*.cpp)
  add_library(official_products_support SHARED
              ${official_products_support_CPPS})
  target_link_libraries(official_products_support PUBLIC satdump_core)
  target_include_directories(official_products_support PUBLIC src)

  target_include_directories(official_products_support
                             PUBLIC ${HDF5_INCLUDE_DIRS})
  target_link_directories(official_products_support PUBLIC ${HDF5_LIBRARY_DIRS})
  target_link_libraries(official_products_support PUBLIC ${HDF5_LIBRARIES}
                                                         hdf5_hl)
  if(MSVC OR BUILD_MSVC)
    install(
      TARGETS official_products_support
      DESTINATION lib/satdump/plugins
      RUNTIME_DEPENDENCY_SET satdump_deps)
  else()
    install(TARGETS official_products_support DESTINATION lib/satdump/plugins)
  endif()
endif()
