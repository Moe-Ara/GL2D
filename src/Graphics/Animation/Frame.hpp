//
// Created by Mohamad on 08/02/2025.
//

#ifndef GL2D_FRAME_HPP
#define GL2D_FRAME_HPP

#include <memory>
#include <string>

#include <glm/vec4.hpp>

#include "GameObjects/Texture.hpp"

namespace Graphics {
    struct Frame {
        int row{0};
        int column{0};
        bool useCustomUV{false};
        glm::vec4 uvRect{0.0f, 0.0f, 1.0f, 1.0f};
        float duration{-1.0f};
        std::string eventName;
        std::shared_ptr<GameObjects::Texture> texture{nullptr};
    };
}

#endif //GL2D_FRAME_HPP
