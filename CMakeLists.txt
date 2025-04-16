cmake_minimum_required(VERSION 3.22)
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_LIST_DIR} NAME)
project("${PROJECT_NAME}" CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

add_executable(${PROJECT_NAME} 
"src/Main.cpp" 
"src/Application.h" 
"src/Application.cpp" 
"src/TLEUtils.h" 
"src/TLEUtils.cpp" 
"src/Satellite.h" 
"src/Satellite.cpp" 
"src/Config.h"
"src/Config.cpp"
)
target_include_directories(${PROJECT_NAME} PUBLIC "${PROJECT_NAME}/src")

add_subdirectory("${CMAKE_SOURCE_DIR}/vendor/raylib")
add_subdirectory("${CMAKE_SOURCE_DIR}/vendor/rlImGui")
add_subdirectory("${CMAKE_SOURCE_DIR}/vendor/sgp4")
add_subdirectory("${CMAKE_SOURCE_DIR}/vendor/cpr")
add_subdirectory("${CMAKE_SOURCE_DIR}/vendor/inifile-cpp")

target_link_libraries(${PROJECT_NAME} raylib)
target_link_libraries(${PROJECT_NAME} rlImGui)
target_link_libraries(${PROJECT_NAME} sgp4)
target_link_libraries(${PROJECT_NAME} cpr)
target_link_libraries(${PROJECT_NAME} inicpp)

file(COPY "${CMAKE_SOURCE_DIR}/res" DESTINATION "${CMAKE_BINARY_DIR}")


# Web support
if (${PLATFORM} STREQUAL "Web")
    # Tell Emscripten to build an .html file.
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
    SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Os")
    set(CMAKE_EXECUTABLE_SUFFIX ".html") # This line is used to set your executable to build with the emscripten html template so that you can directly open it.
    set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".html")
    # set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s USE_GLFW=3 -s ASSERTIONS=1 -s WASM=1 -Os -Wall -s TOTAL_MEMORY=67108864 -s FORCE_FILESYSTEM=1 --preload-file assets/ --shell-file ../src/minshell.html")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s USE_GLFW=3 -s ASSERTIONS=1 -s WASM=1 -Os -Wall -s TOTAL_MEMORY=67108864 -s FORCE_FILESYSTEM=1 --preload-file res/ --shell-file shell.html")
    target_compile_definitions(${PROJECT_NAME} PUBLIC ASSETS_PATH="/assets/") # Set the asset path macro in release mode to a relative path that assumes the assets folder is in the same directory as the game executable
endif()