cmake_minimum_required(VERSION 3.23)
project(GL2D)

set(CMAKE_CXX_STANDARD 23)

# Include vcpkg toolchain file
set(CMAKE_TOOLCHAIN_FILE "C:/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")

# Find packages
find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(Boost REQUIRED COMPONENTS unit_test_framework)

# Define source directories
set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)
set(RES_DIR ${CMAKE_SOURCE_DIR}/assets)
set(SHADERS_DIR ${CMAKE_SOURCE_DIR}/Shaders)
set(MODELS_DIR ${CMAKE_SOURCE_DIR}/models)

# Gather all source and header files dynamically
file(GLOB_RECURSE SOURCES "${SRC_DIR}/*.cpp")
file(GLOB_RECURSE HEADERS "${SRC_DIR}/*.hpp" "${SRC_DIR}/*.h")

# Executable
add_executable(${PROJECT_NAME} main.cpp ${SOURCES}
        Testing/testingGameObject.cpp
        src/Graphics/Window.cpp
        src/Graphics/Window.hpp
        src/Exceptions/Gl2DException.hpp
        src/Exceptions/WindowException.hpp
        src/Graphics/Shader.cpp
        src/Graphics/Shader.hpp
        src/Exceptions/ShaderException.hpp
        src/GameObjects/Vertex.hpp
        src/GameObjects/Mesh.cpp
        src/GameObjects/Mesh.hpp
        src/GameObjects/Texture.cpp
        src/GameObjects/Texture.hpp
        src/third_party/stb/stb_image.cpp
        src/Exceptions/TextureException.hpp
        src/Managers/TextureManager.cpp
        src/Managers/TextureManager.hpp
        src/GameObjects/Sprite.cpp
        src/GameObjects/Sprite.hpp
        src/Graphics/Animation/Frame.hpp
        src/Graphics/Animation/Animation.cpp
        src/Graphics/Animation/Animation.hpp
        src/Graphics/Animation/Animator.cpp
        src/Graphics/Animation/Animator.hpp
        src/Utils/IState.hpp
        src/Utils/IStateMachine.hpp
        src/Graphics/Animation/AnimationStateMachine.cpp
        src/Graphics/Animation/AnimationStateMachine.hpp
        src/Exceptions/AnimationException.hpp
        src/Graphics/Animation/AnimationState.cpp
        src/Graphics/Animation/AnimationState.hpp
        src/InputSystem/InputService.cpp
        src/InputSystem/InputService.hpp
        src/InputSystem/InputBindingsLoader.cpp
        src/InputSystem/InputBindingsLoader.hpp
        src/InputSystem/InputTypes.hpp
        src/InputSystem/InputEventBus.cpp
        src/InputSystem/InputEventBus.hpp
        src/InputSystem/InputCommandRegistry.cpp
        src/InputSystem/InputCommandRegistry.hpp
        src/Graphics/Animation/IAnimationConditionSink.hpp
        src/InputSystem/Commands/JumpCommand.cpp
        src/InputSystem/Commands/JumpCommand.hpp
)

# Copy shaders to build directory
file(COPY ${SHADERS_DIR} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${RES_DIR} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
# Copy models to build directory
file(GLOB MODEL_FILES ${MODELS_DIR}/*.*)
file(COPY ${MODEL_FILES} DESTINATION ${CMAKE_BINARY_DIR}/models)
add_library(stb_image STATIC src/third_party/stb/stb_image.cpp)
target_include_directories(stb_image PUBLIC third_party/stb)
# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
        ${OPENGL_INCLUDE_DIRS}
        ${GLEW_INCLUDE_DIRS}
        ${GLM_INCLUDE_DIRS}
        ${SRC_DIR}
        stb_image
)

# Link libraries
target_link_libraries(${PROJECT_NAME}
        ${OPENGL_LIBRARIES}
        ${GLEW_LIBRARIES}
        ${GLFW3_LIBRARY}
        glfw
        glm::glm
)

# Enable testing
enable_testing()

# Test executable
add_executable(GL2D_TEST Testing/testingGameObject.cpp
        Testing/testingGameObject.cpp
        Testing/AnimationMetadataTests.cpp)

# Include directories for test target
target_include_directories(GL2D_TEST PRIVATE
        ${Boost_INCLUDE_DIRS}
        ${OPENGL_INCLUDE_DIRS}
        ${GLEW_INCLUDE_DIRS}
        ${GLM_INCLUDE_DIRS}
        ${SRC_DIR}
)

# Link Boost.Test library for test target
target_link_libraries(GL2D_TEST
        ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
        ${OPENGL_LIBRARIES}
        ${GLEW_LIBRARIES}
        ${GLFW3_LIBRARY}
        glfw
        glm::glm
)

# Define tests
add_test(NAME GL2D_TEST COMMAND GL2D_TEST)

# Print status messages
message(STATUS "Project: ${PROJECT_NAME}")
message(STATUS "Using vcpkg toolchain: ${CMAKE_TOOLCHAIN_FILE}")
