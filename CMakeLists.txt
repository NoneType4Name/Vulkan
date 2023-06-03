cmake_minimum_required(VERSION 3.4)
set(CMAKE_CXX_STANDARD 20)
project("SomeApp"
        LANGUAGES CXX
        VERSION 0.0.1
        )
set(EXECUTABLE_NAME "window")
set(DEPENDENCIES_FOLDER "include")
message("${CMAKE_PROJECT_NAME} ${CMAKE_PROJECT_VERSION}\n${CMAKE_PROJECT_DESCRIPTION}")

find_package(Vulkan REQUIRED)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory("${CMAKE_SOURCE_DIR}/${DEPENDENCIES_FOLDER}/glfw")
set(BUILD_TESTS OFF)
set(BUILD_EXAMPLES OFF)
add_subdirectory("${CMAKE_SOURCE_DIR}/${DEPENDENCIES_FOLDER}/glm")

add_executable(${EXECUTABLE_NAME} main.cpp)
target_link_libraries(${EXECUTABLE_NAME} Vulkan::Vulkan)
target_link_libraries(${EXECUTABLE_NAME} glfw)
# target_link_libraries(${EXECUTABLE_NAME} "${CMAKE_SOURCE_DIR}/${DEPENDENCIES_FOLDER}/p7.lib")
target_link_libraries(${EXECUTABLE_NAME} glm)