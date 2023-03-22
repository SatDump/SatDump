include (CMakeLists.inc)
if (EMIT_ASN_DEBUG)
	set (CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DEMIT_ASN_DEBUG=1")
endif()

# force inclusion of alloca.h from stdlib.h on macos
remove_definitions(-D_XOPEN_SOURCE=600 -D_POSIX_C_SOURCE=200809L)

add_library (asn1 OBJECT ${ASN_MODULE_SOURCES})
set_property(TARGET asn1 PROPERTY POSITION_INDEPENDENT_CODE ON)

install (FILES ${ASN_MODULE_HEADERS} DESTINATION
	${CMAKE_INSTALL_INCLUDEDIR}/${LA_SUBDIR}/${PROJECT_NAME}/asn1)
