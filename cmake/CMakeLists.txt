
cmake_minimum_required(VERSION 3.4)

include(./sdl.cmake)

project(fray)

set (HEADERS
	../src/bitmap.h
	../src/camera.h
	../src/color.h
	../src/constants.h
	../src/geometry.h
	../src/main.h
	../src/matrix.h
	../src/sdl.h
	../src/shading.h
	../src/util.h
	../src/vector.h
)

set (SOURCES
	../src/bitmap.cpp
	../src/camera.cpp
	../src/geometry.cpp
	../src/main.cpp
	../src/matrix.cpp
	../src/sdl.cpp
	../src/shading.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})
target_link_libraries(${PROJECT_NAME} ${SDL_LIB})

# add custom command for copying the SDL dll to the binary dir
add_custom_command(TARGET ${PROJECT_NAME}
	POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_PROPERTY:SDL,DLL_FILE_LIST> $<TARGET_FILE_DIR:${PROJECT_NAME}>
)
