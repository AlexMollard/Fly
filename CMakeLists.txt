cmake_minimum_required(VERSION 3.15)

# Project name and version
project(Fly VERSION 1.0)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Add source files
file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE HEADERS "src/*.h")

# Add executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE src)

# Set precompiled header
target_precompile_headers(${PROJECT_NAME} PRIVATE src/pch.h)

# Add submodules
add_subdirectory(external/imgui)
add_subdirectory(external/openal-soft)
add_subdirectory(external/libsndfile)
add_subdirectory(external/hello-imgui)

# Link libraries
find_package(OpenAL REQUIRED)
find_package(SndFile REQUIRED)
find_package(ImGui REQUIRED)
find_package(HelloImGui REQUIRED)

target_link_libraries(${PROJECT_NAME} PRIVATE OpenAL::OpenAL sndfile ImGui::ImGui HelloImGui::HelloImGui)

# Set configurations
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release
)

# Post-build commands
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/res $<TARGET_FILE_DIR:${PROJECT_NAME}>
)
