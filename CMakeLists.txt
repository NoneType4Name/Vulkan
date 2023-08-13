# conan install . -of=build -b=missing -pr=std20 -s build_type=Debug -s compiler.cppstd=20 -s compiler=clang -s compiler.version=16 --conf tools.cmake.cmaketoolchain:generator=Ninja
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)
cmake_minimum_required(VERSION 3.5)
if (UNIX)
        add_compile_options(-stdlib=libc++)
        add_link_options(-stdlib=libc++)
endif()
set(CMAKE_CXX_STANDARD 20)
project("SomeApp"
        LANGUAGES CXX
        VERSION 0.0.0.1
        )
# include(cmake/GenerateEmbedFiles.cmake)
set(EXECUTABLE_NAME "window")
message("${CMAKE_PROJECT_NAME} ${CMAKE_PROJECT_VERSION}\n${CMAKE_PROJECT_DESCRIPTION}")

find_package(Vulkan REQUIRED)
# find_package(imgui)

file(GLOB PROJECT_SRC "src/*.cpp" "src/*.h")
if (${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "AMD64")
        set(GLSL_VALIDATOR "$ENV{VULKAN_SDK}/Bin/glslc.exe")
else()
        set(GLSL_VALIDATOR "$ENV{VULKAN_SDK}/Bin32/glslc.exe")
endif()

file(GLOB_RECURSE GLSL_SOURCE_FILES
        "${PROJECT_SOURCE_DIR}/shaders/*"
)
foreach(GLSL ${GLSL_SOURCE_FILES})
  get_filename_component(FILE_NAME ${GLSL} NAME)
  set(SPIRV "${PROJECT_SOURCE_DIR}/bin/shaders/${FILE_NAME}.spv")
  add_custom_command(
    OUTPUT ${SPIRV}
    COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_SOURCE_DIR}/bin/shaders/"
    COMMAND ${GLSL_VALIDATOR} ${GLSL} -o ${SPIRV}
    DEPENDS ${GLSL})
  list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)

add_custom_target(
    Shaders 
    DEPENDS ${SPIRV_BINARY_FILES}
)

find_package(glfw3 REQUIRED)
find_package(spdlog REQUIRED)
find_package(glm)
find_package(tinyobjloader)
find_package(stb)
# find_package(assimp)

add_executable(${EXECUTABLE_NAME} src/main.cpp)
add_dependencies(${EXECUTABLE_NAME} Shaders)
target_link_libraries(${EXECUTABLE_NAME} Vulkan::Vulkan)
target_include_directories(${EXECUTABLE_NAME} PUBLIC ${glfw3_INCLUDE_DIRS})
target_link_libraries(${EXECUTABLE_NAME} glfw)
target_link_libraries(${EXECUTABLE_NAME} glm::glm)
target_link_libraries(${EXECUTABLE_NAME} spdlog::spdlog)
# target_link_libraries(${EXECUTABLE_NAME} assimp::assimp)
target_link_libraries(${EXECUTABLE_NAME} tinyobjloader::tinyobjloader)
target_link_libraries(${EXECUTABLE_NAME} stb::stb)
# target_link_libraries(${EXECUTABLE_NAME} imgui::imgui)

