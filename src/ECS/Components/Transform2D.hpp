#pragma once

#include <glm/ext/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>

namespace ECS {

// World-space 2D transform data. Rotation is expressed in degrees to match the
// existing authoring APIs; systems should perform interpolation before extraction.
struct Transform2D {
    glm::vec2 position{0.0f};
    glm::vec2 scale{1.0f};
    float rotationDegrees{0.0f};
};

[[nodiscard]] inline glm::mat4 toMatrix(const Transform2D& transform) {
    glm::mat4 matrix{1.0f};
    matrix = glm::translate(matrix, glm::vec3(transform.position, 0.0f));
    matrix = glm::rotate(matrix, glm::radians(transform.rotationDegrees), {0.0f, 0.0f, 1.0f});
    return glm::scale(matrix, glm::vec3(transform.scale, 1.0f));
}

} // namespace ECS
