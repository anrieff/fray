
set(SDL_LIB SDL)
add_library(${SDL_LIB} STATIC IMPORTED)

if (WIN32)
	set_target_properties(${SDL_LIB} PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
		"${CMAKE_CURRENT_SOURCE_DIR}/../SDK/SDL/include"
	)

	set_target_properties(${SDL_LIB} PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/../SDK/SDL/lib/x64/SDL.lib")
	set_target_properties(${SDL_LIB} PROPERTIES DLL_FILE_LIST "${CMAKE_CURRENT_SOURCE_DIR}/../SDK/SDL/lib/x64/SDL.dll")
else ()
	# TODO: test
	find_package(SDL REQUIRED)
endif ()
