# Demos/The Lost Heroin/CMakeLists.txt

project(TheLostHeroinDemo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(DEMO_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(DEMO_ASSETS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/assets)
set(DEMO_SHADERS_DIR ${SHADERS_DIR})

file(GLOB_RECURSE DEMO_SOURCES CONFIGURE_DEPENDS "${DEMO_SRC_DIR}/*.cpp")
file(GLOB_RECURSE DEMO_HEADERS CONFIGURE_DEPENDS "${DEMO_SRC_DIR}/*.hpp" "${DEMO_SRC_DIR}/*.h")

add_executable(TheLostHeroin
        ${DEMO_SOURCES}
        ${DEMO_HEADERS}
)

target_include_directories(TheLostHeroin
        PRIVATE
        ${DEMO_SRC_DIR}
)

target_compile_definitions(TheLostHeroin
        PRIVATE
        GL2D_ENGINE_SHADER_DIR=\"${SHADERS_DIR}\"
        DEMO_BACKGROUND_DIR=\"${DEMO_ASSETS_DIR}/PNG\"
)

# Link against the engine target defined in the root CMakeLists
target_link_libraries(TheLostHeroin
        PRIVATE
        gl2d_engine
)

# Make sure engine builds before the demo (usually automatic, but explicit is ok)
add_dependencies(TheLostHeroin gl2d_engine)

# Optional: copy demo-specific assets/shaders
if(EXISTS "${DEMO_ASSETS_DIR}")
    file(COPY ${DEMO_ASSETS_DIR} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/assets")
    file(COPY ${CMAKE_SOURCE_DIR}/assets DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endif()

if(EXISTS "${DEMO_SHADERS_DIR}")
    file(COPY ${DEMO_SHADERS_DIR} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endif()
