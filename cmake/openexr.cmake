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
	# TODO: test
	find_package(OpenEXR REQUIRED)
endif ()
