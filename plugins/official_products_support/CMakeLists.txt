cmake_minimum_required(VERSION 3.12)
project(official_products_support)

find_package(PkgConfig)
pkg_check_modules(HDF5 hdf5)
if(${HDF5_FOUND})

  # Core Plugin
  file(
    GLOB_RECURSE
    official_products_support_CPPS
    main.cpp
    file_utils/*.cpp
    fcidecomp/*.c
    fcidecomp/*.cpp
    hsd2pro/*.cpp
    off2pro/*.cpp
    nc2pro/*.cpp
    nat2pro/*.cpp)

  add_library(official_products_support SHARED
              ${official_products_support_CPPS})
  target_link_libraries(official_products_support PUBLIC satdump_core)
  target_include_directories(official_products_support PUBLIC src)

  target_include_directories(official_products_support
                             PUBLIC ${HDF5_INCLUDE_DIRS})
  target_link_directories(official_products_support PUBLIC ${HDF5_LIBRARY_DIRS})
  target_link_libraries(official_products_support PUBLIC hdf5 hdf5_hl)

  if(MSVC OR BUILD_MSVC)
    install(
      TARGETS official_products_support
      DESTINATION ${CMAKE_INSTALL_LIBDIR}/satdump/plugins
      RUNTIME_DEPENDENCY_SET satdump_deps)
  else()
    install(TARGETS official_products_support
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/satdump/plugins)
  endif()

  # Loader plugin
  if(BUILD_GUI)
    file(GLOB_RECURSE loader_CPPS main_loader.cpp loader/*.cpp)
    add_library(official_products_loader_support SHARED ${loader_CPPS})
    target_link_libraries(official_products_loader_support
                          PUBLIC official_products_support satdump_interface)

    if(MSVC OR BUILD_MSVC)
      install(
        TARGETS official_products_loader_support
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/satdump/plugins
        RUNTIME_DEPENDENCY_SET satdump_deps)
    else()
      install(TARGETS official_products_loader_support
              DESTINATION ${CMAKE_INSTALL_LIBDIR}/satdump/plugins)
    endif()
  endif()

endif()
