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
find_package(glfw3 REQUIRED)
find_package(spdlog REQUIRED)
find_package(glm)

add_executable(${EXECUTABLE_NAME} main.cpp)

target_link_libraries(${EXECUTABLE_NAME} Vulkan::Vulkan)
target_include_directories(${EXECUTABLE_NAME} PUBLIC ${glfw3_INCLUDE_DIRS})
target_link_libraries(${EXECUTABLE_NAME} glfw)
target_link_libraries(${EXECUTABLE_NAME} glm::glm)
target_link_libraries(${EXECUTABLE_NAME} spdlog::spdlog)
