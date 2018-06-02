
set(SDL_LIB SDL_lib)

if (WIN32)
	# On Windows the SDL library is linked as an imported one.
	add_library(${SDL_LIB} STATIC IMPORTED)

	set_target_properties(${SDL_LIB} PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
		"${CMAKE_CURRENT_SOURCE_DIR}/../SDK/SDL/include"
	)

	set_target_properties(${SDL_LIB} PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/../SDK/SDL/lib/x64/SDL.lib")
	set_target_properties(${SDL_LIB} PROPERTIES DLL_FILE_LIST "${CMAKE_CURRENT_SOURCE_DIR}/../SDK/SDL/lib/x64/SDL.dll")
else ()
	find_package(SDL REQUIRED)

	set(SDL_LIB ${SDL_LIBRARY})
endif ()

