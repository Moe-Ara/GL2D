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
add_executable(${PROJECT_NAME} main.cpp ${SOURCES} ${HEADERS}
        src/Physics/Collision/ACollider.cpp
        src/Physics/Collision/ICollider.hpp
        src/Physics/RigidBody.cpp
        src/Physics/RigidBody.hpp
        src/GameObjects/Components/RigidBodyComponent.cpp
        src/GameObjects/Components/RigidBodyComponent.hpp)

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
set(GL2D_TEST_SOURCES
        Testing/testingGameObject.cpp
        Testing/AnimationMetadataTests.cpp
        Testing/SimpleJsonTests.cpp
        Testing/RigidBodyTests.cpp
        Testing/InputEventBusTests.cpp
        Testing/TriggerCollisionTests.cpp
        src/Utils/SimpleJson.cpp
        src/Graphics/Animation/Loaders/AnimationMetadataLoader.cpp
        src/Physics/RigidBody.cpp
        src/Physics/Collision/ACollider.cpp
        src/Physics/Collision/CollisionDispatcher.cpp
        src/Physics/Collision/AABB.cpp
        src/Physics/Collision/AABBCollider.cpp
        src/Physics/Collision/CircleCollider.cpp
        src/Physics/Collision/CapsuleCollider.cpp
        src/InputSystem/InputEventBus.cpp
        src/GameObjects/Entity.cpp
)

add_executable(GL2D_TEST ${GL2D_TEST_SOURCES})

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
