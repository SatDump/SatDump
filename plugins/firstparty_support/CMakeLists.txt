cmake_minimum_required(VERSION 3.12)
project(firstparty_support)

if(NOT ANDROID)
  find_package(PkgConfig)
  set(HDF5_NEW_FOUND=0)
  set(HDF5_FOUND=0)
  pkg_check_modules(HDF5 hdf5)
  pkg_check_modules(HDF5_NEW hdf5 hdf5_hl hdf5_cpp)
endif()

if(${HDF5_FOUND} OR ${HDF5_NEW_FOUND})

  # Core Plugin
  file(
    GLOB_RECURSE
    firstparty_support_CPPS
    main.cpp
    cli/*.cpp
    file_utils/*.cpp
    fcidecomp/*.c
    fcidecomp/*.cpp
    processors/*.cpp
    old/*.cpp
    procfile.cpp
    product_info.cpp
    new/*.cpp)

  add_library(firstparty_support SHARED
              ${firstparty_support_CPPS})
  target_link_libraries(firstparty_support PUBLIC satdump_core)
  target_include_directories(firstparty_support PUBLIC src ${CMAKE_SOURCE_DIR}/plugins/xrit_support)

  target_include_directories(firstparty_support
                             PUBLIC ${HDF5_INCLUDE_DIRS})
  target_link_directories(firstparty_support PUBLIC ${HDF5_LIBRARY_DIRS})

  if(${HDF5_NEW_FOUND})
    target_link_libraries(firstparty_support PUBLIC ${HDF5_LIBRARIES} xrit_support)
  else()
    target_link_libraries(firstparty_support PUBLIC hdf5 hdf5_hl hdf5_cpp xrit_support)
  endif()

  if(MSVC OR BUILD_MSVC)
    install(
      TARGETS firstparty_support
      DESTINATION ${CMAKE_INSTALL_LIBDIR}/satdump/plugins
      RUNTIME_DEPENDENCY_SET satdump_deps)
  else()
    install(TARGETS firstparty_support
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/satdump/plugins)
  endif()

  # Loader plugin
  if(BUILD_GUI)
    file(GLOB_RECURSE loader_CPPS main_loader.cpp loader/*.cpp main_loader_handle.cpp)
    add_library(firstparty_loader_support SHARED ${loader_CPPS})
    target_link_libraries(firstparty_loader_support
                          PUBLIC firstparty_support satdump_interface)

    if(MSVC OR BUILD_MSVC)
      install(
        TARGETS firstparty_loader_support
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/satdump/plugins
        RUNTIME_DEPENDENCY_SET satdump_deps)
    else()
      install(TARGETS firstparty_loader_support
              DESTINATION ${CMAKE_INSTALL_LIBDIR}/satdump/plugins)
    endif()
  endif()

endif()
