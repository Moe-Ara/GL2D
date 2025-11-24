//
// Tileset.hpp
//

#ifndef GL2D_TILESET_HPP
#define GL2D_TILESET_HPP

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <memory>
#include <string>
#include <vector>

namespace GameObjects { class Texture; }

struct Tileset {
    std::string id;
    std::string texturePath;
    std::shared_ptr<GameObjects::Texture> texture;

    glm::ivec2 tileSize{0, 0};
    int rows{0};
    int cols{0};
    int margin{0};
    int spacing{0};

    // UVs per tile index (u0, v0, u1, v1) in normalized atlas coordinates.
    std::vector<glm::vec4> uvs;
    // Optional solidity flags per tile index.
    std::vector<bool> solid;

    [[nodiscard]] glm::vec4 getUV(int index) const {
        if (index < 0 || index >= static_cast<int>(uvs.size())) {
            return glm::vec4{0.0f, 0.0f, 1.0f, 1.0f};
        }
        return uvs[static_cast<size_t>(index)];
    }

    [[nodiscard]] bool isSolid(int index) const {
        if (index < 0 || index >= static_cast<int>(solid.size())) {
            return false;
        }
        return solid[static_cast<size_t>(index)];
    }
};

#endif // GL2D_TILESET_HPP
