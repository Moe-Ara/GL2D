//
// Created by Mohamad on 08/02/2025.
//

#ifndef GL2D_FRAME_HPP
#define GL2D_FRAME_HPP

#include "GameObjects/Texture.hpp"

namespace Graphics {
    struct Frame {
        int row;
        int column;
        std::shared_ptr<GameObjects::Texture> texture = nullptr;
    };
}

#endif //GL2D_FRAME_HPP
