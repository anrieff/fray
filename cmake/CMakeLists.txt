
cmake_minimum_required(VERSION 3.4)

include(./sdl.cmake)
include(./openexr.cmake)
include(./zlib.cmake)

if (WIN32)
# Flag override is required, to properly setup Debug build with Release SDK libraries
	set (CMAKE_CXX_FLAGS_DEBUG "/MD /Zi /Od /Ob0 /D NDEBUG")
endif()

project(fray)

set (HEADERS
	../src/bbox.h
	../src/bitmap.h
	../src/camera.h
	../src/color.h
	../src/constants.h
	../src/cxxptl-sdl.h
	../src/environment.h
	../src/geometry.h
	../src/heightfield.h
	../src/lights.h
	../src/main.h
	../src/matrix.h
	../src/mesh.h
	../src/random_generator.h
	../src/scene.h
	../src/sdl.h
	../src/shading.h
	../src/triangle.h
	../src/util.h
	../src/vector.h
)

set (SOURCES
	../src/bitmap.cpp
	../src/camera.cpp
	../src/cxxptl-sdl.cpp
	../src/environment.cpp
	../src/geometry.cpp
	../src/heightfield.cpp
	../src/lights.cpp
	../src/main.cpp
	../src/matrix.cpp
	../src/mesh.cpp
	../src/random_generator.cpp
	../src/scene.cpp
	../src/sdl.cpp
	../src/shading.cpp
	../src/triangle.cpp
	../src/util.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})
target_link_libraries(${PROJECT_NAME}
	${SDL_LIB}
	${OPENEXR_LIB}
	${ZLIB_LIB}
)

if (WIN32)
	target_compile_definitions(${PROJECT_NAME}
		PRIVATE -D_CRT_SECURE_NO_WARNINGS -D_CRT_SECURE_NO_DEPRECATE
	)
endif()

# add custom command for copying the SDL dll to the binary dir
add_custom_command(TARGET ${PROJECT_NAME}
	POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_PROPERTY:SDL,DLL_FILE_LIST> $<TARGET_FILE_DIR:${PROJECT_NAME}>
)
