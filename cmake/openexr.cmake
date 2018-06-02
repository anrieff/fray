set(OPENEXR_LIB OpenEXR)
add_library(${OPENEXR_LIB} STATIC IMPORTED)

if (WIN32)
	set_target_properties(${OPENEXR_LIB} PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
		"${CMAKE_CURRENT_SOURCE_DIR}/../SDK/OpenEXR/include"
	)

	set_target_properties(${OPENEXR_LIB} PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/../SDK/OpenEXR/lib/x64/IlmImf.lib")

	set(OPENEXR_DEPENDENCY_LIST
		"Half"
		"Iex"
		"IlmThread"
	)

	set(OPENEXR_LIBS_LIST "")

	foreach(exr_target ${OPENEXR_DEPENDENCY_LIST})
		add_library(${exr_target} STATIC IMPORTED)
		set_target_properties(${exr_target} PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/../SDK/OpenEXR/lib/x64/${exr_target}.lib")
		list(APPEND OPENEXR_LIBS_LIST ${exr_target})
	endforeach(exr_target)

	# Now add all the libraries created for the OpenEXR dependecy
	set_target_properties(${OPENEXR_LIB} PROPERTIES INTERFACE_LINK_LIBRARIES "${OPENEXR_LIBS_LIST}")
else ()
	set_target_properties(${OPENEXR_LIB} PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
		"/usr/include/OpenEXR"
	)

	# This is totally hacky way of using default installation of OpenEXR, but due to the lack of
	# default cmake find package implementation, there is not much choice.
	# Since openexr libraries reside in implicity library locations, used by the compiler, to
	# obtain the full file name of the exact library, we need to check its existance in all
	# implicitly defined library directories, until we find one or fail finding any.
	set(_OpenExrLibPath "unknown")
	foreach(_implicit_path ${CMAKE_CXX_IMPLICIT_LINK_DIRECTORIES})
		if (EXISTS "${_implicit_path}/libIlmImf.so")
			set(_OpenExrLibPath ${_implicit_path})
			break()
		endif()
	endforeach()
	if (_OpenExrLibPath STREQUAL "unknown")
		message(FATAL_ERROR "OpenExr library was not found in default library locations")
	endif()

	set_target_properties(${OPENEXR_LIB} PROPERTIES IMPORTED_LOCATION "${_OpenExrLibPath}/libIlmImf.so")

	set(OPENEXR_DEPENDENCY_LIST
		"Half"
		"Iex"
		"IlmThread"
	)

	set(OPENEXR_LIBS_LIST "")
	foreach(exr_target ${OPENEXR_DEPENDENCY_LIST})
		add_library(${exr_target} STATIC IMPORTED)
		set_target_properties(${exr_target} PROPERTIES IMPORTED_LOCATION "${_OpenExrLibPath}/lib${exr_target}.so")
		list(APPEND OPENEXR_LIBS_LIST ${exr_target})
	endforeach(exr_target)

	# Now add all the libraries created for the OpenEXR dependecy
	set_target_properties(${OPENEXR_LIB} PROPERTIES INTERFACE_LINK_LIBRARIES "${OPENEXR_LIBS_LIST}")
endif ()

