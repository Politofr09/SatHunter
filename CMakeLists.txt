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