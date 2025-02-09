//
// Created by Mohamad on 05/02/2025.
//

#ifndef GL2D_VERTEX_HPP
#define GL2D_VERTEX_HPP

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

struct Vertex{
    glm::vec2 position{};
    glm::vec3 color{};
    glm::vec2 uv{};
    bool operator==(const Vertex& other) const{
        return position == other.position && color == other.color && uv==other.uv;
    }
};
#endif //GL2D_VERTEX_HPP
