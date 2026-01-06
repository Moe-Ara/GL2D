cmake_minimum_required(VERSION 3.23)
project(GL2D LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include vcpkg toolchain file (CLion may also set this via profiles)
set(CMAKE_TOOLCHAIN_FILE "C:/vcpkg/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "Vcpkg toolchain file")

# Find packages
find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(Boost REQUIRED COMPONENTS unit_test_framework)
find_package(Qt5 COMPONENTS Widgets REQUIRED)

# Paths
set(SRC_DIR     ${CMAKE_SOURCE_DIR}/src)
set(RES_DIR     ${CMAKE_SOURCE_DIR}/assets)
set(SHADERS_DIR ${CMAKE_SOURCE_DIR}/Shaders)
set(MODELS_DIR  ${CMAKE_SOURCE_DIR}/models)

# Optional normal map generation from sprites (requires Pillow).
option(GL2D_GENERATE_NORMALS "Generate normal maps from sprites before build" OFF)
set(GL2D_NORMAL_STRENGTH 2.0 CACHE STRING "Normal generation strength")
set(GL2D_NORMAL_BLUR     0.0 CACHE STRING "Normal generation blur radius")

# Gather all engine source and header files and rerun the configure stage when files change.
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "${SRC_DIR}/*.cpp")
file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS "${SRC_DIR}/*.hpp" "${SRC_DIR}/*.h")

# Tests
file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/Testing/*.cpp")

# === stb_image as separate lib ===
add_library(stb_image STATIC src/third_party/stb/stb_image.cpp)
target_include_directories(stb_image PUBLIC third_party/stb)

# === ENGINE LIBRARY ===
add_library(gl2d_engine STATIC ${SOURCES} ${HEADERS})

target_include_directories(gl2d_engine
        PUBLIC
        ${OPENGL_INCLUDE_DIRS}
        ${GLEW_INCLUDE_DIRS}
        ${GLM_INCLUDE_DIRS}
        ${SRC_DIR}
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/third_party/stb
)

target_link_libraries(gl2d_engine
        PUBLIC
        ${OPENGL_LIBRARIES}
        ${GLEW_LIBRARIES}
        ${GLFW3_LIBRARY}
        glfw
        glm::glm
        stb_image
)

# (Optional) put library in a predictable place (root of build dir)
set_target_properties(gl2d_engine PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY_DEBUG       "${CMAKE_BINARY_DIR}"
        ARCHIVE_OUTPUT_DIRECTORY_RELEASE     "${CMAKE_BINARY_DIR}"
        ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}"
        ARCHIVE_OUTPUT_DIRECTORY_MINSIZEREL  "${CMAKE_BINARY_DIR}"
)

# === MAIN GAME EXECUTABLE (your current GL2D.exe) ===
add_executable(GL2D main.cpp)

target_link_libraries(GL2D
        PRIVATE
        gl2d_engine
)

add_subdirectory(editor)
add_dependencies(GL2D GL2DEditor)

# Copy shared resources into the build dir (for GL2D + demos)
file(COPY ${SHADERS_DIR} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${RES_DIR}     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

file(GLOB MODEL_FILES ${MODELS_DIR}/*.*)
file(COPY ${MODEL_FILES} DESTINATION ${CMAKE_BINARY_DIR}/models)

# Optional normal generation hooked up to engine (and by extension, anyone using it)
if(GL2D_GENERATE_NORMALS)
    find_package(Python3 COMPONENTS Interpreter REQUIRED)
    file(GLOB_RECURSE SPRITE_IMAGES "${RES_DIR}/*.png")
    set(NORMAL_OUTPUTS "")
    foreach(img ${SPRITE_IMAGES})
        file(RELATIVE_PATH rel "${RES_DIR}" "${img}")
        string(REGEX REPLACE "\\.png$" "_normal.png" normal_rel "${rel}")
        set(out "${CMAKE_CURRENT_BINARY_DIR}/assets/${normal_rel}")
        get_filename_component(out_dir "${out}" DIRECTORY)
        add_custom_command(
                OUTPUT "${out}"
                COMMAND ${CMAKE_COMMAND} -E make_directory "${out_dir}"
                COMMAND ${Python3_EXECUTABLE} "${CMAKE_SOURCE_DIR}/tools/normal_from_sprite.py"
                "${img}" "${out}"
                --strength ${GL2D_NORMAL_STRENGTH}
                --blur ${GL2D_NORMAL_BLUR}
                DEPENDS "${img}" "${CMAKE_SOURCE_DIR}/tools/normal_from_sprite.py"
                COMMENT "Generating normal map: ${normal_rel}"
                VERBATIM
        )
        list(APPEND NORMAL_OUTPUTS "${out}")
    endforeach()
    add_custom_target(generate_sprite_normals ALL DEPENDS ${NORMAL_OUTPUTS})
    add_dependencies(gl2d_engine generate_sprite_normals)
endif()

# === TESTS ===
enable_testing()


add_executable(GL2D_TEST ${TEST_SOURCES})
target_include_directories(GL2D_TEST PRIVATE
        ${Boost_INCLUDE_DIRS}
        ${OPENGL_INCLUDE_DIRS}
        ${GLEW_INCLUDE_DIRS}
        ${GLM_INCLUDE_DIRS}
        ${SRC_DIR}
)

target_link_libraries(GL2D_TEST
        PRIVATE
        gl2d_engine
        ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
)

add_test(NAME GL2D_TEST COMMAND GL2D_TEST)
add_dependencies(GL2D GL2D_TEST)

message(STATUS "Project: ${PROJECT_NAME}")
message(STATUS "Using vcpkg toolchain: ${CMAKE_TOOLCHAIN_FILE}")

# === DEMO(S) ===
add_subdirectory("Demos/The Lost Heroin")
if(TARGET TheLostHeroin)
    add_dependencies(GL2D TheLostHeroin)
endif()
