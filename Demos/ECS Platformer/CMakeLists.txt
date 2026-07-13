add_executable(ECSPlatformer main.cpp)

target_link_libraries(ECSPlatformer PRIVATE gl2d_engine)
target_compile_definitions(ECSPlatformer PRIVATE
    GL2D_ENGINE_SHADER_DIR="${SHADERS_DIR}"
    GL2D_INPUT_BINDINGS_PATH="${RES_DIR}/config/input_bindings.json"
)

# Keep the demo relocatable within the build/package tree. Engine-owned passes
# resolve their shaders from the conventional runtime Shaders directory.
file(COPY "${SHADERS_DIR}" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
