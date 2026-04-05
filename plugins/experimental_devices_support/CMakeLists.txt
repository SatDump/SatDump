cmake_minimum_required(VERSION 3.12)
project(experimental_devices_support)

if(NOT ANDROID)
  file(GLOB_RECURSE experimental_devices_support_CPPS *.cpp)
  add_library(experimental_devices_support SHARED ${experimental_devices_support_CPPS})
  target_link_libraries(experimental_devices_support PUBLIC satdump_core satdump_interface)
  target_include_directories(experimental_devices_support PUBLIC . ../../src-interface)

  install(TARGETS experimental_devices_support DESTINATION ${CMAKE_INSTALL_LIBDIR}/satdump/plugins)

  if(APPLE)
    find_library(IOKIT_LIBRARY IOKit REQUIRED)
    find_library(FOUNDATION_LIBRARY Foundation REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${FOUNDATION_LIBRARY} ${IOKIT_LIBRARY})
  endif()
endif()
