cmake_minimum_required(VERSION 3.23)

set(PROJECT_NAME SAV)
project(${PROJECT_NAME} C)

file(GLOB SOURCES "src/*.c" "src/*.h")

add_executable(${PROJECT_NAME} ${SOURCES})

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_CURRENT_SOURCE_DIR}/assets"
    "$<TARGET_FILE_DIR:SAV>/assets"
)

add_library(RayLib STATIC IMPORTED)

set_target_properties(RayLib PROPERTIES
	IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/lib/raylib.lib"
)

if(WIN32)
	set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE RayLib winmm)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(${PROJECT_NAME} PRIVATE src)
