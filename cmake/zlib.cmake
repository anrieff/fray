
set(ZLIB_LIB ZLIB)
add_library(${ZLIB_LIB} STATIC IMPORTED)

if (WIN32)
	set_target_properties(${ZLIB_LIB} PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
		"${CMAKE_CURRENT_SOURCE_DIR}/../SDK/zlib/include"
	)

	set_target_properties(${ZLIB_LIB} PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/../SDK/zlib/lib/x64/zlib.lib")
else ()
	# TODO: test
	find_package(ZLIB REQUIRED)
endif ()
